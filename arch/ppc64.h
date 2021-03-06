/* SPDX-License-Identifier: GPL-3.0 */
#ifndef PPC64_H
#define PPC64_H
#include <arpa/inet.h>
#include <stdint.h>

#define INSN_ALIGNMENT 4

/* Code generation */
/* %r3: repeat count */
/* %r4: branch history */
static const uint32_t code1[] = {
	0x80040000, /*	0: lwz %r0,0(%r4)	*/
	0x38840004, /*	addi %r4,%r4,4		*/
	0x28200000, /*	cmpli 0,1,%r0,0		*/
	0x4082FFF4, /*	bc 4,2,0b		*/
	0x48DEDBEF, /*	b 1f			*/
};
#define LOOP1_BRANCH_OFFSET 12
#define LINK_BRANCH_OFFSET (sizeof(code1) - 4)

static const uint32_t code2[] = {
	0x80040000, /*	1: lwz %r0,0(%r4)	*/
	0x38840004, /*	addi %r4,%r4,4		*/
	0x28200000, /*	cmpli 0,1,%r0,0		*/
	0x4082FFF4, /*	bc 4,2,1b		*/
	0x3863FFFF, /*	addi %r3,%r3,-1		*/
	0x28230000, /*	cmpli 0,1,%r3,0		*/
	0x4D820020, /*	bclr 12,2,0		*/
	0x4BCFEBBE, /*	b 0b			*/
};
#define LOOP2_BRANCH_OFFSET 12
#define BACKLINK_BRANCH_OFFSET (sizeof(code2) - 4)

static void link12(char *dst1, char *dst2)
{
	uint32_t *b1 = (uint32_t *)(dst1 + LINK_BRANCH_OFFSET);
	uint32_t *b2 = (uint32_t *)(dst2 + BACKLINK_BRANCH_OFFSET);
	*b1 = (*b1 & 0xFF000000) | ((dst2 - (char *)b1) & 0xFFFFFF);
	*b2 = (*b2 & 0xFF000000) | ((dst1 - (char *)b2) & 0xFFFFFF);
}

/* Time measurements */
struct timer {
	uint16_t *dts;
	uint16_t *dt;
	unsigned long long t0;
};

static void timer_init_1(struct timer *t)
{
	(void)t;
}

static unsigned long long readcyclecounter(void)
{
	unsigned long long t;
	__asm__ __volatile__("mfspr %[t],268" : [t] "=r"(t));
	return t;
}

static void timer_start(struct timer *t)
{
	t->t0 = readcyclecounter();
}

static void timer_end(struct timer *t)
{
	unsigned long long dt = readcyclecounter() - t->t0;
	*t->dt = dt >= 0xFFFF ? 0xFFFF : htons((uint16_t)dt);
	t->dt++;
}

#endif
