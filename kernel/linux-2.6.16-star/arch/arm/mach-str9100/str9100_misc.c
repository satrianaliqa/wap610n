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

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/mach/map.h>
#include <asm/hardware.h>

#include <linux/delay.h>
#include "star_gsw_phy.h"

// enable I-Scratchpad
int str9100_enable_ispad(u32 base_addr)
{
	u32 ispad_config = 0;
	u32 flush_ispad = 0;
	u32 ispad_size = 4; // 8K on STR9100
	u32 cp15 = 1, cp15_off = 0;

	// Configure Base
	ispad_config |= (base_addr & 0xfffffc00);

	// Configure Size
	ispad_config &= ~(0xf << 2);
	ispad_config |= ((ispad_size & 0xf) << 2);	/* | (0x1 << 2); */

	// Enable
	ispad_config |= 0x1;

	// 1. set cp15, cr1-1(ECR) register value to 0x1
	// 2. set up the base and size configuration
	// 3. Invalidate IScratchpad All(flushed ISpad)
	// 4. clear cp15, cr1-1(ECR) register value to 0x0
	__asm__ __volatile__ (
	"mcr p15,0,%0,c1,c1,0\n\t"
	"mcr p15,0,%1,c9,c1,1\n\t"
	"mcr p15,0,%2,c7,c5,5\n\t"
	"mcr p15,0,%3,c1,c1,0\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	:
	: "r"(cp15), "r"(ispad_config), "r"(flush_ispad), "r"(cp15_off));
	
	return 0;
}

#ifdef CONFIG_CPU_DSPAD_ENABLE
#define STR9100_DSPAD_ALIGN_MASK	0x03

//#define STR9100_DSPAD_DBG

u8 *str9100_dspad_begin;
u8 *str9100_dspad_end;
u8 *str9100_dspad_pos;

void *str9100_dspad_alloc(size_t size)
{
	void *ptr;
	int aligned_sz;

	aligned_sz = (size + STR9100_DSPAD_ALIGN_MASK) & ~STR9100_DSPAD_ALIGN_MASK;
	if (str9100_dspad_pos + aligned_sz >= str9100_dspad_end) {
		ptr = kmalloc(size, GFP_ATOMIC);
		printk("Failed to allocate %d bytes from D-scratchpad, available %d\n",
			size, str9100_dspad_end - str9100_dspad_pos);
		printk("Allocated via kmalloc at %p\n", ptr);
	} else {
		ptr = str9100_dspad_pos;
		str9100_dspad_pos += aligned_sz;
#ifdef STR9100_DSPAD_DBG
		printk("Allocated %d bytes @ %p, free %d\n", size, ptr, str9100_dspad_end - str9100_dspad_pos);
#endif
	}
	return ptr;
}
EXPORT_SYMBOL(str9100_dspad_alloc);

void str9100_dspad_free(void *ptr)
{
#ifdef STR9100_DSPAD_DBG
	printk("Free D-scratchpad at %p ", ptr);
#endif
	if (((u8 *)ptr < str9100_dspad_begin) || ((u8 *)ptr > str9100_dspad_end)) {
#ifdef STR9100_DSPAD_DBG
		printk("via kfree\n");
#endif
		kfree(ptr);
	} else {
		/* must be freed in reverse order only */
#ifdef STR9100_DSPAD_DBG
		printk("\n");
#endif
		str9100_dspad_pos = (u8 *)ptr;
	}
}
EXPORT_SYMBOL(str9100_dspad_free);
#endif

// enable D-Scratchpad
int str9100_enable_dspad(unsigned long base_addr, unsigned long size)
{
        u32 dspad_config = 0;
        u32 flush_dspad = 0;
        u32 dspad_size = 4; // 8K on STR9100
        u32 cp15 = 1, cp15_off = 0;

        // Configure Base
        dspad_config |= (base_addr & 0xfffffc00);

        // Configure Size
        dspad_config &= ~(0xf << 2);
        dspad_config |= ((dspad_size & 0xf) << 2); 	/* | (0x1 << 2); */

        // Enable
        dspad_config |= 0x1;

        __asm__ __volatile__ (
        "mcr p15,0,%0,c1,c1,0\n\t"
        "mcr p15,0,%1,c9,c1,0\n\t"
        "mcr p15,0,%2,c7,c6,5\n\t"
        "mcr p15,0,%3,c1,c1,0\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        :
        : "r"(cp15), "r"(dspad_config), "r"(flush_dspad), "r"(cp15_off));

#ifdef CONFIG_CPU_DSPAD_ENABLE
	str9100_dspad_begin = str9100_dspad_pos = (u8 *)base_addr;
	str9100_dspad_end = str9100_dspad_begin + size;
	printk("D-scratchpad of size %lu at %lx\n", size, base_addr);
#endif
        return 0;
}

