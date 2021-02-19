# AVX-512 positional popcount with 15-fold CSA reduction
# by Robert Clausecker <fuz@fuz.su>
# from github.com/clausecker/pospop@1.2.1
# with slight copy-editing

	.intel_syntax noprefix
	.section .rodata
	.align	16
magic:	.quad	0x0706050403020100
	.quad	0x8040201008040201
	.int	0x55555555
	.int	0x33333333
	.int	0x0f0f0f0f
	.size magic, .-magic

	.section .text
        .type   countavx512, @function
	.balign 16
countavx512:
        test    rcx, rcx
        cmove   rsi, rcx
        vmovq   xmm1, [magic+rip]
        vpbroadcastq zmm31, [magic+8+rip]
        vpternlogd zmm30, zmm30, zmm30, 0xFF
        vpxord  ymm25, ymm25, ymm25
        vpxor   ymm0, ymm0, ymm0
        vpunpcklbw xmm1, xmm1, xmm1
        vpermq  ymm1, ymm1, 0x50
        vpunpcklwd ymm1, ymm1, ymm1
        vpmovzxdq zmm1, ymm1
        vpshufd zmm29, zmm1, 0xA0
        mov     edx, esi
        and     edx, 0x3F
        jz      .L22089
        mov     r8, -1
        sub     rsi, rdx
        shlx    r9, r8, rcx
        not     r9
        cmp     rcx, 64
        cmovl   r8, r9
        shlx    r8, r8, rdx
        kmovq   k1, r8
        vmovdqu8 zmm4 {k1}{z}, [rsi]
        add     rsi, 64
        lea     rcx, [rcx+rdx-0x40]
        mov     edx, 4
.L22088:vpunpckhqdq xmm6, xmm4, xmm4
        sub     edx, 1
        vpbroadcastq zmm5, xmm4
        vpbroadcastq zmm6, xmm6
        vpshufb zmm5, zmm5, zmm29
        vpshufb zmm6, zmm6, zmm29
        vshufi64x2 zmm4, zmm4, zmm4, 0x39
        vptestmb k1, zmm5, zmm31
        vptestmb k2, zmm6, zmm31
        vpsubb  zmm0 {k1}, zmm0, zmm30
        vpsubb  zmm0 {k2}, zmm0, zmm30
        jnz     .L22088
.L22089:vpunpcklbw zmm8, zmm0, zmm25
        vpunpckhbw zmm9, zmm0, zmm25
        sub     rcx, 960
        jl      .L22092
        vpbroadcastd zmm28, [magic+16+rip]
        vpbroadcastd zmm27, [magic+20+rip]
        vpbroadcastd zmm26, [magic+24+rip]
        mov     eax, 65527
