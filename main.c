#define _GNU_SOURCE
#include <assert.h>
#include <endian.h>
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

#if defined(__s390x__)
#include "s390x.h"
#elif defined(__x86_64__)
#include "x86_64.h"
#else
#error Unsupported architecture
#endif

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

static void output(void *base, size_t length, struct timer *t)
{
	uint64_t header[] = { htobe64(1), htobe64((uint64_t)base),
			      htobe64((uint64_t)length),
			      htobe64(INSN_ALIGNMENT) };
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
	for (size_t i = offset; i <= imax; i += INSN_ALIGNMENT) {
		emit1(p + i);
		size_t jmax = offset + length - sizeof(code2);
		for (size_t j = i + sizeof(code1); j <= jmax;
		     j += INSN_ALIGNMENT) {
			emit2(p + j);
			link12(p + i, p + j);
			timer_start(&t);
			((int (*)(long, int *))(p + i))(repeat, pattern);
			timer_end(&t);
		}
	}
	output(p + offset, length, &t);
}
