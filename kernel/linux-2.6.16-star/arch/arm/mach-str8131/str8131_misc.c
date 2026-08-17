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

static struct map_desc str8131_map_desc[64];
static int str8131_map_desc_count;

#define REG_DEBUG_CMD_BUFFER_SIZE	128
#define REG_DEBUG_RESULT_BUFFER_SIZE	256

struct proc_dir_entry *str8131_proc_dir;
EXPORT_SYMBOL(str8131_proc_dir);
static struct proc_dir_entry *star_reg_debug_proc_entry;
static char str8131_reg_debug_cmd_buf[REG_DEBUG_CMD_BUFFER_SIZE];
static char str8131_reg_debug_result_buf[REG_DEBUG_RESULT_BUFFER_SIZE];

void str8131_register_map_desc(struct map_desc *map, int count)
{
	if (count) {
		if (str8131_map_desc) {
			int i;
			for (i = 0; i < count; i++) {
				str8131_map_desc[i].virtual = map->virtual;
				str8131_map_desc[i].pfn = map->pfn;
				str8131_map_desc[i].length = map->length;
				str8131_map_desc[i].type = map->type;
				map++;
			}
			str8131_map_desc_count = count;
		}
	}
}

u32 str8131_query_map_desc_by_phy(u32 addr)
{
	struct map_desc *map;
	int i;
	u32 ret_addr = 0;
	for (i = 0; i < str8131_map_desc_count; i++) {
		map = &str8131_map_desc[i];
		if (addr >= (map->pfn << PAGE_SHIFT) && addr < ((map->pfn << PAGE_SHIFT) + map->length)) {
			ret_addr = map->virtual + (addr - (map->pfn << PAGE_SHIFT));
			break;
		}
	}

	return ret_addr;
}

u32 str8131_query_map_desc_by_vir(u32 addr)
{
	struct map_desc *map;
	int i;
	u32 ret_addr = 0;
	for (i = 0; i < str8131_map_desc_count; i++) {
		map = &str8131_map_desc[i];
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

	if (str8131_reg_debug_cmd_buf[0]) {
		count = strlen(str8131_reg_debug_cmd_buf);
		sprintf(buffer, str8131_reg_debug_cmd_buf, count);
		num += count;
	}
	if (str8131_reg_debug_result_buf[0]) {
		count = strlen(str8131_reg_debug_result_buf);
		sprintf(buffer + num, str8131_reg_debug_result_buf, count);
		num += count;
	}

	return num;
}

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
			vir_addr = str8131_query_map_desc_by_phy(addr);
			sprintf(str8131_reg_debug_cmd_buf,
				"dump 0x%08x\n",
				addr);
			if (!vir_addr) goto err_out;
			sprintf(str8131_reg_debug_result_buf,
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
			vir_addr = str8131_query_map_desc_by_phy(addr);
			if (!vir_addr) goto err_out;
			*(volatile unsigned int __force *)(vir_addr) = data;
			sprintf(str8131_reg_debug_cmd_buf,
				"write 0x%08x 0x%08x\n",
				addr, data);
			sprintf(str8131_reg_debug_result_buf,
				"physical addr: 0x%08x content: 0x%08x\n",
				addr,
				*(volatile unsigned int __force *)(vir_addr));
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
	star_reg_debug_proc_entry = create_proc_entry("str8131/reg_debug", S_IFREG | S_IRUGO, NULL);
	if (star_reg_debug_proc_entry) {
		star_reg_debug_proc_entry->read_proc = star_reg_debug_read_proc;
		star_reg_debug_proc_entry->write_proc = star_reg_debug_write_proc;
	}

	return 0;
}

static int __init str8131_proc_dir_create(void)
{
	str8131_proc_dir = proc_mkdir("str8131", NULL);
	if (str8131_proc_dir) {
		str8131_proc_dir->owner = THIS_MODULE;
	} else {
		printk("Error: cannot crete str8131 proc dir entry at /proc/str8131\n");
		return -EINVAL;
	}

	if (str8131_map_desc_count) {
		(void)star_reg_debug_proc_init();
	}

	return 0;
}

extern int __init str8131_counter_setup(void);
static int __init str8131_misc_init(void)
{
	str8131_proc_dir_create();
	str8131_counter_setup();
	return 0;
}

module_init(str8131_misc_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Star Semi Corporation");
MODULE_DESCRIPTION("STR8131 MISC");