static struct map_desc str9100_map_desc[64];
static int str9100_map_desc_count;
#define REG_DEBUG_CMD_BUFFER_SIZE	128
#define REG_DEBUG_RESULT_BUFFER_SIZE	256
static struct proc_dir_entry *star_reg_debug_proc_entry;
static char str9100_reg_debug_cmd_buf[REG_DEBUG_CMD_BUFFER_SIZE];
static char str9100_reg_debug_result_buf[REG_DEBUG_RESULT_BUFFER_SIZE];

struct proc_dir_entry *str9100_proc_dir;
EXPORT_SYMBOL(str9100_proc_dir);

void str9100_register_map_desc(struct map_desc *map, int count)
{
	if (count) {
		if (str9100_map_desc) {
			int i;
			for (i = 0; i < count; i++) {
				str9100_map_desc[i].virtual = map->virtual;
				str9100_map_desc[i].pfn = map->pfn;
				str9100_map_desc[i].length = map->length;
				str9100_map_desc[i].type = map->type;
				map++;
			}
			str9100_map_desc_count = count;
		}
	}
}

u32 str9100_query_map_desc_by_phy(u32 addr)
{
	struct map_desc *map;
	int i;
	u32 ret_addr = 0;
	for (i = 0; i < str9100_map_desc_count; i++) {
		map = &str9100_map_desc[i];
		if (addr >= (map->pfn << PAGE_SHIFT) && addr < ((map->pfn << PAGE_SHIFT) + map->length)) {
			ret_addr = map->virtual + (addr - (map->pfn << PAGE_SHIFT));
			break;
		}
	}

	return ret_addr;
}

u32 str9100_query_map_desc_by_vir(u32 addr)
{
	struct map_desc *map;
	int i;
	u32 ret_addr = 0;
	for (i = 0; i < str9100_map_desc_count; i++) {
		map = &str9100_map_desc[i];
		if (addr >= map->virtual && addr < (map->virtual + map->length)) {
			ret_addr = (map->pfn << PAGE_SHIFT) + (addr - map->virtual);
			break;
		}
	}

	return ret_addr;
}

static int star_reg_debug_read_proc(char *buffer, char **start, off_t offset,
	int length, int *eof, void *data)
{
	int count;
	int num = 0;

	if (str9100_reg_debug_cmd_buf[0]) {
		count = strlen(str9100_reg_debug_cmd_buf);
		sprintf(buffer, str9100_reg_debug_cmd_buf, count);
		num += count;
	}
	if (str9100_reg_debug_result_buf[0]) {
		count = strlen(str9100_reg_debug_result_buf);
		sprintf(buffer + num, str9100_reg_debug_result_buf, count);
		num += count;
	}

	return num;
}

//Tim Wang, for PHY issue debug only
static int star_gsw_read_phy(u8 phy_addr, u8 phy_reg, u16 volatile *read_data)
{
	u32 status;
	int i;

	// clear previous rw_ok status
	GSW_PHY_CONTROL = (0x1 << 15);

        // 20061013 descent 
	// for ORION EOC
        GSW_QUEUE_STATUS_TEST_1 &= ~( 0XF << 16);

        GSW_PHY_CONTROL   &= ~(0x1<<0);

        GSW_QUEUE_STATUS_TEST_1 |= (((phy_addr >> 1) & 0xF) << 16);
        // 20061013 descent end


	GSW_PHY_CONTROL = ((phy_addr & 0x1) | ((phy_reg & 0x1F) << 8) | (0x1 << 14));

	for (i = 0; i < 0x1000; i++) {
		status = GSW_PHY_CONTROL;
		if (status & (0x1 << 15)) {
			// clear the rw_ok status, and clear other bits value
			GSW_PHY_CONTROL = (0x1 << 15);
			*read_data = (u16) ((status >> 16) & 0xFFFF);
			return (1);
		} else {
			udelay(10);
		}
	}

	return (0);
}

static int star_gsw_write_phy(u8 phy_addr, u8 phy_reg, u16 write_data)
{
	int i;

	// clear previous rw_ok status
	GSW_PHY_CONTROL = (0x1 << 15);

        // 20061013 descent 
	// for ORION EOC
        GSW_QUEUE_STATUS_TEST_1 &= ~( 0XF << 16);

        GSW_PHY_CONTROL   &= ~(0x1<<0);

        GSW_QUEUE_STATUS_TEST_1 |= (((phy_addr >> 1) & 0xF) << 16);
        // 20061013 descent end


	GSW_PHY_CONTROL = ((phy_addr & 0x1) |
		((phy_reg & 0x1F) << 8) |
		(0x1 << 13) | ((write_data & 0xFFFF) << 16));

	for (i = 0; i < 0x1000; i++) {
		if ((GSW_PHY_CONTROL) & (0x1 << 15)) {
			// clear the rw_ok status, and clear other bits value
			GSW_PHY_CONTROL = (0x1 << 15);
			return (1);
		} else {
			udelay(10);
		}
	}

	return (0);
}

