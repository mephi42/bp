#ifndef MIPS64_H
#define MIPS64_H
#include <arpa/inet.h>
#include <stdint.h>

#define INSN_ALIGNMENT 4

/* Code generation */
/* a0: repeat count */
/* a1: branch history */
static const uint32_t code1[] = {
	0x9CAC0000, /*	0: lwu $t0,0($a1)	*/
	0x64A50004, /*	daddiu $a1,$a1,4	*/
	0x1180FFFD, /*	beq $t0,$zero,0b	*/
	0x00200825, /*	nop			*/
	0x1000DDBF, /*	b 1f			*/
	0x00200825, /*	nop			*/
};

static const uint32_t code2[] = {
	0x9CAC0000, /*	1: lwu $t0,0($a1)	*/
	0x64A50004, /*	daddiu $a1,$a1,4	*/
	0x1180FFFD, /*	beq $t0,$zero,1b	*/
	0x00200825, /*	nop			*/
	0x6484FFFF, /*	daddiu $a0,$a0,-1	*/
	0x1080CFBB, /*	beq $a0,$zero,0b	*/
	0x00200825, /*	nop			*/
	0x03E00008, /*	jr $ra			*/
	0x00200825, /*	nop			*/
};

static void link12(char *dst1, char *dst2)
{
	uint32_t *b1 = (uint32_t *)(dst1 + sizeof(code1) - 8);
	uint32_t *b2 = (uint32_t *)(dst2 + sizeof(code2) - 16);
	*b1 = (*b1 & 0xFFFF0000) | (((dst2 - (char *)b1) >> 2) & 0xFFFF);
	*b2 = (*b2 & 0xFFFF0000) | (((dst1 - (char *)b2) >> 2) & 0xFFFF);
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

static unsigned long long readcyclecounter()
{
	unsigned long long t;
	__asm__ __volatile__("rdhwr %[t],$2\n" : [t] "=r"(t));
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
