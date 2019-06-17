/* SPDX-License-Identifier: GPL-3.0 */
#ifndef APPLE_H
#define APPLE_H
#include <libkern/OSByteOrder.h>

#define htobe64 OSSwapHostToBigInt64

static void pin_to_single_cpu(void)
{
	// macOS cannot really do this
	// maybe use https://github.com/07151129/ThreadBinder some day?
}

#endif
