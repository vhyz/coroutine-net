#include "co_context.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RBX 0
#define RBP 1
#define R12 2
#define R13 3
#define R14 4
#define R15 5
#define RIP 6
#define RSP 7
#define CW_MXCSR 8

static __thread void* g_fpucw_mxcsr;

void co_context_thread_init() {
    co_save_fpucw_mxcsr(&g_fpucw_mxcsr);
}

void co_makecontext(co_context* ctx, context_func func) {
    memset(ctx->regs, 0, sizeof(ctx->regs));
    char* rsp = ctx->stack + ctx->stack_size;
    rsp = (char*)((((uintptr_t)rsp) & -16L) - 8);
    ctx->regs[RSP] = rsp;
    ctx->regs[RIP] = func;
    ctx->regs[CW_MXCSR] = g_fpucw_mxcsr;
}
