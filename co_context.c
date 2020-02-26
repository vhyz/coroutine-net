#include "co_context.h"
#include <stdint.h>

#define RBX 0
#define RBP 1
#define R12 2
#define R13 3
#define R14 4
#define R15 5
#define RIP 6
#define RSP 7

void co_makecontext(co_context* ctx, context_func func) {
    char* rsp = ctx->stack + ctx->stack_size;
    rsp = (char*)((((uintptr_t)rsp) & -16L) - 8);
    ctx->regs[RSP] = rsp;
    ctx->regs[RIP] = func;
}
