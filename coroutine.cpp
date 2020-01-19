#include "coroutine.h"
#include <cassert>
#include <iostream>
#include <vector>
#include "co_context.h"

#define DEFAULT_SCHEDULE_CAP 1024
#define DEFAULT_STACK_SIZE 128 * 1024

static inline int min(int a, int b) { return a > b ? b : a; }
static inline int max(int a, int b) { return a > b ? a : b; }

static void CoroutineFunction();

class Schedule {
   public:
    Schedule(size_t stackSize)
        : stack_size_(max(stackSize, DEFAULT_STACK_SIZE)),
          num_co_(0),
          running_(-1),
          co_array_(DEFAULT_SCHEDULE_CAP) {
        int main_co_id = coCreate();
        assert(main_co_id == 0);

        co_array_[main_co_id]->status = COROUTINE_RUNNING;
        running_ = main_co_id;
    }

    int findNextCoPositon() {
        int ret = -1;
        if (num_co_ >= co_array_.size()) {
            ret = co_array_.size();
            co_array_.resize(co_array_.size() * 2);
        } else {
            int i;
            for (i = 0; i < co_array_.size(); ++i) {
                int id = (num_co_ + i) % co_array_.size();
                if (co_array_[id] == nullptr) {
                    ret = id;
                    break;
                } else if (co_array_[id]->status == COROUTINE_DEAD) {
                    ret = id;
                    break;
                }
            }
        }
        return ret;
    }

    int coCreate(const CoroutineCallBack& cb = CoroutineCallBack()) {
        int id = findNextCoPositon();
        if (co_array_[id] == nullptr) {
            co_array_[id] = new Coroutine(id == 0 ? 0 : stack_size_);
        }
        Coroutine* co = co_array_[id];
        co->status = COROUTINE_SUSPEND;
        ++num_co_;

        if (cb) {
            assert(co->stack != NULL);
            co->cb = cb;

            co->ctx.stack = co->stack;
            co->ctx.stack_size = stack_size_;
            co_makecontext(&co->ctx, (void (*)(void*))CoroutineFunction, NULL);
        }

        return id;
    }

    void coYield() {
        int id = running_;
        if (id == 0) {
            return;
        }

        Coroutine* co = co_array_[id];
        Coroutine* pre_co = co_array_[co->pre_co];
        co->status = COROUTINE_SUSPEND;
        pre_co->status = COROUTINE_RUNNING;
        running_ = co->pre_co;

        co_swapcontext(&co->ctx, &pre_co->ctx);
    }

    void coResume(int id) {
        Coroutine* co = co_array_[id];
        Coroutine* cur_co = co_array_[running_];

        assert(co->status == COROUTINE_SUSPEND);

        if (co->status == COROUTINE_SUSPEND) {
            cur_co->status = COROUTINE_RESUME_OTHER;
            co->pre_co = running_;
            co->status = COROUTINE_RUNNING;
            running_ = id;
            co_swapcontext(&cur_co->ctx, &co->ctx);
        } else {
            // TODO: error, can't resume coroutine whose status is not suspend
            fprintf(stderr,
                    "can't resume coroutine whose status is not suspend\n");
        }
    }

    int coRunning() { return running_; }

    int coStatus(int id) {
        if (co_array_[id] == NULL) {
            return COROUTINE_DEAD;
        }
        return co_array_[id]->status;
    }

    void coFunction() {
        int id = running_;
        Coroutine* co = co_array_[id];
        co->cb();

        num_co_--;
        co->status = COROUTINE_DEAD;

        Coroutine* pre_co = co_array_[co->pre_co];
        pre_co->status = COROUTINE_RUNNING;
        running_ = co->pre_co;

        co_setcontext(&pre_co->ctx);
    }

    ~Schedule() {
        for (Coroutine* co : co_array_) {
            if (co) {
                delete co;
            }
        }
    }

   private:
    class Coroutine {
       public:
        CoroutineCallBack cb;
        int pre_co;
        char* stack;
        co_context ctx;
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

    size_t stack_size_;
    int num_co_;
    int running_;
    std::vector<Coroutine*> co_array_;
};

static thread_local Schedule* gSchedule;

void CoroutineEnvInit(size_t stsize) { gSchedule = new Schedule(stsize); }

void CoroutineEnvDestory() { delete gSchedule; }

int CoroutineCreate(const CoroutineCallBack& cb) {
    return gSchedule->coCreate(cb);
}

void CoroutineResume(int id) { gSchedule->coResume(id); }

void CoroutineYield() { gSchedule->coYield(); }

int CoroutineRunning() { return gSchedule->coRunning(); }

int CoroutineStatus(int id) { return gSchedule->coStatus(id); }

static void CoroutineFunction() { gSchedule->coFunction(); }

int CoroutineGo(const CoroutineCallBack& cb) {
    int co = CoroutineCreate(cb);
    CoroutineResume(co);
    return co;
}