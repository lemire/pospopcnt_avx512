#define _GNU_SOURCE
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <sys/syscall.h>

#if FLAGSIZE == 64
typedef uint64_t flags_type;
#else
typedef uint32_t flags_type;
#endif

#ifndef TIME_GOAL
/* run each benchmark for at least this many seconds */
#define TIME_GOAL 2.0
#endif

#ifdef ALIGN
#define memory_allocate(size) aligned_alloc(64, (size) + 63 & ~63)
#else
#define memory_allocate(size) malloc(size)
#endif

#include "pospopcnt_avx512bw.h"
#include "pospopcnt.h"

typedef void pospopcnt_u16(const uint16_t *data, uint32_t len, flags_type *flags);
extern void count16avx2(flags_type *flags, const uint16_t *data, uint32_t len);
extern void count16avx512(flags_type *flags, const uint16_t *data, uint32_t len);
static void
pospopcnt_count16avx512(const uint16_t *data, uint32_t len, flags_type *flags)
{
	count16avx512(flags, data, len);
}

static void
pospopcnt_count16avx2(const uint16_t *data, uint32_t len, flags_type *flags)
{
	count16avx2(flags, data, len);
}

static const struct pospopcnt_u16_method {
	const char *name;
	pospopcnt_u16 *method;
} methods[] = {
	{ "scalar", pospopcnt_u16_scalar },
	{ "avx512bw_harvey_seal_1KB", pospopcnt_u16_avx512bw_harvey_seal_1KB },
	{ "avx512bw_harvey_seal_512B", pospopcnt_u16_avx512bw_harvey_seal_512B },
	{ "avx512bw_harvey_seal_256B", pospopcnt_u16_avx512bw_harvey_seal_256B },
	{ "count16avx512", pospopcnt_count16avx512 },
	{ "count16avx2", pospopcnt_count16avx2 },
	{ NULL, NULL },
};

static int event_group = -1, num_counters = 0;
enum { EVENT_COUNT = 2 };
static struct event {
	uint32_t type;
	int fd;
	uint64_t conf;
} events[EVENT_COUNT] = {
	{ PERF_TYPE_HARDWARE, -1, PERF_COUNT_HW_CPU_CYCLES },
	{ PERF_TYPE_HARDWARE, -1, PERF_COUNT_HW_INSTRUCTIONS },
};

/* set up performance counters so performance measurements can be taken */
static void init_counters()
{
	int i;
	struct perf_event_attr attribs;

	memset(&attribs, 0, sizeof attribs);
	attribs.exclude_kernel = 1;
	attribs.exclude_hv = 1;
	attribs.sample_period = 0;
	attribs.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;

	for (i = 0; i < EVENT_COUNT; i++) {
		attribs.type = events[i].type;
		attribs.config = events[i].conf;
		events[i].fd = syscall(SYS_perf_event_open, &attribs, 0, -1, event_group, 0);
		if (events[i].fd == -1) {
			perror("perf_event_open");
			continue;
		}

		num_counters++;

		if (event_group == -1)
			event_group = events[i].fd;
	}
}

/* state of performance counters at one point in time */
struct counters {
	struct timespec ts;
	uint64_t counters[2 * EVENT_COUNT + 1];
};

typedef int testfunc(struct counters *c, void *payload, size_t n, size_t m);

/* initialise hardware perf counters */

/* set counters to their current value */
static int
reset_counters(struct counters *c)
{
	ssize_t count;
	int res;

	res = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &c->ts);
	if (res == -1) {
		perror("clock_gettime");
		return (-1);
	}

	if (event_group != -1) {
		count = read(event_group, c->counters, (2 * num_counters + 1) * sizeof c->counters[0]);
		if (count < 0) {
			perror("read(event_group, ...)");
			return (-1);
		}
	}

	return (0);
}

/* compute the difference between two timespec as a double */
static double
tsdiff(struct timespec start, struct timespec end)
{
	time_t sec;
	long nsec;

	sec = end.tv_sec - start.tv_sec;
	nsec = end.tv_nsec - start.tv_nsec;
	if (nsec < 0) {
		sec--;
		nsec += 1000000000L;
	}

	return (sec + nsec * 1.0e-9);
}

