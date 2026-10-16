// Forced into the sljit build on the Switch, where homebrew can't map executable memory.
// sljit then fails to generate code, which makes code mods fail to load cleanly.
#ifndef BM64_SLJIT_SWITCH_CONFIG_H
#define BM64_SLJIT_SWITCH_CONFIG_H

#define SLJIT_EXECUTABLE_ALLOCATOR 0
#define SLJIT_UTIL_STACK 0
#define SLJIT_MALLOC_EXEC(size, exec_allocator_data) ((void*)0)
#define SLJIT_FREE_EXEC(ptr, exec_allocator_data) ((void)0)

#endif
