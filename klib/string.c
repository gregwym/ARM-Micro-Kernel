#include <klib.h>

void *memcpy(void *dst, const void *src, size_t len)
{
	size_t i;

	if ((uintptr_t)dst % sizeof(long) == 0 &&
	    (uintptr_t)src % sizeof(long) == 0 &&
	    len % sizeof(long) == 0) {

		long *d = dst;
		const long *s = src;

		for (i=0; i<len/sizeof(long); i++) {
			d[i] = s[i];
		}
	}
	else {
		char *d = dst;
		const char *s = src;

		for (i=0; i<len; i++) {
			d[i] = s[i];
		}
	}

	return dst;
}

void *memset(void *ptr, int ch, size_t len)
{
        char *p = ptr;
        size_t i;

        for (i=0; i<len; i++) {
                p[i] = ch;
        }

        return ptr;
}

int strcmp(const char *src, const char *dst) {
	int ret = 0;
	while( ! (ret = *(unsigned char *)src - *(unsigned char *)dst) && *dst) ++src, ++dst;
	if ( ret < 0 ) ret = -1 ;
	else if ( ret > 0 ) ret = 1 ;
	return( ret );
}

char *strncpy(char *dst, const char *src, size_t n) {
	char *start = dst;
	n--;
	while (n && *src){
		n--;
		*dst++ = *src++;
	}
	*dst = '\0';
	return start;
}

size_t strlen(const char *s) {
  const char *eos = s;
  while (*eos++);
  return (size_t) (eos - s - 1);
}
