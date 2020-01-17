#include "coroutine.h"
#include <ucontext.h>
#include <cassert>
#include <iostream>
#include <vector>

#define DEFAULT_SCHEDULE_CAP 1024
#define DEFAULT_STACK_SIZE 128 * 1024

static inline int min(int a, int b) { return a > b ? b : a; }
static inline int max(int a, int b) { return a > b ? a : b; }

static void coroutine_function();

class Schedule {
   public:
    Schedule(size_t stackSize)
        : stackSize_(max(stackSize, DEFAULT_STACK_SIZE)),
          coSize_(0),
          running_(-1),
          coArray_(DEFAULT_SCHEDULE_CAP) {
        int mainCoId = coCreate();
        assert(mainCoId == 0);

        coArray_[mainCoId]->status = COROUTINE_RUNNING;
        running_ = mainCoId;
    }

    int findNextCoPositon() {
        int ret = -1;
        if (coSize_ >= coArray_.size()) {
            ret = coArray_.size();
            coArray_.resize(coArray_.size() * 2);
        } else {
            int i;
            for (i = 0; i < coArray_.size(); ++i) {
                int id = (coSize_ + i) % coArray_.size();
                if (coArray_[id] == nullptr) {
                    ret = id;
                    break;
                } else if (coArray_[id]->status == COROUTINE_DEAD) {
                    ret = id;
                    break;
                }
            }
        }
        return ret;
    }

    int coCreate(const CoroutineCallBack& cb = CoroutineCallBack()) {
        int id = findNextCoPositon();
        if (coArray_[id] == nullptr) {
            coArray_[id] = new Coroutine(id == 0 ? 0 : stackSize_);
        }
        Coroutine* co = coArray_[id];
        co->status = COROUTINE_SUSPEND;
        ++coSize_;

        if (cb) {
            assert(co->stack != NULL);
            co->cb = cb;

            getcontext(&co->ctx);
            co->ctx.uc_stack.ss_sp = co->stack;
            co->ctx.uc_stack.ss_size = stackSize_;
            co->ctx.uc_link = nullptr;

            makecontext(&co->ctx, (void (*)(void))coroutine_function, 0);
        }

        return id;
    }

    void coYield() {
        int id = running_;
        if (id == 0) {
            return;
        }

        Coroutine* co = coArray_[id];
        Coroutine* pre_co = coArray_[co->preCo];
        co->status = COROUTINE_SUSPEND;
        pre_co->status = COROUTINE_RUNNING;
        running_ = co->preCo;
        swapcontext(&co->ctx, &pre_co->ctx);
    }

    void coResume(int id) {
        Coroutine* co = coArray_[id];
        Coroutine* curCo = coArray_[running_];

        assert(co->status == COROUTINE_SUSPEND);

        if (co->status == COROUTINE_SUSPEND) {
            curCo->status = COROUTINE_RESUME_OTHER;
            co->preCo = running_;
            co->status = COROUTINE_RUNNING;
            running_ = id;
            swapcontext(&curCo->ctx, &co->ctx);
        } else {
            // TODO: error, can't resume coroutine whose status is not suspend
            fprintf(stderr,
                    "can't resume coroutine whose status is not suspend\n");
        }
    }

    int coRunning() { return running_; }

    int coStatus(int id) {
        if (coArray_[id] == NULL) {
            return COROUTINE_DEAD;
        }
        return coArray_[id]->status;
    }

    void coFunction() {
        int id = running_;
        Coroutine* co = coArray_[id];
        co->cb();

        coSize_--;
        co->status = COROUTINE_DEAD;

        Coroutine* preCo = coArray_[co->preCo];
        preCo->status = COROUTINE_RUNNING;
        running_ = co->preCo;

        setcontext(&preCo->ctx);
    }

    ~Schedule() {
        for (Coroutine* co : coArray_) {
            if (co) {
                delete co;
            }
        }
    }

   private:
    class Coroutine {
       public:
        CoroutineCallBack cb;
        int preCo;
        char* stack;
        ucontext_t ctx;
        int status;

        Coroutine(size_t stack_size) {
            if (stack_size == 0) {
                stack = nullptr;
            } else {
                stack = static_cast<char*>(operator new(stack_size));
            }
        }

        ~Coroutine() {
            if (stack) {
                operator delete(stack);
            }
        }
    };

    size_t stackSize_;
    int coSize_;
    int running_;
    std::vector<Coroutine*> coArray_;
};

static thread_local Schedule* gSchedule;

void coroutine_env_init(size_t stsize) { gSchedule = new Schedule(stsize); }

void coroutine_env_destory() { delete gSchedule; }

int coroutine_create(const CoroutineCallBack& cb) {
    return gSchedule->coCreate(cb);
}

void coroutine_resume(int id) { gSchedule->coResume(id); }

void coroutine_yield() { gSchedule->coYield(); }

int coroutine_running() { return gSchedule->coRunning(); }

int coroutine_status(int id) { return gSchedule->coStatus(id); }

static void coroutine_function() { gSchedule->coFunction(); }

int coroutine_go(const CoroutineCallBack& cb) {
    int co = coroutine_create(cb);
    coroutine_resume(co);
    return co;
}