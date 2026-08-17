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

#include <linux/mm.h>
#include <linux/init.h>
#include <linux/config.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/serial_8250.h>
#ifdef CONFIG_SPI_CETUSPLUS
#include <linux/spi/spi.h>
#if defined(CONFIG_MTD_M25P80) || defined(CONFIG_MTD_M25P80_MODULE)
#include <linux/spi/flash.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#endif
#endif // CONFIG_SPI_CETUSPLUS

#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/system.h>
#include <asm/memory.h>
#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#if 0 // on ASIC
#define CETUSPLUS_UART_XTAL 24000000
#else // on FPGA
#define CETUSPLUS_UART_XTAL 13000000
#endif

#define EARLY_REGISTER_CONSOLE
#define DEVICE_REGISTER_MULTIPLE

/*
 * Standard IO mapping
 */
static struct map_desc cetusplus_std_desc[] __initdata = {
	{
		.virtual	= SYSVA_GDMAC_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_GDMAC_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_SWITCH_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_SWITCH_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_I2C_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_I2C_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_DRAMC_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_DRAMC_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_SMC_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_SMC_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_MISC_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_MISC_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_POWER_MANAGEMENT_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_POWER_MANAGEMENT_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_UART0_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_UART0_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_UART1_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_UART1_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_TIMER_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_TIMER_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_WATCHDOG_TIMER_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_WATCHDOG_TIMER_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_RTC_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_RTC_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_GPIOA_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_GPIOA_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_GPIOB_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_GPIOB_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_IDP_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_IDP_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_PCI_BRIDGE_CONFIG_DATA_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_PCI_BRIDGE_CONFIG_DATA_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_PCI_BRIDGE_CONFIG_ADDR_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_PCI_BRIDGE_CONFIG_ADDR_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_USB20_CONFIG_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_USB20_CONFIG_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_VITESSE_SWITCH_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_VITESSE_SWITCH_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}, {
		.virtual	= SYSVA_VIC_BASE_ADDR,
		.pfn		= __phys_to_pfn(SYSPA_VIC_BASE_ADDR),
		.length		= SZ_4K,
		.type		= MT_DEVICE
	}
};

#ifdef EARLY_REGISTER_CONSOLE
static struct uart_port cetusplus_serial_ports[] = {
	{
		.membase	= (char*)(SYSVA_UART0_BASE_ADDR),
		.mapbase	= (SYSPA_UART0_BASE_ADDR),
		.irq		= INTC_UART0_BIT_INDEX,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= CETUSPLUS_UART_XTAL,
		.line		= 0,
		.type		= PORT_16550A,
		.fifosize	= 16
	} , {
		.membase	= (char*)(SYSVA_UART1_BASE_ADDR),
		.mapbase	= (SYSPA_UART1_BASE_ADDR),
		.irq		= INTC_UART1_BIT_INDEX,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= CETUSPLUS_UART_XTAL,
		.line		= 1,
		.type		= PORT_16550A,
		.fifosize	= 16
	}
};
#else
static struct resource cetusplus_uart0_resources[] = {
	[0] = {
		.start	= SYSPA_UART0_BASE_ADDR,
		.end	= SYSPA_UART0_BASE_ADDR + 0xff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= INTC_UART0_BIT_INDEX,
		.end	= INTC_UART0_BIT_INDEX,
		.flags	= IORESOURCE_IRQ
	}
};

static struct resource cetusplus_uart1_resources[] = {
	[0] = {
		.start	= SYSPA_UART1_BASE_ADDR,
		.end	= SYSPA_UART1_BASE_ADDR + 0xff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= INTC_UART1_BIT_INDEX,
		.end	= INTC_UART1_BIT_INDEX,
		.flags	= IORESOURCE_IRQ
	}
};

static struct plat_serial8250_port cetusplus_uart0_data[] = {
	{
		.membase	= (char*)(SYSVA_UART0_BASE_ADDR),
		.mapbase	= (SYSPA_UART0_BASE_ADDR),
		.irq		= INTC_UART0_BIT_INDEX,
		.uartclk	= CETUSPLUS_UART_XTAL,
		.regshift	= 2,
		.iotype		= UPIO_MEM,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
	},
	{  },
};

static struct plat_serial8250_port cetusplus_uart1_data[] = {
	{
		.membase	= (char*)(SYSVA_UART1_BASE_ADDR),
		.mapbase	= (SYSPA_UART1_BASE_ADDR),
		.irq		= INTC_UART1_BIT_INDEX,
		.uartclk	= CETUSPLUS_UART_XTAL,
		.regshift	= 2,
		.iotype		= UPIO_MEM,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
	},
	{  },
};

static struct platform_device cetusplus_uart0_device = {
	.name = "serial8250",
	.id = 0,
	.dev.platform_data = cetusplus_uart0_data,
	.num_resources = 2,
	.resource = cetusplus_uart0_resources,
};

static struct platform_device cetusplus_uart1_device = {
	.name = "serial8250",
	.id = 1,
	.dev.platform_data = cetusplus_uart1_data,
	.num_resources = 2,
	.resource = cetusplus_uart1_resources,
};
#endif

