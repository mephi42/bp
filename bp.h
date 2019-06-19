#ifndef BP_H
#define BP_H
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
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

#if INTPTR_MAX == INT32_MAX
#define GAS_PTR ".long"
#else
#define GAS_PTR ".quad"
#endif

#define SECTION_BPS(s) "\t.section bps, \"a\", @progbits\n" s "\t.previous\n"

#define START_BPS()                                                            \
	extern void (*bps_start)(long, int *);                                 \
	__asm__(SECTION_BPS("bps_start:\n"))

#define DEFINE_BP(name, offset1, body1, offset12, body2)                       \
	extern void name(long, int *);                                         \
	__asm__("\t.align 0x1000\n"                                            \
		"\t.org .+" #offset1 "\n"                                      \
		".globl " #name "\n"                                           \
		"\t.type " #name ", @function\n" #name ":\n" body1             \
		"\t.org .+" #offset12                                          \
		"\n" body2 SECTION_BPS("\t" GAS_PTR " " #name "\n"))

#define END_BPS(pattern_s, repeat)                                             \
	extern void (*bps_end)(long, int *);                                   \
	__asm__(SECTION_BPS("bps_end:\n"));                                    \
	int main(void)                                                         \
	{                                                                      \
		void *pattern = alloc_pattern(pattern_s, repeat);              \
		for (void (**bps)(long, int *) = &bps_start; bps != &bps_end;  \
		     bps++)                                                    \
			(*bps)(repeat, pattern);                               \
	}

#endif
