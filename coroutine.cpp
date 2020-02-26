#include "coroutine.h"
#include <list>
#include <memory>
#include <unordered_set>

class Scheduler;

static thread_local std::unique_ptr<Scheduler> g_scheduler;

class Scheduler {
   public:
    Scheduler(size_t stack_size, size_t max_free_co_list_size)
        : stack_size_(stack_size),
          max_free_co_list_size_(max_free_co_list_size) {
        main_co_ = new Coroutine(0);
        main_co_->status == COROUTINE_RUNNING;
        running_co_ = main_co_;
    }

    Coroutine* FindFreeCoroutine() {
        Coroutine* co;
        if (free_co_list_.empty()) {
            co = new Coroutine(stack_size_);
        } else {
            co = free_co_list_.back();
            free_co_list_.pop_back();
        }
        running_co_set_.insert(co);
        return co;
    }

    Coroutine* CoCreate(const CoroutineCallBack& cb = CoroutineCallBack()) {
        Coroutine* co = FindFreeCoroutine();
        co->status = COROUTINE_SUSPEND;
        if (cb) {
            co->cb = cb;
            co->ctx.stack = co->stack;
            co->ctx.stack_size = stack_size_;
            co_makecontext(&co->ctx, CoFunction);
        }
        return co;
    }

    void CoResume(Coroutine* co) {
        if (co->status == COROUTINE_SUSPEND) {
            Coroutine* cur_co = running_co_;
            cur_co->status = COROUTINE_RESUME_OTHER;
            co->pre_co = cur_co;
            co->status = COROUTINE_RUNNING;
            running_co_ = co;
            co_swapcontext(&cur_co->ctx, &co->ctx);
        } else {
            // TODO: error, can't resume coroutine whose status is not suspend
            fprintf(stderr,
                    "can't resume coroutine whose status is not suspend\n");
        }
    }

    void CoYield() {
        if (running_co_ == main_co_) {
            return;
        }

        Coroutine* cur_co = running_co_;
        Coroutine* pre_co = running_co_->pre_co;
        cur_co->status = COROUTINE_SUSPEND;
        pre_co->status = COROUTINE_RUNNING;
        running_co_ = pre_co;

        co_swapcontext(&cur_co->ctx, &pre_co->ctx);
    }

    static void CoFunction() {
        Scheduler* scheduler = g_scheduler.get();
        Coroutine* co = scheduler->running_co_;
        co->cb();
        co->cb = nullptr;

        co->status = COROUTINE_DEAD;
        Coroutine* pre_co = co->pre_co;
        pre_co->status = COROUTINE_RUNNING;
        scheduler->running_co_ = pre_co;
        scheduler->running_co_set_.erase(co);
        if (scheduler->free_co_list_.size() >
            scheduler->max_free_co_list_size_) {
            delete co;
        } else {
            scheduler->free_co_list_.push_back(co);
        }

        co_setcontext(&pre_co->ctx);
    }

    Coroutine* CoRunning() {
        return running_co_;
    }

    ~Scheduler() {
        for (Coroutine* co : running_co_set_) {
            delete co;
        }
        for (Coroutine* co : free_co_list_) {
            delete co;
        }
    }

   private:
    size_t stack_size_;
    size_t max_free_co_list_size_;
    Coroutine* main_co_;
    Coroutine* running_co_;
    std::unordered_set<Coroutine*> running_co_set_;
    std::list<Coroutine*> free_co_list_;
};

void Coroutine::InitCoroutineEnv(size_t stack_size,
                                 size_t max_free_co_list_size) {
    g_scheduler.reset(new Scheduler(stack_size, max_free_co_list_size));
}

Coroutine* Coroutine::Create(const CoroutineCallBack& cb) {
    return g_scheduler->CoCreate(cb);
}

void Coroutine::Resume() {
    g_scheduler->CoResume(this);
}

void Coroutine::Yield() {
    g_scheduler->CoYield();
}

int Coroutine::Status() {
    return status;
}

Coroutine* Coroutine::GetRunningCoroutine() {
    return g_scheduler->CoRunning();
}

Coroutine* Coroutine::Go(const CoroutineCallBack& cb) {
    Coroutine* co = Coroutine::Create(cb);
    co->Resume();
    return co;
}