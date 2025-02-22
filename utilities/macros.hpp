#pragma once
#include <assert.h>

#ifdef NDEBUG
#define assume(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
#else
#define assume(cond) assert(cond)
#endif

#ifdef _MSC_VER
#define MPD_INLINE(T) __forceinline T 
#define MPD_NOINLINE(T) __declspec(noinline) T
#define MPD_TEMPLATE
#else
#define MPD_INLINE(T)	T __attribute__((always_inline))
#define MPD_NOINLINE(T)	T __attribute__((noinline))
#define MPD_TEMPLATE template
#endif
