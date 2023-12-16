#ifndef PONG_BASE_H
#define PONG_BASE_H

#define FALSE 0
#define TRUE 1

#define INTERNAL static
#define LOCAL static
#define GLOBAL static

#define STRINGIFY(_s) #_s
#define TO_STRING(_x) STRINGIFY(_x)

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

/* TODO: Types and later make it precise using compiler checks, c version checks and 'stdint.h' */
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

#endif //PONG_BASE_H