static int star_gsw_set_phy_addr(u8 mac_port, u8 phy_addr)
{
	u32 status = 0;	/* for failure indication */

	if ((mac_port > 1) || (phy_addr > 31)) {
		return status;
	}

	if (mac_port == 0) {
		GSW_PORT_MIRROR &= ~(0x3 << 0); /* clear bit[1:0] for PHY_ADDR[1:0] */
		GSW_PORT_MIRROR &= ~(0x3 << 4); /* clear bit[5:4] for PHY_ADDR[3:2] */
		GSW_QUEUE_STATUS_TEST_1 &= ~(0x1 << 25); /* clear bit[25] for PHY_ADDR[4] */
		GSW_PORT_MIRROR |= (((phy_addr >> 0) & 0x3) << 0);
		GSW_PORT_MIRROR |= (((phy_addr >> 2) & 0x3) << 4);
		GSW_QUEUE_STATUS_TEST_1 |= (((phy_addr >> 4) & 0x1) << 25);
		status = 1; /* for ok indication */
	} else if (mac_port == 1) {
		GSW_PORT_MIRROR &= ~(0x1 << 6); /* clear bit[6] for PHY_ADDR[0] */
		GSW_PORT_MIRROR &= ~(0x7 << 8); /* clear bit[10:8] for PHY_ADDR[3:1] */
		GSW_QUEUE_STATUS_TEST_1 &= ~(0x1 << 26); /* clear bit[26] for PHY_ADDR[4] */
		GSW_PORT_MIRROR |= (((phy_addr >> 0) & 0x1) << 6);
		GSW_PORT_MIRROR |= (((phy_addr >> 1) & 0x7) << 8);
		GSW_QUEUE_STATUS_TEST_1 |= (((phy_addr >> 4) & 0x1) << 26);
		status = 1; /* for ok indication */
	}

	return status;
}

//U-MEDIA, work around for RTL8201CP	
static void reset_mac(void)
{
	GSW_MAC_PORT_0_CONFIG &= (~(0x1 << 7));  //clear AN
	return;
}

static void reset_rtl8201cp(void)
{
	u32 mac_port_config;
	u16 phy_data = 0;
	u8 phy_addr=1;
	u8 phy_reg=0;
	int i;
	
	star_gsw_read_phy(phy_addr, phy_reg, &phy_data);
	phy_data |= (0x1 << 11);	//down
	star_gsw_write_phy(phy_addr, phy_reg, phy_data);
	for (i=0;i<1000;i++) {
		udelay(1000);
	}
	star_gsw_read_phy(phy_addr, phy_reg, &phy_data);
	phy_data &= (~(0x1 << 11));  //power on
	phy_data |= (0x1 << 15);  //software reset
	star_gsw_write_phy(phy_addr, phy_reg, phy_data);
	for (i=0;i<100;i++) {			
		udelay(1000);		
	}		
	return;
	
	phy_data = 0x2100;
	star_gsw_write_phy(phy_addr, phy_reg, phy_data);
	for (i=0;i<100;i++) {			
		udelay(1000);		
	}		
	GSW_MAC_PORT_0_CONFIG &= (~(0x1 << 7));  //clear AN
	for (i=0;i<100;i++) {			
		udelay(1000);		
	}		
	return;	
	star_gsw_set_phy_addr(0, 1);
	// power-down or up the PHY
	star_gsw_read_phy(phy_addr, phy_reg, &phy_data);
	phy_data |= (0x1 << 11);	//down
	star_gsw_write_phy(phy_addr, phy_reg, phy_data);
	for (i=0;i<1000;i++) {
		udelay(1000);
	}
	phy_data &= (~(0x1 << 11));  //power on
	phy_data |= (0x1 << 15);  //software reset
	star_gsw_write_phy(phy_addr, phy_reg, phy_data);
}

static u16 read_rtl8201cp(u8 phy_reg)
{
	u32 mac_port_config;
	u16 phy_data = 0;
	u8 phy_addr=1;
	int i;
	
	star_gsw_set_phy_addr(0, 1);
	// power-down or up the PHY
	star_gsw_read_phy(phy_addr, phy_reg, &phy_data);
	return phy_data;
}


static void check_phy(void)
{
	u32 mac_port_config;
	u16 phy_data = 0;
	u8 phy_addr=1;
	u8 phy_reg=0;
	u8 port=0;
	int i;
	
	if (read_rtl8201cp(3)==0x8201) {
		star_gsw_set_phy_addr(port, phy_addr);
		GSW_MAC_PORT_0_CONFIG |= (0x1 << 7);  //enable AN
		reset_rtl8201cp();
	}
}


