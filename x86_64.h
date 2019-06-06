#ifndef X86_64_H
#define X86_64_H
#include <arpa/inet.h>
#include <x86intrin.h>

#define INSN_ALIGNMENT 1

/* Code generation */
/* %rdi: repeat count */
/* %rsi: branch history */
static const char code1[] = {
	0x83, 0x3E, 0x00, /*			0: cmpl $0,(%rsi)	*/
	0x48, 0x8D, 0x76, 0x04, /*		lea 4(%rsi),%rsi	*/
	0x74, 0xF7, /*				je 0b			*/
	0xE9, 0xDE, 0xAD, 0xBE, 0xEF, /*	j 1f			*/
};

static void emit1(char *dst1)
{
	memcpy(dst1, code1, sizeof(code1));
}

static const char code2[] = {
	0x83, 0x3E, 0x00, /*			1: cmpl $0,(%rsi)	*/
	0x48, 0x8D, 0x76, 0x04, /*		lea 4(%rsi),%rsi	*/
	0x74, 0xF7, /*				je 1b			*/
	0x48, 0xFF, 0xCF, /*			dec %rdi		*/
	0x0F, 0x84, 0xCA, 0xFE, 0xBA, 0xBE, /*	jz 0b			*/
	0xC3 /*					ret			*/
};

static void emit2(char *dst2)
{
	memcpy(dst2, code2, sizeof(code2));
}

static void link12(char *dst1, char *dst2)
{
	*(int *)(dst1 + sizeof(code1) - 4) = dst2 - (dst1 + sizeof(code1));
	*(int *)(dst2 + sizeof(code2) - 5) = dst1 - (dst2 + sizeof(code2) - 1);
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
