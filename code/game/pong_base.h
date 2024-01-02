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

#define STRINGIFY_HELPER(_s) #_s
#define STRINGIFY(_x) STRINGIFY_HELPER(_x)

#define CAST(_x) (_x)

#define ARRAY_COUNT(_x) (sizeof((_x)) / sizeof((_x[0]))) /* in number of elements */

#define MIN(_x, _y) ((_x) < (_y) ? (_x) : (_y))

#define ABS(_x) ( ((_x) < 0 ? -(_x) : (_x)) )

#define CLAMP(_x, _min, _max) ( ((_x) < (_min)) ? (_min) : (((_x) > (_max)) ? (_max) : (_x)) )

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

#endif //PONG_BASE_H
