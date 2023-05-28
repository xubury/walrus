#include <core/thread.h>
#include <core/platform.h>
#include <core/memory.h>
#include <core/semaphore.h>

#if WR_PLATFORM == WR_PLATFORM_WINDOWS
#include <windows.h>
#endif

struct Walrus_Thread {
    Walrus_ThreadFn   fn;
    Walrus_Semaphore* sem;
    bool              running;
    void*             userdata;
    u32               stack_size;

#if WR_PLATFORM == WR_PLATFORM_WINDOWS
    HANDLE handle;
    DWORD  thread_id;
    DWORD  exitcode;
#endif
};

static i32 thread_entry(Walrus_Thread* thread)
{
    walrus_semaphore_post(thread->sem, 1);
    i32 result = thread->fn(thread, thread->userdata);
    return result;
}

#if WR_PLATFORM == WR_PLATFORM_WINDOWS
static DWORD thread_fn(void* arg)
{
    Walrus_Thread* thread = (Walrus_Thread*)arg;
    i32            result = thread_entry(thread);
    return result;
}
#else
#error "Unsupported platform"
#endif

Walrus_Thread* walrus_thread_create(void)
{
    Walrus_Thread* thread = walrus_new(Walrus_Thread, 1);

    thread->fn         = NULL;
    thread->running    = false;
    thread->userdata   = NULL;
    thread->stack_size = 0;

    thread->handle    = INVALID_HANDLE_VALUE;
    thread->thread_id = UINT32_MAX;
    thread->exitcode  = 0;

    return thread;
}

void walrus_thread_destroy(Walrus_Thread* thread)
{
    if (thread->running) {
        walrus_thread_shutdown(thread);
    }
    walrus_semaphore_destroy(thread->sem);
    walrus_free(thread);
}

bool walrus_thread_init(Walrus_Thread* thread, Walrus_ThreadFn fn, void* userdata, u32 stack_size)
{
    thread->fn         = fn;
    thread->userdata   = userdata;
    thread->stack_size = stack_size;
    thread->sem        = walrus_semaphore_create();

#if WR_PLATFORM == WR_PLATFORM_WINDOWS
    thread->handle = CreateThread(NULL, stack_size, thread_fn, thread, 0, NULL);
    if (thread->handle == NULL) {
        return false;
    }
#else
#error "Unsupported platform"
#endif

    thread->running = true;
    walrus_semaphore_wait(thread->sem, -1);

    return true;
}

void walrus_thread_shutdown(Walrus_Thread* thread)
{
    if (!thread->running) {
        return;
    }
#if WR_PLATFORM == WR_PLATFORM_WINDOWS
    WaitForSingleObject(thread->handle, INFINITE);
    GetExitCodeThread(thread->handle, &thread->exitcode);
    CloseHandle(thread->handle);
    thread->handle = NULL;
#else
#error "Unsupported platform"
#endif

    thread->running = false;
}
