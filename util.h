#ifndef _UTIL_H
#define _UTIL_H

#include <stdio.h>
#include <stdlib.h>

#define M2M_COLOR(c, b) "\e[" #c ";" #b "m"
#define M2M_NORM "\e[0m"
#define M2M_RED M2M_COLOR(0, 31)

#define M2M_FATAL(s) do { \
	fprintf(stderr, M2M_RED "fatal: " M2M_NORM "%s" "\n", s); \
} while (0)

void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
bool readfile(const char *file, char **buf, size_t *len, bool text);

#endif