#if 0
static u64 usb_dmamask = 0xffffffffULL;
static struct resource cetusplus_usb11_resources[] = {
	[0] = {
		.start	= SYSPA_USB20_CONFIG_BASE_ADDR,
		.end	= SYSPA_USB20_CONFIG_BASE_ADDR + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= INTC_USB20_BIT_INDEX,
		.end	= INTC_USB20_BIT_INDEX,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device cetusplus_usb11_device = {
	.name		= "cetusplus-ohci",
	.id		= -1,
	.dev = {
		.dma_mask		= &usb_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.resource	= cetusplus_usb11_resources,
	.num_resources	= ARRAY_SIZE(cetusplus_usb11_resources),
};

static struct resource cetusplus_usb20_resources[] = {
	[0] = {
		.start	= SYSPA_USB20_CONFIG_BASE_ADDR,
		.end	= SYSPA_USB20_CONFIG_BASE_ADDR + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= INTC_USB20_BIT_INDEX,
		.end	= INTC_USB20_BIT_INDEX,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device cetusplus_usb20_device = {
	.name		= "cetusplus-ehci",
	.id		= -1,
	.dev		= {
		.dma_mask		= &usb_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.resource	= cetusplus_usb20_resources,
	.num_resources	= ARRAY_SIZE(cetusplus_usb20_resources),
};
#endif

#ifdef CONFIG_SPI_CETUSPLUS
#if defined(CONFIG_MTD_M25P80) || defined(CONFIG_MTD_M25P80_MODULE)
static struct mtd_partition cetusplus_spi_flash_partitions[] = {
	{
		.name =		"ARMBOOT",
		.offset =	CONFIG_ARMBOOT_OFFSET,
		.size =		CONFIG_KERNEL_OFFSET-CONFIG_ARMBOOT_OFFSET,
	},{
		.name =		"Linux Kernel",
		.offset =	CONFIG_KERNEL_OFFSET,
		.size =		CONFIG_ROOTFS_OFFSET-CONFIG_KERNEL_OFFSET,
	},{
		.name =		"MTD Disk1",
		.offset =	CONFIG_ROOTFS_OFFSET,
		.size =		CONFIG_CFG_OFFSET-CONFIG_ROOTFS_OFFSET,
	},{
		.name =		"MTD Disk2",
		.offset =	CONFIG_CFG_OFFSET,
		.size =		0x800000-CONFIG_CFG_OFFSET,
	}
};

static struct flash_platform_data cetusplus_spi_flash_data = {
	.name = "m25p80",
	.parts = cetusplus_spi_flash_partitions,
	.nr_parts = ARRAY_SIZE(cetusplus_spi_flash_partitions),
	.type = "m25p64",
};

static struct spi_board_info cetusplus_spi_board_info[] = {
        {
		.modalias	= "m25p80",
		.chip_select	= 0,
		.max_speed_hz	= 25 * 1000 * 1000,
		.bus_num	= 1,
		.platform_data = &cetusplus_spi_flash_data,
        },
};
#endif

static u64 spi_dmamask = 0xffffffffUL;
static struct resource cetusplus_spi_resources[] = {
	[0] = {
		.start	= SYSPA_SPI_BASE_ADDR,
		.end	= SYSPA_SPI_BASE_ADDR + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= INTC_SPI_BIT_INDEX,
		.end	= INTC_SPI_BIT_INDEX,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device cetusplus_spi_master_device = {
	.name		= "cetusplus_spi",
	.id		= -1,
	.dev		= {
		.dma_mask		= &spi_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.resource	= cetusplus_spi_resources,
	.num_resources	= ARRAY_SIZE(cetusplus_spi_resources),
};
#endif // CONFIG_SPI_CETUSPLUS

#ifdef DEVICE_REGISTER_MULTIPLE
static struct platform_device *cetusplus_devices[] __initdata = {
#ifndef EARLY_REGISTER_CONSOLE
	&cetusplus_uart0_device,
	&cetusplus_uart1_device,
#endif
#ifdef CONFIG_SPI_CETUSPLUS
	&cetusplus_spi_master_device,
#endif
#if 0
	&cetusplus_usb11_device,
	&cetusplus_usb20_device
#endif
};
#endif

static void __init cetusplus_fixup(struct machine_desc *desc,
	struct tag *tags, char **cmdline, struct meminfo *mi)
{
        mi->nr_banks = 1;
	mi->bank[0].start = CONFIG_SYSTEM_DRAM_BASE;
	mi->bank[0].size = CONFIG_SYSTEM_DRAM_SIZE << 20;
	mi->bank[0].node = 0;
}

/* ######################################################################### */
#ifdef CONFIG_CPU_ISPAD_ENABLE 
extern unsigned long __ispad_begin; 
#endif
u32 PLL_clock;
u32 CPU_clock;
u32 AHB_clock;
u32 APB_clock;
EXPORT_SYMBOL(PLL_clock);
EXPORT_SYMBOL(CPU_clock);
EXPORT_SYMBOL(AHB_clock);
EXPORT_SYMBOL(APB_clock);
// This function is called just after the
// page table and cpu have been initialized
void __init cetusplus_early_init(void)
{
	int pll2cpu;
	int cpu2ahb;
	int ahb2apb;

#if 1 // On FPGA
	PLL_clock = 13000000;
	CPU_clock = 13000000;
	AHB_clock = 13000000;
	APB_clock = 13000000;
#else // On ASIC
	switch (PWRMGT_SYSTEM_CLOCK_CONTROL_REG & 0xF) {
	case PWRMGT_PLL_FREQUENCY_166MHZ:
		PLL_clock = 166000000;
		break;
	case PWRMGT_PLL_FREQUENCY_200MHZ:
		PLL_clock = 200000000;
		break;
	case PWRMGT_PLL_FREQUENCY_233MHZ:
		PLL_clock = 233000000;
		break;
	case PWRMGT_PLL_FREQUENCY_266MHZ:
		PLL_clock = 266000000;
		break;
	case PWRMGT_PLL_FREQUENCY_300MHZ:
		PLL_clock = 300000000;
		break;
	case PWRMGT_PLL_FREQUENCY_333MHZ:
		PLL_clock = 333000000;
		break;
	case PWRMGT_PLL_FREQUENCY_366MHZ:
		PLL_clock = 366000000;
		break;
	case PWRMGT_PLL_FREQUENCY_400MHZ:
		PLL_clock = 400000000;
		break;
	}

	pll2cpu = (PWRMGT_SYSTEM_CLOCK_CONTROL_REG >> 4) & 0x3;
	if ((pll2cpu == 0) || (pll2cpu == 1)) {
		pll2cpu += 1;
	} else if ((pll2cpu == 2) || (pll2cpu == 3)) {
		pll2cpu = 4;
	}

	cpu2ahb = (PWRMGT_SYSTEM_CLOCK_CONTROL_REG >> 6) & 0x3;
	if ((cpu2ahb == 0) || (cpu2ahb == 1)) {
		cpu2ahb += 1;
	} else if ((cpu2ahb == 2) || (cpu2ahb == 3)) {
		cpu2ahb = 4;
	}

	ahb2apb = 2;

	CPU_clock = PLL_clock / pll2cpu;
	AHB_clock = CPU_clock / cpu2ahb;
	APB_clock = AHB_clock / ahb2apb;
#endif

	printk("PLL clock at %dMHz\n", PLL_clock / 1000000);
	printk("CPU clock at %dMHz\n", CPU_clock / 1000000);
	printk("AHB clock at %dMHz\n", AHB_clock / 1000000);
	printk("APB clock at %dMHz\n", APB_clock / 1000000);
}
/* ######################################################################### */

void __init cetusplus_init(void)
{
#ifdef DEVICE_REGISTER_MULTIPLE
	platform_add_devices(cetusplus_devices, ARRAY_SIZE(cetusplus_devices));
#ifdef CONFIG_SPI_CETUSPLUS
#if defined(CONFIG_MTD_M25P80) || defined(CONFIG_MTD_M25P80_MODULE)
	spi_register_board_info(cetusplus_spi_board_info, ARRAY_SIZE(cetusplus_spi_board_info));
#endif
#endif // CONFIG_SPI_CETUSPLUS
#else // DEVICE_REGISTER_MULTIPLE
#ifndef EARLY_REGISTER_CONSOLE
	platform_device_register(&cetusplus_uart0_device);
	platform_device_register(&cetusplus_uart1_device);
#endif
#ifdef CONFIG_SPI_CETUSPLUS
	platform_device_register(&cetusplus_spi_master_device);
#if defined(CONFIG_MTD_M25P80) || defined(CONFIG_MTD_M25P80_MODULE)
	spi_register_board_info(cetusplus_spi_board_info, ARRAY_SIZE(cetusplus_spi_board_info));
#endif
#endif
	platform_device_register(&cetusplus_usb11_device);
	platform_device_register(&cetusplus_usb20_device);
#endif
}

extern void cetusplus_register_map_desc(struct map_desc *map, int count);
void __init cetusplus_map_io(void)
{
	iotable_init(cetusplus_std_desc, ARRAY_SIZE(cetusplus_std_desc));
	cetusplus_register_map_desc(cetusplus_std_desc, ARRAY_SIZE(cetusplus_std_desc));
#ifdef EARLY_REGISTER_CONSOLE
	early_serial_setup(&cetusplus_serial_ports[0]);
	early_serial_setup(&cetusplus_serial_ports[1]);
#endif
}

extern void cetusplus_init_irq(void);
extern struct sys_timer cetusplus_timer;

MACHINE_START(CETUSPLUS, "STAR CETUSPLUS")
	.phys_io	= SYSPA_UART0_BASE_ADDR,
	.io_pg_offst	= ((SYSVA_UART0_BASE_ADDR) >> 18) & 0xfffc, // virtual, physical
	.fixup		= cetusplus_fixup,
	.map_io		= cetusplus_map_io,
	.init_irq	= cetusplus_init_irq,
	.timer		= &cetusplus_timer,
	.boot_params	= 0x0100,
	.init_machine	= cetusplus_init,
MACHINE_END

