#define _GNU_SOURCE
#include <assert.h>
#include <getopt.h>
#include <netinet/in.h>
#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* Code generation */
/* %r2: repeat count */
/* %r3: branch history, e.g. &repeat(1,1,1,0,1,1,0) */
static const char code1[] = {
	0xE3, 0x00, 0x30, 0x00, 0x00, 0x12, /* 0: lt %r0,(%r3)  */
	0x41, 0x30, 0x30, 0x04, /*                la %r3,4(%r3) */
	0xC0, 0x74, 0xFF, 0xFF, 0xFF, 0xFB, /*    brcl 7,0b     */
	0xC0, 0xF4, 0xDE, 0xAD, 0xBE, 0xEF, /*    j 1f          */
};

static void emit1(char *dst1)
{
	memcpy(dst1, code1, sizeof(code1));
}

static const char code2[] = {
	0xE3, 0x00, 0x30, 0x00, 0x00, 0x12, /* 1: lt %r0,(%r3)  */
	0x41, 0x30, 0x30, 0x04, /*                la %r3,4(%r3) */
	0xC0, 0x74, 0xFF, 0xFF, 0xFF, 0xFB, /*    brcl 7,1b     */
	0xA7, 0x2B, 0xFF, 0xFF, /*                aghi %r2,-1   */
	0xC0, 0x74, 0xCA, 0xFE, 0xBA, 0xBE, /*    brcl 7,0b     */
	0x07, 0xFE, /*                            br %r14       */
};

static void emit2(char *dst2)
{
	memcpy(dst2, code2, sizeof(code2));
}
static void link12(char *dst1, char *dst2)
{
	*(int *)(dst1 + sizeof(code1) - 4) =
		(dst2 - (dst1 + sizeof(code1) - 6)) / 2;
	*(int *)(dst2 + sizeof(code2) - 6) =
		(dst1 - (dst2 + sizeof(code2) - 8)) / 2;
}

/* Time measurements */
struct timer {
	uint16_t *dts;
	uint16_t *dt;
	long t0;
	long dummy2;
	long dummy3;
};
static void timer_init(struct timer *t, size_t length)
{
	t->dts = calloc((length / 2) * (length / 2), sizeof(uint16_t));
	assert(t->dts != NULL);
	t->dt = t->dts;
	t->dummy3 = (long)&t->dummy3;
}
static void timer_start(struct timer *t)
{
	t->t0 = 0;
	__asm__ volatile("ectg %[t0],%[dummy2],%[dummy3]\n"
			 "lcgr %%r0,%%r0\n"
			 "stg %%r0,%[t0]\n"
			 : [t0] "+m"(t->t0), [dummy3] "+r"(t->dummy3)
			 : [dummy2] "m"(t->dummy2)
			 : "r0", "r1");
}
static void timer_end(struct timer *t)
{
	__asm__ volatile("ectg %[t0],%[dummy2],%[dummy3]\n"
			 "stg %%r0,%[t0]\n"
			 : [t0] "+m"(t->t0), [dummy3] "+r"(t->dummy3)
			 : [dummy2] "m"(t->dummy2)
			 : "r0", "r1");
	*t->dt = t->t0 >= 0xffff ? 0xffff : htons((uint16_t)t->t0);
	t->dt++;
}

/* User interface */
static void *alloc_pattern(const char *s, int repeat)
{
	size_t len = strlen(s);
	const int count = len * repeat;
	int *d = malloc(count * sizeof(int));
	assert(d != NULL);
	for (size_t i = 0; i < len; i++) {
		d[i] = s[i] == '0' ? 0 : 1;
	}
	for (int i = len; i < count; i += len) {
		memcpy(&d[i], &d[0], len);
	}
	return d;
}
static void output(size_t length, size_t offset, struct timer *t)
{
	uint32_t header[] = { htonl(1), htonl((uint32_t)length),
			      htonl((uint32_t)offset) };
	fwrite(header, sizeof(header), 1, stdout);
	fwrite(t->dts, t->dt - t->dts, sizeof(unsigned short), stdout);
}

static const char *shortopts = "l:o:p:r:";

static const struct option longopts[] = {
	{ "length", required_argument, NULL, 'l' },
	{ "offset", required_argument, NULL, 'o' },
	{ "pattern", required_argument, NULL, 'p' },
	{ "repeat", required_argument, NULL, 'r' },
	{ NULL, 0, NULL, 0 },
};

/* Main logic */
int main(int argc, char **argv)
{
	size_t length = 8192;
	size_t offset = 0;
	const char *pattern_s = "1110110";
	int repeat = 128;
	int index = 0;
	while (1) {
		int c = getopt_long(argc, argv, shortopts, longopts, &index);
		if (c == -1) {
			break;
		}
		switch (c) {
		case 'l':
			length = atoi(optarg);
			break;
		case 'o':
			offset = atoi(optarg);
			break;
		case 'p':
			pattern_s = optarg;
			break;
		case 'r':
			repeat = atoi(optarg);
			break;
		}
	}
	if (length < sizeof(code1) + sizeof(code2)) {
		fprintf(stderr, "%s: length must be at least %zu\n", argv[0],
			sizeof(code1) + sizeof(code2));
		return EXIT_FAILURE;
	}

	cpu_set_t cpus;
	CPU_ZERO(&cpus);
	CPU_SET(sched_getcpu(), &cpus);
	int ret = sched_setaffinity(getpid(), sizeof(cpus), &cpus);
	assert(ret == 0);
	void *pattern = alloc_pattern(pattern_s, repeat);
	const int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
	const int flags = MAP_PRIVATE | MAP_ANONYMOUS;
	char *p = mmap(NULL, offset + length, prot, flags, -1, 0);
	assert(p != MAP_FAILED);
	struct timer t;
	timer_init(&t, length);
	size_t imax = offset + length - sizeof(code1) - sizeof(code2);
	for (size_t i = offset; i <= imax; i += 2) {
		emit1(p + i);
		size_t jmax = offset + length - sizeof(code2);
		for (size_t j = i + sizeof(code1); j <= jmax; j += 2) {
			emit2(p + j);
			link12(p + i, p + j);
			timer_start(&t);
			((int (*)(int, int *))(p + i))(repeat, pattern);
			timer_end(&t);
		}
	}
	output(length, offset, &t);
}
