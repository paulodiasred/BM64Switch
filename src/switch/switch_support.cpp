// Platform glue for the Nintendo Switch (libnx/newlib).

#include <cstdio>
#include <pthread.h>
#include <switch.h>

extern "C" {
    // newlib declares these, but libnx doesn't implement them. fmt only uses them to keep a single print call from
    // interleaving with other threads' output, so they can do nothing.
    void flockfile(FILE*) {}
    void funlockfile(FILE*) {}

    // libnx gives threads a 128 KiB stack unless asked otherwise, and std::thread can't ask. The recompiled game code
    // and the GL driver need far more (desktop platforms give 1-8 MiB), so the link wraps pthread_create
    // (-Wl,--wrap=pthread_create) to give every thread without an explicit size a larger stack.
    constexpr size_t default_thread_stack_size = 2 * 1024 * 1024;

    // libnx TLS slots (also backing pthread keys) live in the thread's hardware TLS region from offset 0x108 to 0x1E0,
    // and libnx doesn't clear them for new threads, so they hold whatever the previous user of that region left.
    // SDL keeps its per-thread data in one, and reading a stale value there crashes, so start every thread with all
    // slots cleared, as pthread keys require.
    constexpr s32 tls_slot_count = (0x1E0 - 0x108) / sizeof(void*);

    struct ThreadStart {
        void* (*start_routine)(void*);
        void* arg;
    };

    static void* thread_trampoline(void* param) {
        ThreadStart start = *static_cast<ThreadStart*>(param);
        delete static_cast<ThreadStart*>(param);
        for (s32 slot = 0; slot < tls_slot_count; slot++) {
            threadTlsSet(slot, nullptr);
        }
        return start.start_routine(start.arg);
    }

    int __real_pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg);

    int __wrap_pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg) {
        pthread_attr_t sized_attr;
        size_t stack_size = 0;
        if (attr != nullptr) {
            sized_attr = *attr;
            pthread_attr_getstacksize(&sized_attr, &stack_size);
        } else {
            pthread_attr_init(&sized_attr);
        }
        if (stack_size == 0) {
            pthread_attr_setstacksize(&sized_attr, default_thread_stack_size);
        }
        ThreadStart* start = new ThreadStart{ start_routine, arg };
        int result = __real_pthread_create(thread, &sized_attr, thread_trampoline, start);
        if (result != 0) {
            delete start;
        }
        return result;
    }
}
