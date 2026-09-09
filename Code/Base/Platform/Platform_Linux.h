#pragma once
#if EE_PLATFORM_LINUX

//-------------------------------------------------------------------------
// Linux Platform Defines
//-------------------------------------------------------------------------

#ifndef EE_FORCE_INLINE
    #define EE_FORCE_INLINE inline __attribute__((always_inline))
#endif

//-------------------------------------------------------------------------
// Enable specific warnings — GCC/Clang equivalents
//-------------------------------------------------------------------------

// GCC/Clang don't use MSVC pragma warning numbers; keep no-ops for compatibility

//-------------------------------------------------------------------------
// Dev Defines
//-------------------------------------------------------------------------

#define EE_DISABLE_OPTIMIZATION _Pragma("GCC push_options") _Pragma("GCC optimize (\"O0\")")
#define EE_ENABLE_OPTIMIZATION  _Pragma("GCC pop_options")

#if EE_DEVELOPMENT_TOOLS
    #include <signal.h>
    #define EE_DEBUG_BREAK() raise(SIGTRAP)
#endif

// POSIX feature macros
#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#endif
