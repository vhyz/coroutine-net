#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct {
    void* regs[15];
    char* stack;
    size_t stack_size;
} co_context;

typedef void (*context_func)(void* args);

int co_makecontext(co_context* ctx, context_func func, void* args);
int co_swapcontext(co_context* octx, const co_context* ctx);
int co_setcontext(co_context* ctx);
int co_getcontext(co_context* ctx);

#ifdef __cplusplus
}
#endif