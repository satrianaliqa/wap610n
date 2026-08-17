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

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/timex.h>
#include <linux/proc_fs.h>
#include <linux/module.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>

u64 volatile str8131_counter_tick;
EXPORT_SYMBOL(str8131_counter_tick);

static struct proc_dir_entry *str8131_counter_proc_entry;

#if 0
// this is defined in include/asm/arch-str8131/system.h
u64 str8131_read_counter(void)
{
	return (str8131_counter_tick + TIMER2_COUNTER_REG);
}
EXPORT_SYMBOL(str8131_read_counter);
#endif

static int match1=0;
static int match2=0;
static void str8131_setup_counter(void)
{
	unsigned long control_value;
	unsigned long mask_value;    
	unsigned long val;    

	control_value = TIMER1_TIMER2_CONTROL_REG;
	mask_value = TIMER1_TIMER2_INTERRUPT_MASK_REG;

	TIMER2_COUNTER_REG		= 0;
	TIMER2_AUTO_RELOAD_VALUE_REG	= 0;
	TIMER2_MATCH_VALUE1_REG		= match1;
	TIMER2_MATCH_VALUE2_REG		= match2;

	// Clock Source: PCLK
	control_value &= ~(1 << TIMER2_CLOCK_SOURCE_BIT_INDEX);

	// UP Count Mode
	control_value &= ~(1 << TIMER2_UP_DOWN_COUNT_BIT_INDEX);

	// un-mask match1, match2, and overflow interrupt sources
	mask_value &= ~(0x7 << 3);

	// mask match1, match2 interrupt sources
	//mask_value |= (0x3 << 3);
	val=0;
	if(!match1) val|=0x1;
	if(!match2) val|=0x2;
	mask_value |= (val<< 3);

	TIMER1_TIMER2_CONTROL_REG = control_value;
	TIMER1_TIMER2_INTERRUPT_MASK_REG = mask_value;
}

static void str8131_counter_enable(void)
{
	unsigned long control_value;

	control_value = TIMER1_TIMER2_CONTROL_REG;

	// enable overflow mode
	control_value |= (1 << TIMER2_OVERFLOW_ENABLE_BIT_INDEX);

	// enable the timer
	control_value |= (1 << TIMER2_ENABLE_BIT_INDEX);

	TIMER1_TIMER2_CONTROL_REG = control_value;
}

#if 0
static void str8131_counter_disable(void)
{
	unsigned long control_value;

	control_value = TIMER1_TIMER2_CONTROL_REG;

	// enable overflow mode
	control_value &= ~(1 << TIMER2_OVERFLOW_ENABLE_BIT_INDEX);

	// enable the timer
	control_value &= ~(1 << TIMER2_ENABLE_BIT_INDEX);

	TIMER1_TIMER2_CONTROL_REG = control_value;
}
#endif /* Disable str8100_counter_disable */

/*
 * IRQ handler for the timer
 */
static irqreturn_t
str8131_counter_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
#ifndef CONFIG_VIC_INTERRUPT
	// clear counter interrrupt status
	TIMER1_TIMER2_INTERRUPT_STATUS_REG &= ~(1 << TIMER2_OVERFLOW_INTERRUPT_BIT_INDEX);
#endif
	str8131_counter_tick += (1ULL << 32);
	if(match1){
		TIMER2_MATCH_VALUE1_REG=TIMER2_COUNTER_REG+match1;
		TIMER1_TIMER2_INTERRUPT_MASK_REG |= (0x1<<3);
	}
	if(match2){
		TIMER2_MATCH_VALUE2_REG=TIMER2_COUNTER_REG+match2;
		TIMER1_TIMER2_INTERRUPT_MASK_REG |= (0x2<<3);
	}
	return IRQ_HANDLED;
}

static struct irqaction str8131_counter_irq = {
	.name		= "STR9100 Counter Tick",
	.flags		= SA_INTERRUPT,
	.handler	= str8131_counter_interrupt,
};

static int str8131_counter_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	return sprintf(page, "str8131_counter_tick: %llu\n", str8131_counter_tick + TIMER2_COUNTER_REG);
}


static int
str8131_counter_write_proc(struct file *file, const char __user *buffer,
	unsigned long count, void *data)
{
	char *str;
	char *cmd;

	if (count > 0) {
		str = (char *)buffer,
		cmd = strsep(&str, "\t \n");
		if (!cmd) goto err_out;
		if (strcmp(cmd, "match1") == 0) {
			u32 addr;
			char *arg = strsep(&str, "\t \n");
			if (!arg) goto err_out;
			addr = simple_strtoul(arg, &arg, 10);
			match1=addr;

		} else if (strcmp(cmd, "match2") == 0) {
			u32 addr;
			char *arg = strsep(&str, "\t \n");
			if (!arg) goto err_out;
			addr = simple_strtoul(arg, &arg, 10);
			match2=addr;


		} else {
			goto err_out;
		}
	}
	if(match1){
		TIMER2_MATCH_VALUE1_REG=TIMER2_COUNTER_REG+match1;
		TIMER1_TIMER2_INTERRUPT_MASK_REG |= (0x1<<3);
	}
	if(match2){
		TIMER2_MATCH_VALUE2_REG=TIMER2_COUNTER_REG+match2;
		TIMER1_TIMER2_INTERRUPT_MASK_REG |= (0x2<<3);
	}

	return count;

err_out:
	return -EFAULT;
}
/*
 * Set up timer interrupt, and return the current time in seconds.
 */
int __init str8131_counter_setup(void)
{
	str8131_setup_counter();
	setup_irq(INTC_TIMER2_BIT_INDEX, &str8131_counter_irq);
	str8131_counter_enable();
	str8131_counter_proc_entry = create_proc_entry("str8131/counter", S_IFREG | S_IRUGO, NULL);
	if (str8131_counter_proc_entry) {
		str8131_counter_proc_entry->read_proc = str8131_counter_read_proc;
		str8131_counter_proc_entry->write_proc = str8131_counter_write_proc;
	}

	return 0;
}