.L22090:vmovdqa64 zmm0, [rsi]
        vmovdqa64 zmm1, [rsi+0x1*0x40]
        vmovdqa64 zmm4, [rsi+0x2*0x40]
        vmovdqa64 zmm2, [rsi+0x3*0x40]
        vmovdqa64 zmm3, [rsi+0x4*0x40]
        vmovdqa64 zmm5, [rsi+0x5*0x40]
        vmovdqa64 zmm6, [rsi+0x6*0x40]
        vmovdqa64 zmm7, zmm0
        vpternlogd zmm0, zmm1, zmm4, 0x96
        vpternlogd zmm1, zmm7, zmm4, 0xE8
        vmovdqa64 zmm4, [rsi+0x7*0x40]
        vmovdqa64 zmm7, zmm3
        vpternlogd zmm3, zmm2, zmm5, 0x96
        vpternlogd zmm2, zmm7, zmm5, 0xE8
        vmovdqa64 zmm5, [rsi+0x8*0x40]
        vmovdqa64 zmm7, zmm0
        vpternlogd zmm0, zmm3, zmm6, 0x96
        vpternlogd zmm3, zmm7, zmm6, 0xE8
        vmovdqa64 zmm6, [rsi+0x9*0x40]
        vmovdqa64 zmm7, zmm1
        vpternlogd zmm1, zmm2, zmm3, 0x96
        vpternlogd zmm2, zmm7, zmm3, 0xE8
        vmovdqa64 zmm3, [rsi+0xA*0x40]
        vmovdqa64 zmm7, zmm0
        vpternlogd zmm0, zmm4, zmm5, 0x96
        vpternlogd zmm4, zmm7, zmm5, 0xE8
        vmovdqa64 zmm5, [rsi+0xB*0x40]
        vmovdqa64 zmm7, zmm0
        vpternlogd zmm0, zmm3, zmm6, 0x96
        vpternlogd zmm3, zmm7, zmm6, 0xE8
        vmovdqa64 zmm6, [rsi+0xC*0x40]
        vmovdqa64 zmm7, zmm1
        vpternlogd zmm1, zmm3, zmm4, 0x96
        vpternlogd zmm3, zmm7, zmm4, 0xE8
        vmovdqa64 zmm4, [rsi+0xD*0x40]
        vmovdqa64 zmm7, zmm0
        vpternlogd zmm0, zmm5, zmm6, 0x96
        vpternlogd zmm5, zmm7, zmm6, 0xE8
        vmovdqa64 zmm6, [rsi+0xE*0x40]
        vmovdqa64 zmm7, zmm0
        vpternlogd zmm0, zmm4, zmm6, 0x96
        vpternlogd zmm4, zmm7, zmm6, 0xE8
        vmovdqa64 zmm7, zmm1
        vpternlogd zmm1, zmm4, zmm5, 0x96
        vpternlogd zmm4, zmm7, zmm5, 0xE8
        vmovdqa64 zmm7, zmm2
        vpternlogd zmm2, zmm3, zmm4, 0x96
        vpternlogd zmm3, zmm7, zmm4, 0xE8
        add     rsi, 960
        vpsrld  zmm4, zmm0, 1
        vpaddd  zmm5, zmm1, zmm1
        vpsrld  zmm6, zmm2, 1
        vpaddd  zmm7, zmm3, zmm3
        vpternlogd zmm0, zmm5, zmm28, 0xE4
        vpternlogd zmm1, zmm4, zmm28, 0xD8
        vpternlogd zmm2, zmm7, zmm28, 0xE4
        vpternlogd zmm3, zmm6, zmm28, 0xD8
        vpsrld  zmm4, zmm0, 2
        vpsrld  zmm6, zmm1, 2
        vpslld  zmm5, zmm2, 2
        vpslld  zmm7, zmm3, 2
        vpternlogd zmm2, zmm4, zmm27, 0xD8
        vpternlogd zmm3, zmm6, zmm27, 0xD8
        vpternlogd zmm0, zmm5, zmm27, 0xE4
        vpternlogd zmm1, zmm7, zmm27, 0xE4
        vpunpcklbw zmm6, zmm2, zmm3
        vpunpckhbw zmm3, zmm2, zmm3
        vpunpcklbw zmm5, zmm0, zmm1
        vpunpckhbw zmm2, zmm0, zmm1
        vpunpcklwd zmm4, zmm5, zmm6
        vpunpckhwd zmm5, zmm5, zmm6
        vpunpcklwd zmm6, zmm2, zmm3
        vpunpckhwd zmm7, zmm2, zmm3
        vpandd  zmm0, zmm4, zmm26
        vpsrld  zmm4, zmm4, 4
        vpandd  zmm4, zmm4, zmm26
        vpandd  zmm1, zmm5, zmm26
        vpsrld  zmm5, zmm5, 4
        vpandd  zmm5, zmm5, zmm26
        vpandd  zmm2, zmm6, zmm26
        vpsrld  zmm6, zmm6, 4
        vpandd  zmm6, zmm6, zmm26
        vpandd  zmm3, zmm7, zmm26
        vpsrld  zmm7, zmm7, 4
        vpandd  zmm7, zmm7, zmm26
        vpaddb  zmm0, zmm0, zmm2
        vpaddb  zmm1, zmm1, zmm3
        vpaddb  zmm2, zmm4, zmm6
        vpaddb  zmm3, zmm5, zmm7
        vpunpckldq zmm4, zmm0, zmm2
        vpunpckhdq zmm5, zmm0, zmm2
        vpunpckldq zmm6, zmm1, zmm3
        vpunpckhdq zmm7, zmm1, zmm3
        vshufi64x2 zmm0, zmm4, zmm5, 0x44
        vshufi64x2 zmm1, zmm4, zmm5, 0xEE
        vshufi64x2 zmm2, zmm6, zmm7, 0x44
        vshufi64x2 zmm3, zmm6, zmm7, 0xEE
        vpaddb  zmm0, zmm0, zmm1
        vpaddb  zmm2, zmm2, zmm3
        vshufi64x2 zmm1, zmm0, zmm2, 0x88
        vshufi64x2 zmm0, zmm0, zmm2, 0xDD
        vpaddb  zmm0, zmm0, zmm1
        vpunpcklbw zmm1, zmm0, zmm25
        vpunpckhbw zmm2, zmm0, zmm25
        vpaddw  zmm8, zmm8, zmm1
        vpaddw  zmm9, zmm9, zmm2
        sub     eax, 120
        cmp     eax, 120
        jge     .L22091
        call    rbx
        vpxor   ymm8, ymm8, ymm8
        vpxor   ymm9, ymm9, ymm9
        mov     eax, 65535
