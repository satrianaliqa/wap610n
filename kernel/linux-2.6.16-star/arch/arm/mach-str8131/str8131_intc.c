/*******************************************************************************
 *
 *  Copyright(c) 2006 Star Semiconductor Corporation, All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program; if not, write to the Free Software Foundation, Inc., 59
 *  Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  The full GNU General Public License is included in this distribution in the
 *  file called LICENSE.
 *
 *  Contact Information:
 *  Technology Support <tech@starsemi.com>
 *  Star Semiconductor 4F, No.1, Chin-Shan 8th St, Hsin-Chu,300 Taiwan, R.O.C
 *
 ******************************************************************************/

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#include <asm/hardware.h>
#include <asm/mach/irq.h>
#include <asm/irq.h>

#define INTC_TRIGGER_UNKNOWN -1

typedef struct
{
	int	mode;
	int	level;
} intc_trigger_t;

static intc_trigger_t intc_trigger_table[] =
{
	{ INTC_EDGE_TRIGGER,	INTC_RISING_EDGE	},	// 0
	{ INTC_EDGE_TRIGGER,	INTC_RISING_EDGE	},	// 1
	{ INTC_EDGE_TRIGGER,	INTC_FALLING_EDGE	},	// 2
	{ INTC_EDGE_TRIGGER,	INTC_RISING_EDGE	},	// 3
	{ INTC_TRIGGER_UNKNOWN,	INTC_TRIGGER_UNKNOWN	},	// 4
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_LOW		},	// 5
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_LOW		},	// 6
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_HIGH	},	// 7
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_HIGH	},	// 8
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_HIGH	},	// 9
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_HIGH	},	// 10
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_HIGH	},	// 11
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_HIGH	},	// 12
	{ INTC_TRIGGER_UNKNOWN,	INTC_TRIGGER_UNKNOWN	},	// 13
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_HIGH	},	// 14
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_LOW		},	// 15
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_LOW		},	// 16
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_HIGH	},	// 17
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_HIGH	},	// 18
	{ INTC_EDGE_TRIGGER,	INTC_RISING_EDGE	},	// 19
	{ INTC_EDGE_TRIGGER,	INTC_RISING_EDGE	},	// 20
	{ INTC_EDGE_TRIGGER,	INTC_RISING_EDGE	},	// 21
	{ INTC_EDGE_TRIGGER,	INTC_RISING_EDGE	},	// 22
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_LOW		},	// 23
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_LOW		},	// 24
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_LOW		},	// 25
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_LOW		},	// 26
	{ INTC_LEVEL_TRIGGER,	INTC_ACTIVE_LOW		},	// 27
	{ INTC_EDGE_TRIGGER,	INTC_RISING_EDGE	},	// 28
	{ INTC_TRIGGER_UNKNOWN,	INTC_TRIGGER_UNKNOWN	},	// 29
	{ INTC_TRIGGER_UNKNOWN,	INTC_TRIGGER_UNKNOWN	},	// 30
	{ INTC_EDGE_TRIGGER,	INTC_RISING_EDGE	},	// 31
};

static u32 intc_edge_trigger_bitmap;

/*
 * Configure interrupt trigger mode to be level trigger or edge trigger
 */
static inline void str8131_set_irq_mode(unsigned int irq, unsigned int mode)
{
	volatile unsigned int val;

	if (irq < 0 || irq > NR_IRQS) {
		return;
	}

	if ((mode != INTC_LEVEL_TRIGGER) &&
		(mode != INTC_EDGE_TRIGGER)) {
		return;
	}

	val = INTC_INTERRUPT_TRIGGER_MODE_REG;

	if (mode == INTC_LEVEL_TRIGGER) {
		if (val & (1UL << irq)) {
			val &= ~(1UL << irq);
			INTC_INTERRUPT_TRIGGER_MODE_REG = val;
		}
	} else {
		intc_edge_trigger_bitmap |= (1UL << irq);
		if (!(val & (1UL << irq))) {
			val |= (1UL << irq);
			INTC_INTERRUPT_TRIGGER_MODE_REG = val;
		}
	}
}	

