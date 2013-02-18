#ifndef _KERN_TYPES_H_
#define _KERN_TYPES_H_

typedef char      int8_t;		/* 8-bit signed integer */
typedef short     int16_t;		/* 16-bit signed integer */
typedef int       int32_t;		/* 32-bit signed integer */

typedef unsigned char      u_int8_t;	/* 8-bit unsigned integer */
typedef unsigned short     u_int16_t;	/* 16-bit unsigned integer */
typedef unsigned int       u_int32_t;	/* 32-bit unsigned integer */

typedef unsigned size_t;		/* Size of a memory region */
typedef int intptr_t;			/* Signed pointer-sized integer */
typedef unsigned int uintptr_t;	/* Unsigned pointer-sized integer */

/*
 * Number of bits per byte.
 */

#define CHAR_BIT  8

/*
 * Null pointer.
 */

#undef NULL
#define NULL ((void *)0)

#define TRUE	0xffffffff
#define FALSE	0x00000000

typedef int32_t off_t;   /* Offset within file */
typedef int32_t tid_t;   /* Task ID */
typedef int32_t time_t;  /* Time in seconds */

#endif // _KERN_TYPES_H_