.L22091:sub     rcx, 960
        jge     .L22090
.L22092:vpxor   ymm0, ymm0, ymm0
        sub     ecx, -952
        jl      .L22094
.L22093:vpbroadcastq zmm4, [rsi]
        add     rsi, 8
        vpshufb zmm4, zmm4, zmm29
        sub     ecx, 8
        vptestmb k1, zmm4, zmm31
        vpsubb  zmm0 {k1}, zmm0, zmm30
        jge     .L22093
.L22094:sub     ecx, -8
        jle     .L22095
        vpbroadcastq zmm4, [rsi]
        vpshufb zmm4, zmm4, zmm29
        xor     eax, eax
        bts     eax, ecx
        dec     eax
        kmovb   k1, eax
        vmovdqa64 zmm4 {k1}{z}, zmm4
        vptestmb k1, zmm4, zmm31
        vpsubb  zmm0 {k1}, zmm0, zmm30
.L22095:vpunpcklbw zmm1, zmm0, zmm25
        vpunpckhbw zmm2, zmm0, zmm25
        vpaddw  zmm8, zmm8, zmm1
        vpaddw  zmm9, zmm9, zmm2
        call    rbx
        vzeroupper
        ret

# not ported to uint32_t flags yet
	.balign 16
accum8:
        .type   accum8, @function
        vpmovzxwq zmm0, xmm8
        vextracti64x2 xmm1, zmm8, 0x01
        vpmovzxwq zmm1, xmm1
        vextracti64x2 xmm2, zmm8, 0x02
        vpmovzxwq zmm2, xmm2
        vextracti64x2 xmm3, zmm8, 0x03
        vpmovzxwq zmm3, xmm3
        vpmovzxwq zmm4, xmm9
        vextracti64x2 xmm5, zmm9, 0x01
        vpmovzxwq zmm5, xmm5
        vextracti64x2 xmm6, zmm9, 0x02
        vpmovzxwq zmm6, xmm6
        vextracti64x2 xmm7, zmm9, 0x03
        vpmovzxwq zmm7, xmm7
        vpaddq  zmm0, zmm0, zmm2
        vpaddq  zmm1, zmm1, zmm3
        vpaddq  zmm4, zmm4, zmm6
        vpaddq  zmm5, zmm5, zmm7
        vpaddq  zmm0, zmm0, zmm1
        vpaddq  zmm4, zmm4, zmm5
        vpaddq  zmm0, zmm0, zmm4
        vpaddq  zmm0, zmm0, [rdi]
        vmovdqu64 [rdi], zmm0
        ret

	.balign 16
accum16:
        .type   accum16, @function
        vpmovzxwd zmm0, ymm8
        vextracti64x4 ymm2, zmm8, 0x01
        vpmovzxwd zmm2, ymm2
        vpmovzxwd zmm4, ymm9
        vextracti64x4 ymm6, zmm9, 0x01
        vpmovzxwd zmm6, ymm6
        vpaddd  zmm0, zmm0, zmm2
        vpaddd  zmm4, zmm4, zmm6
        vshufi64x2 zmm1, zmm0, zmm4, 0x44
        vshufi64x2 zmm5, zmm0, zmm4, 0xEE
        vpaddd  zmm1, zmm1, zmm5
        vpaddd  zmm1, zmm1, [rdi]
        vmovdqu64 [rdi], zmm1
        ret

# not ported to uint32_t flags yet
	.balign 16