/* compute the difference between the two counter vectors */
static void
counterdiff(uint64_t out[EVENT_COUNT], uint64_t start[], uint64_t end[])
{
	int i, j;

	for (i, j = 0; i < EVENT_COUNT; i++) {
		if (events[i].fd == -1)
			out[i] = 0;
		else {
			out[i] = end[2*j+1] - start[2*j+1];
			j++;
		}
	}
}

/* print test results */
/* https://golang.org/design/14313-benchmark-format */
static void
print_results(
    const char *name,
    struct counters *start, struct counters *end,
    size_t n, size_t m) {
	uint64_t counts[EVENT_COUNT];
	double elapsed;

	elapsed = tsdiff(start->ts, end->ts);
	counterdiff(counts, start->counters, end->counters);

	if (name == NULL || name[0] == '\0')
		name = " ";

	printf("Benchmark%c%s/%zuB\t%10zu\t"
	    "%.8g ns/op\t%.8g MB/s",
	    toupper(name[0]), name+1, n, m,
	    (elapsed * 1e9) / m, (1e-6 * n * m) / elapsed);

	if (num_counters == EVENT_COUNT) {
		printf("\t%.8g cy/B\t%.8g ins/B\t%.8g ipc\n",
		    counts[0]/((double)n * m), counts[1]/((double)n * m),
		    (double)counts[1] / counts[0]);
	} else
		putchar('\n');
}

/* run one test case for the specified n and print the result */
static void
run_test(const char *name, testfunc *test, void *payload, size_t n)
{
	struct counters start, end;
	size_t m = 1;
	int first_run = 1;

	/* repeatedly run benchmark and adjust m until result is meaningful */
	for (;; first_run = 0) {
		double elapsed;
		size_t newm;
		int res;

		/* printf("RUN %s: n = %zu m = %zu\n", name, n, m); */

		res = reset_counters(&start);
		if (res != 0) {
			printf("FAIL\t%s\n", name);
			return;
		}

		res = test(&start, payload, n, m);
		if (res != 0) {
			printf("FAIL\t%s\n", name);
			return;
		}

		res = reset_counters(&end);
		if (res != 0) {
			printf("FAIL\t%s\n", name);
			return;
		}

		elapsed = tsdiff(start.ts, end.ts);
		if (elapsed < TIME_GOAL) {
			if (elapsed < TIME_GOAL * 0.5)
				m *= 2;
			else {
				/* try to overshoot 1s time goal slightly */
				newm = ceil(m * TIME_GOAL * 1.05 / elapsed);
				m = newm > m ? newm : m + 1;
			}

			continue;
		}

		/* make sure to perform at least one warm-up iteration */
		if (!first_run)
			break;
	}

	print_results(name, &start, &end, n, m);
}

/* test positiona population count function payload with an n byte array */
typedef int testfunc(struct counters *c, void *payload, size_t n, size_t m);
static int
test_pospop_u16(struct counters *c, void *payload, size_t n, size_t m)
{
	pospopcnt_u16 *pospop = (pospopcnt_u16 *)payload;
	uint16_t *data;
	size_t len = n / sizeof *data, i;
	long int num;
	int res;
	flags_type flags[16], accum = 0;
	volatile flags_type sum;

	memset(flags, 0, sizeof flags);
	data = memory_allocate(len * sizeof *data);
	if (data == NULL) {
		perror("memory_allocate");
		return (-1);
	}

	srand48(42);
	for (i = 0; i < len; i += 2) {
		num = mrand48();
		data[i] = num & 0xffff;
		data[i+1] = num >> 16 & 0xffff;
	}

	/* skip initialisation step in benchmark measurements */
	res = reset_counters(c);
	if (res != 0)
		return (-1);

	for (i = 0; i < m; i++)
		pospop(data, len, flags);

	/* make sure the result is used */
	accum = 0;
	for (i = 0; i < 16; i++)
		accum += flags[i];
	sum = accum;

	free(data);

	return (0);
}

extern int
main(int argc, char *argv[])
{
	size_t n;
	int i, j;
	char *fake_argv[] = { argv[0], "100000", NULL };

	setlinebuf(stdout);
	init_counters();

	if (argc < 2) {
		argv = fake_argv;
		argc = 2;
	}

	for (j = 1; j < argc; j++) {
		n = atoll(argv[j]);
		for (i = 0; methods[i].method != NULL; i++) {
			run_test(methods[i].name, test_pospop_u16, methods[i].method, n);
		}
	}
}
