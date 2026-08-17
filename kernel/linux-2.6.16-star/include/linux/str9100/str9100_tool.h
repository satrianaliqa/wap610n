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

#ifndef STAR_TOOL_H
#define STAR_TOOL_H

#include <linux/version.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,4,32)
#define LINUX24 1
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#define LINUX26 1
#endif


#include <linux/types.h>




//extern int create_proc_dir;
//extern struct proc_dir_entry *str9100_gsw_procdir;


#define PROC_STR "str9100"



#ifdef LINUX24
struct proc_dir_entry *create_proc_str9100(const char* proc_str);
char *get_flash_env(const char *env_name);
const char *get_flash_type(volatile u16 *saddr);
#endif

#ifdef LINUX26
extern char *get_flash_env(const char *env_name);
extern const char *get_flash_type(void);
#endif


#endif



