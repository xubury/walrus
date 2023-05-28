#include <engine/thread_pool.h>
#include <core/memory.h>
#include <core/list.h>
#include <core/queue.h>
#include <core/thread.h>
#include <core/mutex.h>
#include <core/macro.h>
#include <core/assert.h>

typedef struct {
    Walrus_List      *workers;
    Walrus_Queue     *tasks;
    u8                num_threads;
    Walrus_Mutex     *mutex;
    Walrus_Semaphore *sem;
    bool              stop;
} ThreadPool;

struct Walrus_TaskResult {
    i32               exitcode;
    Walrus_Semaphore *sem;
};

typedef struct {
    ThreadTaskFn fn;
    void        *userdata;
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

            task->fn(task->userdata);
            walrus_free(task);
        }
    }
    return 0;
}

void walrus_thread_pool_init(u8 num_threads)
{
    s_pool              = walrus_new(ThreadPool, 1);
    s_pool->workers     = NULL;
    s_pool->tasks       = walrus_queue_alloc();
    s_pool->sem         = walrus_semaphore_create();
    s_pool->mutex       = walrus_mutex_create();
    s_pool->num_threads = num_threads;
    s_pool->stop        = false;

    for (u8 i = 0; i < num_threads; ++i) {
        Walrus_Thread *thread = walrus_thread_create();
        walrus_thread_init(thread, worker_fn, NULL, 0);
        s_pool->workers = walrus_list_append(s_pool->workers, thread);
    }
}

void walrus_thread_pool_shutdown(void)
{
    walrus_mutex_lock(s_pool->mutex);
    s_pool->stop = true;
    walrus_semaphore_post(s_pool->sem, s_pool->num_threads);
    walrus_mutex_unlock(s_pool->mutex);

    Walrus_List *node = s_pool->workers;
    while (node) {
        Walrus_Thread *thread = node->data;
        walrus_thread_shutdown(thread);
        node = node->next;
    }

    walrus_mutex_destroy(s_pool->mutex);
    walrus_semaphore_destroy(s_pool->sem);
    walrus_queue_free(s_pool->tasks);
    walrus_list_free(s_pool->workers);
    walrus_free(s_pool);
    s_pool = NULL;
}

void walrus_thread_pool_queue(ThreadTaskFn func, void *userdata)
{
    ThreadTask *task = walrus_new(ThreadTask, 1);
    task->fn         = func;
    task->userdata   = userdata;
    walrus_mutex_lock(s_pool->mutex);
    walrus_queue_push(s_pool->tasks, task);
    walrus_mutex_unlock(s_pool->mutex);
    walrus_semaphore_post(s_pool->sem, 1);
}
