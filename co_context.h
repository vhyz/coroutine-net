#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct {
    void* regs[9];
    char* stack;
    size_t stack_size;
} co_context;

typedef void (*context_func)();

void co_context_thread_init();
void co_makecontext(co_context* ctx, context_func func);
void co_swapcontext(co_context* octx, const co_context* ctx);
void co_setcontext(co_context* ctx);
void co_save_fpucw_mxcsr(void **);

#ifdef __cplusplus
}
#endif