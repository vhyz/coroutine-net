.global co_swapcontext 
co_swapcontext: 
    movq %rbx, 0(%rdi) 
    movq %rbp, 8(%rdi) 
    movq %r12, 16(%rdi) 
    movq %r13, 24(%rdi) 
    movq %r14, 32(%rdi) 
    movq %r15, 40(%rdi) 
    movq (%rsp), %rcx   
    movq %rcx, 48(%rdi)  // ret address
    leaq 8(%rsp), %rcx 
    movq %rcx, 56(%rdi)  // stack pointer
    fnstcw 64(%rdi) 
    stmxcsr 68(%rdi)
 
    movq 0(%rsi), %rbx 
    movq 8(%rsi), %rbp 
    movq 16(%rsi), %r12 
    movq 24(%rsi), %r13 
    movq 32(%rsi), %r14 
    movq 40(%rsi), %r15 
    movq 48(%rsi), %rax
    movq 56(%rsi), %rsp 
    fldcw 64(%rsi)    
    ldmxcsr 68(%rsi)  
    jmp  *%rax            // jmp ret address

.global co_setcontext 
co_setcontext: 
    movq 0(%rdi), %rbx 
    movq 8(%rdi), %rbp 
    movq 16(%rdi), %r12 
    movq 24(%rdi), %r13 
    movq 32(%rdi), %r14 
    movq 40(%rdi), %r15 
    movq 48(%rdi), %rax
    movq 56(%rdi), %rsp 
    fldcw 64(%rdi)    
    ldmxcsr 68(%rdi)  
    jmp  *%rax            // jmp ret address

.global co_save_fpucw_mxcsr
co_save_fpucw_mxcsr:
    fnstcw 0(%rdi) 
    stmxcsr 4(%rdi)
    ret