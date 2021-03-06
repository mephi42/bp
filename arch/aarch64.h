/* SPDX-License-Identifier: GPL-3.0 */
#ifndef AARCH64_H
#define AARCH64_H
#include <arpa/inet.h>
#include <stdint.h>

#define INSN_ALIGNMENT 4

/* Code generation */
/* x0: repeat count */
/* x1: branch history */
static const uint32_t code1[] = {
	0xB9400022, /*	0: ldr w2,[x1]	*/
	0x91001021, /*	add x1,x1,4	*/
	0x35FFFFC2, /*	cbnz w2,0b	*/
	0x14CFEBBE, /*	b 1f		*/
};
#define LOOP1_BRANCH_OFFSET 8
#define LINK_BRANCH_OFFSET (sizeof(code1) - 4)

static const uint32_t code2[] = {
	0xB9400022, /*	1: ldr w2,[x1]	*/
	0x91001021, /*	add x1,x1,4	*/
	0x35FFFFC2, /*	cbnz w2,1b	*/
	0xD1000400, /*	sub x0,x0,1	*/
	0xB5FFFF00, /*	cbnz x0,0b	*/
	0xD65F03C0, /*	ret		*/
};
#define LOOP2_BRANCH_OFFSET 8
#define BACKLINK_BRANCH_OFFSET (sizeof(code2) - 8)

static void link12(char *dst1, char *dst2)
{
	uint32_t *b1 = (uint32_t *)(dst1 + LINK_BRANCH_OFFSET);
	uint32_t *b2 = (uint32_t *)(dst2 + BACKLINK_BRANCH_OFFSET);
	*b1 = (*b1 & 0xFC000000) | (((dst2 - (char *)b1) >> 2) & 0x3FFFFFF);
	*b2 = (*b2 & 0xFF00001F) |
	      ((((dst1 - (char *)b2) >> 2) & 0x7FFFF) << 5);
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
	__asm__ __volatile__("isb\n"
			     "mrs %[t],cntvct_el0\n"
			     : [t] "=r"(t));
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
