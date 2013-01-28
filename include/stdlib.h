#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <kernel.h>
#include <task.h>
#include <types.h>

void assert(int boolean, char * msg);

int strcmp(const char *src, const char *dst);

char *strcpy(char *dst, const char *src);

char *strncpy(char *dst, const char *src, int n);

int strlen(const char *s) ;
#endif // __STDLIB_H__
