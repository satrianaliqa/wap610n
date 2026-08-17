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

#include <linux/platform_device.h>
#include <asm/arch/hardware.h>

static int str8131_ehci_setup(struct usb_hcd *hcd)
{
	struct ehci_hcd		*ehci = hcd_to_ehci(hcd);
	int			retval;

	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(readl(&ehci->caps->hc_capbase));
	dbg_hcs_params(ehci, "reset");
	dbg_hcc_params(ehci, "reset");

	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

	retval = ehci_halt(ehci);
	if (retval)
		return retval;

	return ehci_init(hcd);
}
#ifdef	CONFIG_PM
//mkl070226: the functionality of suspend/resume is not complete
static int ehci_suspend (struct usb_hcd *hcd)
{
	printk("%s: not implemented, just pass it\n",__FUNCTION__);
	return 0;
}
static int ehci_resume (struct usb_hcd *hcd)
{
	printk("%s: not implemented, just pass it\n",__FUNCTION__);
	return 0;
}
#endif

static const struct hc_driver str8131_ehci_driver = {
	.description =		hcd_name,
        .product_desc =		"str8131-ehci",
        .hcd_priv_size =	sizeof(struct ehci_hcd),
	/*
	 * generic hardware linkage
	 */
	.irq =			ehci_irq,
	.flags =		HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 */
	.reset =		str8131_ehci_setup, 
	.start =		ehci_run,
#ifdef	CONFIG_PM
//mkl070226: the functionality of suspend/resume is not complete
	.suspend =		ehci_suspend,
	.resume =		ehci_resume,

	.bus_suspend =		ehci_suspend,
	.bus_resume =		ehci_resume,
#endif
	.stop =			ehci_stop,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ehci_urb_enqueue,
	.urb_dequeue =		ehci_urb_dequeue,
	.endpoint_disable =	ehci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ehci_hub_status_data,
	.hub_control =		ehci_hub_control,
};

static void __init str8131_usb20_config_reg_init(void)
{
#if 0
	__asm__ __volatile__(
		"mov	r1, #0			\n"
		"mcr	p15, 0, r1, c7, c5, 0	\n"
		"mov	r1, #0			\n"
		"mcr	p15, 0, r1, c7, c14, 0	\n"
	);
#endif
	__raw_writel(0x106, SYSVA_USB20_CONFIG_BASE_ADDR + 0x04);
	__raw_writel((3 << 5) | 0x20000, SYSVA_USB20_OPERATION_BASE_ADDR + 0x40);
	mdelay(100);
}

int str8131_ehci_usb_hcd_probe(const struct hc_driver *driver, struct platform_device *pdev)
{
	struct usb_hcd *hcd;
	char *name = "str8131-ehci";
	int retval = 0;

	str8131_usb20_config_reg_init();
	hcd = usb_create_hcd(driver, &pdev->dev, name);
	if (!hcd) { 
		retval = -ENOMEM;
		return retval;
	}
	hcd->regs = (unsigned int *)SYSVA_USB20_OPERATION_BASE_ADDR;
	hcd->rsrc_start = SYSPA_USB20_OPERATION_BASE_ADDR;
	hcd->rsrc_len = 4096;
	hcd->driver = driver;
	retval = usb_add_hcd(hcd, INTC_USB20_BIT_INDEX, SA_INTERRUPT);
	if (retval == 0) {
		return retval;
	}
	printk("str8131 ehci init fail, %d\n", retval);
	usb_put_hcd(hcd);
	return retval;
}

int str8131_ehci_usb_hcd_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	usb_remove_hcd(hcd);
	usb_put_hcd(hcd);
	return 0;
}

static int str8131_ehci_hcd_drv_probe(struct platform_device *pdev)
{
	return str8131_ehci_usb_hcd_probe(&str8131_ehci_driver, pdev);
}

static struct platform_driver str8131_ehci_hcd_driver = {
	.probe		= str8131_ehci_hcd_drv_probe,
	.remove		= str8131_ehci_usb_hcd_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "str8131-ehci",
	},
};

static int __init str8131_ehci_hcd_init(void)
{
	if (usb_disabled())
		return -ENODEV;

	pr_debug("%s: block sizes: qh %Zd qtd %Zd itd %Zd sitd %Zd\n",
		hcd_name,
		sizeof(struct ehci_qh), sizeof(struct ehci_qtd),
		sizeof(struct ehci_itd), sizeof(struct ehci_sitd));

	return platform_driver_register(&str8131_ehci_hcd_driver);
}
module_init(str8131_ehci_hcd_init);

static void __exit str8131_ehci_hcd_cleanup(void)
{
	platform_driver_unregister(&str8131_ehci_hcd_driver);
}
module_exit(str8131_ehci_hcd_cleanup);

