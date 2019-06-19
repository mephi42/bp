// SPDX-License-Identifier: GPL-3.0
#define _GNU_SOURCE
#include <assert.h>
#include <getopt.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#if defined(__aarch64__)
#include "arch/aarch64.h"
#elif defined(__mips64)
#include "arch/mips64.h"
#elif defined(__powerpc64__)
#include "arch/ppc64.h"
#elif defined(__s390x__)
#include "arch/s390x.h"
#elif defined(__x86_64__)
#include "arch/x86_64.h"
#else
#error Unsupported architecture
#endif

#if defined(__linux__)
#include "os/linux.h"
#elif defined(__APPLE__)
#include "os/apple.h"
#else
#error Unsupported OS
#endif

#include "bp.h"

static void print_asm_header(void)
{
	printf("#include \"bp.h\"\n"
	       "START_BPS();\n");
}

static void print_asm_buf(const char *buf, size_t size)
{
	for (size_t i = 0; i < size; i++)
		printf("\t\"\\t.byte 0x%.2x\\n\"\n", buf[i] & 0xff);
}

static const char *gas_ptr = sizeof(long) == 4 ? ".long" : ".quad";

static void print_asm(const char *base, size_t offset1, size_t offset2)
{
	printf("extern void bp_0x%zx_0x%zx(long, int *);\n"
	       "__asm__(\n"
	       "\t\"\\t.align 0x1000\\n\"\n"
	       "\t\"\\t.org .+0x%zx\\n\"\n"
	       "\t\".globl bp_0x%zx_0x%zx\\n\"\n"
	       "\t\"\\t.type bp_0x%zx_0x%zx, @function\\n\"\n"
	       "\t\"bp_0x%zx_0x%zx:\\n\"\n",
	       offset1, offset2, offset1, offset1, offset2, offset1, offset2,
	       offset1, offset2);
	print_asm_buf(base + offset1, sizeof(code1));
	printf("\t\"\\t.org .+0x%zx\\n\"\n",
	       offset2 - (offset1 + sizeof(code1)));
	print_asm_buf(base + offset2, sizeof(code2));
	printf("\t\"\\t.section bps, \\\"a\\\",@progbits\\n\"\n"
	       "\t\"\\t%s bp_0x%zx_0x%zx\\n\"\n"
	       "\t\"\\t.previous\\n\");\n",
	       gas_ptr, offset1, offset2);
}

static void print_asm_footer(const char *pattern_s, int repeat)
{
	printf("END_BPS(\"%s\", %d);\n", pattern_s, repeat);
}

static void timer_init(struct timer *t, size_t length1, size_t length2)
{
	t->dts = calloc((length1 / INSN_ALIGNMENT) * (length2 / INSN_ALIGNMENT),
			sizeof(uint16_t));
	assert(t->dts != NULL);
	t->dt = t->dts;
	timer_init_1(t);
}

static void output(void *base, size_t offset1, size_t length1, size_t offset2,
		   size_t length2, struct timer *t)
{
	uint64_t header[] = {
		htobe64(1),
		htobe64((uint64_t)base),
		htobe64(INSN_ALIGNMENT),
		htobe64((uint64_t)offset1),
		htobe64((uint64_t)length1),
		htobe64(sizeof(code1)),
		htobe64(LOOP1_BRANCH_OFFSET),
		htobe64(LINK_BRANCH_OFFSET),
		htobe64((uint64_t)offset2),
		htobe64((uint64_t)length2),
		htobe64(sizeof(code2)),
		htobe64(LOOP2_BRANCH_OFFSET),
		htobe64(BACKLINK_BRANCH_OFFSET),
	};
	fwrite(header, sizeof(header), 1, stdout);
	fwrite(t->dts, t->dt - t->dts, sizeof(unsigned short), stdout);
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static const char *shortopts = "al:L:o:O:p:r:";

static const struct option longopts[] = {
	{ "length1", required_argument, NULL, 'l' },
	{ "length2", required_argument, NULL, 'L' },
	{ "offset1", required_argument, NULL, 'o' },
	{ "offset2", required_argument, NULL, 'O' },
	{ "pattern", required_argument, NULL, 'p' },
	{ "print-asm", no_argument, NULL, 'a' },
	{ "repeat", required_argument, NULL, 'r' },
	{ NULL, 0, NULL, 0 },
};

/* Main logic */
int main(int argc, char **argv)
{
	size_t length1 = 8192;
	size_t length2 = 8192;
	size_t offset1 = 0;
	size_t offset2 = 0;
	const char *pattern_s = "1110110";
	int print_asm_p = 0;
	int repeat = 128;
	int fail_p = 0;
	while (1) {
		int index = 0;
		int c = getopt_long(argc, argv, shortopts, longopts, &index);
		if (c == -1)
			break;
		switch (c) {
		case 'a':
			print_asm_p = 1;
			break;
		case 'l':
			length1 = atoi(optarg);
			break;
		case 'L':
			length2 = atoi(optarg);
			break;
		case 'o':
			offset1 = atoi(optarg);
			break;
		case 'O':
			offset2 = atoi(optarg);
			break;
		case 'p':
			pattern_s = optarg;
			break;
		case 'r':
			repeat = atoi(optarg);
			break;
		default:
			fail_p = 1;
			break;
		}
	}
	if (length1 < sizeof(code1)) {
		fprintf(stderr, "%s: length1 must be at least %zu\n", argv[0],
			sizeof(code1));
		fail_p = 1;
	}
	if (length2 < sizeof(code2)) {
		fprintf(stderr, "%s: length2 must be at least %zu\n", argv[0],
			sizeof(code2));
		fail_p = 1;
	}
	if (offset1 % INSN_ALIGNMENT) {
		fprintf(stderr, "%s: offset1 must be divisible by %d\n",
			argv[0], INSN_ALIGNMENT);
		fail_p = 1;
	}
	if (offset2 % INSN_ALIGNMENT) {
		fprintf(stderr, "%s: offset2 must be divisible by %d\n",
			argv[0], INSN_ALIGNMENT);
		fail_p = 1;
	}
	if (fail_p)
		return EXIT_FAILURE;

	pin_to_single_cpu();
	void *pattern = alloc_pattern(pattern_s, repeat);
	const int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
	const int flags = MAP_PRIVATE | MAP_ANONYMOUS;
	size_t size = MAX(offset1 + length1, offset2 + length2);
	char *p = mmap(NULL, size, prot, flags, -1, 0);
	assert(p != MAP_FAILED);
	struct timer t;
	timer_init(&t, length1, length2);
	if (print_asm_p)
		print_asm_header();
	size_t imax = offset1 + length1 - sizeof(code1);
	for (size_t i = offset1; i <= imax; i += INSN_ALIGNMENT) {
		memcpy(p + i, code1, sizeof(code1));
		size_t j0 = MAX(i + sizeof(code1), offset2);
		size_t jmax = offset2 + length2 - sizeof(code2);
		for (size_t j = j0; j <= jmax; j += INSN_ALIGNMENT) {
			memcpy(p + j, code2, sizeof(code2));
			link12(p + i, p + j);
			if (print_asm_p) {
				print_asm(p, i, j);
				continue;
			}
			__builtin___clear_cache(p + i, p + i + sizeof(code1));
			__builtin___clear_cache(p + j, p + j + sizeof(code2));
			timer_start(&t);
			((void (*)(long, int *))(p + i))(repeat, pattern);
			timer_end(&t);
		}
	}
	if (print_asm_p)
		print_asm_footer(pattern_s, repeat);
	else
		output(p, offset1, length1, offset2, length2, &t);
}
