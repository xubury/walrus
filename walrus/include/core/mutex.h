#pragma once

typedef struct Walrus_Mutex Walrus_Mutex;

Walrus_Mutex *walrus_mutex_create(void);
void          walrus_mutex_destroy(Walrus_Mutex *mutex);

void walrus_mutex_lock(Walrus_Mutex *mutex);
void walrus_mutex_unlock(Walrus_Mutex *mutex);
