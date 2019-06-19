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

#define START_BPS()                                                            \
	extern void (*bps_start)(long, int *);                                 \
	__asm__("\t.section bps, \"a\",@progbits\n"                            \
		"bps_start:\n"                                                 \
		"\t.previous\n")

#define END_BPS(pattern_s, repeat)                                             \
	extern void (*bps_end)(long, int *);                                   \
	__asm__("\t.section bps, \"a\",@progbits\n"                            \
		"bps_end:\n"                                                   \
		"\t.previous\n");                                              \
	int main(void)                                                         \
	{                                                                      \
		void *pattern = alloc_pattern(pattern_s, repeat);              \
		for (void (**bps)(long, int *) = &bps_start; bps != &bps_end;  \
		     bps++)                                                    \
			(*bps)(repeat, pattern);                               \
	}

#endif
