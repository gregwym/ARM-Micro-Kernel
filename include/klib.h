/*
 * Kernel Library
 */
#ifndef __KLIB_H__
#define __KLIB_H__


void assert(int boolean, char * msg);

/* String functions */
int strcmp(const char *src, const char *dst);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, int n);

/* Print */
#include <intern/bwio.h>
#include <intern/debug.h>


#endif // __KLIB_H__