//Tim Wang, endof PHY issue debug



static int
star_reg_debug_write_proc(struct file *file, const char __user *buffer,
	unsigned long count, void *data)
{
	char *str;
	char *cmd;

	if (count > 0) {
		str = (char *)buffer,
		cmd = strsep(&str, "\t \n");
		if (!cmd) goto err_out;
		if (strcmp(cmd, "dump") == 0) {
			u32 addr;
			u32 vir_addr;
			char *arg = strsep(&str, "\t \n");
			if (!arg) goto err_out;
			addr = simple_strtoul(arg, &arg, 16);
			if (addr & 0x3) goto err_out;
			vir_addr = str9100_query_map_desc_by_phy(addr);
			sprintf(str9100_reg_debug_cmd_buf,
				"dump 0x%08x\n",
				addr);
			if (!vir_addr) goto err_out;
			sprintf(str9100_reg_debug_result_buf,
				"physical addr: 0x%08x content: 0x%08x\n",
				addr,
				*(volatile unsigned int __force *)(vir_addr));
		} else if (strcmp(cmd, "write") == 0) {
			u32 addr;
			u32 vir_addr;
			u32 data;
			char *arg = strsep(&str, "\t \n");
			if (!arg) goto err_out;
			addr = simple_strtoul(arg, &arg, 16);
			arg = strsep(&str, "\t \n");
			if (!arg) goto err_out;
			data = simple_strtoul(arg, &arg, 16);
			if (addr & 0x3) goto err_out;
			vir_addr = str9100_query_map_desc_by_phy(addr);
			if (!vir_addr) goto err_out;
			*(volatile unsigned int __force *)(vir_addr) = data;
			sprintf(str9100_reg_debug_cmd_buf,
				"write 0x%08x 0x%08x\n",
				addr, data);
			sprintf(str9100_reg_debug_result_buf,
				"physical addr: 0x%08x content: 0x%08x\n",
				addr,
				*(volatile unsigned int __force *)(vir_addr));
		} else if (strcmp(cmd, "pci33") == 0) {
			printk("enable pci 33M mode\n");
			HAL_PWRMGT_ENABLE_PCI_BRIDGE_33M();
		} else if (strcmp(cmd, "pci66") == 0) {
			printk("enable pci 66M mode\n");
			HAL_PWRMGT_ENABLE_PCI_BRIDGE_66M();
//Tim Wang, debug PHY issue
		} else if (strcmp(cmd, "reset_mac") == 0) {
			reset_mac();
		} else if (strcmp(cmd, "reset_phy") == 0) {
			reset_rtl8201cp();
		} else if (strcmp(cmd, "check_phy") == 0) {
			check_phy();
		} else if (strcmp(cmd, "read_phy") == 0) {
			sprintf(str9100_reg_debug_cmd_buf,
				"read_phy GSW_MAC_PORT_0_CONFIG=0x%08x\n",
				GSW_MAC_PORT_0_CONFIG);
			sprintf(str9100_reg_debug_result_buf, 
				"phy 1- content reg0=0x%04x \t reg1=0x%04x \t reg2=0x%04x \t reg3=0x%04x\n", 
				read_rtl8201cp(0), read_rtl8201cp(1), read_rtl8201cp(2), read_rtl8201cp(3));
		} else {
			goto err_out;
		}
	}

	return count;

err_out:
	return -EFAULT;
}

static int __init star_reg_debug_proc_init(void)
{
	star_reg_debug_proc_entry = create_proc_entry("str9100/reg_debug", S_IFREG | S_IRUGO, NULL);
	if (star_reg_debug_proc_entry) {
		star_reg_debug_proc_entry->read_proc = star_reg_debug_read_proc;
		star_reg_debug_proc_entry->write_proc = star_reg_debug_write_proc;
	}

	return 0;
}

static int __init str9100_proc_dir_create(void)
{
	str9100_proc_dir = proc_mkdir("str9100", NULL);
	if (str9100_proc_dir) {
		str9100_proc_dir->owner = THIS_MODULE;
	} else {
		printk("Error: cannot crete str9100 proc dir entry at /proc/str9100\n");
		return -EINVAL;
	}

	if (str9100_map_desc_count) {
		(void)star_reg_debug_proc_init();
	}

	return 0;
}

extern int __init str9100_counter_setup(void);
static int __init str9100_misc_init(void)
{
	str9100_proc_dir_create();
	str9100_counter_setup();
	return 0;
}

module_init(str9100_misc_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Star Semi Corporation");
MODULE_DESCRIPTION("STR9100 MISC");

