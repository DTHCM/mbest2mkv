#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "util.h"

void *xmalloc(size_t size)
{
	void *p;
	p = malloc(size);

	if (!p) {
		fprintf(stderr, "Out of memory.\n");
		exit(12);
	}

	return p;
}

void *xcalloc(size_t nmemb, size_t size)
{
	void *p;
	p = calloc(nmemb, size);

	if (!p) {
		fprintf(stderr, "Out of memory.\n");
		exit(12);
	}

	return p;
}

bool readfile(const char *file, char **buf, size_t *len, bool text)
{
	FILE *f = fopen(file, text ? "r" : "rb");
	if (!f)
		return false;

	fseek(f, 0, SEEK_END);
	size_t f_size = ftell(f);
	*len = f_size + 1;
	rewind(f);
	*buf = xmalloc(*len);

	fread(*buf, 1, f_size, f);
	fclose(f);

	(*buf)[*len-1] = '\0';

	return true;
}

