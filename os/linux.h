/* SPDX-License-Identifier: GPL-3.0 */
#ifndef LINUX_H
#define LINUX_H
#include <endian.h>
#include <sched.h>

static void pin_to_single_cpu(void)
{
	cpu_set_t cpus;
	CPU_ZERO(&cpus);
	CPU_SET(sched_getcpu(), &cpus);
	int ret = sched_setaffinity(getpid(), sizeof(cpus), &cpus);
	assert(ret == 0);
}

#endif