accum32:
        .type   accum32, @function
        vextracti64x2 xmm2, zmm8, 0x02
        vextracti64x2 xmm3, zmm9, 0x02
        vpmovzxwq zmm0, xmm8
        vpmovzxwq zmm1, xmm9
        vpmovzxwq zmm2, xmm2
        vpmovzxwq zmm3, xmm3
        vpaddq  zmm0, zmm0, zmm2
        vpaddq  zmm1, zmm1, zmm3
        vpaddq  zmm0, zmm0, [rdi]
        vpaddq  zmm1, zmm1, [rdi+0x1*0x40]
        vmovdqu64 [rdi], zmm0
        vmovdqu64 [rdi+0x1*0x40], zmm1
        vextracti64x2 xmm0, zmm8, 0x01
        vextracti64x2 xmm1, zmm9, 0x01
        vextracti64x2 xmm2, zmm8, 0x03
        vextracti64x2 xmm3, zmm9, 0x03
        vpmovzxwq zmm0, xmm0
        vpmovzxwq zmm1, xmm1
        vpmovzxwq zmm2, xmm2
        vpmovzxwq zmm3, xmm3
        vpaddq  zmm0, zmm0, zmm2
        vpaddq  zmm1, zmm1, zmm3
        vpaddq  zmm0, zmm0, [rdi+0x2*0x40]
        vpaddq  zmm1, zmm1, [rdi+0x3*0x40]
        vmovdqu64 [rdi+0x2*0x40], zmm0
        vmovdqu64 [rdi+0x3*0x40], zmm1
        ret

# not ported to uint32_t flags yet
	.balign 16
accum64:
        .type   accum64, @function
        vpmovzxwq zmm3, xmm8
        vpmovzxwq zmm4, xmm9
        vpaddq  zmm3, zmm3, [rdi]
        vpaddq  zmm4, zmm4, [rdi+0x1*0x40]
        vmovdqu64 [rdi], zmm3
        vmovdqu64 [rdi+0x1*0x40], zmm4
        vextracti64x2 xmm3, zmm8, 0x01
        vextracti64x2 xmm4, zmm9, 0x01
        vpmovzxwq zmm3, xmm3
        vpmovzxwq zmm4, xmm4
        vpaddq  zmm3, zmm3, [rdi+0x2*0x40]
        vpaddq  zmm4, zmm4, [rdi+0x3*0x40]
        vmovdqu64 [rdi+0x2*0x40], zmm3
        vmovdqu64 [rdi+0x3*0x40], zmm4
        vextracti64x2 xmm3, zmm8, 0x02
        vextracti64x2 xmm4, zmm9, 0x02
        vpmovzxwq zmm3, xmm3
        vpmovzxwq zmm4, xmm4
        vpaddq  zmm3, zmm3, [rdi+0x4*0x40]
        vpaddq  zmm4, zmm4, [rdi+0x5*0x40]
        vmovdqu64 [rdi+0x4*0x40], zmm3
        vmovdqu64 [rdi+0x5*0x40], zmm4
        vextracti64x2 xmm3, zmm8, 0x03
        vextracti64x2 xmm4, zmm9, 0x03
        vpmovzxwq zmm3, xmm3
        vpmovzxwq zmm4, xmm4
        vpaddq  zmm3, zmm3, [rdi+0x6*0x40]
        vpaddq  zmm4, zmm4, [rdi+0x7*0x40]
        vmovdqu64 [rdi+0x6*0x40], zmm3
        vmovdqu64 [rdi+0x7*0x40], zmm4
        ret

	.balign 16
	.globl count8avx512
        .type   count8avx512, @function
count8avx512:
	push	rbp
	mov	rbp, rsp
	push	rbx
	mov	rcx, rdx
        lea     rbx, [accum8+rip]
        call    countavx512
	pop	rbx
	pop	rbp
        ret
        .size   count8avx512, .-count8avx512

	.balign 16
	.globl count16avx512
        .type   count16avx512, @function
count16avx512:
	push	rbp
	mov	rbp, rsp
	push	rbx
	mov	rcx, rdx
        lea     rbx, [accum16+rip]
	shl     rcx, 1
        call    countavx512
	pop	rbx
	pop	rbp
        ret
        .size   count16avx512, .-count16avx512

	.balign 16
	.globl count32avx512
        .type   count32avx512, @function
count32avx512:
	push	rbp
	mov	rbp, rsp
	push	rbx
	mov	rcx, rdx
        lea     rbx, [accum32+rip]
	shl	rcx, 2
        call    countavx512
	pop	rbx
	pop	rbp
        ret
        .size   count32avx512, .-count32avx512

	.balign 16
	.globl count64avx512
        .type   count64avx512, @function
count64avx512:
	push	rbp
	mov	rbp, rsp
	push	rbx
	mov	rcx, rdx
        lea     rbx, [accum64+rip]
	shl	rcx, 3
        call    countavx512
	pop	rbx
	pop	rbp
        ret
        .size   count64avx512, .-count64avx512
