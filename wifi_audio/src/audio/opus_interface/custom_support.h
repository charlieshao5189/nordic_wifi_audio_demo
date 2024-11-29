/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include "opus_defines.h"

#define OVERRIDE_OPUS_ALLOC
#define OVERRIDE_OPUS_REALLOC
#define OVERRIDE_OPUS_FREE

static OPUS_INLINE void *opus_alloc(size_t size)
{
	printk("\nopus_alloce memory size: %d bytes\n", size);
	return k_malloc(size);
}

static OPUS_INLINE void *opus_realloc(void *ptr, size_t size)
{
	// Zephyr doesn't have k_realloc, so you may need to manually implement it
	void *new_ptr = k_malloc(size);
	if (new_ptr && ptr) {
		memcpy(new_ptr, ptr, size); // Copy data
		k_free(ptr);                // Free old memory
	}
	return new_ptr;
}

static OPUS_INLINE void opus_free(void *ptr)
{
	k_free(ptr);
}
