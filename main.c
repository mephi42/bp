#define _GNU_SOURCE
#include <assert.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/*
 *     runner:
 *         lt %r0,(%r3)      E3 00 30 00 00 12
 *         la %r3,4(%r3)     41 30 30 04
 *         brcl 7,runner     C0 74 FF FF FF FB
 *         j 0f              C0 F4 xx xx xx xx
 *         # nops
 *     0:
 *         lt %r0,(%r3)      E3 00 30 00 00 12
 *         la %r3,4(%r3)     41 30 30 04
 *         brcl 7,0b         C0 74 FF FF FF FB
 *         aghi %r2,-1       A7 2B FF FF
 *         brcl 7,runner     C0 74 xx xx xx xx
 *         br %r14           07 FE
 *
 * - %r2 = repeat count
 * - %r3 = &repeat(1,1,1,0,1,1,0)
 */

int main() {
	cpu_set_t cpus;
	CPU_ZERO(&cpus);
	CPU_SET(sched_getcpu(), &cpus);
	int ret = sched_setaffinity(getpid(), sizeof(cpus), &cpus);
	assert(ret == 0);
	const int repeat = 16; /*256+*/
	const int count = repeat * 7;
	int *d = malloc(count * sizeof(int));
	assert(d != NULL);
	for (int i = 0; i < count; i += 7) {
		d[i] = 1; d[i + 1] = 1; d[i + 2] = 1; d[i + 3] = 0;
		d[i + 4] = 1; d[i + 5] = 1; d[i + 6] = 0;
	}
	const size_t length = 8192;
	char *p = mmap(NULL, length, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	assert(p != MAP_FAILED);
	static char code1[] = {
		0xE3, 0x00, 0x30, 0x00, 0x00, 0x12,
		0x41, 0x30, 0x30, 0x04,
		0xC0, 0x74, 0xFF, 0xFF, 0xFF, 0xFB,
		0xC0, 0xF4, 0xDE, 0xAD, 0xBE, 0xEF,
	};
	static char code2[] = {
		0xE3, 0x00, 0x30, 0x00, 0x00, 0x12,
		0x41, 0x30, 0x30, 0x04,
		0xC0, 0x74, 0xFF, 0xFF, 0xFF, 0xFB,
		0xA7, 0x2B, 0xFF, 0xFF,
		0xC0, 0x74, 0xCA, 0xFE, 0xBA, 0xBE,
		0x07, 0xFE,
	};
	long *dts = calloc((length / 2) * (length / 2), sizeof(long));
	assert(dts != NULL);
	long *dt_current = dts;
	for (size_t i = 0; i < length - sizeof(code1); i += 2) {
		memcpy(p + i, code1, sizeof(code1));
		for (size_t j = i + sizeof(code1); j < length - sizeof(code2); j += 2) {
			*(int *)(p + i + sizeof(code1) - 4) = (j - (i + sizeof(code1) - 6)) / 2;
			memcpy(p + j, code2, sizeof(code2));
			*(int *)(p + j + sizeof(code2) - 6) = (i - (j + sizeof(code2) - 8)) / 2;
			long dummy2, dummy3 = (long)&dummy3;
			__asm__ volatile("ectg %[dt],%[dummy2],%[dummy3]\n"
					"lcgr %%r0,%%r0\n"
					"stg %%r0,%[dt]\n"
					: [dt] "+m" (*dt_current)
					, [dummy3] "+r" (dummy3)
					: [dummy2] "m" (dummy2)
					: "r0", "r1");
			((int (*)(int, int *))(p + i))(count, d);
			__asm__ volatile("ectg %[dt],%[dummy2],%[dummy3]\n"
					"stg %%r0,%[dt]\n"
					: [dt] "+m" (*dt_current)
					, [dummy3] "+r" (dummy3)
					: [dummy2] "m" (dummy2)
					: "r0", "r1");
			dt_current++;
		}
	}
	dt_current = dts;
	for (size_t i = 0; i < length - sizeof(code1); i += 2) {
		for (size_t j = 0; j < i + sizeof(code1); j += 2) {
			printf("0 ");
		}
		for (size_t j = i + sizeof(code1); j < length - sizeof(code2); j += 2) {
			printf("%ld ", *dt_current);
			dt_current++;
		}
		printf("\n");
	}
}