/*
 * Configure interrupt trigger level to be Active High/Low or Rising/Falling Edge
 */
static inline void str8131_set_irq_level(unsigned int irq, unsigned int level)
{
	volatile unsigned int val;

	if (irq < 0 || irq > NR_IRQS) {
		return;
	}

	if ((level != INTC_ACTIVE_HIGH) &&
		(level != INTC_ACTIVE_LOW) &&
		(level != INTC_RISING_EDGE) &&
		(level != INTC_FALLING_EDGE)) {
		return;
	}

	val = INTC_INTERRUPT_TRIGGER_LEVEL_REG;

	if ((level == INTC_ACTIVE_HIGH) ||
		(level == INTC_RISING_EDGE)) {
		if (val & (1UL << irq)) {
			val &= ~(1UL << irq);
			INTC_INTERRUPT_TRIGGER_LEVEL_REG = val;
		}
	} else {
		if (!(val & (1UL << irq))) {
			val |= (1UL << irq);
			INTC_INTERRUPT_TRIGGER_LEVEL_REG = val;
		}
	}
}

/*
 * Configure interrupt trigger mode and trigger level
 */
void str8131_set_interrupt_trigger(unsigned int irq, unsigned int mode, unsigned int level)
{
	str8131_set_irq_mode(irq, mode);
	str8131_set_irq_level(irq, level);
}
EXPORT_SYMBOL(str8131_set_interrupt_trigger);

/*
 * Mask/Disable this interrupt source
 */
void str8131_mask_irq(unsigned int irq)
{
//	if (irq != INTC_NIC_RXRC_BIT_INDEX && irq != INTC_NIC_RXQF_BIT_INDEX) {
		// Mask/Disable this interrupt source
		INTC_INTERRUPT_MASK_REG = (1UL << irq);
//	}
}

/*
 * Un-Mask/Enable this interrupt source
 */
void str8131_unmask_irq(unsigned int irq)
{
#ifndef CONFIG_VIC_INTERRUPT
	// Clear interrupt status of the interrupt source which is edge-triggered
	INTC_EDGE_INTERRUPT_SOURCE_CLEAR_REG = (1UL << irq);
#endif
	// Enable this interrupt source
	INTC_INTERRUPT_MASK_CLEAR_REG = (1UL << irq);
}

struct irqchip str8131_irqchip = {
	.ack	= str8131_mask_irq,
	.mask	= str8131_mask_irq,
	.unmask	= str8131_unmask_irq,
};

void __init str8131_init_irq(void)
{
	int i;

	INTC_INTERRUPT_MASK_REG = 0xFFFFFFFF;
	INTC_EDGE_INTERRUPT_SOURCE_CLEAR_REG = 0xFFFFFFFF;
	INTC_SOFTWARE_INTERRUPT_CLEAR_REG = 0xFFFFFFFF;
	INTC_SOFTWARE_PRIORITY_MASK_REG = 0x0;
	INTC_FIQ_SELECT_REG = 0x0;
#ifdef CONFIG_VIC_INTERRUPT
	for (i = 0; i < NR_IRQS; i++) {
		(*((u32 volatile *)(SYSVA_VIC_BASE_ADDR + 0x40 + (i << 2)))) = i;
	}
	INTC_VECTOR_INTERRUPT_ENABLE_REG = 1;
#else
	INTC_VECTOR_INTERRUPT_ENABLE_REG = 0;
#endif

	for (i = 0; i < NR_IRQS; i++) {
		if (intc_trigger_table[i].mode != INTC_TRIGGER_UNKNOWN) {
			str8131_set_irq_mode(i, intc_trigger_table[i].mode);
			str8131_set_irq_level(i, intc_trigger_table[i].level);
		}
	}
	
	for (i = 0; i < NR_IRQS;  i++) {
		set_irq_chip(i, &str8131_irqchip);
		set_irq_handler(i, do_level_IRQ);
		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
	}
}

