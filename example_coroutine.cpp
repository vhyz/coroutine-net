#include <stdio.h>
#include "coroutine.h"

int main() {
    Coroutine::InitCoroutineEnv();
    Coroutine* co1 = Coroutine::Create([]() {
        printf("A\n");
        Coroutine::Yield();
        printf("C\n");
        Coroutine::Yield();
        printf("E\n");
    });
    Coroutine* co2 = Coroutine::Create([]() {
        printf("B\n");
        Coroutine::Yield();
        printf("D\n");
        Coroutine::Yield();
        printf("F\n");
    });
    while (co1->Status() && co2->Status()) {
        co1->Resume();
        co2->Resume();
    }
    return 0;
}