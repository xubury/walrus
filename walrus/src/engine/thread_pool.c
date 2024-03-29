#include <engine/thread_pool.h>
#include <core/memory.h>
#include <core/list.h>
#include <core/queue.h>
#include <core/thread.h>
#include <core/mutex.h>
#include <core/macro.h>
#include <core/assert.h>

typedef struct {
    Walrus_Thread   **workers;
    Walrus_Queue     *tasks;
    u8                num_threads;
    Walrus_Mutex     *mutex;
    Walrus_Semaphore *sem;
    bool              stop;
} ThreadPool;

typedef struct {
    ThreadTaskFn         fn;
    void                *userdata;
    Walrus_ThreadResult *res;
} ThreadTask;

static ThreadPool *s_pool;

static i32 worker_fn(Walrus_Thread *self, void *userdata)
{
    walrus_unused(self);
    walrus_unused(userdata);
    while (true) {
        walrus_semaphore_wait(s_pool->sem, -1);
        walrus_mutex_lock(s_pool->mutex);

        if (s_pool->stop) {
            walrus_mutex_unlock(s_pool->mutex);
            break;
        }

        if (s_pool->tasks->length == 0) {
            walrus_mutex_unlock(s_pool->mutex);
        }
        else {
            ThreadTask *task = walrus_queue_pop(s_pool->tasks);
            walrus_mutex_unlock(s_pool->mutex);

            i32 code = task->fn(task->userdata);
            if (task->res) {
                task->res->exit_code = code;
                walrus_semaphore_post(task->res->sem, 1);
            }
            walrus_free(task);
        }
    }
    return 0;
}

void walrus_thread_pool_init(u8 num_threads)
{
    s_pool              = walrus_new(ThreadPool, 1);
    s_pool->workers     = walrus_new(Walrus_Thread *, num_threads);
    s_pool->tasks       = walrus_queue_alloc();
    s_pool->sem         = walrus_semaphore_create();
    s_pool->mutex       = walrus_mutex_create();
    s_pool->num_threads = num_threads;
    s_pool->stop        = false;

    for (u8 i = 0; i < num_threads; ++i) {
        s_pool->workers[i] = walrus_thread_create();
        walrus_thread_init(s_pool->workers[i], worker_fn, NULL, 0);
    }
}

void walrus_thread_pool_shutdown(void)
{
    walrus_mutex_lock(s_pool->mutex);
    s_pool->stop = true;
    walrus_semaphore_post(s_pool->sem, s_pool->num_threads);
    walrus_mutex_unlock(s_pool->mutex);

    for (u8 i = 0; i < s_pool->num_threads; ++i) {
        walrus_thread_shutdown(s_pool->workers[i]);
        walrus_thread_destroy(s_pool->workers[i]);
    }

    walrus_mutex_destroy(s_pool->mutex);
    walrus_semaphore_destroy(s_pool->sem);
    walrus_queue_free(s_pool->tasks);
    walrus_free(s_pool->workers);
    walrus_free(s_pool);
    s_pool = NULL;
}

void walrus_thread_pool_queue(ThreadTaskFn func, void *userdata, Walrus_ThreadResult *res)
{
    ThreadTask *task = walrus_new(ThreadTask, 1);
    task->fn         = func;
    task->userdata   = userdata;
    task->res        = res;
    if (res != NULL) {
        res->sem       = walrus_semaphore_create();
        res->exit_code = 0;
    }
    walrus_mutex_lock(s_pool->mutex);
    walrus_queue_push(s_pool->tasks, task);
    walrus_mutex_unlock(s_pool->mutex);
    walrus_semaphore_post(s_pool->sem, 1);
}

i32 walrus_thread_pool_result_get(Walrus_ThreadResult *res, i32 ms)
{
    if (res->sem) {
        walrus_semaphore_wait(res->sem, ms);
        walrus_semaphore_destroy(res->sem);
        res->sem = NULL;
    }
    return res->exit_code;
}
