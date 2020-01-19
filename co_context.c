#include "co_context.h"
#include <stdint.h>

#define RBX 0
#define RBP 1
#define R12 2
#define R13 3
#define R14 4
#define R15 5
#define RDI 6
#define RSI 7
#define RDX 8
#define RCX 9
#define R8 10
#define R9 11
#define RIP 12
#define RSP 13

int co_makecontext(co_context* ctx, context_func func, void* args) {
    char* rsp = ctx->stack + ctx->stack_size;
    rsp = (char*)((((uintptr_t)rsp) & -16L) - 8);
    ctx->regs[RSP] = rsp;
    ctx->regs[RIP] = func;
    ctx->regs[RDI] = args;
}