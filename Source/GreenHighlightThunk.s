# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Ultimate-Universe

.text
.globl esm_call_green_highlight
.def esm_call_green_highlight; .scl 2; .type 32; .endef
.seh_proc esm_call_green_highlight
esm_call_green_highlight:
    # Wrapper ABI: RCX=target, RDX=game, R8=building, XMM3=frame interpolation.
    # FUN_14040BD60 ABI: RCX=game, RDX=building, R8D=mode(0), XMM3=frame,
    # hidden live-in R9D=colour(0=green), stack param5=1.
    movq %rcx, %r10
    movq %rdx, %rcx
    movq %r8, %rdx
    xorl %r8d, %r8d
    xorl %r9d, %r9d
    subq $40, %rsp
    .seh_stackalloc 40
    .seh_endprologue
    movl $1, 32(%rsp)
    callq *%r10
    addq $40, %rsp
    retq
.seh_endproc
