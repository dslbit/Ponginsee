#ifndef PONG_BASE_H
#define PONG_BASE_H

/* NOTE: CLANG compiler identification */
#if defined(__clang__)
#define COMPILER_CLANG 1
#endif

/* NOTE: MSVC compiler identification */
#if !defined(__clang__) && defined(_MSC_VER)
#define COMPILER_MSVC 1
#endif

/* NOTE: GCC compiler identification */
#if !defined(__clang__) && !defined(_MSC_VER) && defined(__GNUC__)
#define COMPILER_GCC 1
#endif

/* NOTE: Supported compiler NOT FOUND */
#if !defined(__clang__) && !defined(_MSC_VER) && !defined(__GNUC__)
#error A SUPPORTED COMPILER WAS NOT FOUND! SUPPORTED COMPILERS: CLANG/MSVC/GCC - Only tested on MSVC
#endif

/* NOTE: C89 inline macro only for MSVC compiler - TODO: inline macro for other compilers (when testing on them) */
#if !defined(__cplusplus) && (__STDC_VERSION__ < 199901L)

#if defined(MSVC_COMPILER)
#define INLINE __inline
#else
#define INLINE
#endif

#else
#define INLINE inline
#endif /* !defined(__cplusplus) && (__STDC_VERSION__ < 199901L) */

#define FALSE 0
#define TRUE 1

#define INTERNAL static
#define LOCAL static
#define GLOBAL static

#define PI 3.1415926535f

#define KILOBYTE(_x) ((_x) * 1024LL)
#define MEGABYTE(_x) (KILOBYTE((_x)) * 1024LL)
#define GIGABYTE(_x) (MEGABYTE((_x)) * 1024LL)

#define STRINGIFY_HELPER(_s) #_s
#define STRINGIFY(_x) STRINGIFY_HELPER(_x)

#define CAST(_x) (_x)

#define ARRAY_COUNT(_x) (sizeof((_x)) / sizeof((_x[0]))) /* in number of elements */

#define MIN(_x, _y) ((_x) < (_y) ? (_x) : (_y))

#define ABS(_x) ( ((_x) < 0 ? -(_x) : (_x)) )

#define CLAMP(_x, _min, _max) ( ((_x) < (_min)) ? (_min) : (((_x) > (_max)) ? (_max) : (_x)) )

#define SIGN_OF(_x) (((_x) < 0) ? (-1) : (1))

#if defined(__cplusplus)
#define EXTERN_OPEN extern "C" {
#else
#define EXTERN_OPEN
#endif

#if defined(__cplusplus)
#define EXTERN_CLOSE }
#else
#define EXTERN_CLOSE
#endif

#if defined(__cplusplus)
#define EXTERNIZE extern "C"
#else
#define EXTERNIZE
#endif

#define ASSERT(_exp, _msg) if (!(_exp)) { *((int *) 0) = 0; }

/* Pre-defined C/C++ Macros: https://sourceforge.net/p/predef/wiki/Home/ */
#if ( defined(__STDC__) || (__STDC_VERSION__ < 199901L) ) || ( defined(__cplusplus) && (__cplusplus < 201103L) )
typedef signed   int        B32;

typedef unsigned char       U8;
typedef unsigned short      U16;
typedef unsigned int        U32;
typedef unsigned long long  U64;

typedef signed   char       S8;
typedef signed   short      S16;
typedef signed   int        S32;
typedef signed   long long  S64;

typedef float               F32;
typedef double              F64;
#else
#include <stdint.h>

typedef int32_t  B32;

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t   S8;
typedef int16_t  S16;
typedef int32_t  S32;
typedef int64_t  S64;

typedef float    F32;
typedef double   F64;
#endif /* #if (__STDC_VERSION__ < 199901L) */

#define UINT32_MAX (0xffffffff)

INTERNAL INLINE void debug_zero_array(void *array, U64 size) { /* size in bytes */
  U64 i;
  
  for (i = 0; i < size; ++i) {
    *(CAST(U8 *)array + i) = 0;
  }
}

extern U32 global_random_seed = 1;

/* NOTE: Generate a random number between 0.0 and 1.0 - Xorshift */
INTERNAL INLINE F32 random_f32() {
  F32 result;
  U32 x;
  
  x = global_random_seed;
  x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
  global_random_seed = x;
  result = CAST(F32) (CAST(F64) x / CAST(F64) UINT32_MAX);
  return result;
}

/* NOTE: Generate a random number in range - using Xorshift */
INTERNAL INLINE F32 random_f32_range(F32 min, F32 max) {
  F32 result;
  
  result = (max - min) * random_f32() + min;
  return result;
}

/* NOTE: I really need a string library */
INTERNAL U64 debug_string_len(S8 *str) {
  U64 result;
  for (result = 0; *str; result++, str++);
  return result;
}

#endif /* PONG_BASE_H */
