#include <stdio.h>
#include "coroutine.h"

int main() {
    coroutine_env_init(0);
    int co1 = coroutine_create([]() {
        printf("A\n");
        coroutine_yield();
        printf("C\n");
        coroutine_yield();
        printf("E\n");
    });
    int co2 = coroutine_create([]() {
        printf("B\n");
        coroutine_yield();
        printf("D\n");
        coroutine_yield();
        printf("F\n");
    });
    while (coroutine_status(co1) && coroutine_status(co2)) {
        coroutine_resume(co1);
        coroutine_resume(co2);
    }
    coroutine_env_destory();
    return 0;
}