#ifndef BP_H
#define BP_H
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void *alloc_pattern(const char *s, int repeat)
{
	size_t len = strlen(s);
	size_t count = len * repeat;
	int *d = malloc(count * sizeof(int));
	assert(d != NULL);
	for (size_t i = 0; i < len; i++)
		d[i] = s[i] == '0' ? 0 : 1;
	for (size_t i = len; i < count; i += len)
		memcpy(&d[i], &d[0], len * sizeof(int));
	return d;
}

#endif
