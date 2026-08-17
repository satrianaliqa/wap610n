/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Misc boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>


/* -------------------------------------------------------------------- */

//june.chen, 2010-11-25, for passing arguments to kernel in go function
#ifdef CONFIG_UMEDIA_DISABLE_CONSOLE
extern uint g_disable_console_flag;
static struct tag *params;

static void set_linux_args(bd_t *bd)
{
	//step 1, set arg tag CORE
	params = (struct tag *) bd->bi_boot_params;
	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size (tag_core);
	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;
	params = tag_next (params);

	//step 2, set arg tag CMD
	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size = (sizeof (struct tag_header) + strlen (CONFIG_UMEDIA_KERNEL_BOOTARGS) + 1 + 4) >> 2;
	strcpy (params->u.cmdline.cmdline, CONFIG_UMEDIA_KERNEL_BOOTARGS);
	params = tag_next (params);

	//step3, set arg tag NONE
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

#endif
//june.chen end

int do_go (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
#if defined(CONFIG_I386)
	DECLARE_GLOBAL_DATA_PTR;
#endif
	ulong	addr, rc;
	int     rcode = 0;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	addr = simple_strtoul(argv[1], NULL, 16);

	printf ("## Starting application at 0x%08lX ...\n", addr);

	/*
	 * pass address parameter as argv[0] (aka command name),
	 * and all remaining args
	 */
#if defined(CONFIG_I386)
	/*
	 * x86 does not use a dedicated register to pass the pointer
	 * to the global_data
	 */
	argv[0] = (char *)gd;
#endif
#if !defined(CONFIG_NIOS)
//june.chen, 2010-11-25, pass args to linux kernel 
#ifdef CONFIG_UMEDIA_DISABLE_CONSOLE
	if(g_disable_console_flag == 1){
		DECLARE_GLOBAL_DATA_PTR;
		bd_t *bd = gd->bd;
		set_linux_args(bd);

	}
	rc = ((ulong (*)(int, char *[]))addr) (--argc, &argv[1]);
#else
	rc = ((ulong (*)(int, char *[]))addr) (--argc, &argv[1]);
#endif
//june.chen end
#else
	/*
	 * Nios function pointers are address >> 1
	 */
	rc = ((ulong (*)(int, char *[]))(addr>>1)) (--argc, &argv[1]);
#endif
	if (rc != 0) rcode = 1;

	printf ("## Application terminated, rc = 0x%lX\n", rc);
	return rcode;
}

/* -------------------------------------------------------------------- */

U_BOOT_CMD(
	go, CFG_MAXARGS, 1,	do_go,
	"go      - start application at address 'addr'\n",
	"addr [arg ...]\n    - start application at address 'addr'\n"
	"      passing 'arg' as arguments\n"
);

extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

U_BOOT_CMD(
	reset, 1, 0,	do_reset,
	"reset   - Perform RESET of the CPU\n",
	NULL
);
