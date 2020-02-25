.global co_swapcontext 
co_swapcontext: 
    // 保存6个数据寄存器 
    movq %rbx, 0(%rdi) 
    movq %rbp, 8(%rdi) 
    movq %r12, 16(%rdi) 
    movq %r13, 24(%rdi) 
    movq %r14, 32(%rdi) 
    movq %r15, 40(%rdi) 
    // 保存6个参数寄存器 
    movq %rdi, 48(%rdi) 
    movq %rsi, 56(%rdi) 
    movq %rdx, 64(%rdi) 
    movq %rcx, 72(%rdi) 
    movq %r8,  80(%rdi) 
    movq %r9,  88(%rdi) 
    movq (%rsp), %rcx  
    movq %rcx, 96(%rdi)  
    leaq 8(%rsp), %rcx 
    movq %rcx, 104(%rdi) 
    fnstcw 112(%rdi) 
    stmxcsr 116(%rdi)

    // 恢复上下文 
    movq %rsi, %rax 
    movq 0(%rax), %rbx 
    movq 8(%rax), %rbp 
    movq 16(%rax), %r12 
    movq 24(%rax), %r13 
    movq 32(%rax), %r14 
    movq 40(%rax), %r15 
    movq 48(%rax), %rdi 
    movq 56(%rax), %rsi 
    movq 72(%rax), %rcx 
    movq 80(%rax), %r8 
    movq 88(%rax), %r9 
    movq 104(%rax), %rsp 
    movq 96(%rax), %rdx 
    pushq %rdx 
    movq 64(%rax), %rdx  
    fldcw 112(%rax)    
    ldmxcsr 116(%rax)  
    xorl %eax, %eax  
    ret

.global co_setcontext 
co_setcontext: 
    movq %rdi, %rax
    movq 0(%rax), %rbx 
    movq 8(%rax), %rbp 
    movq 16(%rax), %r12 
    movq 24(%rax), %r13 
    movq 32(%rax), %r14 
    movq 40(%rax), %r15 
    
    movq 48(%rax), %rdi 
    movq 56(%rax), %rsi 
    movq 72(%rax), %rcx 
    movq 80(%rax), %r8 
    movq 88(%rax), %r9 
    movq 104(%rax), %rsp 
    movq 96(%rax), %rdx 
    pushq %rdx 
    movq 64(%rax), %rdx  
    fldcw 112(%rax)    
    ldmxcsr 116(%rax)  
    xorl %eax, %eax  
    ret

.global co_getcontext
co_getcontext:
    movq %rbx, 0(%rdi) 
    movq %rbp, 8(%rdi) 
    movq %r12, 16(%rdi) 
    movq %r13, 24(%rdi) 
    movq %r14, 32(%rdi) 
    movq %r15, 40(%rdi) 
    
    movq %rdi, 48(%rdi) 
    movq %rsi, 56(%rdi) 
    movq %rdx, 64(%rdi) 
    movq %rcx, 72(%rdi) 
    movq %r8,  80(%rdi) 
    movq %r9,  88(%rdi) 
    movq (%rsp), %rcx  
    movq %rcx, 96(%rdi)  
    leaq 8(%rsp), %rcx 
    movq %rcx, 104(%rdi) 
    fnstcw 112(%rdi) 
    stmxcsr 116(%rdi)
    xorl %eax, %eax  
    ret