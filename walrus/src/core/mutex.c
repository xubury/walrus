#include <core/mutex.h>
#include <core/memory.h>
#include <core/platform.h>

#if WR_PLATFORM == WR_PLATFORM_WINDOWS
#include <windows.h>
#endif

struct _Walrus_Mutex {
#if WR_PLATFORM == WR_PLATFORM_WINDOWS
    CRITICAL_SECTION section;
#endif
};

Walrus_Mutex *walrus_mutex_create(void)
{
    Walrus_Mutex *mutex = walrus_new(Walrus_Mutex, 1);

#if WR_PLATFORM == WR_PLATFORM_WINDOWS
    InitializeCriticalSection(&mutex->section);
#else
#error "Unsupported platform"
#endif

    return mutex;
}

void walrus_mutex_destroy(Walrus_Mutex *mutex)
{
#if WR_PLATFORM == WR_PLATFORM_WINDOWS
    DeleteCriticalSection(&mutex->section);
#else
#error "Unsupported platform"
#endif
    walrus_free(mutex);
}

void walrus_mutex_lock(Walrus_Mutex *mutex)
{
#if WR_PLATFORM == WR_PLATFORM_WINDOWS
    EnterCriticalSection(&mutex->section);
#else
#error "Unsupported platform"
#endif
}

void walrus_mutex_unlock(Walrus_Mutex *mutex)
{
#if WR_PLATFORM == WR_PLATFORM_WINDOWS
    LeaveCriticalSection(&mutex->section);
#else
#error "Unsupported platform"
#endif
}
