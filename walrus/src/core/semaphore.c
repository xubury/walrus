#include <core/semaphore.h>
#include <core/platform.h>
#include <core/memory.h>
#include <core/assert.h>

#if WR_PLATFORM == WR_PLATFORM_WINDOWS
#include <windows.h>
#endif

struct Walrus_Semaphore {
#if WR_PLATFORM == WR_PLATFORM_WINDOWS
    HANDLE handle;
#endif
};

Walrus_Semaphore *walrus_semaphore_create(void)
{
    Walrus_Semaphore *sem = walrus_new(Walrus_Semaphore, 1);

#if WR_PLATFORM == WR_PLATFORM_WINDOWS
    sem->handle = CreateSemaphoreA(NULL, 0, LONG_MAX, NULL);
    walrus_assert(sem->handle != NULL);
#else
#error "Unsupported platform"
#endif

    return sem;
}

void walrus_semaphore_destroy(Walrus_Semaphore *semaphore)
{
#if WR_PLATFORM == WR_PLATFORM_WINDOWS
    CloseHandle(semaphore);
#else
#error "Unsupported platform"
#endif
    walrus_free(semaphore);
}

bool walrus_semaphore_wait(Walrus_Semaphore *semaphore, i32 ms)
{
#if WR_PLATFORM == WR_PLATFORM_WINDOWS
    DWORD milliseconds = (ms < 0) ? INFINITE : ms;
    return WaitForSingleObject(semaphore->handle, milliseconds) == WAIT_OBJECT_0;
#else
#error "Unsupported platform"
#endif
}

void walrus_semaphore_post(Walrus_Semaphore *semaphore, u32 count)
{
#if WR_PLATFORM == WR_PLATFORM_WINDOWS
    ReleaseSemaphore(semaphore->handle, count, NULL);
#else
#error "Unsupported platform"
#endif
}
