/* SPDX-License-Identifier: GPL-3.0 */
#ifndef S390X_H
#define S390X_H
#include <arpa/inet.h>
#include <stdint.h>

#define INSN_ALIGNMENT 2

/* Code generation */
/* %r2: repeat count */
/* %r3: branch history */
static const char code1[] = {
	0xE3, 0x00, 0x30, 0x00, 0x00, 0x12, /*		0: lt %r0,(%r3)	*/
	0x41, 0x30, 0x30, 0x04, /*			la %r3,4(%r3)	*/
	0xC0, 0x74, 0xFF, 0xFF, 0xFF, 0xFB, /*		brcl 7,0b	*/
	0xC0, 0xF4, 0xDE, 0xAD, 0xBE, 0xEF, /*		j 1f		*/
};
#define LOOP1_BRANCH_OFFSET 10
#define LINK_BRANCH_OFFSET (sizeof(code1) - 6)

static const char code2[] = {
	0xE3, 0x00, 0x30, 0x00, 0x00, 0x12, /*		1: lt %r0,(%r3)	*/
	0x41, 0x30, 0x30, 0x04, /*			la %r3,4(%r3)	*/
	0xC0, 0x74, 0xFF, 0xFF, 0xFF, 0xFB, /*		brcl 7,1b	*/
	0xA7, 0x2B, 0xFF, 0xFF, /*			aghi %r2,-1	*/
	0xC0, 0x74, 0xCA, 0xFE, 0xBA, 0xBE, /*		brcl 7,0b	*/
	0x07, 0xFE, /*					br %r14		*/
};
#define LOOP2_BRANCH_OFFSET 10
#define BACKLINK_BRANCH_OFFSET (sizeof(code2) - 8)

static void link12(char *dst1, char *dst2)
{
	*(int32_t *)(dst1 + LINK_BRANCH_OFFSET + 2) =
		(dst2 - (dst1 + LINK_BRANCH_OFFSET)) / 2;
	*(int32_t *)(dst2 + BACKLINK_BRANCH_OFFSET + 2) =
		(dst1 - (dst2 + BACKLINK_BRANCH_OFFSET)) / 2;
}

/* Time measurements */
struct timer {
	uint16_t *dts;
	uint16_t *dt;
	long t0;
	long dummy3;
};

static void timer_init_1(struct timer *t)
{
	t->dummy3 = (long)&t->dummy3;
}

static void timer_start(struct timer *t)
{
	t->t0 = 0;
	__asm__ volatile("ectg %[t0],%[dummy2],%[dummy3]\n"
			 "lcgr %%r0,%%r0\n"
			 "stg %%r0,%[t0]\n"
			 : [t0] "+m"(t->t0), [dummy3] "+r"(t->dummy3)
			 : [dummy2] "m"(t->dummy3)
			 : "r0", "r1");
}

static void timer_end(struct timer *t)
{
	__asm__ volatile("ectg %[t0],%[dummy2],%[dummy3]\n"
			 "stg %%r0,%[t0]\n"
			 : [t0] "+m"(t->t0), [dummy3] "+r"(t->dummy3)
			 : [dummy2] "m"(t->dummy3)
			 : "r0", "r1");
	*t->dt = t->t0 >= 0xFFFF ? 0xFFFF : htons((uint16_t)t->t0);
	t->dt++;
}

#endif
