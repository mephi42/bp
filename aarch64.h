#ifndef AARCH64_H
#define AARCH64_H

#define INSN_ALIGNMENT 4

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define INSN(a, b, c, d) d, c, b, a
#else
#define INSN(a, b, c, d) a, b, c, d
#endif

/* Code generation */
/* x0: repeat count */
/* x1: branch history */
static const char code1[] = {
	INSN(0xB9, 0x40, 0x00, 0x22), /*	0: ldr w2,[x1]		*/
	INSN(0x91, 0x00, 0x10, 0x21), /*	add x1,x1,4		*/
	INSN(0x35, 0xFF, 0xFF, 0xC2), /*	cbnz w2,0b		*/
	INSN(0x14, 0xCF, 0xEB, 0xBE), /*	b 1f			*/
};

static void emit1(char *dst1)
{
	memcpy(dst1, code1, sizeof(code1));
}

static const char code2[] = {
	INSN(0xB9, 0x40, 0x00, 0x22), /*	1: ldr w2,[x1]		*/
	INSN(0x91, 0x00, 0x10, 0x21), /*	add x1,x1,4		*/
	INSN(0x35, 0xFF, 0xFF, 0xC2), /*	cbnz w2,1b		*/
	INSN(0xD1, 0x00, 0x04, 0x00), /*	sub x0,x0,1		*/
	INSN(0xB5, 0xFF, 0xFF, 0x00), /*	cbnz x0,0b		*/
	INSN(0xD6, 0x5F, 0x03, 0xC0), /*	ret			*/
};

static void emit2(char *dst2)
{
	memcpy(dst2, code2, sizeof(code2));
}

static void link12(char *dst1, char *dst2)
{
	int *b1 = (int *)(dst1 + sizeof(code1) - 4);
	int *b2 = (int *)(dst2 + sizeof(code2) - 8);
	*b1 = (*b1 & 0xfc000000) | (((dst2 - (char *)b1) >> 2) & 0x3ffffff);
	*b2 = (*b2 & 0xff00001f) |
	      ((((dst1 - (char *)b2) >> 2) & 0x7ffff) << 5);
	__builtin___clear_cache(dst1, dst1 + sizeof(code1));
	__builtin___clear_cache(dst2, dst2 + sizeof(code2));
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
	*t->dt = dt >= 0xffff ? 0xffff : htons((uint16_t)dt);
	t->dt++;
}

#endif
