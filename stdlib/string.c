#include <stdlib.h>

int strcmp(const char *src, const char *dst) {
	int ret = 0;
	while( ! (ret = *(unsigned char *)src - *(unsigned char *)dst) && *dst) ++src, ++dst;
	if ( ret < 0 ) ret = -1 ;
	else if ( ret > 0 ) ret = 1 ;
	return( ret );
}

char *strcpy(char *dst, const char *src) {
	char *cp = dst;
	while (*cp++ = *src++);
	return dst;
}

char *strncpy(char *dst, const char *src, int n) {
	char *start = dst;
	n--;
	while (n){
		n--;
		*dst++ = *src++;
	}
	*dst = '\0';
	return start;
}

int strlen(const char *s) {
  const char *eos = s;
  while (*eos++);
  return (int) (eos - s);
}
