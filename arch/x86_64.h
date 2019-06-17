/* SPDX-License-Identifier: GPL-3.0 */
#ifndef X86_64_H
#define X86_64_H
#include <arpa/inet.h>
#include <stdint.h>
#include <x86intrin.h>

#define INSN_ALIGNMENT 1

/* Code generation */
/* %rdi: repeat count */
/* %rsi: branch history */
static const char code1[] = {
	0x83, 0x3E, 0x00, /*			0: cmpl $0,(%rsi)	*/
	0x48, 0x8D, 0x76, 0x04, /*		lea 4(%rsi),%rsi	*/
	0x75, 0xF7, /*				jne 0b			*/
	0xE9, 0xDE, 0xAD, 0xBE, 0xEF, /*	j 1f			*/
};
#define LOOP1_BRANCH_OFFSET 7
#define LINK_BRANCH_OFFSET (sizeof(code1) - 5)

static const char code2[] = {
	0x83, 0x3E, 0x00, /*			1: cmpl $0,(%rsi)	*/
	0x48, 0x8D, 0x76, 0x04, /*		lea 4(%rsi),%rsi	*/
	0x75, 0xF7, /*				jne 1b			*/
	0x48, 0xFF, 0xCF, /*			dec %rdi		*/
	0x0F, 0x85, 0xCA, 0xFE, 0xBA, 0xBE, /*	jnz 0b			*/
	0xC3 /*					ret			*/
};
#define LOOP2_BRANCH_OFFSET 7
#define BACKLINK_BRANCH_OFFSET (sizeof(code2) - 7)

static void link12(char *dst1, char *dst2)
{
	*(int32_t *)(dst1 + LINK_BRANCH_OFFSET + 1) =
		dst2 - (dst1 + LINK_BRANCH_OFFSET + 5);
	*(int32_t *)(dst2 + BACKLINK_BRANCH_OFFSET + 2) =
		dst1 - (dst2 + BACKLINK_BRANCH_OFFSET + 6);
}

/* Time measurements */
struct timer {
	uint16_t *dts;
	uint16_t *dt;
	unsigned long long t0;
};

static void timer_init_1(struct timer *t)
{
}

static void timer_start(struct timer *t)
{
	unsigned int dummy;
	t->t0 = __rdtscp(&dummy);
}

static void timer_end(struct timer *t)
{
	unsigned int dummy;
	unsigned long long dt = __rdtscp(&dummy) - t->t0;
	*t->dt = dt >= 0xFFFF ? 0xFFFF : htons((uint16_t)dt);
	t->dt++;
}

#endif
