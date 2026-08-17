/*
 *  linux/include/asm-arm/mach/mmc.h
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *  
 */
#ifndef ASMARM_MACH_MMC_H
#define ASMARM_MACH_MMC_H

#include <linux/mmc/protocol.h>

struct mmc_platform_data {
	unsigned int ocr_mask;			/* available voltages */
	__u32 (*translate_vdd)(struct device *, unsigned int);
	unsigned int (*status)(struct device *);
};

#endif
