#ifndef PPC64_H
#define PPC64_H

#define INSN_ALIGNMENT 4

#ifdef __LITTLE_ENDIAN__
#define INSN(a, b, c, d) d, c, b, a
#else
#define INSN(a, b, c, d) a, b, c, d
#endif

/* Code generation */
/* %r3: repeat count */
/* %r4: branch history */
static const char code1[] = {
	INSN(0x80, 0x04, 0x00, 0x00), /*	0: lwz %r0,0(%r4)	*/
	INSN(0x38, 0x84, 0x00, 0x04), /*	addi %r4,%r4,4		*/
	INSN(0x28, 0x20, 0x00, 0x00), /*	cmpli 0,1,%r0,0		*/
	INSN(0x40, 0x82, 0xFF, 0xF4), /*	bc 4,2,0b		*/
	INSN(0x48, 0xDE, 0xDB, 0xEF), /*	b 1f			*/
};

static void emit1(char *dst1)
{
	memcpy(dst1, code1, sizeof(code1));
}

static const char code2[] = {
	INSN(0x80, 0x04, 0x00, 0x00), /*	1: lwz %r0,0(%r4)	*/
	INSN(0x38, 0x84, 0x00, 0x04), /*	addi %r4,%r4,4		*/
	INSN(0x28, 0x20, 0x00, 0x00), /*	cmpli 0,1,%r0,0		*/
	INSN(0x40, 0x82, 0xFF, 0xF4), /*	bc 4,2,1b		*/
	INSN(0x38, 0x63, 0xFF, 0xFF), /*	addi %r3,%r3,-1		*/
	INSN(0x28, 0x23, 0x00, 0x00), /*	cmpli 0,1,%r3,0		*/
	INSN(0x4D, 0x82, 0x00, 0x20), /*	bclr 12,2,0		*/
	INSN(0x4B, 0xCF, 0xEB, 0xBE), /*	b 0b			*/
};

static void emit2(char *dst2)
{
	memcpy(dst2, code2, sizeof(code2));
}

static void link12(char *dst1, char *dst2)
{
	int *b1 = (int *)(dst1 + sizeof(code1) - 4);
	int *b2 = (int *)(dst2 + sizeof(code2) - 4);
	*b1 = (*b1 & 0xff000000) | ((dst2 - (char *)b1) & 0xffffff);
	*b2 = (*b2 & 0xff000000) | ((dst1 - (char *)b2) & 0xffffff);
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
	*t->dt = dt >= 0xffff ? 0xffff : htons((uint16_t)dt);
	t->dt++;
}

#endif
