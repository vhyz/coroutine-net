#include <stdio.h>
#include "coroutine.h"

int main() {
    CoroutineEnvInit(0);
    int co1 = CoroutineCreate([]() {
        printf("A\n");
        CoroutineYield();
        printf("C\n");
        CoroutineYield();
        printf("E\n");
    });
    int co2 = CoroutineCreate([]() {
        printf("B\n");
        CoroutineYield();
        printf("D\n");
        CoroutineYield();
        printf("F\n");
    });
    while (CoroutineStatus(co1) && CoroutineStatus(co2)) {
        CoroutineResume(co1);
        CoroutineResume(co2);
    }
    CoroutineEnvDestory();
    return 0;
}