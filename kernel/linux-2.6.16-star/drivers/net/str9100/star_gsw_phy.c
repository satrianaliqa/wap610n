/*******************************************************************************
 *
 *
 *   Copyright(c) 2003 - 2006 Star Semiconductor Corporation, All rights reserved.
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *   more details.
 *
 *   You should have received a copy of the GNU General Public License along with
 *   this program; if not, write to the Free Software Foundation, Inc., 59
 *   Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *   The full GNU General Public License is included in this distribution in the
 *   file called LICENSE.
 *
 *   Contact Information:
 *   Technology Support <tech@starsemi.com>
 *   Star Semiconductor 4F, No.1, Chin-Shan 8th St, Hsin-Chu,300 Taiwan, R.O.C
 *
 ********************************************************************************/
#include <linux/fs.h>
#include "star_gsw_phy.h"
#include "star_gsw.h"
#include "star_gsw_config.h"

//Tim Wang, support Link LED for WES610N
#include <asm/arch/star_gpio.h>
#include <linux/str9100/str9100_gpio.h>
#include <linux/timer.h>
// Macros that convert a number from msecs/secs to jiffies, for setting the kernel timers:
#define MSECS_TO_JIFFIES(X) ((X) * HZ / 1000)
#define SECS_TO_JIFFIES(X) ((X) * HZ )
// Shorthand form of the macro:
#define _MS(X) ((X) * HZ / 1000)
static struct timer_list link_led_timer;
#define GPIO_ON  PIN_TRIG_HIGH
#define GPIO_OFF PIN_TRIG_LOW

#define int32 int
#define uint32 u32



#if 0
void init_switch()
{
        u32 sw_config;

	PDEBUG("init_switch\n");
        /*
         * Configure GSW configuration
         */
        sw_config = GSW_SWITCH_CONFIG;

#if 0
        // orignal virgon configuration
        // enable fast aging
        sw_config |= (0xF);

        // CRC stripping
        sw_config |= (0x1 << 21);

        // IVL learning
        sw_config |= (0x1 << 22);

        // HNAT disable
        sw_config &= ~(0x1 << 23);

        GSW_SWITCH_CONFIG = sw_config;

        sw_config = GSW_SWITCH_CONFIG;
#endif

        /* configure switch */
        sw_config = GSW_SWITCH_CONFIG;

        sw_config &= ~0xF;      /* disable aging */
        sw_config |= 0x1;       /* disable aging */

#ifdef JUMBO_ENABLE

        // CRC stripping and GSW_CFG_MAX_LEN_JMBO
        sw_config |= (GSW_CFG_CRC_STRP | GSW_CFG_MAX_LEN_JMBO);
#else
        // CRC stripping and 1536 bytes
        sw_config |= (GSW_CFG_CRC_STRP | GSW_CFG_MAX_LEN_1536);
#endif

        /* IVL */
        sw_config |= GSW_CFG_IVL;

        /* disable HNAT */
        sw_config &= ~GSW_CFG_HNAT_EN;

        GSW_SWITCH_CONFIG = sw_config;
}
#endif






// add by descent 2006/07/05
// for configure packet forward and rate control
void init_packet_forward(int port)
{
	u32 mac_port_config;

	PDEBUG("port%d configure\n", port);
	if (port==0)
	{
		mac_port_config = GSW_MAC_PORT_0_CONFIG;
	}
	else if (port==1)
	{
		mac_port_config = GSW_MAC_PORT_1_CONFIG;
	}
	else
	{
		printk("MAC port number %d is not support!", port);
		return;
	}
		
	if (STR9100_GSW_BROADCAST_RATE_CONTROL)
		mac_port_config |=  (0x1 << 31); // STR9100_GSW_BROADCAST_RATE_CONTROLL on
	else
		mac_port_config &=  (~(0x1 << 31)); // STR9100_GSW_BROADCAST_RATE_CONTROLL off

	if (STR9100_GSW_MULTICAST_RATE_CONTROL)
		mac_port_config |=  (0x1 << 30); // STR9100_GSW_MULTICAST_RATE_CONTROLL on
	else
		mac_port_config &=  (~(0x1 << 30)); // STR9100_GSW_MULTICAST_RATE_CONTROLL off

	if (STR9100_GSW_UNKNOW_PACKET_RATE_CONTROL)
		mac_port_config |=  (0x1 << 29); // STR9100_GSW_UNKNOW_PACKET_RATE_CONTROLL on
	else
		mac_port_config &=  (~(0x1 << 29)); // STR9100_GSW_UNKNOW_PACKET_RATE_CONTROLL off


	if (STR9100_GSW_DISABLE_FORWARDING_BROADCAST_PACKET)
		mac_port_config |=  (0x1 << 27); // STR9100_GSW_DISABLE_FORWARDING_BROADCAST_PACKET on
	else
		mac_port_config &=  (~(0x1 << 27)); // STR9100_GSW_DISABLE_FORWARDING_BROADCAST_PACKET off

	if(STR9100_GSW_DISABLE_FORWARDING_MULTICAST_PACKET)
	{
		mac_port_config |=  (0x1 << 26); // STR9100_GSW_DISABLE_FORWARDING_MULTICAST_PACKET on
		PDEBUG("STR9100_GSW_DISABLE_FORWARDING_MULTICAST_PACKET on\n");
	}
	else
	{
		mac_port_config &=  (~(0x1 << 26)); // STR9100_GSW_DISABLE_FORWARDING_MULTICAST_PACKET off
		PDEBUG("STR9100_GSW_DISABLE_FORWARDING_MULTICAST_PACKET off\n");
	}

	if(STR9100_GSW_DISABLE_FORWARDING_UNKNOW_PACKET)
		mac_port_config |=  (0x1 << 25); // STR9100_GSW_DISABLE_FORWARDING_UNKNOW_PACKET on
	else
		mac_port_config &=  (~(0x1 << 25)); // STR9100_GSW_DISABLE_FORWARDING_UNKNOW_PACKET off

	//GSW_MAC_PORT_0_CONFIG = mac_port_config;
	if (port==0)
		GSW_MAC_PORT_0_CONFIG = mac_port_config;
	if (port==1)
		GSW_MAC_PORT_1_CONFIG = mac_port_config;
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

#ifdef DORADO2_PCI_FASTPATH_MAC_PORT1_LOOPBACK
static int star_gsw_config_mac_port1_loopback(void)
{
	u32 mac_port_base;
	u32 mac_port_config;
	int i;

	PRINT_INFO("\nconfigure mac port1 loopback\n");
	
	mac_port_base = GSW_PORT1_CFG_REG;

	mac_port_config = __REG(mac_port_base);

	// disable PHY's AN
	mac_port_config &= ~(0x1 << 7);

	// enable RGMII-PHY mode
	mac_port_config |= (0x1 << 15);

	// reversed RGMII mode
	mac_port_config |= (0x1 << 14);

	// enable GSW MAC port 0
	mac_port_config &= ~(0x1 << 18);

	__REG(mac_port_base) = mac_port_config;

	// SA learning disable
	mac_port_config |= (0x1 << 19);

	// disable TX flow control
	mac_port_config &= ~(0x1 << 12);

	// disable RX flow control
	mac_port_config &= ~(0x1 << 11);

	// force duplex
	mac_port_config |= (0x1 << 10);

	// force speed at 1000Mbps
	mac_port_config &= ~(0x3 << 8);
	mac_port_config |= (0x2 << 8);

	__REG(mac_port_base) = mac_port_config;

	// adjust MAC port 1 RX/TX clock skew
	GSW_BIST_RESULT_TEST_0 &= ~((0x3 << 28) | (0x3 << 30));
	//GSW_BIST_RESULT_TEST_0 |= ((0x2 << 28) | (0x2 << 30));
	GSW_BIST_RESULT_TEST_0 |= (0x2 << 30);

	return 0;
}
#endif

int VSC8201_phy_power_down(int phy_addr, int y)
{
	u16 phy_data = 0;
	// power-down or up the PHY
	star_gsw_read_phy(phy_addr, 0, &phy_data);
	if (y==1) // down
		phy_data |= (0x1 << 11);
	if (y==0) // up
		phy_data |= (~(0x1 << 11));
	star_gsw_write_phy(phy_addr, 0, phy_data);
	return 0;

}

static int star_gsw_config_VSC8201(u8 mac_port, u8 phy_addr)	// include cicada 8201
{
	u32 mac_port_base = 0;
	u32 mac_port_config;
	u16 phy_reg;
	int i;

	PRINT_INFO("\nconfigure VSC8201\n");
	PDEBUG("mac port : %d phy addr : %d\n", mac_port, phy_addr);
	/*
	 * Configure MAC port 0
	 * For Cicada CIS8201 single PHY
	 */
	if (mac_port == 0) {
		PDEBUG("port 0\n");
		mac_port_base = GSW_PORT0_CFG_REG;
	}
	if (mac_port == 1) {
		PDEBUG("port 1\n");
		mac_port_base = GSW_PORT1_CFG_REG;
	}

	star_gsw_set_phy_addr(mac_port, phy_addr);
	//star_gsw_set_phy_addr(1, 1);

	mac_port_config = __REG(mac_port_base);

	// enable PHY's AN
	mac_port_config |= (0x1 << 7);

	// enable RGMII-PHY mode
	mac_port_config |= (0x1 << 15);

	// enable GSW MAC port 0
	mac_port_config &= ~(0x1 << 18);

	__REG(mac_port_base)=  mac_port_config;

	/*
	 * Configure Cicada's CIS8201 single PHY
	 */
	/* 2007.04.24 Richard.Liu Marked, we don't need VSC8201 lookback mode */
#if 0
#ifdef CONFIG_STAR9100_SHNAT_PCI_FASTPATH
	/* near-end loopback mode */
	star_gsw_read_phy(phy_addr, 0x0, &phy_reg);
	phy_reg |= (0x1 << 14);
	star_gsw_write_phy(phy_addr, 0x0, phy_reg);
#endif
#endif

	star_gsw_read_phy(phy_addr, 0x1C, &phy_reg);

	// configure SMI registers have higher priority over MODE/FRC_DPLX, and ANEG_DIS pins
	phy_reg |= (0x1 << 2);

	star_gsw_write_phy(phy_addr, 0x1C, phy_reg);

	star_gsw_read_phy(phy_addr, 0x17, &phy_reg);

	// enable RGMII MAC interface mode
	phy_reg &= ~(0xF << 12);
	phy_reg |= (0x1 << 12);

	// enable RGMII I/O pins operating from 2.5V supply
	phy_reg &= ~(0x7 << 9);
	phy_reg |= (0x1 << 9);

	star_gsw_write_phy(phy_addr, 0x17, phy_reg);

	star_gsw_read_phy(phy_addr, 0x4, &phy_reg);

	// Enable symmetric Pause capable
	phy_reg |= (0x1 << 10);

	star_gsw_write_phy(phy_addr, 0x4, phy_reg);

	mac_port_config = __REG(mac_port_base);

	/* 2007.04.24 Richard.Liu Marked, we don't need VSC8201 lookback mode */
//#ifdef CONFIG_STAR9100_SHNAT_PCI_FASTPATH
#if 0
	// near-end loopback mode, must disable AN
	mac_port_config &= ~(0x1 << 7);

	// SA learning disable
	mac_port_config |= (0x1 << 19);

	// disable TX flow control
	mac_port_config &= ~(0x1 << 12);

	// disable RX flow control
	mac_port_config &= ~(0x1 << 11);

	// force duplex
	mac_port_config |= (0x1 << 10);

	// force speed at 1000Mpbs
	mac_port_config &= ~(0x3 << 8);
	mac_port_config |= (0x2 << 8);
#else
	// enable PHY's AN
	mac_port_config |= (0x1 << 7);
#endif

	__REG(mac_port_base) = mac_port_config;

	/*
	 * Enable PHY1 AN restart bit to restart PHY1 AN
	 */
	star_gsw_read_phy(phy_addr, 0x0, &phy_reg);

	phy_reg |= (0x1 << 9) | (0x1 << 12);

	star_gsw_write_phy(phy_addr, 0x0, phy_reg);

	/*
	 * Polling until PHY0 AN restart is complete
	 */
	for (i = 0; i < 0x1000; i++) {
		star_gsw_read_phy(phy_addr, 0x1, &phy_reg);

		if ((phy_reg & (0x1 << 5)) && (phy_reg & (0x1 << 2))) {
			printk("0x1 phy reg: %x\n", phy_reg);
			break;
		} else {
			udelay(100);
		}
	}

	mac_port_config = __REG(mac_port_base);

	if (((mac_port_config & 0x1) == 0) || (mac_port_config & 0x2)) {
		printk("Check MAC/PHY%s Link Status : DOWN!\n", (mac_port == 0 ? "0" : "1"));
	} else {
		printk("Check MAC/PHY%s Link Status : UP!\n", (mac_port == 0 ? "0" : "1"));
		/*
		 * There is a bug for CIS8201 PHY operating at 10H mode, and we use the following
		 * code segment to work-around
		 */
		star_gsw_read_phy(phy_addr, 0x05, &phy_reg);

		if ((phy_reg & (0x1 << 5)) && (!(phy_reg & (0x1 << 6))) && (!(phy_reg & (0x1 << 7))) && (!(phy_reg & (0x1 << 8)))) {	/* 10H,10F/100F/100H off */
			star_gsw_read_phy(phy_addr, 0x0a, &phy_reg);

			if ((!(phy_reg & (0x1 << 10))) && (!(phy_reg & (0x1 << 11)))) {	/* 1000F/1000H off */
				star_gsw_read_phy(phy_addr, 0x16, &phy_reg);

				phy_reg |= (0x1 << 13) | (0x1 << 15);	// disable "Link integrity check(B13)" & "Echo mode(B15)"

				star_gsw_write_phy(phy_addr, 0x16, phy_reg);
			}
		}
	}

	if (mac_port == 0) {
		// adjust MAC port 0 RX/TX clock skew
		GSW_BIST_RESULT_TEST_0 &= ~((0x3 << 24) | (0x3 << 26));
		GSW_BIST_RESULT_TEST_0 |= ((0x2 << 24) | (0x2 << 26));
	}

	if (mac_port == 1) {
		// adjust MAC port 1 RX/TX clock skew
		GSW_BIST_RESULT_TEST_0 &= ~((0x3 << 28) | (0x3 << 30));
		GSW_BIST_RESULT_TEST_0 |= ((0x2 << 28) | (0x2 << 30));
	}

	return 0;
}


static int star_gsw_config_VSC8601(u8 mac_port, u8 phy_addr)	
{
	u32 mac_port_base = 0;
        u32 mac_port_config;
	#if ((defined CONFIG_GPB239M) || (defined CONFIG_GPB239S) || (defined CONFIG_GPB261))
	#else
       u16 phy_data;
	#endif
	
	printk("INIT VSC8601\n");

        if (mac_port == 0) {
		PDEBUG("port 0\n");
		mac_port_base = GSW_PORT0_CFG_REG;
	}
	if (mac_port == 1) {
	        PDEBUG("port 1\n");
	        mac_port_base = GSW_PORT1_CFG_REG;
	}
        star_gsw_set_phy_addr(mac_port, phy_addr);
				
        mac_port_config = __REG(mac_port_base);

	#if ((defined CONFIG_GPB239M) || (defined CONFIG_GPB239S) || (defined CONFIG_GPB261))
	// disable PHY's AN
	mac_port_config &= ~(0x1 << 7);

	// force speed = 100Mbps
	//mac_port_config &= ~(0x3 << 8);
	//mac_port_config |= (0x1 << 8);
	
	// force speed = 1000Mbps
	mac_port_config &= ~(0x3 << 8);
	mac_port_config |= (0x2 << 8);
	
	// force full-duplex
	mac_port_config |= (0x1 << 10);
	#else
    mac_port_config |= (0x1 << 7);
	#endif

        // enable RGMII-PHY mode
        mac_port_config |= (0x1 << 15);

        // enable GSW MAC port 0
        mac_port_config &= ~(0x1 << 18);

        __REG(mac_port_base)=  mac_port_config;
	udelay(1000);

	#if ((defined CONFIG_GPB239M) || (defined CONFIG_GPB239S) || (defined CONFIG_GPB261))
	#else
        star_gsw_read_phy(phy_addr, 3, &phy_data);
        if ((phy_data & 0x000F) == 0x0000) {
	        u16 tmp16;
		printk("VSC8601 Type A Chip\n");
		star_gsw_write_phy(phy_addr, 31, 0x52B5);
		star_gsw_write_phy(phy_addr, 16, 0xAF8A);

		phy_data = 0x0;
		star_gsw_read_phy(phy_addr, 18, &tmp16);
		phy_data |= (tmp16 & ~0x0);
		star_gsw_write_phy(phy_addr, 18, phy_data);
                phy_data = 0x0008;
                star_gsw_read_phy(phy_addr, 17, &tmp16);
                phy_data |= (tmp16 & ~0x000C);
                star_gsw_write_phy(phy_addr, 17, phy_data);
                star_gsw_write_phy(phy_addr, 16, 0x8F8A);
                star_gsw_write_phy(phy_addr, 16, 0xAF86);
                phy_data = 0x0008;
                star_gsw_read_phy(phy_addr, 18, &tmp16);
		phy_data |= (tmp16 & ~0x000C);
		star_gsw_write_phy(phy_addr, 18, phy_data);
                phy_data = 0x0;
                star_gsw_read_phy(phy_addr, 17, &tmp16);
                phy_data |= (tmp16 & ~0x0);
                star_gsw_write_phy(phy_addr, 17, phy_data);

                star_gsw_write_phy(phy_addr, 16, 0x8F8A);

                star_gsw_write_phy(phy_addr, 16, 0xAF82);

                phy_data = 0x0;
                star_gsw_read_phy(phy_addr, 18, &tmp16);
                phy_data |= (tmp16 & ~0x0);
                star_gsw_write_phy(phy_addr, 18, phy_data);

                phy_data = 0x0100;
                star_gsw_read_phy(phy_addr, 17, &tmp16);
                phy_data |= (tmp16 & ~0x0180);
                star_gsw_write_phy(phy_addr, 17, phy_data);

                star_gsw_write_phy(phy_addr, 16, 0x8F82);

                star_gsw_write_phy(phy_addr, 31, 0x0);

                //Set port type: single port
		star_gsw_read_phy(phy_addr, 9, &phy_data);
		phy_data &= ~( 0x1 << 10);
		star_gsw_write_phy(phy_addr, 9, phy_data);
	} else if ((phy_data & 0x000F) == 0x0001) {
		printk("VSC8601 Type B Chip\n");
		star_gsw_read_phy(phy_addr, 23, &phy_data);
		phy_data |= ( 0x1 << 8); //set RGMII timing skew
		star_gsw_write_phy(phy_addr, 23, phy_data);
	}
        star_gsw_write_phy(phy_addr, 31, 0x0001); // change to extended registers
        star_gsw_read_phy(phy_addr, 28, &phy_data);
        phy_data &= ~(0x3 << 14); // RGMII TX timing skew
      //  phy_data |=  (0x3 << 14); // 2.0ns
        phy_data &= ~(0x3 << 12); // RGMII RX timing skew
      //  phy_data |=  (0x3 << 12); // 2.0ns
        star_gsw_write_phy(phy_addr, 28, phy_data);
        star_gsw_write_phy(phy_addr, 31, 0x0000); // change to normal registers

        mac_port_config = __REG(mac_port_base);
        mac_port_config |= (0x1 << 7); // enable phy's AN
        __REG(mac_port_base) = mac_port_config;

        star_gsw_read_phy(phy_addr, 4, &phy_data);
        phy_data |= (0x1 << 10); // enable flow control (Symmetric PAUSE frame)
        star_gsw_write_phy(phy_addr, 4, phy_data);

        star_gsw_read_phy(phy_addr, 0, &phy_data);
        phy_data |= (0x1 << 9) | (0x1 << 12); // restart phy's AN
        star_gsw_write_phy(phy_addr, 0, phy_data);
	#endif
		
//	
        mac_port_config = __REG(mac_port_base);
        //mac_port_config &= ~(0x1 << 18); // enable mac port 1
        mac_port_config |= (0x1 << 18); // disable mac port 1
        //mac_port_config |= (0x1 << 19); // disable SA learning
	mac_port_config &= ~(0x1 << 19); // enable SA learning
	mac_port_config &= ~(0x1 << 24); // disable ingress check
	// forward unknown, multicast and broadcast packets to CPU
	mac_port_config &= ~((0x1 << 25) | (0x1 << 26) | (0x1 << 27));
	// storm rate control for unknown, multicast and broadcast packets
	//mac_port_config |= ((0x1 << 29) | (0x1 << 30) | ((u32)0x1 << 31));
	__REG(mac_port_base) = mac_port_config;
	
	if (mac_port == 0) {
		// adjust MAC port 0 RX/TX clock skew
		GSW_BIST_RESULT_TEST_0 &= ~((0x3 << 24) | (0x3 << 26));
#ifdef CONFIG_GPB239S
		GSW_BIST_RESULT_TEST_0 |= (0x2 << 26);
#else
		GSW_BIST_RESULT_TEST_0 |= ((0x3 << 24) | (0x3 << 26));
#endif
	}

	if (mac_port == 1) {
		// adjust MAC port 1 RX/TX clock skew
		GSW_BIST_RESULT_TEST_0 &= ~((0x3 << 28) | (0x3 << 30));
#if ((defined CONFIG_GPB239M) || (defined CONFIG_GPB261))
		GSW_BIST_RESULT_TEST_0 |= (0x2 << 30);
#else
		GSW_BIST_RESULT_TEST_0 |= ((0x3 << 28) | (0x3 << 30));
#endif
	}

	return 0;                                                                                
}

#if (defined CONFIG_DORADO2) || (defined CONFIG_GPB239M) || (defined CONFIG_GPB239S) || (defined CONFIG_GPB261)
static void star_gsw_config_VSC8X01()
{
       	u16  phy_id = 0;

#if (defined CONFIG_DORADO2) || (defined CONFIG_GPB239M) || (defined CONFIG_GPB261)
	star_gsw_set_phy_addr(1,1);
	star_gsw_read_phy(1, 0x02, &phy_id);
//	printk("phy id = %X\n", phy_id);
	if (phy_id == 0x000F) //VSC8201
	       	star_gsw_config_VSC8201(1,1);
	else
		star_gsw_config_VSC8601(1,1);
#elif defined CONFIG_GPB239S
		star_gsw_set_phy_addr(0,0);
		star_gsw_read_phy(0, 0x02, &phy_id);
	//	printk("phy id = %X\n", phy_id);
		if (phy_id == 0x000F) //VSC8201
				star_gsw_config_VSC8201(0,0);
		else
			star_gsw_config_VSC8601(0,0);
#endif
}
#endif


// add by descent 2006/07/10
// port : 0 => port0 ; port : 1 => port1
// y = 1 ; disable AN
int disable_AN(int port, int y)
{
	u32 mac_port_config;
	if (port==0)
	{
		mac_port_config = GSW_MAC_PORT_0_CONFIG;
	}
	else if (port==1)
	{
		mac_port_config = GSW_MAC_PORT_1_CONFIG;
	}
	else
	{
		printk("MAC port number %d is not support!", port);
		return (1);
	}

	// disable PHY's AN
	if (y==1)
	{
	  PDEBUG("disable AN\n");
	  mac_port_config &= ~(0x1 << 7);
	}

	// enable PHY's AN
	if (y==0)
	{
	  PDEBUG("enable AN\n");
	  mac_port_config |= (0x1 << 7);
	}

	if (port==0)
		GSW_MAC_PORT_0_CONFIG = mac_port_config;
	if (port==1)
		GSW_MAC_PORT_1_CONFIG = mac_port_config;
	return 0;
}

int disable_AN_VSC7385(int y)
{
	u32 mac_port_config;
	mac_port_config = GSW_MAC_PORT_0_CONFIG;

	// disable PHY's AN
	if (y==1)
	{
	  PDEBUG("disable AN\n");
	  mac_port_config &= ~(0x1 << 7);
	}

	// enable PHY's AN
	if (y==0)
	{
	  PDEBUG("enable AN\n");
	  mac_port_config |= (0x1 << 7);
	}

	GSW_MAC_PORT_0_CONFIG = mac_port_config;
	return 0;
}

void star_gsw_config_VSC7385(void)
{
	u32 mac_port_config;


	printk("\nconfigure VSC7385\n");
	/*
	 * Configure GSW's MAC port 0
	 * For ASIX's 5-port GbE Switch setting
	 * 1. No SMI (MDC/MDIO) connection between Orion's MAC port 0 and ASIX's MAC port 4
	 * 2. Force Orion's MAC port 0 to be 1000Mbps, and full-duplex, and flow control on
	 */
	mac_port_config = GSW_MAC_PORT_0_CONFIG;


	// enable RGMII-PHY mode
	mac_port_config |= (0x1 << 15);

	// force speed = 1000Mbps
	mac_port_config &= ~(0x3 << 8);
	mac_port_config |= (0x2 << 8);

	// force full-duplex
	mac_port_config |= (0x1 << 10);

	// force Tx/Rx flow-control on
	mac_port_config |= (0x1 << 11) | (0x1 << 12);

	GSW_MAC_PORT_0_CONFIG = mac_port_config;

	udelay(1000);

	mac_port_config = GSW_MAC_PORT_0_CONFIG;

	if (((mac_port_config & 0x1) == 0) || (mac_port_config & 0x2)) {
		printk("Check MAC/PHY 0 Link Status : DOWN!\n");
	} else {
		printk("Check MAC/PHY 0 Link Status : UP!\n");
	}

	/* adjust MAC port 0 /RX/TX clock skew */
	GSW_BIST_RESULT_TEST_0 &= ~((0x3 << 24) | (0x3 << 26));
	GSW_BIST_RESULT_TEST_0 |= ((0x2 << 24) | (0x2 << 26));
}



void star_gsw_config_ASIX()
{
	u32 mac_port_config;

	printk("configure port0 ASIX\n");
	mac_port_config = GSW_MAC_PORT_0_CONFIG;

	//Disable AN
	mac_port_config &= (~(0x1 << 7));

	//force speed to 1000Mbps
	mac_port_config &= (~(0x3 << 8));
	mac_port_config |= (0x2 << 8);	//jacky

	//force tx and rx follow control
	mac_port_config |= (0x1 << 11) | (0x1 << 12);

	//force full deplex
	mac_port_config |= 0x1 << 10;

	//RGMII ENABLR
	mac_port_config |= 0x1 << 15;

	GSW_MAC_PORT_0_CONFIG = mac_port_config;

	udelay(1000);

	/* adjust MAC port 0 RX/TX clock skew */
	GSW_BIST_RESULT_TEST_0 &= ~((0x3 << 24) | (0x3 << 26));
	GSW_BIST_RESULT_TEST_0 |= ((0x2 << 24) | (0x2 << 26));
	
	// configure MAC port 0 pad drive strength = 10/100 mode
	
	//*(u32 volatile *) (0xf770001C) |= (0x1 << 2);

	PWRMGT_PAD_DRIVE_STRENGTH_CONTROL |= (0x1 << 2);
}



// agere power down/up
int AGERE_phy_power_down(int phy_addr, int y)
{
	u16 phy_data = 0;
	// power-down or up the PHY
	star_gsw_read_phy(phy_addr, 0, &phy_data);
	if (y==1) // down
		phy_data |= (0x1 << 11);
	if (y==0) // up
		phy_data &= (~(0x1 << 11));
	star_gsw_write_phy(phy_addr, 0, phy_data);
	return 0;
}

// add by descent 2006/07/31
// standard phy register 0 and offset is 11 is power down
int std_phy_power_down(int phy_addr, int y)
{
	u16 phy_data = 0, phy_id;
	// power-down or up the PHY
	star_gsw_read_phy(phy_addr, 0, &phy_data);
	if (y==1) // down
		phy_data |= (0x1 << 11);
//june.chen, 2011-02-15, reset phy when using 8306G (ie. DUT is WES610N)
#if 0
	if (y==0) // up
		phy_data &= (~(0x1 << 11));
#else
	if(y == 0){
		star_gsw_read_phy(1, 3, &phy_id);

		phy_data &= (~(0x1 << 11));
		//Only Reset phy when using 8306G	
		if (phy_id == 0xC852) {
			phy_data |= (0x1 << 15);
		}
	}
#endif
	star_gsw_write_phy(phy_addr, 0, phy_data);
	return 0;
}

//june.chen, 2011-02-14, add this function for doing phy reset each time device is opened
#if 0
int std_phy_reset(int phy_addr)
{
        u16 phy_data = 0;
        // reset phy
        star_gsw_read_phy(phy_addr, 0, &phy_data);
        phy_data |= (0x1 << 15);
        star_gsw_write_phy(phy_addr, 0, phy_data);
        return 0;
}
#endif

//Tim Wang, power-down or up all the PHY,
//For RTL8201CP, actually we only need to power-down PHY 1.(WAP610N/WET610N)
//For RTL8306SG, we set PHY 0 to PHY 6, refer to the datasheet for PHY 0~6 mapping.(WES610N)
#define MAX_PHY_NUMBER 6
int all_phy_power_down(int y)
{
	int i;

//june.chen, 2011-02-14, modify to try to fix ethernet link donw issue after reboot many times
//	for (i=0;i<=6;i++) {
	for (i=0;i<=6;i++) {
		std_phy_power_down(i, y);
		udelay(1000);
#if 0
		 if(y == 0){
                        std_phy_reset(i);
                        udelay(100);
                        printk("reset phy port %d\n", i);
                }
#endif
	}
	return 0;
}

/*
 *  AGERE PHY is attached to MAC PORT 1
 * with phy_addr 1
 */
void star_gsw_config_AGERE()
{
	u32 mac_port_config;
	u16 phy_data = 0;
	int i;

	printk("configure port1 AGERE\n");

	/*
	 * Configure MAC port 1
	 * For Agere Systems's ET1011 single PHY
	 */
	mac_port_config = GSW_MAC_PORT_1_CONFIG;

	// disable PHY's AN
	mac_port_config &= ~(0x1 << 7);

	// enable RGMII-PHY mode
	mac_port_config |= (0x1 << 15);

	GSW_MAC_PORT_1_CONFIG = mac_port_config;

#if 1
	/*
	 * configure Agere's ET1011 Single PHY
	 */
	/* Configure Agere's ET1011 by Agere's programming note */
	//1. power-down the PHY
	star_gsw_read_phy(1, 0, &phy_data);
	phy_data |= (0x1 << 11);
	star_gsw_write_phy(1, 0, phy_data);

	//2. Enable PHY programming mode
	star_gsw_read_phy(1, 18, &phy_data);
	phy_data |= (0x1 << 0);
	phy_data |= (0x1 << 2);
	star_gsw_write_phy(1, 18, phy_data);

	//3.Perform some PHY register with the Agere-specfic value
	star_gsw_write_phy(1, 16, 0x880e);
	star_gsw_write_phy(1, 17, 0xb4d3);

	star_gsw_write_phy(1, 16, 0x880f);
	star_gsw_write_phy(1, 17, 0xb4d3);

	star_gsw_write_phy(1, 16, 0x8810);
	star_gsw_write_phy(1, 17, 0xb4d3);

	star_gsw_write_phy(1, 16, 0x8817);
	star_gsw_write_phy(1, 17, 0x1c00);

	star_gsw_write_phy(1, 16, 0x8805);
	star_gsw_write_phy(1, 17, 0xb03e);

	star_gsw_write_phy(1, 16, 0x8806);
	star_gsw_write_phy(1, 17, 0xb03e);

	star_gsw_write_phy(1, 16, 0x8807);
	star_gsw_write_phy(1, 17, 0xff00);

	star_gsw_write_phy(1, 16, 0x8808);
	star_gsw_write_phy(1, 17, 0xe110);

	star_gsw_write_phy(1, 16, 0x300d);
	star_gsw_write_phy(1, 17, 0x0001);

	//4. Disable PHY programming mode
	star_gsw_read_phy(1, 18, &phy_data);
	phy_data &= ~(0x1 << 0);
	phy_data &= ~(0x1 << 2);
	star_gsw_write_phy(1, 18, phy_data);

	//5. power-up the PHY
	star_gsw_read_phy(1, 0, &phy_data);
	phy_data &= ~(0x1 << 11);
	star_gsw_write_phy(1, 0, phy_data);

	star_gsw_read_phy(1, 22, &phy_data);

	// enable RGMII MAC interface mode : RGMII/RMII (dll delay or trace delay) mode
	phy_data &= ~(0x7 << 0);

	// phy_data |= (0x6 << 0); // RGMII/RMII dll delay mode : not work!!
	phy_data |= (0x4 << 0);	// RGMII/RMII trace delay mode

	star_gsw_write_phy(1, 22, phy_data);
#endif

	mac_port_config = GSW_MAC_PORT_1_CONFIG;

	// enable PHY's AN
	mac_port_config |= (0x1 << 7);

	GSW_MAC_PORT_1_CONFIG = mac_port_config;

	/*
	 * Enable flow-control on (Symmetric PAUSE frame)
	 */
	star_gsw_read_phy(1, 0x4, &phy_data);

	phy_data |= (0x1 << 10);

	star_gsw_write_phy(1, 0x4, phy_data);

	/*
	 * Enable PHY1 AN restart bit to restart PHY1 AN
	 */
	star_gsw_read_phy(1, 0x0, &phy_data);

	phy_data |= (0x1 << 9) | (0x1 << 12);

	star_gsw_write_phy(1, 0x0, phy_data);

	/*
	 * Polling until PHY1 AN restart is complete and PHY1 link status is UP
	 */
	for (i = 0; i < 0x2000; i++) {
		star_gsw_read_phy(1, 0x1, &phy_data);
		if ((phy_data & (0x1 << 5)) && (phy_data & (0x1 << 2))) {
			break;
		}
	}

	// adjust MAC port 1 RX/TX clock skew
	GSW_BIST_RESULT_TEST_0 &= ~((0x3 << 28) | (0x3 << 30));
	GSW_BIST_RESULT_TEST_0 |= ((0x2 << 28) | (0x3 << 30));

	udelay(100);

	mac_port_config = GSW_MAC_PORT_1_CONFIG;
	if (!(mac_port_config & 0x1) || (mac_port_config & 0x2)) {
		/*
		 * Port 1 PHY link down or no TXC in Port 1
		 */
		PDEBUG("PHY1: Link Down, 0x%08x!\n", mac_port_config);
		return;
	}

}



int str9100_gsw_config_mac_port0()
{
        PDEBUG("str9100_gsw_config_mac_port0\n");
        INIT_PORT0_PHY
	INIT_PORT0_MAC
        PORT0_LINK_DOWN
        return 0;
}

int str9100_gsw_config_mac_port1()
{
        INIT_PORT1_PHY
	INIT_PORT1_MAC
        PORT1_LINK_DOWN
        //PORT1_LINK_UP
        return 0;
}


#define PHY_CONTROL_REG_ADDR 0x00
#define PHY_AN_ADVERTISEMENT_REG_ADDR 0x04


int icp_101a_init (int port)
{
	u32 mac_port_config;
        u16 phy_data = 0;


	PRINT_INFO("init IC+101A\n");

	if (port==0)
	{
		mac_port_config = GSW_MAC_PORT_0_CONFIG;
	}
	else if (port==1)
	{
		mac_port_config = GSW_MAC_PORT_1_CONFIG;
	}
	else
	{
		printk("MAC port number %d is not support!", port);
		return (1);
	}
	
	if (!(mac_port_config & (0x1 << 5))) {
		if (!star_gsw_read_phy (port, PHY_AN_ADVERTISEMENT_REG_ADDR, &phy_data))
	    	{
			PDEBUG("\n PORT%d, enable local flow control capability Fail\n", port);
			return (1);
	    	}
		else
	    	{
	      		// enable PAUSE frame capability
			phy_data |= (0x1 << 10);

	      		if (!star_gsw_write_phy (port, PHY_AN_ADVERTISEMENT_REG_ADDR, phy_data))
			{
				PDEBUG("\nPORT%d, enable PAUSE frame capability Fail\n", port);
				return (1);
			}
	    	}
	}


	// restart PHY0 AN
	if (!star_gsw_read_phy (port, PHY_CONTROL_REG_ADDR, &phy_data)) {
		PDEBUG ("\n restart PHY%d AN Fail \n", port);
		return (1);
	}
	else {
		// enable PHY0 AN restart
		phy_data |= (0x1 << 9);

		if (!star_gsw_write_phy (port, PHY_CONTROL_REG_ADDR, phy_data)) {
			PDEBUG ("\n  enable PHY0 AN restart \n");
			return (1);
		}
	}



	while (1)
	{
		PDEBUG ("\n Polling  PHY%d AN \n", port);
		star_gsw_read_phy (port, PHY_CONTROL_REG_ADDR, &phy_data);

		if (phy_data & (0x1 << 9)) {
		  continue;
		}
		else {
			PDEBUG ("\n PHY%d AN restart is complete \n", port);
			break;
		}
	}

	return 0;
}

//Tim Wang, access RTL8306G register
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef SUCCESS
#define SUCCESS 	0
#endif

#ifndef FAILED
#define FAILED -1
#endif

#define RTL8306_PHY_NUMBER	7
#define RTL8306_REGSPERPAGE  32
#define RTL8306_REGSPERPHY	68
#define RTL8306_REG_NUMBER  ((RTL8306_REGSPERPHY)*(RTL8306_PHY_NUMBER))
#define RTL8306_PAGE_NUMBER 4
#define RTL8306_REGPAGE0		0x0
#define RTL8306_REGPAGE1		0x1
#define RTL8306_REGPAGE2		0x2
#define RTL8306_REGPAGE3		0x3
#define RTL8306_PORT_NUMBER 6
#define RTL8306_PORT0 		0x0
#define RTL8306_PORT1 		0x1
#define RTL8306_PORT2 		0x2
#define RTL8306_PORT3 		0x3
#define RTL8306_PORT4 		0x4
#define RTL8306_PORT5 		0x5
#define RTL8306_NOCPUPORT 7


/*
@func int | rtl8306_getAsicPhyReg | Read Asic  PHY Register.
@parm u32 | phyad | Specify Phy address (0 ~6).
@parm u32 | regad | Specify register address (0 ~31).
@parm u32 | npage | Specify page number (0 ~3).
@parm u32 * | pvalue | The pointer of value read back from PHY register.
@rvalue SUCCESS 
@rvalue FAILED
@comm
Use this function you could read all configurable registers of RTL8306,
*/

int rtl8306_getAsicPhyReg(u32 phyad, u32 regad, u32 npage, u32 *pvalue) {
	u32 rdata;

	if ((phyad >= RTL8306_PHY_NUMBER) || (regad >= RTL8306_REGSPERPAGE) ||
		(npage >= RTL8306_PAGE_NUMBER))	
		return FAILED;

	/* Select PHY Register Page through configuring PHY 0 Register 16 [bit1 bit15] */
	star_gsw_read_phy(0, 16, &rdata); 
	switch (npage) {
	case RTL8306_REGPAGE0:
		star_gsw_write_phy(0, 16, (rdata & 0x7FFF) | 0x0002);
		break;
	case RTL8306_REGPAGE1:
		star_gsw_write_phy(0, 16, rdata | 0x8002 );
		break;
	case RTL8306_REGPAGE2:
		star_gsw_write_phy(0, 16, rdata & 0x7FFD);
		break;
	case RTL8306_REGPAGE3:
		star_gsw_write_phy(0, 16, (rdata & 0xFFFD) | 0x8000);
		break;
	default:
		return FAILED;
	}

	star_gsw_read_phy(phyad, regad, pvalue);
	*pvalue = *pvalue & 0xFFFF;
	return SUCCESS;
}	

/*
@func int | rtl8306_setAsicPhyReg | Write Asic  PHY Register.
@parm u32 | phyad | Specify Phy address (0 ~6).
@parm u32 | regad | Specify register address (0 ~31).
@parm u32 | npage | Specify page number (0 ~3).
@parm u32 | value | Value to be write into the register.
@rvalue SUCCESS 
@rvalue FAILED
@comm
Use this function you could write all configurable registers of RTL8306, 
*/
int rtl8306_setAsicPhyReg(u32 phyad, u32 regad, u32 npage, u32 value) {
	u32 rdata; 

	if ((phyad >= RTL8306_PHY_NUMBER) || (regad >= RTL8306_REGSPERPAGE) ||
		(npage >= RTL8306_PAGE_NUMBER))	
		return FAILED;
	/* Select PHY Register Page through configuring PHY 0 Register 16 [bit1 bit15] */
	value = value & 0xFFFF;
	star_gsw_read_phy(0, 16, &rdata); 
	switch (npage) {
	case RTL8306_REGPAGE0:
		star_gsw_write_phy(0, 16, (rdata & 0x7FFF) | 0x0002);
		break;
	case RTL8306_REGPAGE1:
		star_gsw_write_phy(0, 16, rdata | 0x8002 );
		break;
	case RTL8306_REGPAGE2:
		star_gsw_write_phy(0, 16, rdata & 0x7FFD);
		break;
	case RTL8306_REGPAGE3:
		star_gsw_write_phy(0, 16, (rdata & 0xFFFD) | 0x8000);
		break;
	default:
		return FAILED;
	}
	
	star_gsw_write_phy(phyad, regad, value);
	return SUCCESS;
}

/*
@func int | rtl8306_setAsicPhyRegBit | Write one bit of Asic  PHY Register.
@parm u32 | phyad | Specify Phy address (0 ~6).
@parm u32 | regad | Specify register address (0 ~31).
@parm u32 | bit | Specify bit position(0 ~ 15).
@parm u32 | npage | Specify page number (0 ~3).
@parm u32 | value | Value to be write(0, 1).
@rvalue SUCCESS 
@rvalue FAILED
@comm
Use this function  you could write each bit of  all configurable registers of RTL8306.
*/
int rtl8306_setAsicPhyRegBit(u32 phyad, u32 regad, u32 bit, u32 npage,  u32 value) {
	u32 rdata;
	if ((phyad >= RTL8306_PHY_NUMBER) || (regad >= RTL8306_REGSPERPAGE) || 
		(npage >= RTL8306_PAGE_NUMBER) || (bit > 15) || (value >1))
		return FAILED;
	rtl8306_getAsicPhyReg(phyad, regad,  npage, &rdata);
	if (value) 
		rtl8306_setAsicPhyReg(phyad, regad, npage, rdata | (1 << bit));
	else
		rtl8306_setAsicPhyReg(phyad, regad, npage, rdata & (~(1 << bit)));
	return SUCCESS;
}

/*
@func int | rtl8306_getAsicPhyRegBit | Read one bit of Asic  PHY Register.
@parm u32 | phyad | Specify Phy address (0 ~6).
@parm u32 | regad | Specify register address (0 ~31).
@parm u32 | bit | Specify bit position(0 ~ 15).
@parm u32 | npage | Specify page number (0 ~3).
@parm u32 * | pvalue | The pointer of value read back.
@rvalue SUCCESS 
@rvalue FAILED
@comm
Use this function you could read each bit of  all configurable registers of RTL8306.
*/
int rtl8306_getAsicPhyRegBit(u32 phyad, u32 regad, u32 bit, u32 npage,  u32 * pvalue) {
	u32 rdata;

	if ((phyad >= RTL8306_PHY_NUMBER) || (regad >= RTL8306_REGSPERPAGE) ||
		(npage >= RTL8306_PAGE_NUMBER) || (bit > 15) || (pvalue == NULL))
		return FAILED;	
	rtl8306_getAsicPhyReg(phyad, regad, npage, &rdata);
	if (rdata & (1 << bit))
		*pvalue =1;
	else 
		*pvalue =0;
		
	return SUCCESS;
}

/*
@func int | rtl8306_setAsicMirrorVlan | Configure switch whether enable the inter_VLAN mirror function .
@parm u32  | enabled | Whether RTL8306 mirror function ignore the VLAN member set domain limitation
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_setAsicMirrorVlan(u32 enabled) {
	rtl8306_setAsicPhyRegBit(2, 23, 6, 3, enabled == FALSE ? 1:0);
	return SUCCESS;
}

/*
@func int | rtl8306_getAsicMirrorVlan | Get switch whether enable the inter_VLAN mirror function .
@parm u32*  | enabled | Whether RTL8306 mirror function ignore the VLAN member set domain limitation
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_getAsicMirrorVlan(u32 * enabled) {
	u32 bitValue;
	
	if (enabled == NULL) 
		return FAILED;
	rtl8306_getAsicPhyRegBit(2, 23, 6, 3, &bitValue);
	*enabled = (bitValue== 0 ? TRUE:FALSE);
	return SUCCESS;
}


/*
@func int | rtl8306_setAsicMirrorPort | Set asic Mirror port
@parm u32 | mirport | Specify mirror port number
@parm u32 | rxport  | Specify Rx mirror port mask
@parm u32 | txport | Specify Tx mirror port mask
@parm u32 | enFilter | Whether enable mirror port to filter packet from itself
@rvalue SUCCESS 
@rvalue FAILED
@comm
mirport could be 0 ~ 5, represent physical port number, 7 means that no port has 
mirror ability. rxport and txport is 6 bit value, each bit corresponds one port
*/
int rtl8306_setAsicMirrorPort(u32 mirport, u32 rxport, u32 txport, u32 enFilter) {
	u32 regValue;
	
	if ((mirport > 7) ||(rxport > 0x3F) || (txport > 0x3F) )
		return FAILED;

	/*Set Mirror Port*/
	rtl8306_getAsicPhyReg(2, 22, 3, &regValue);
	regValue = (regValue & 0xC7FF) | (mirport << 11);
	rtl8306_setAsicPhyReg(2, 22, 3, regValue);
	/*Whether enable mirror port to filter the mirrored packet sent from itself */
	rtl8306_setAsicPhyRegBit(6, 21, 7, 3, enFilter == TRUE  ? 1:0);
	/*Set Ports Whose RX Data are Mirrored */
	rtl8306_getAsicPhyReg(6, 21, 3, &regValue);
	regValue = (regValue & 0xFFC0) | rxport ;
	rtl8306_setAsicPhyReg(6, 21, 3, regValue);	
	/*Set Ports Whose TX Data are Mirrored */
	rtl8306_getAsicPhyReg(6, 21, 3, &regValue);
	regValue = (regValue & 0xC0FF) | (txport << 8);
	rtl8306_setAsicPhyReg(6, 21, 3, regValue);
	return SUCCESS;
}

/*
@func int | rtl8306_getAsicMirrorPort | Get mirror information
@parm u32* | mirport | mirror port
@parm u32* | rxport | Rx mirrored port mask
@parm u32* | txport | Tx mirrored port mask
@parm u32* | enFilter | Whether enable mirror port to filter packet from itself
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/

int rtl8306_getAsicMirrorPort(u32 *mirport, u32 *rxport, u32 *txport, u32 *enFilter) {
	u32 regValue;
	u32 bitValue;
	
	if ((mirport == NULL) ||(rxport == NULL) || (txport == NULL)) 
		return FAILED;
	/*Get Mirror Port*/
	rtl8306_getAsicPhyReg(2, 22, 3, &regValue);
	*mirport = (regValue & 0x3800) >> 11;
	/*Whether enable mirror port to filter the mirrored packet sent from itself */
	rtl8306_getAsicPhyRegBit(6, 21, 7, 3, &bitValue);
	*enFilter = (bitValue == 1 ? TRUE:FALSE);
	/*Get Ports Whose RX Data are Mirrored*/	
	rtl8306_getAsicPhyReg(6, 21, 3, &regValue);	
	*rxport = regValue & 0x3F;	
	/*Get Ports Whose TX Data are Mirrored */	
	rtl8306_getAsicPhyReg(6, 21, 3, &regValue);	
	*txport = (regValue & 0x3F00) >> 8;		
	return SUCCESS;
}

/*
@func int | rtl8306_setAsicMirrorMacAddress | Set DA/SA Mac address for mirror packet
@parm u8* | macAddr | Specify Mac address
@parm u32 | enabled | Enable mirror packet by DA/SA
@rvalue SUCCESS 
@rvalue FAILED
@comm
mirror port could mirror packet by its SA or DA.
*/
int rtl8306_setAsicMirrorMacAddress(u8 *macAddr, u32 enabled) {

	
	if (macAddr == NULL)
		return FAILED;
	if (enabled == FALSE) {
		rtl8306_setAsicPhyRegBit(6, 21, 14, 3, 0);
	} else {
		rtl8306_setAsicPhyRegBit(6, 21, 14, 3, 1);
		rtl8306_setAsicPhyReg(6, 22, 3, (macAddr[1] << 8) | macAddr[0]);
		rtl8306_setAsicPhyReg(6, 23, 3, (macAddr[3] << 8) | macAddr[2]);
		rtl8306_setAsicPhyReg(6, 24, 3, (macAddr[5] << 8) | macAddr[4]);
	}
	return SUCCESS;
}

/*
@func int | rtl8306_getAsicMirrorMacAddress | Get DA/SA Mac address for mirrored packet
@parm u8* | macAddr | the Mac address
@parm u32* | enabled | Whether enable mirror packet by DA/SA
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_getAsicMirrorMacAddress(u8 *macAddr, u32 *enabled) {
	u32 regValue;
	u32 bitValue;
	
	if (macAddr == NULL)
		return FAILED;
	rtl8306_getAsicPhyRegBit(6, 21, 14, 3, &bitValue);
	*enabled = (bitValue == 1 ? TRUE : FALSE);
	rtl8306_getAsicPhyReg(6, 22, 3, &regValue);
	macAddr[0] = regValue & 0xFF;
	macAddr[1] = (regValue & 0xFF00) >> 8;
	rtl8306_getAsicPhyReg(6, 23, 3, &regValue);
	macAddr[2] = regValue & 0xFF;
	macAddr[3] = (regValue & 0xFF00) >> 8;
	rtl8306_getAsicPhyReg(6, 24, 3, &regValue);
	macAddr[4] = regValue & 0xFF;
	macAddr[5] = (regValue & 0xFF00) >> 8;
	return SUCCESS;
}

typedef struct rtl8306_mirrorPara_s {
    u32 mirport;
    u32 rxport;
    u32 txport;
    u8 macAddr[6];
    u32 enMirMac;
} rtl8306_mirrorPara_t;

/*
@func int | rtl8306_setMirror | Set mirror ability
@parm rtl8306_mirrorPara_t | mir | mirror parameter
@struct rtl8306_mirrorPara_t | This structure describes mirror parameter
@field u32 | mirport | Specify mirror port number
@field u32 | rxport  | Specify Rx mirror port mask
@field u32 | txport | Specify Tx mirror port mask
@field u8 | macAddr[6] | Specify Mac address
@field u32 | enMirMac | Enable mirror packet by DA/SA 
@rvalue SUCCESS
@rvalue FAILED
@comm
mirport could be physical port 0 ~5, 7 means that no port has 
mirror ability. rxport and txport is 6 bit value, each bit corresponds 
one port, if one bit is set 1, it means that all Rx or Tx packet of that port
will mirrored to mirror port. mirror port could also mirror packet by SA or DA, 
you could set one Mac address.
*/
int rtl8306_setMirror(rtl8306_mirrorPara_t mir) {

    /*Enable mirror leaky*/
    //if (mir.mirport != 7)
       // rtl8306_setAsicMirrorVlan(TRUE);
    if (rtl8306_setAsicMirrorPort(mir.mirport, mir.rxport, mir.txport, TRUE) == FAILED)
        return FAILED;

    //if (rtl8306_setAsicMirrorMacAddress(mir.macAddr, mir.enMirMac) == FAILED)
       // return FAILED;
    return SUCCESS;
    
}



/*
@func rtl8306_getMirror | Get mirror configuration
@parm rtl8306_mirrorPara_t* | mir
@rvalue SUCCESS
@rvalue FAILED
*/
int rtl8306_getMirror(rtl8306_mirrorPara_t *mir) {
    u32 enFilter;

    if (rtl8306_getAsicMirrorPort(&mir->mirport, &mir->rxport, &mir->txport, &enFilter) == FAILED)
        return FAILED;
    if (rtl8306_getAsicMirrorMacAddress(mir->macAddr, &mir->enMirMac) == FAILED)
        return FAILED;

    return SUCCESS;

}

#if 0
/*
@func int | rtl8306_init | init the asic
@rvalue SUCCESS 
@rvalue FAILED
*/
int rtl8306_init(void) {

    asicVersionPara_t AsicVer;
    u32 regval;

#ifdef   RTL8306_TBLBAK
        u32 cnt;
        /*Vlan default value*/
        rtl8306_TblBak.vlanConfig.enVlan = FALSE;
        rtl8306_TblBak.vlanConfig.enArpVlan = FALSE;
        rtl8306_TblBak.vlanConfig.enLeakVlan = FALSE;
        rtl8306_TblBak.vlanConfig.enVlanTagOnly = FALSE;
        rtl8306_TblBak.vlanConfig.enIngress =  FALSE;
        rtl8306_TblBak.vlanConfig.enTagAware = FALSE;
        rtl8306_TblBak.vlanConfig.enIPMleaky = FALSE;
        rtl8306_TblBak.vlanConfig.enMirLeaky = FALSE;
        for (cnt = 0; cnt < 6; cnt++) {
            rtl8306_TblBak.vlanConfig_perport[cnt].vlantagInserRm = RTL8306_VLAN_UNDOTAG;
            rtl8306_TblBak.vlanConfig_perport[cnt].en1PRemark = FALSE;
            rtl8306_TblBak.vlanConfig_perport[cnt].enNulPvidRep =  FALSE;
        }
        for (cnt = 0; cnt < 16; cnt++) {
            rtl8306_TblBak.vlanTable[cnt].vid = cnt;
            if ((cnt % 5) == 4 ) {            
                rtl8306_TblBak.vlanTable[cnt].memberPortMask = 0x1F;                
            } 
            else  {
                rtl8306_TblBak.vlanTable[cnt].memberPortMask = (0x1<<4) | (0x1 << (cnt % 5));
            }
        }
        for (cnt = 0; cnt < 6; cnt++) {

            rtl8306_TblBak.vlanPvidIdx[cnt] = (u328)cnt;
            rtl8306_TblBak.dot1DportCtl[cnt] = RTL8306_SPAN_FORWARD;
        }
        rtl8306_TblBak.En1PremarkPortMask = 0;
        rtl8306_TblBak.dot1PremarkCtl[0] = 0x3;
        rtl8306_TblBak.dot1PremarkCtl[1] = 0x4;
        rtl8306_TblBak.dot1PremarkCtl[2] = 0x5;
        rtl8306_TblBak.dot1PremarkCtl[3] = 0x6;
        
        for (cnt = 0; cnt < RTL8306_ACL_ENTRYNUM; cnt++) {
            rtl8306_TblBak.aclTbl[cnt].phy_port = RTL8306_ACL_INVALID;
            rtl8306_TblBak.aclTbl[cnt].proto = RTL8306_ACL_ETHER;
            rtl8306_TblBak.aclTbl[cnt].data = 0;
            rtl8306_TblBak.aclTbl[cnt].action = RTL8306_ACT_PERMIT;
            rtl8306_TblBak.aclTbl[cnt].pri = RTL8306_PRIO0;            
        }
        rtl8306_TblBak.mir.mirPort = 0x7;
        rtl8306_TblBak.mir.mirRxPortMask = 0;
        rtl8306_TblBak.mir.mirTxPortMask = 0;
        rtl8306_TblBak.mir.enMirself = FALSE;
        rtl8306_TblBak.mir.enMirMac = FALSE;
        rtl8306_TblBak.mir.mir_mac[0] = 0x0;
        rtl8306_TblBak.mir.mir_mac[1] = 0x0;
        rtl8306_TblBak.mir.mir_mac[2] = 0x0;
        rtl8306_TblBak.mir.mir_mac[3] = 0x0;
        rtl8306_TblBak.mir.mir_mac[4] = 0x0;
        rtl8306_TblBak.mir.mir_mac[5] = 0x0;        

#endif

    /*Fix EQC problem in Version B of RTL8306 series*/

    rtl8306_getAsicVersionInfo(&AsicVer);
    if ((AsicVer.chipid == RTL8306_CHIPID) && 
        (AsicVer.vernum == RTL8306_VERNUM) && 
        (AsicVer.revision == 0x0)  )
    {
        rtl8306_setAsicPhyReg(2, 26, 0, 0x0056);
    }

    /*green featue for Version E*/
    if ((AsicVer.chipid == RTL8306_CHIPID) && 
        (AsicVer.vernum == RTL8306_VERNUM) && 
        (AsicVer.revision == 0x3))
    {
        rtl8306_setAsicPhyRegBit(0, 16, 11, 0, 1);  

        rtl8306_getAsicPhyReg(0, 26, 0, &regval);  
        regval &= ~0x7007;
        regval |= 0x3003;
        rtl8306_setAsicPhyReg(0, 26, 0, regval);
        
        rtl8306_getAsicPhyReg(1, 29, 0, &regval);  
        regval &= ~0xFF;
        regval |= 0xC4;
        rtl8306_setAsicPhyReg(1, 29, 0, regval);

        rtl8306_setAsicPhyRegBit(0, 16, 11, 0, 0);  
    }

    
    rtl8306_setAsicCPUPort(RTL8306_NOCPUPORT, FALSE);
    rtl8306_setAsicStormFilterEnable(RTL8306_BROADCASTPKT, TRUE);
    
    return SUCCESS;
} 

#endif

int rtl8306_getVendorID(u32 *id)
{
	if (id == NULL)
		return FAILED;
	rtl8306_getAsicPhyReg(4, 31, 0, id);
	/* bit[8:9]*/
	*id = (*id & 0x300) >> 8;
	return SUCCESS;
}

/*
@func int | rtl8306_asicSoftReset | Soft reset the asic
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_asicSoftReset(void) {
	int i;
	u32 bitLink = 0;
	u32 bitSPD = 0;
	u32 bitUDP = 0;
      u32 regval;

      /*get chip version */
      rtl8306_setAsicPhyRegBit(0, 16, 11, 0, 1);
      rtl8306_getAsicPhyReg(4, 26, 0, &regval);
      rtl8306_setAsicPhyRegBit(0, 16, 11, 0, 0);
      regval = (regval & 0xE000) >> 13;
   
      if (regval)
      {
          /*software reset, just set the bit is ok for C version*/
	    rtl8306_setAsicPhyRegBit(0, 16, 12, 0, 1);
           return SUCCESS;             
      }
    
	/*read port5 link status*/
	rtl8306_getAsicPhyRegBit( 6, 1, 2, 0, &bitLink);
	/*read port5 speed and duplex*/
	rtl8306_getAsicPhyRegBit(6, 0, 13, 0, &bitSPD);
	rtl8306_getAsicPhyRegBit(6, 0, 8, 0, &bitUDP);

	/*disable all mac Tx & Rx*/
	for(i = 0; i < 5 ;i++){
		rtl8306_setAsicPhyRegBit(i, 24, 10, 0, 0);
		rtl8306_setAsicPhyRegBit(i, 24, 11, 0, 0);
	}
	/*trunk port3 & port4*/
	rtl8306_setAsicPhyRegBit(0, 16, 6, 0, 0);
	rtl8306_setAsicPhyRegBit(0, 19, 11, 0, 0);

	/*disable port5*/
	rtl8306_setAsicPhyRegBit(6, 22, 15, 0, 0);	
	/*software reset*/
	rtl8306_setAsicPhyRegBit(0, 16, 12, 0, 1);
	/*disable port3 & port4 trunk*/
	rtl8306_setAsicPhyRegBit(0, 16, 6, 0, 1);
	rtl8306_setAsicPhyRegBit(0, 19, 11, 0, 1);
	/*enable all mac Tx & Rx*/
	for(i = 0; i < 5 ;i++){
		rtl8306_setAsicPhyRegBit(i, 24, 10, 0, 1);
		rtl8306_setAsicPhyRegBit(i, 24, 11, 0, 1);
	}
	/*restore port5 speed & duplex & link status*/
	rtl8306_setAsicPhyRegBit(6, 22, 15, 0, bitLink);
	rtl8306_setAsicPhyRegBit(6, 0, 13, 0, bitSPD);
	rtl8306_setAsicPhyRegBit(6, 0, 8, 0, bitUDP);
	return SUCCESS;
}



/*
@func int | rtl8306_setAsicQosPortQueueNum | Set port Tx queue number
@parm u32 | num | Tx queue numbers (1 ~ 4)
@rvalue SUCCESS 
@rvalue FAILED
@comm
Queue number is global configuration for each port
*/
int rtl8306_setAsicQosPortQueueNum(u32 num) {
	u32 regValue;
	
	if ((num ==0) ||(num > 4) )
		return FAILED;
	rtl8306_getAsicPhyReg(2, 22, 3, &regValue);		
	rtl8306_setAsicPhyReg(2, 22, 3, (regValue & 0xFFF3) | ((num-1) << 2));	
	/*A soft-reset is required after configuring queue num*/
	 rtl8306_asicSoftReset( );	
	return SUCCESS;	
}

/*
@func int | rtl8306_getAsicQosPortQueueNum | Get port Tx queue number
@parm u32* | num | Tx queue numbers
*/
int rtl8306_getAsicQosPortQueueNum(u32 *num) {
	u32 regValue;
	
	if (num == NULL) 
		return FAILED;
	rtl8306_getAsicPhyReg(2, 22, 3, &regValue);
	*num =    ((regValue & 0xC) >> 2) + 1;
	return SUCCESS;

}

/* PHY auto-negotiation advertisement and 
link partner ability registers field definitions
*/
#define RTL8306_NEXT_PAGE_ENABLED                           (1 << 15)
#define RTL8306_ACKNOWLEDGE                                 (1 << 14)
#define RTL8306_REMOTE_FAULT                                (1 << 13)
#define RTL8306_CAPABLE_PAUSE                               (1 << 10)
#define RTL8306_CAPABLE_100BASE_T4                          (1 << 9)
#define RTL8306_CAPABLE_100BASE_TX_FD                       (1 << 8)
#define RTL8306_CAPABLE_100BASE_TX_HD                       (1 << 7)
#define RTL8306_CAPABLE_10BASE_TX_FD                        (1 << 6)
#define RTL8306_CAPABLE_10BASE_TX_HD                        (1 << 5)
#define RTL8306_SELECTOR_MASK                               0x1F
#define RTL8306_SELECTOR_OFFSET                             0


#define RTL8306_IGMP 0
#define RTL8306_MLD  1
#define RTL8306_PORT_RX  0
#define RTL8306_PORT_TX  1
#define RTL8306_QUEUE0	0
#define RTL8306_QUEUE1	1
#define RTL8306_QUEUE2	2
#define RTL8306_QUEUE3	3
#define RTL8306_ACL_PRIO 0
#define RTL8306_DSCP_PRIO 1
#define RTL8306_1QBP_PRIO 2
#define RTL8306_PBP_PRIO 3
#define RTL8306_CPUTAG_PRIO 4


#define RTL8306_QOS_SET0			0
#define RTL8306_QOS_SET1 		1
#define RTL8306_DSCP_EF			0
#define RTL8306_DSCP_AFL1		1
#define RTL8306_DSCP_AFM1		2
#define RTL8306_DSCP_AFH1		3
#define RTL8306_DSCP_AFL2		4	
#define RTL8306_DSCP_AFM2		5		
#define RTL8306_DSCP_AFH2		6		
#define RTL8306_DSCP_AFL3		7			
#define RTL8306_DSCP_AFM3		8
#define RTL8306_DSCP_AFH3		9
#define RTL8306_DSCP_AFL4		10	
#define RTL8306_DSCP_AFM4		11		
#define RTL8306_DSCP_AFH4		12
#define RTL8306_DSCP_NC			13
#define RTL8306_DSCP_REG_PRI		14			
#define RTL8306_DSCP_BF			15

#define RTL8306_DSCP_USERA		0
#define RTL8306_DSCP_USERB		1
#define RTL8306_IPADD_A	0
#define RTL8306_IPADD_B	1

#define RTL8306_FCO_SET0			0x0
#define RTL8306_FCO_SET1			0x1
#define RTL8306_FCOFF			0x0
#define RTL8306_FCON				0x1
#define RTL8306_FCO_DSC			0x0
#define RTL8306_FCO_QLEN	 		0x1
#define RTL8306_FCO_FULLTHR		0x0
#define RTL8306_FCO_OVERTHR 		0x1

#define RTL8306_ACL_ENTRYNUM	16
#define RTL8306_ACL_INVALID		0x6
#define RTL8306_ACL_ANYPORT  	0x7
#define RTL8306_ACL_ETHER		0x0
#define RTL8306_ACL_TCP			0x1
#define RTL8306_ACL_UDP			0x2
#define RTL8306_ACL_TCPUDP		0x3


#define RTL8306_MIB_CNT1			0
#define RTL8306_MIB_CNT2			1
#define RTL8306_MIB_CNT3			2
#define RTL8306_MIB_CNT4			3
#define RTL8306_MIB_CNT5			4
#define RTL8306_MIB_RESET		0
#define RTL8306_MIB_START		1
#define RTL8306_MIB_BYTE			0
#define RTL8306_MIB_PKT			1



int rtl8306_setAsicQosQueueFlowControlThr(u32 queue, u32 type, u32 onoff, u32 set, u32 value, u32 enabled) {
	u32 regValue, mask;
	u32 selection;
	u32 reg, shift;
	
	if ((queue > RTL8306_QUEUE3) || (type > RTL8306_FCO_QLEN) || (onoff > RTL8306_FCON) || (set > RTL8306_FCO_SET1))
		return FAILED;
	selection = (set << 2) | (onoff <<1) |type;
	switch (selection) {
	case 0 : 		/*set 0, turn off, DSC*/
		if (queue == RTL8306_QUEUE0) {
			reg = 17;
			mask = 0xFFF0;
			shift = 0;
		} else if  (queue == RTL8306_QUEUE1 ) {
			reg = 17;
			mask = 0xF0FF;
			shift = 8;
		} else if (queue == RTL8306_QUEUE2 ) {
			reg = 20;
			mask = 0xFFF0;
			shift = 0;
		} else  {
			reg = 20;
			mask = 0xF0FF;
			shift = 8;
		}
		rtl8306_getAsicPhyReg(5, reg, 2, &regValue);
		regValue = (regValue & mask) | (value << shift);
		rtl8306_setAsicPhyReg(5, reg, 2, regValue);						
		break;
	case 1 :		/*set 0, turn off, QLEN*/
		if (queue == RTL8306_QUEUE0) {
			reg = 17;
			mask = 0xFF0F;
			shift = 4;
		} else if  (queue == RTL8306_QUEUE1 ) {
			reg = 17;
			mask = 0x0FFF;
			shift = 12;
		} else if (queue == RTL8306_QUEUE2 ) {
			reg = 20;
			mask = 0xFF0F;
			shift = 4;
		} else  {
			reg = 20;
			mask = 0x0FFF;
			shift = 12;
		}
		rtl8306_getAsicPhyReg(5, reg, 2, &regValue);
		regValue = (regValue & mask) | (value << shift);
		rtl8306_setAsicPhyReg(5, reg, 2, regValue);						
		break;
	case 2 :		/*set 0, turn on, DSC*/
		if (queue == RTL8306_QUEUE0) 
			reg = 18;
		else if  (queue == RTL8306_QUEUE1 ) 
			reg = 19;
		else if (queue == RTL8306_QUEUE2 )
			reg = 21;
		else  
			reg = 22;
		rtl8306_getAsicPhyReg(5, reg, 2, &regValue);
		regValue = (regValue & 0xFFC0) | value;
		rtl8306_setAsicPhyReg(5, reg, 2, regValue);					
		break;
	case 3:		/*set 0, turn  on, QLEN*/
		if (queue == RTL8306_QUEUE0) 
			reg = 18;
		 else if  (queue == RTL8306_QUEUE1 ) 
			reg = 19;
		 else if (queue == RTL8306_QUEUE2 ) 
			reg = 21;
		 else  
			reg = 22;	
		if (queue != RTL8306_QUEUE3)  {
			rtl8306_getAsicPhyReg(5, reg, 2, &regValue);
			regValue = (regValue & 0xC0FF) | (value << 8);
			rtl8306_setAsicPhyReg(5, reg, 2, regValue);
		}  else {
			rtl8306_getAsicPhyReg(5, reg, 2, &regValue);
			regValue = (regValue & 0x3FF) | (value << 10);
			rtl8306_setAsicPhyReg(5, reg, 2, regValue);			
		}	
		break;
	case 4:		/*set 1, turn off, DSC*/
		if (queue == RTL8306_QUEUE0) {
			reg = 23;
			mask = 0xFFF0;
			shift =0;
		} else if  (queue == RTL8306_QUEUE1 ) {
			reg = 23;
			mask = 0xF0FF;
			shift =8;
		} else if (queue == RTL8306_QUEUE2 ) {
			reg = 26;
			mask = 0xFFF0;
			shift =0;
		} else  {
			reg = 26;
			mask = 0xF0FF;
			shift =8;		
		}
		rtl8306_getAsicPhyReg(5, reg, 2, &regValue);
		regValue = (regValue & mask) | (value << shift);
		rtl8306_setAsicPhyReg(5, reg, 2, regValue);						
		break;
	case 5:		/*set 1, turn off, QLEN*/
		if (queue == RTL8306_QUEUE0) {
			reg = 23;
			mask = 0xFF0F;
			shift = 4;
		} else if  (queue == RTL8306_QUEUE1 ) {
			reg = 23;
			mask = 0x0FFF;
			shift = 12;
		} else if (queue == RTL8306_QUEUE2 ) {
			reg = 26;
			mask = 0xFF0F;
			shift = 4;
		} else  {
			reg = 26;
			mask = 0x0FFF;
			shift = 12;		
		}
		rtl8306_getAsicPhyReg(5, reg, 2, &regValue);
		regValue = (regValue & mask) | (value << shift);
		rtl8306_setAsicPhyReg(5, reg, 2, regValue);								
		break;
	case 6:		/*set 1, turn on, DSC*/
		if (queue == RTL8306_QUEUE0) 
			reg = 24;
		else if  (queue == RTL8306_QUEUE1 ) 
			reg =25;
		else if (queue == RTL8306_QUEUE2 ) 
			reg = 27;
		else  
			reg = 28;		
		rtl8306_getAsicPhyReg(5, reg, 2, &regValue);
		regValue = (regValue & 0xFFC0) | value;
		rtl8306_setAsicPhyReg(5, reg, 2, regValue);						
		break;
	case 7:		/*set 1, turn  on, QLEN*/
		if (queue == RTL8306_QUEUE0) 
			reg = 24;
		else if  (queue == RTL8306_QUEUE1 ) 
			reg =25;
		else if (queue == RTL8306_QUEUE2 ) 
			reg = 27;
		else  
			reg = 28;		
		if (queue != RTL8306_QUEUE3)  {
			rtl8306_getAsicPhyReg(5, reg, 2, &regValue);
			regValue = (regValue & 0xC0FF) | (value << 8);
			rtl8306_setAsicPhyReg(5, reg, 2, regValue);
		}  else {
			rtl8306_getAsicPhyReg(5, reg, 2, &regValue);
			regValue = (regValue & 0x3FF) | (value << 10);
			rtl8306_setAsicPhyReg(5, reg, 2, regValue);			
		}	
		break;
		
	default:
		return FAILED;
	}

	/*Enable/Disable Flow control of the specified queue*/
	switch (queue) {
	case RTL8306_QUEUE0:
		if (set == RTL8306_FCO_SET0)
			rtl8306_setAsicPhyRegBit(5, 22, 6, 2, enabled == FALSE ? 1:0);
		else 
			rtl8306_setAsicPhyRegBit(5, 28, 6, 2, enabled == FALSE ? 1:0);			
		break;
	case RTL8306_QUEUE1:
		if (set == RTL8306_FCO_SET0)
			rtl8306_setAsicPhyRegBit(5, 22, 7, 2, enabled == FALSE ? 1:0);
		else 
			rtl8306_setAsicPhyRegBit(5, 28, 7, 2, enabled == FALSE ? 1:0);					
		break;
	case RTL8306_QUEUE2:
		if (set == RTL8306_FCO_SET0)
			rtl8306_setAsicPhyRegBit(5, 22, 8, 2, enabled == FALSE ? 1:0);
		else 
			rtl8306_setAsicPhyRegBit(5, 28, 8, 2, enabled == FALSE ? 1:0);					
		break;
	case RTL8306_QUEUE3:
		if (set == RTL8306_FCO_SET0)
			rtl8306_setAsicPhyRegBit(5, 22, 9, 2, enabled == FALSE ? 1:0);
		else 
			rtl8306_setAsicPhyRegBit(5, 28, 9, 2, enabled == FALSE ? 1:0);					
		break;
	default:
		return FAILED;
	}			
	return SUCCESS;
}


int rtl8306_getAsicQosPortFLowControlThr(u32 port, u32 *onthr, u32 *offthr, u32 direction) {
	u32 regValue;

	if ((port > RTL8306_PORT5) || (onthr == NULL) || (offthr == NULL) || (direction > 1))
		return FAILED;
	/*Port 5 corresponding PHY6*/	
	if (port == RTL8306_PORT5 )  
		port ++ ;  		
	if (direction == RTL8306_PORT_TX) 
		rtl8306_getAsicPhyReg(port, 20, 2, &regValue);	
	else 
		rtl8306_getAsicPhyReg(port, 19, 3, &regValue);	
	*onthr = regValue & 0xFF;
	*offthr = (regValue & 0xFF00) >> 8;
	return SUCCESS;
}

 int rtl8306_setAsicQosPortFlowControlMode(u32 port, u32 set) {

	if ((port > RTL8306_PORT5) || (set > RTL8306_FCO_SET1))
		return FAILED;
	if (port < RTL8306_PORT5) 
		rtl8306_setAsicPhyRegBit(port, 18, 12, 2, set);
	else 
		rtl8306_setAsicPhyRegBit(6, 18, 12, 2, set);
	return SUCCESS;
}


int rtl8306_setAsicQosPortFlowControlThr(u32 port, u32 onthr, u32 offthr, u32 direction ) {
	u32 regValue;

	if ((port > RTL8306_PORT5) || (direction > 1))
		return FAILED;
	regValue = (offthr << 8) + onthr;
	/*Port 5 corresponding PHY6*/	
	if (port == RTL8306_PORT5 )  
		port ++ ;  	
	if (direction == RTL8306_PORT_TX) 
		rtl8306_setAsicPhyReg(port, 20, 2, regValue);
	else 
		rtl8306_setAsicPhyReg(port, 19, 3, regValue);
	return SUCCESS;
}


/*
@func int | rtl8306_setAsicQosTxQueueStrictPriority | Set Qos Tx queue strict priority
@parm u32 | queue | Specify queue number
@parm u32 | set | RTL8306_QOS_SET0 or RTL8306_QOS_SET1
@parm u32 | enabled | Enabled strict priority
@rvalue SUCCESS 
@rvalue FAILED
@comm
In four queues, only RTL8306_QUEUE3(queue 3) and RTL8306_QUEUE2(queue 2)
could be set strict priority. Altogether, there are total 2 group setting, 
which are RTL8306_QOS_SET0 and RTL8306_QOS_SET1.
*/
int rtl8306_setAsicQosTxQueueStrictPriority(u32 queue, u32 set, u32 enabled) {
    	
    
    if ((queue < RTL8306_QUEUE2)  || (set > 1)) 
		return FAILED;
	
	switch(queue) {
	case RTL8306_QUEUE2:
		if (set == 0)
			rtl8306_setAsicPhyRegBit(5, 21, 7, 3, enabled == TRUE ? 1:0);
		else
			rtl8306_setAsicPhyRegBit(5, 26, 7, 3, enabled == TRUE ? 1:0);		
		break;
	case RTL8306_QUEUE3:
		if (set == 0)
			rtl8306_setAsicPhyRegBit(5, 21, 15, 3, enabled == TRUE ? 1:0);
		else
			rtl8306_setAsicPhyRegBit(5, 26, 15, 3, enabled == TRUE ? 1:0);
		break;
	default:
		return FAILED;		
	}
       			
	return SUCCESS;
}

/*
@func int | rtl8306_getAsicQosTxQueueStrictPriority | Get Asic Tx queue strict priority ability
@parm u32 | queue | Specify queue number
@parm uin32  | set | Specify which set 
@parm u32* | enabled | Enabled or Disabled
@rvalue SUCCESS 
@rvalue FAILED
@comm
In four queues, only RTL8306_QUEUE3(queue 3) and RTL8306_QUEUE2(queue 2)
could be set strict priority. Altogether, there are total 2 group setting, 
which are RTL8306_QOS_SET0 and RTL8306_QOS_SET1.

*/
int rtl8306_getAsicQosTxQueueStrictPriority(u32 queue, u32 set, u32 *enabled) {
	u32 bitValue;

	if ((queue < RTL8306_QUEUE2) || (set > 1) || (enabled == NULL)) 
		return FAILED;
	switch(queue) {
	case RTL8306_QUEUE2:
		if (set == 0)
			rtl8306_getAsicPhyRegBit(5, 21, 7, 3, &bitValue);
		else
			rtl8306_getAsicPhyRegBit(5, 26, 7, 3, &bitValue);
		break;
	case RTL8306_QUEUE3:
		if (set == 0)
			rtl8306_getAsicPhyRegBit(5, 21, 15, 3, &bitValue);
		else
			rtl8306_getAsicPhyRegBit(5, 26, 15, 3, &bitValue);
		break;
	default:
		return FAILED;		
	}
	*enabled = (bitValue == 1 ? TRUE:FALSE);
	
	return SUCCESS;		
}


/*
@func int | rtl8306_setAsicQosAutoTurnOffFlowControl |Set Auto Turn Off function of Flow Control Ability.
@parm u32 | enabled | Enabled or Disabled
@rvalue SUCCESS 
@rvalue FAILED
@comm
Enable. Enable Auto turn off low priority queue's flow control ability 1~2 sec whenever the port received a high 
priority frame. The flow control ability will re-enabled when there is not received and high priority frame during 
1~2 sec.
*/

int rtl8306_setAsicQosAutoTurnOffFlowControl(u32 enabled)
{

    rtl8306_setAsicPhyRegBit(3, 23, 0, 0, enabled ? 1:0);
    return SUCCESS;
}

/*
@func int | rtl8306_setAsicQosTxQueueLeakyBucket | Set Tx queue Leaky bucket
@parm u32 | queue | Specify queue number
@parm u32 | set | Specify the set
@parm u32 | burstsize | queue leaky bucket burst size ( 0 ~ 48)
@parm u32 | rate | Set queue rate (0 ~ 1526)
@rvalue SUCCESS 
@rvalue FAILED
@comm
There are two sets configuration for bandwidth control, one is RTL8306_QOS_SET0, 
the other is RTL8306_QOS_SET1, each port could choose which one it uses. In four
Tx queues of each port, only RTL8306_QUEUE3 and RTL8306_QUEUE2 have leaky 
bucket to limit queue rate, queue leaky bucket has burst size to adjust traffic, 
burstsize uinit is KB, so Max burst size is 48KB. Queue rate unit is 64Kbps, so 
the config step is 64Kbps, 0 means disable the queue leaky bucket.
*/
int rtl8306_setAsicQosTxQueueLeakyBucket(u32 queue, u32 set, u32 burstsize, u32 rate) {
	u32 regValue;

	if ((queue < RTL8306_QUEUE2) || (set > 1) || (burstsize > 0x30) ||(rate > 0x5F6 ))
		return FAILED;
	switch(queue) {
	case RTL8306_QUEUE2:
		if(set == 0) {
			rtl8306_getAsicPhyReg(5, 17, 3, &regValue);
			regValue = (regValue & 0xC0FF) | (burstsize << 8);
			rtl8306_setAsicPhyReg(5, 17, 3, regValue);
			rtl8306_getAsicPhyReg(5, 18, 3, &regValue);
			regValue = (regValue & 0xF800) | (rate & 0x7FF );
			rtl8306_setAsicPhyReg(5, 18, 3, regValue);
		} else {
			rtl8306_getAsicPhyReg(5, 22, 3, &regValue);
			regValue = (regValue & 0xC0FF) | (burstsize << 8);
			rtl8306_setAsicPhyReg(5, 22, 3, regValue);
			rtl8306_getAsicPhyReg(5, 23, 3, &regValue);
			regValue = (regValue & 0xF800) | (rate & 0x7FF);
			rtl8306_setAsicPhyReg(5, 23, 3, regValue);
		}
		break;
	case RTL8306_QUEUE3:
		if(set == 0) {
			rtl8306_getAsicPhyReg(5, 17, 3, &regValue);
			regValue = (regValue & 0xFFC0) | burstsize;
			rtl8306_setAsicPhyReg(5, 17, 3, regValue);
			rtl8306_getAsicPhyReg(5, 19, 3, &regValue);
			regValue = (regValue & 0xF800) | (rate & 0x7FF);
			rtl8306_setAsicPhyReg(5, 19, 3, regValue);
			
		} else {
			rtl8306_getAsicPhyReg(5, 22, 3, &regValue);
			regValue = (regValue & 0xFFC0) | burstsize;
			rtl8306_setAsicPhyReg(5, 22, 3, regValue);
			rtl8306_getAsicPhyReg(5, 24, 3, &regValue);
			regValue = (regValue & 0xF800) | (rate & 0x7FF);
			rtl8306_setAsicPhyReg(5, 24, 3, regValue);
		}
		break;
	default:
		return FAILED;
	}
	/*A soft reset is required after confiugre LB size*/
	 rtl8306_asicSoftReset( );		
	
	return SUCCESS;
}

/*
@func int | rtl8306_getAsicQosTxQueueLeakyBucket | Get Tx queue leaky bucket configuration
@parm u32 | queue | Specify queue number
@parm u32 | set | Specify which set
@parm u32* | burstsize | queue leaky burst size
@parm u32* | rate | queue rate
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/

int rtl8306_getAsicQosTxQueueLeakyBucket(u32 queue, u32 set, u32 *burstsize, u32 *rate) {
	u32 regValue;

	if ((queue < RTL8306_QUEUE2) || (set > 1) || 
		(burstsize == NULL) || (rate ==NULL))
		return FAILED;
	switch(queue) {
	case RTL8306_QUEUE2 :
		if (set == 0)  {
			rtl8306_getAsicPhyReg(5, 17, 3, &regValue);
			*burstsize = (regValue & 0x3F00) >>8;
			rtl8306_getAsicPhyReg(5, 18, 3, &regValue);
			*rate = regValue &  0x7FF;
		} else {
			rtl8306_getAsicPhyReg(5, 22, 3, &regValue);
			*burstsize = (regValue & 0x3F00) >>8;
			rtl8306_getAsicPhyReg(5, 23, 3, &regValue);
			*rate = regValue & 0x7FF;
		} 
		break;
	case RTL8306_QUEUE3:
		if (set == 0) {
			rtl8306_getAsicPhyReg(5, 17, 3, &regValue);
			*burstsize = regValue & 0x3F;
			rtl8306_getAsicPhyReg(5, 19, 3, &regValue);
			*rate = regValue & 0x7FF;
		
		} else {
			rtl8306_getAsicPhyReg(5, 22, 3, &regValue);
			*burstsize = regValue & 0x3F;
			rtl8306_getAsicPhyReg(5, 24, 3, &regValue);
			*rate = regValue & 0x7FF;
		}
		break;
	default:
		return FAILED;
	}	
	
	return SUCCESS;
} 

/*
@func int | rtl8306_setAsicQosPortScheduleMode | Set port schedule mode
@parm u32 | port | Specify port number
@parm u32 | set |  Specify set
@parm u32 | quemask | Queue mask for enable queue leaky buckt
@rvalue SUCCESS 
@rvalue FAILED
@comm
There are two sets configuration for schedule mode including strict priority 
enable/disable, queue weight and queue leaky bucket, every port could select
one of them. Queue leaky bucket of each port could be enable separately, so 
you can set queue mask to enable/disable them, because only queue 3 and queue 2
have leaky bucket, only bit 3 and bit 2 of quemask have effect, bit 3 represents
queue 3 and set 1 to enable it.
*/
int rtl8306_setAsicQosPortScheduleMode(u32 port, u32 set, u32 quemask) {
	u32 regValue;
	
	if ((port > RTL8306_PORT5) ||(set > 1))
		return FAILED;
	/*Port 5 corresponding PHY6*/	
	if (port == RTL8306_PORT5 )  
		port ++ ; 
	quemask = ((quemask & 0x8) >> 3 ) | ((quemask & 0x4) >> 1);
	rtl8306_getAsicPhyReg(port, 18, 2, &regValue);
	regValue = (regValue & 0x97FF) | (quemask << 13) | (set & 0x1) << 11;
	rtl8306_setAsicPhyReg(port, 18, 2, regValue);
	return SUCCESS;
}

/*
@func int | rtl8306_getAsicQosPortScheduleMode | Get port schedule mode
@parm u32 | port | Specify port number
@parm u32* | set |  set number
@parm u32* | quemask | queue leaky buckets mask
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_getAsicQosPortScheduleMode(u32 port, u32 *set, u32 *quemask) {

	u32 regValue;
	
	if ((port > RTL8306_PORT5) ||(set == NULL) || (quemask == NULL))
		return FAILED;	
	/*Port 5 corresponding PHY6*/	
	if (port == RTL8306_PORT5 )  
		port ++ ;  
	//rtl8306_getAsicPhyRegBit(port, 18, 11, 2, set);	
	rtl8306_getAsicPhyReg(port, 18, 2, &regValue );
	*set = (regValue >> 11) & 0x1;
	*quemask = (regValue >> 13) & 0x3;
	*quemask = ((*quemask & 0x1) << 3) | ((*quemask & 0x2) << 1);
	return SUCCESS;
}

/*
@func int | rtl8306_setAsicQosPortRate | Set port bandwidth control
@parm u32 | port | Specify port number (0~5)
@parm u32 | rate | Port rate (0~1526)
@parm u32 | direction | Input or output bandwidth control
@parm u32 | enabled | enable bandwidth control
@rvalue SUCCESS 
@rvalue FAILED
@comm
For each port, both input and output bandwidth could be configured, 
RTL8306_PORT_RX represents port input bandwidth control, 
RTL8306_PORT_TX represents port output bandwidth control.
port rate unit is 64Kbps. For output rate control, enable/disable is configured per port, 
but for input rate control, it is for all port.

*/
int rtl8306_setAsicQosPortRate(u32 port, u32 rate, u32 direction, u32 enabled) {
	u32 regValue;
	
	if ((port > RTL8306_PORT5) || (rate > 0x5F6) || (direction > 1))
		return FAILED;
	/*Port 5 corresponding PHY6*/	
	if (port == RTL8306_PORT5 )  
		port ++ ;  		
	if (direction == RTL8306_PORT_RX) {  /*configure port Rx rate*/
		if (enabled == FALSE) {
			rtl8306_setAsicPhyRegBit(0, 21, 15, 3, 1);			
		} else {
			rtl8306_setAsicPhyRegBit(0, 21, 15, 3, 0);			
			rtl8306_getAsicPhyReg(port, 21, 2, &regValue);
			regValue = (regValue & 0xF800) | (rate & 0x7FF);
			rtl8306_setAsicPhyReg(port, 21, 2, regValue);
		}		
	} else { 	  /*configure port Tx rate*/
		if (enabled == FALSE) {
			rtl8306_setAsicPhyRegBit(port, 18, 15, 2, 0);			
		} else {
			rtl8306_setAsicPhyRegBit(port, 18, 15, 2, 1);
			rtl8306_getAsicPhyReg(port, 18, 2, &regValue);
			regValue = (regValue & 0xF800) | (rate & 0x7FF);
			rtl8306_setAsicPhyReg(port, 18, 2, regValue);
		}
	}
	return SUCCESS;
}

/*
@func int | rtl8306_getAsicQosPortRate | Get asic port rate configuration
@parm u32* | rate | the rate
@parm u32* | direction | Input or output bandwidth control
@parm u32* | enabled | enabled or disabled bandwidth control
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_getAsicQosPortRate(u32 port, u32 *rate, u32 direction, u32 *enabled) {
	u32 regValue;

	if ((port > RTL8306_PORT5) || (rate == NULL) || (direction > RTL8306_PORT_TX) || (enabled == NULL))
		return FAILED;
	/*Port 5 corresponding PHY6*/	
	if (port == RTL8306_PORT5 )  
		port ++ ;  
#if 0	
	rtl8306_getAsicPhyRegBit(port, 18, 15, 2, enabled);
	if (direction == RTL8306_PORT_RX) {	/*Get port Rx rate*/
		rtl8306_getAsicPhyReg(port, 21, 2, &regValue);
		*rate = regValue & 0x7FF;				
	} else { 			/*Get port Tx rate*/
		rtl8306_getAsicPhyReg(port, 18, 2, &regValue);
		*rate = regValue & 0x7FF;
	}
#endif
	if (direction == RTL8306_PORT_RX) {	/*Get port Rx rate*/
		rtl8306_getAsicPhyRegBit(0, 21, 15, 3, &regValue);
		*enabled = (regValue == 1 ? FALSE:TRUE);
		rtl8306_getAsicPhyReg(port, 21, 2, &regValue);
		*rate = regValue & 0x7FF;				
	} else { 			/*Get port Tx rate*/
		rtl8306_getAsicPhyRegBit(port, 18, 15, 2, enabled);		
		rtl8306_getAsicPhyReg(port, 18, 2, &regValue);
		*rate = regValue & 0x7FF;
	}
	return SUCCESS;
}

/*
@func int | rtl8306_setAsicQosRxRateGlobalControl | Set input bandwidth global control
@parm u32 | hisize | Rx leaky bucket token high size (0 ~63)
@parm u32 | losize | Rx leaky bucket token low size(0 ~63)
@parm u32 | preamble | Whether include preamble
@rvalue SUCCESS 
@rvalue FAILED
@comm
Input bandwidth global control is realized by setting leaky bucket high/low threshold,
the u32 is KB.
*/
int rtl8306_setAsicQosRxRateGlobalControl(u32 hisize, u32 losize, u32 preamble) {
	u32 regValue;

	
	if ((hisize > 0x3F ) || (losize > 0x3F) || (preamble >1))
		return FAILED;

	rtl8306_getAsicPhyReg(0, 21, 3, &regValue);
	regValue = (regValue & 0x80C0) | ((preamble == TRUE ? 1:0) << 14) |(hisize <<8) | (losize);
	rtl8306_setAsicPhyReg(0, 21, 3, regValue);
	return SUCCESS;
} 

/*
@func int | rtl8306_getAsicQosRxRateGlobalControl | Get input bandwidth global control
@parm u32* | hisize | Rx leaky bucket token high size (0 ~63)
@parm u32* | losize | Rx leaky bucket token low size(0 ~63)
@parm u32* | preamble | Whether include preamble
*/
int rtl8306_getAsicQosRxRateGlobalControl(u32 *hisize, u32 *losize, u32 *preamble) {
	u32 regValue;

	if ((hisize == NULL) || (losize == NULL) || (preamble == NULL))
		return FAILED;
	rtl8306_getAsicPhyReg(0, 21, 3, &regValue);	
	*preamble = (regValue & 0x4000) ? TRUE:FALSE;
	*hisize = (regValue & 0x3F00) >> 8;
	*losize = regValue & 0x3F;

	return SUCCESS;
}

/*
@func int | rtl8306_setAsicQosPktPriorityAssign | Packet priority selection
@parm u32 | type | Specify priority type
@parm u32 | level | Specify priority level (0 ~4)
@rvalue SUCCESS 
@rvalue FAILED
@comm
The asic could recognize 6 type priorities which could exist at same time,
in all of them, there are 4 type priorities could be set priority level,
they are<nl> 
	RTL8306_ACL_PRIO 	-  ACL-based  priority<nl>
	RTL8306_DSCP_PRIO -  DSCP-based priority<nl>
	RTL8306_1QBP_PRIO -  1Q-based priority<nl>
	RTL8306_PBP_PRIO  -  Port-based priority<nl>
each one could be set level from 0 to 4, when they 	coexist, arbitration
module will choose the highest level prority type as packet priority.
	
*/
int rtl8306_setAsicQosPktPriorityAssign(u32 type, u32 level) {
	u32 regValue;

	if ((type > 3) || (level > 4))
		return FAILED;
	rtl8306_getAsicPhyReg(1, 21, 3, &regValue);
	switch(type) {
	case RTL8306_ACL_PRIO:
		if (level == 0)
			regValue = regValue & 0x0FFF;
		else 
			regValue = (regValue & 0x0FFF) | (1 << (level - 1 + 12));
		break;
	case  RTL8306_DSCP_PRIO:
		if (level == 0)
			regValue = regValue & 0xF0FF;
		else 
			regValue = (regValue & 0xF0FF) | (1 << (level- 1 + 8 ));
		break;
	case RTL8306_1QBP_PRIO:
		if (level == 0)
			regValue = regValue & 0xFF0F;
		else
			regValue = (regValue & 0xFF0F) | (1 << (level -1 + 4 ));
		break;
	case RTL8306_PBP_PRIO:
		if (level == 0)
			regValue = regValue & 0xFFF0;
		else 
			regValue = (regValue & 0xFFF0) | (1 << (level -1));
		break;
	default:
		return FAILED;
	}	
	rtl8306_setAsicPhyReg(1, 21, 3, regValue);
	return SUCCESS;
}

/*
@func int | rtl8306_getAsicQosPktPriorityAssign | Get pkt priority assign level
@parm u32 | type | Specify priority type
@parm u32* | level | priority level
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/

int rtl8306_getAsicQosPktPriorityAssign(u32 type, u32 *level) {
	u32 regValue;

	if ((type > 3) ||(level == NULL))
		return FAILED;
	rtl8306_getAsicPhyReg(1, 21, 3, &regValue);
	switch(type) {
	case RTL8306_ACL_PRIO :
		regValue = (regValue & 0xF000) >> 12;								
		break;
	case RTL8306_DSCP_PRIO:
		regValue = (regValue & 0x0F00) >> 8;
		break;
	case RTL8306_1QBP_PRIO:
		regValue = (regValue & 0x00F0) >> 4;
		break;
	case RTL8306_PBP_PRIO:
		regValue = (regValue & 0x000F);
		break;
	default :
		return FAILED;
	}
	
	switch(regValue) {
	case 0x0:
		*level = 0;
		break;
	case 0x1:
		*level = 1;
		break;
	case 0x2:
		*level = 2;
		break;
	case 0x4:
		*level = 3;
		break;
	case 0x8:
		*level =4;
		break;
	default:
		return FAILED;
	}
	
	return SUCCESS;
}

/*
@func int | rtl8306_setAsicQosPrioritytoQIDMapping | Set priority to Queue ID mapping
@parm u32 | priority | priority vale (0 ~ 3)
@parm u32 | qid | Queue id (0~3)
@rvalue SUCCESS 
@rvalue FAILED
@comm
Packets could be classified into specified queue through their priority. 
we can use this function to set pkt priority with queue id mapping

*/
int rtl8306_setAsicQosPrioritytoQIDMapping(u32 priority, u32 qid) {
	u32 regValue;

	if ((qid >3) || (priority > 3)) 
		return FAILED;
	rtl8306_getAsicPhyReg(1, 22, 3, &regValue);
	switch(priority) {
	case 0:
		regValue = (regValue & 0xFFFC) | qid;	
		break;
	case 1:
		regValue = (regValue & 0xFFF3) | (qid << 2);
		break;
	case 2:
		regValue = (regValue & 0xFFCF) | (qid << 4);
		break;
	case 3:
		regValue = (regValue & 0xFF3F) | (qid << 6);
		break;
	default:
		return FAILED;
	}
	rtl8306_setAsicPhyReg(1, 22, 3, regValue);	
	return SUCCESS;
}

/*
@func int | rtl8306_getAsicQosPrioritytoQIDMapping | Get pkt priority and qid mapping
@parm u32 | priority | Packet priority
@parm u32* | qid | queue id
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_getAsicQosPrioritytoQIDMapping(u32 priority, u32 *qid) {
	u32 regValue;
	
	if ((priority > 3) || (qid == NULL))
		return FAILED;
	rtl8306_getAsicPhyReg(1, 22, 3, &regValue);
	switch(priority) {
	case 0:
		*qid = regValue & 0x3;
		break;
	case 1:
		*qid = (regValue & 0xC) >>2;
		break;
	case 2:
		*qid = (regValue & 0x30) >> 4;
		break;
	case 3:
		*qid = (regValue & 0xC0) >> 6;
		break;
	default:
		return FAILED;
	}	
	return SUCCESS;	
	
}

/*
@func int | rtl8306_setAsicQosPortBasedPriority | Set port-based priority
@parm u32 | port | Specify port number (0~5)
@parm u32 | priority | priority value (0 ~3)
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_setAsicQosPortBasedPriority(u32 port, u32 priority) {
	u32 regValue;

	if ((port > RTL8306_PORT5) ||(priority > 3))
		return FAILED;
	/*Port 5 corresponding PHY6*/	
	if (port == RTL8306_PORT5 )  
		port ++ ;  
	rtl8306_getAsicPhyReg(port, 17, 2, &regValue);
	regValue = (regValue & 0xE7FF) | (priority << 11);
	rtl8306_setAsicPhyReg(port, 17, 2, regValue);
	return SUCCESS;
}

/*
@func int | rtl8306_getAsicQosPortBasedPriority | Get port-based priority
@parm u32 | port | Specify port number (0~5)
@parm u32* | priority | Priority value
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_getAsicQosPortBasedPriority(u32 port, u32 *priority) {
	u32 regValue;

	if ((port > RTL8306_PORT5) ||(priority == NULL))
		return FAILED;
	if (port < RTL8306_PORT5) 
		rtl8306_getAsicPhyReg(port, 17, 2, &regValue);
	else
		rtl8306_getAsicPhyReg(6, 17, 2, &regValue);
	*priority = (regValue & 0x1800) >> 11;
	
	return SUCCESS;
}

/*
@func int | rtl8306_setAsicQos1QBasedPriority | Set 1Q-based default priority for port
@parm u32 | port | Specify port number (0~5)
@parm u32 | priority | Priority value (0~3)
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_setAsicQos1QBasedPriority(u32 port, u32 priority) {
	u32 regValue;

	if ((port > RTL8306_PORT5) || (priority > 3) )
		return FAILED;
	/*Port 5 corresponding PHY6*/	
	if (port == RTL8306_PORT5 )  
		port ++ ;  
	rtl8306_getAsicPhyReg(port, 17, 2, &regValue);
	regValue = (regValue & 0x9FFF) | (priority << 13);
	rtl8306_setAsicPhyReg(port, 17, 2, regValue);
	return SUCCESS;
}

/*
@func int | rtl8306_getAsicQos1QBasedPriority | Get port 1Q-based default priority
@parm u32 | port | Specify port number (0~5)
@parm u32* | priority | Priority value
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_getAsicQos1QBasedPriority(u32 port, u32 *priority) {
	u32 regValue;

	if ((port > RTL8306_PORT5) || (priority == NULL))
		return FAILED;
	if (port < RTL8306_PORT5) 
		rtl8306_getAsicPhyReg(port, 17, 2, &regValue);
	else
		rtl8306_getAsicPhyReg(6, 17, 2, &regValue);
	*priority = (regValue & 0x6000) >> 13;
	

	return SUCCESS;
}

#define RTL8306_1QTAG_PRIO0		0
#define RTL8306_1QTAG_PRIO1		1
#define RTL8306_1QTAG_PRIO2		2
#define RTL8306_1QTAG_PRIO3		3
#define RTL8306_1QTAG_PRIO4		4
#define RTL8306_1QTAG_PRIO5		5
#define RTL8306_1QTAG_PRIO6 		6
#define RTL8306_1QTAG_PRIO7		7
#define RTL8306_PRIO0		0
#define RTL8306_PRIO1		1
#define RTL8306_PRIO2		2
#define RTL8306_PRIO3		3


/*
@func int | rtl8306_setAsicQos1QtagPriorityto2bitPriority | Set Asic 1Q-tag priority mapping to 2-bit priority
@parm u32 | tagprio | 1Q-tag proirty (0~7, 3 bit value)
@parm u32 | prio | 2-bit priority
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_setAsicQos1QtagPriorityto2bitPriority(u32 tagprio, u32 prio) {	
	u32 regValue;

	if ((tagprio > RTL8306_1QTAG_PRIO7) || (prio > RTL8306_PRIO3 ))
		return FAILED;
	rtl8306_getAsicPhyReg(2, 24, 3, &regValue);
	switch(tagprio) {
	case RTL8306_1QTAG_PRIO0:	
		regValue = (regValue & 0xFFFC) | prio;
		break;
	case RTL8306_1QTAG_PRIO1:
		regValue = (regValue & 0xFFF3) |(prio << 2);
		break;
	case RTL8306_1QTAG_PRIO2:
		regValue = (regValue & 0xFFCF) | (prio << 4);
		break;
	case RTL8306_1QTAG_PRIO3:
		regValue = (regValue & 0xFF3F) | (prio << 6);
		break;
	case RTL8306_1QTAG_PRIO4:
		regValue = (regValue & 0xFCFF) | (prio << 8);
		break;
	case RTL8306_1QTAG_PRIO5:
		regValue = (regValue & 0xF3FF) | (prio << 10);
		break;
	case RTL8306_1QTAG_PRIO6:
		regValue = (regValue & 0xCFFF) | (prio << 12);
		break;
	case RTL8306_1QTAG_PRIO7:
		regValue = (regValue & 0x3FFF) | (prio << 14);
		break;
	default:
		return FAILED;
	}
	rtl8306_setAsicPhyReg(2, 24, 3, regValue);
	return SUCCESS;
}

/*
@func int | rtl8306_getAsicQos1QtagPriorityto2bitPriority | Get 1Q-tag priority to 2-bit priority 
@parm u32 | tagprio | 1Q-tag priority
@parm u32* | prio | 2-bit priority
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_getAsicQos1QtagPriorityto2bitPriority(u32 tagprio, u32 *prio) {
	u32 regValue;
	
	if ((tagprio > RTL8306_1QTAG_PRIO7) || (prio == NULL ))
		return FAILED;
	rtl8306_getAsicPhyReg(2, 24, 3, &regValue);
	switch(tagprio) {
	case RTL8306_1QTAG_PRIO0:	
		*prio = regValue & 0x3;
		break;
	case RTL8306_1QTAG_PRIO1:
		*prio = (regValue & 0xC) >> 2;
		break;
	case RTL8306_1QTAG_PRIO2:
		*prio = (regValue & 0x30) >> 4;
		break;
	case RTL8306_1QTAG_PRIO3:
		*prio = (regValue & 0xC0) >> 6;
		break;
	case RTL8306_1QTAG_PRIO4:
		*prio = (regValue & 0x300) >> 8;
		break;
	case RTL8306_1QTAG_PRIO5:
		*prio = (regValue & 0xC00) >> 10;
		break;
	case RTL8306_1QTAG_PRIO6:
		*prio = (regValue & 0x3000) >> 12;
		break;
	case RTL8306_1QTAG_PRIO7:
		*prio = (regValue & 0xC000) >> 14;
		break;
	default:
		return FAILED;
	}

	return SUCCESS;	
}

/*
@func int | rtl8306_setAsicQosDSCPBasedPriority | Set DSCP-based priority
@parm u32 | type | Specify DSCP priority type
@parm u32 | priority | priority value (0~3)
@rvalue SUCCESS 
@rvalue FAILED
@comm
There are 16 kinds of DSCP priority type<nl>
	RTL8306_DSCP_EF<nl>			
	RTL8306_DSCP_AFL1<nl>		
	RTL8306_DSCP_AFM1<nl>		
	RTL8306_DSCP_AFH1<nl>		
	RTL8306_DSCP_AFL2<nl>			
	RTL8306_DSCP_AFM2<nl>			
	RTL8306_DSCP_AFH2<nl>				
	RTL8306_DSCP_AFL3<nl>					
	RTL8306_DSCP_AFM3<nl>		
	RTL8306_DSCP_AFH3<nl>		
	RTL8306_DSCP_AFL4<nl>			
	RTL8306_DSCP_AFM4<nl>				
	RTL8306_DSCP_AFH4<nl>		
	RTL8306_DSCP_NC<nl>			
	RTL8306_DSCP_REG_PRI<nl>					
	RTL8306_DSCP_BF<nl>			
RTL8306_DSCP_REG_PRI is user setting other DSCP priority through two
registers. 				 
*/
int rtl8306_setAsicQosDSCPBasedPriority(u32 type, u32 priority) {
	u32 regValue1, regValue2;

	if ((type > RTL8306_DSCP_BF) ||(priority > RTL8306_PRIO3))
		return FAILED;
	
	rtl8306_getAsicPhyReg(1, 23, 3, &regValue1);
	rtl8306_getAsicPhyReg(1, 24, 3, &regValue2);
	switch(type) {
	case RTL8306_DSCP_EF:
		regValue1 = (regValue1 & 0xFFFC) | priority;
		rtl8306_setAsicPhyReg(1, 23, 3, regValue1);
		break;
	case RTL8306_DSCP_AFL1:
		regValue1 = (regValue1 & 0xFFF3) | (priority << 2);
		rtl8306_setAsicPhyReg(1, 23, 3, regValue1);
		break;
	case RTL8306_DSCP_AFM1:
		regValue1 = (regValue1 & 0xFFCF) | (priority << 4);
		rtl8306_setAsicPhyReg(1, 23, 3, regValue1);		
		break;
	case RTL8306_DSCP_AFH1:
		regValue1 = (regValue1 & 0xFF3F) | (priority << 6);
		rtl8306_setAsicPhyReg(1, 23, 3, regValue1);		
		break;
	case RTL8306_DSCP_AFL2:
		regValue1 = (regValue1 & 0xFCFF) | (priority << 8);
		rtl8306_setAsicPhyReg(1, 23, 3, regValue1);		
		break;
	case RTL8306_DSCP_AFM2:
		regValue1 = (regValue1 & 0xF3FF) | (priority << 10);
		rtl8306_setAsicPhyReg(1, 23, 3, regValue1);		
		break;
	case RTL8306_DSCP_AFH2:
		regValue1 = (regValue1 & 0xCFFF) |(priority << 12);
		rtl8306_setAsicPhyReg(1, 23, 3, regValue1);		
		break;
	case RTL8306_DSCP_AFL3:
		regValue1 = (regValue1 & 0x3FFF) | (priority << 14);
		rtl8306_setAsicPhyReg(1, 23, 3, regValue1);				
		break;
	case RTL8306_DSCP_AFM3:		
		regValue2 = (regValue2 & 0xFFFC) | priority;
		rtl8306_setAsicPhyReg(1, 24, 3, regValue2);		
		break;
	case RTL8306_DSCP_AFH3:
		regValue2 = (regValue2 & 0xFFF3) | (priority <<2);
		rtl8306_setAsicPhyReg(1, 24, 3, regValue2);				
		break;
	case RTL8306_DSCP_AFL4:
		regValue2 = (regValue2 & 0xFFCF) | (priority <<4);
		rtl8306_setAsicPhyReg(1, 24, 3, regValue2);						
		break;
	case RTL8306_DSCP_AFM4:
		regValue2 = (regValue2 & 0xFF3F) | (priority << 6);
		rtl8306_setAsicPhyReg(1, 24, 3, regValue2);						
		break;
	case RTL8306_DSCP_AFH4:
		regValue2 = (regValue2 & 0xFCFF) | (priority << 8);
		rtl8306_setAsicPhyReg(1, 24, 3, regValue2);						
		break;
	case RTL8306_DSCP_NC:
		regValue2 = (regValue2 & 0xF3FF) | (priority << 10);
		rtl8306_setAsicPhyReg(1, 24, 3, regValue2);						
		break;
	case RTL8306_DSCP_REG_PRI:
		regValue2 = (regValue2 & 0xCFFF) | (priority << 12);
		rtl8306_setAsicPhyReg(1, 24, 3, regValue2);						
		break;
	case RTL8306_DSCP_BF:
		regValue2 = (regValue2 & 0x3FFF) | (priority << 14);
		rtl8306_setAsicPhyReg(1, 24, 3, regValue2);						
		break;
	default:
		return FAILED;
	}
	return SUCCESS;
}

/*
@func int | rtl8306_setAsicQosTxQueueWeight | Set Tx queue weight
@parm u32 | queue | Specify queue number
@parm u32 | weight | weight value
@parm u32 | set | Specify the set
@rvalue SUCCESS 
@rvalue FAILED
@comm
There are two sets configuration RTL8306_QOS_SET0 and RTL8306_QOS_SET1
for bandwidth control.
*/
int rtl8306_setAsicQosTxQueueWeight(u32 queue, u32 weight, u32 set ) {
	u32 regValue;
	
	if ((queue > 3) || (weight > 0x7F) || (set > 1))
		return FAILED;
	switch(queue) {
	case RTL8306_QUEUE0:
		if (set == 0) { 
			rtl8306_getAsicPhyReg(5, 20, 3, &regValue);
			regValue = (regValue & 0xFF80) | weight;	
			rtl8306_setAsicPhyReg(5, 20, 3, regValue);			
		} else { 
			rtl8306_getAsicPhyReg(5, 25, 3, &regValue);
			regValue = (regValue & 0xFF80) | weight;
			rtl8306_setAsicPhyReg(5, 25, 3, regValue);						
		}	
		break;
	case RTL8306_QUEUE1:
		if (set == 0)  {
			rtl8306_getAsicPhyReg(5, 20, 3, &regValue);
			regValue = (regValue & 0x80FF) | (weight << 8);
			rtl8306_setAsicPhyReg(5, 20, 3, regValue);			
		} else {
			rtl8306_getAsicPhyReg(5, 25, 3, &regValue);
			regValue = (regValue & 0x80FF) | (weight << 8);
			rtl8306_setAsicPhyReg(5, 25, 3, regValue);			
		}
		break;
	case RTL8306_QUEUE2:
		if (set == 0) {
			rtl8306_getAsicPhyReg(5, 21, 3, &regValue);
			regValue = (regValue & 0xFF80) | weight;	
			rtl8306_setAsicPhyReg(5, 21, 3, regValue);			
		} else {
			rtl8306_getAsicPhyReg(5, 26, 3, &regValue);
			regValue = (regValue & 0xFF80) | weight;
			rtl8306_setAsicPhyReg(5, 26, 3, regValue);			
		}
		break;
	case RTL8306_QUEUE3:
		if (set == 0) {
			rtl8306_getAsicPhyReg(5, 21, 3, &regValue);
			regValue = (regValue & 0x80FF) | (weight << 8);
			rtl8306_setAsicPhyReg(5, 21, 3, regValue);			
		} else {
			rtl8306_getAsicPhyReg(5, 26, 3, &regValue);
			regValue = (regValue & 0x80FF) | (weight << 8);
			rtl8306_setAsicPhyReg(5, 26, 3, regValue);			
		}
		break;
	default:
		return FAILED;
	}

	return SUCCESS;
}

/*
@func int | rtl8306_setAsicQosPriorityEnable | Set Qos priority enable for port
@parm u32 | port | Specify port number (0 ~5)
@parm u32 | type | Specify priority type
@parm u32 | enabled | enable the priority
@rvalue SUCCESS 
@rvalue FAILED
@comm
There are 4 type priority for each port enable/disable:
	RTL8306_DSCP_PRIO  - DSCP-based priority
	RTL8306_1QBP_PRIO  - 1Q-based priority
	RTL8306_PBP_PRIO   - port-based priority
	RTL8306_CPUTAG_PRIO - cpu tag priority
*/
int rtl8306_setAsicQosPriorityEnable(u32 port, u32 type, u32 enabled) {
      u32 duplex, speed, nway;

      
	if (port > RTL8306_PORT5)
		return FAILED;
       /*save mac 4 or port status when operate reg.22*/    
       if (port == 4) {
            rtl8306_getAsicPhyRegBit(5, 0, 13, 0, &speed);
            rtl8306_getAsicPhyRegBit(5, 0, 12, 0, &nway);
            rtl8306_getAsicPhyRegBit(5, 0, 8, 0, &duplex);            
       } else if (port == 5) {
            rtl8306_getAsicPhyRegBit(6, 0, 13, 0, &speed);
            rtl8306_getAsicPhyRegBit(6, 0, 12, 0, &nway);
            rtl8306_getAsicPhyRegBit(6, 0, 8, 0, &duplex);            
       }
	/*Port 5 corresponding PHY6*/	
	if (port == RTL8306_PORT5 )  
		port ++ ;  
	switch(type) {
	case RTL8306_DSCP_PRIO:
		rtl8306_setAsicPhyRegBit(port, 22, 9, 0, enabled == FALSE ? 1:0 );
		break;
	case RTL8306_1QBP_PRIO:
		rtl8306_setAsicPhyRegBit(port, 22, 10, 0, enabled == FALSE ? 1:0 );
		break;
	case RTL8306_PBP_PRIO:
		rtl8306_setAsicPhyRegBit(port, 22, 8, 0, enabled == FALSE ? 1:0 );
		break;
	case RTL8306_CPUTAG_PRIO:
		rtl8306_setAsicPhyRegBit(port, 17, 1, 2, enabled == TRUE ? 1:0);
		break;
	default:
		return FAILED;
	}	
       /*restore mac 4 or port status when operate reg.22*/    
      if (port == 4) {
            rtl8306_setAsicPhyRegBit(5, 0, 13, 0, speed);
            rtl8306_setAsicPhyRegBit(5, 0, 12, 0, nway);
            rtl8306_setAsicPhyRegBit(5, 0, 8, 0, duplex);
            
      } else if (port == 6)  {  /*for port++ when port 5*/
           rtl8306_setAsicPhyRegBit(6, 0, 13, 0, speed);
           rtl8306_setAsicPhyRegBit(6, 0, 12, 0, nway);
           rtl8306_setAsicPhyRegBit(6, 0, 8, 0, duplex);
      }      
	return SUCCESS;
}

/*
@func int | rtl8306_setAsicQosIPAddressPriority | Set IP address priority
@parm u32 | priority | priority value (0~3)
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int rtl8306_setAsicQosIPAddressPriority(u32 priority) {
	u32 regValue;

	if (priority > 3)
		return FAILED;
	rtl8306_getAsicPhyReg(2, 22, 3, &regValue);
	rtl8306_setAsicPhyReg(2, 22, 3, (regValue & 0xFFFC) |priority);
	return SUCCESS;
}

/*
@func int | rtl8306_setAsicQosIPAddress | Set IP address
@parm u32 | entry | specify entry
@parm u32 | ip | ip address
@parm u32 | mask | ip mask
@parm u32 | enabled | enable the entry
@rvalue SUCCESS 
@rvalue FAILED
@comm
There are two entries RTL8306_IPADD_A and RTL8306_IPADD_B
for user setting ip address, if ip address of packet matches
the entry, the packet will be assign the priority of ip address
priority which is configured by rtl8306_setAsicQosIPAddressPriority
*/
int rtl8306_setAsicQosIPAddress(u32 entry, u32 ip, u32 mask, u32 enabled) {
	u32 regValue;
	
	if (entry > 1) 
		return FAILED;
	switch(entry) {
	case RTL8306_IPADD_A:
#if 0		
		regValue = ip & 0xFFFF;
		rtl8306_setAsicPhyReg(1, 17, 0, regValue);
		regValue = (ip & 0xFFFF0000) >> 16;
		rtl8306_setAsicPhyReg(1, 16, 0, regValue);
		regValue = mask & 0xFFFF;
		rtl8306_setAsicPhyReg(2, 17, 0, regValue);
		regValue = (mask & 0xFFFF0000) >> 16;
		rtl8306_setAsicPhyReg(2, 16, 0, regValue);
#endif		
		if (enabled == TRUE) {
			rtl8306_setAsicPhyRegBit(0, 17, 14, 0, 1);
			regValue = ip & 0xFFFF;
			rtl8306_setAsicPhyReg(1, 17, 0, regValue);
			regValue = (ip & 0xFFFF0000) >> 16;
			rtl8306_setAsicPhyReg(1, 16, 0, regValue);
			regValue = mask & 0xFFFF;
			rtl8306_setAsicPhyReg(2, 17, 0, regValue);
			regValue = (mask & 0xFFFF0000) >> 16;
			rtl8306_setAsicPhyReg(2, 16, 0, regValue);
		}	
		else 
			rtl8306_setAsicPhyRegBit(0, 17, 14, 0, 0);
		break;
	case RTL8306_IPADD_B:
#if 0		
		regValue = ip & 0xFFFF;
		rtl8306_setAsicPhyReg(1, 19, 0, regValue);
		regValue = (ip & 0xFFFF0000) >> 16;
		rtl8306_setAsicPhyReg(1, 18, 0, regValue);
		regValue = mask & 0xFFFF;
		rtl8306_setAsicPhyReg(2, 19, 0, regValue);
		regValue = (mask & 0xFFFF0000) >> 16;
		rtl8306_setAsicPhyReg(2, 18, 0, regValue);
#endif		
		if (enabled == TRUE) {
			rtl8306_setAsicPhyRegBit(0, 17, 6, 0, 1);
			regValue = ip & 0xFFFF;
			rtl8306_setAsicPhyReg(1, 19, 0, regValue);
			regValue = (ip & 0xFFFF0000) >> 16;
			rtl8306_setAsicPhyReg(1, 18, 0, regValue);
			regValue = mask & 0xFFFF;
			rtl8306_setAsicPhyReg(2, 19, 0, regValue);
			regValue = (mask & 0xFFFF0000) >> 16;
			rtl8306_setAsicPhyReg(2, 18, 0, regValue);
		}
		else 
			rtl8306_setAsicPhyRegBit(0, 17, 6, 0, 0);		
		break;
	default:
		return FAILED;
	}	
	return SUCCESS;
}


#define RTL8306S(x)		(((x) == 0 || (x) == 2)?TRUE:FALSE)
#define RTL8306SD(x)		(((x) == 1)?TRUE:FALSE)
#define RTL8306SDM(x)	(((x) == 3)?TRUE:FALSE)



/*
@func int | rtl8306_initQos | Init qos configuration
@parm u32 | qnum | Sepcify Tx queue number (4~1)for each port
@comm
when qnum = 1, only queue 0 is enabled;when qnum = 2, queue 0,1 are enabled;
when qnum = 3, queue 0,1,2 are enabled; when qnum =4, all four queues are enabled;
4 queues are recomended to use full qos function, While RTL8306S  could support 2 queues
at most.
*/
int rtl8306_initQos(u32 qnum) {
    u32 port;
    u32 prio;
    u32 vendorID, regVal;

        
    
   rtl8306_getVendorID(&vendorID);
   /*for RTL8306S*/
   if(RTL8306S(vendorID))  {


        switch (qnum) {
        case 1:
            /*for one queue, use default value*/
            rtl8306_getAsicPhyReg(1, 20, 0, &regVal);
            regVal = (regVal & 0xFC00) | 68;
            rtl8306_setAsicPhyReg(1, 20, 0, regVal);
            break;
        case 2:
            /*for two queue it should change shareband*/
            rtl8306_getAsicPhyReg(1, 20, 0, &regVal);
            regVal = (regVal & 0xFC00) | 100;
            rtl8306_setAsicPhyReg(1, 20, 0, regVal);
            break;
        default:
            return FAILED;
        }
        
        return SUCCESS;
   }
   
   /* for RTL8306SD/RTL8306SDM */
   
    /*Set queue number*/
   if ( rtl8306_setAsicQosPortQueueNum(qnum) == FAILED)
        return FAILED;
   
    /*set set 1 flow control is for cpu port, set 0 is for other ports, default all ports use set 0*/




   if ((qnum == 4) || (qnum == 3))  {

        /*set 0 flow control*/
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_QLEN, RTL8306_FCON, 0, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_QLEN, RTL8306_FCOFF, 0,15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_DSC, RTL8306_FCON, 0, 20, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_DSC, RTL8306_FCOFF,0 , 13, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_QLEN, RTL8306_FCON, 0, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_QLEN, RTL8306_FCOFF, 0,15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_DSC, RTL8306_FCON, 0, 20, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_DSC, RTL8306_FCOFF,0 , 13, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_QLEN, RTL8306_FCON, 0, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_QLEN, RTL8306_FCOFF,0, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_DSC, RTL8306_FCON, 0, 20, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_DSC, RTL8306_FCOFF,0 , 13, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_QLEN, RTL8306_FCON, 0, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_QLEN, RTL8306_FCOFF,0, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_DSC, RTL8306_FCON, 0, 20, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_DSC, RTL8306_FCOFF,0 , 13, TRUE);	

        /*set 1 flow control*/
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_QLEN, RTL8306_FCON, 1, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_QLEN, RTL8306_FCOFF,1, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_DSC, RTL8306_FCON, 1, 40, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_DSC, RTL8306_FCOFF,1 , 13, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_QLEN, RTL8306_FCON, 1, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_QLEN, RTL8306_FCOFF, 1, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_DSC, RTL8306_FCON, 1, 40, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_DSC, RTL8306_FCOFF, 1 , 13, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_QLEN, RTL8306_FCON, 1, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_QLEN, RTL8306_FCOFF,1, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_DSC, RTL8306_FCON, 1, 63, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_DSC, RTL8306_FCOFF,1 , 13, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_QLEN, RTL8306_FCON, 1, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_QLEN, RTL8306_FCOFF,1, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_DSC, RTL8306_FCON, 1, 63, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_DSC, RTL8306_FCOFF,1 , 13, TRUE);	

        

    }

    if ( qnum == 2) {

        /*set 0 flow control*/
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_QLEN, RTL8306_FCON, 0, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_QLEN, RTL8306_FCOFF,0, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_DSC, RTL8306_FCON, 0,  26, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_DSC, RTL8306_FCOFF,0 , 13, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_QLEN, RTL8306_FCON, 0, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_QLEN, RTL8306_FCOFF, 0, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_DSC, RTL8306_FCON, 0, 26, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_DSC, RTL8306_FCOFF,0 , 13, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_QLEN, RTL8306_FCON, 0, 7, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_QLEN, RTL8306_FCOFF,0, 2, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_DSC, RTL8306_FCON, 0, 13, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_DSC, RTL8306_FCOFF,0 , 6, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_QLEN, RTL8306_FCON, 0, 7, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_QLEN, RTL8306_FCOFF,0, 2, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_DSC, RTL8306_FCON, 0, 13, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_DSC, RTL8306_FCOFF,0 , 6, TRUE);	

        /*set 1 flow control*/
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_QLEN, RTL8306_FCON, 1, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_QLEN, RTL8306_FCOFF,1, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_DSC, RTL8306_FCON, 1, 26, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_DSC, RTL8306_FCOFF,1 , 13, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_QLEN, RTL8306_FCON, 1, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_QLEN, RTL8306_FCOFF, 1, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_DSC, RTL8306_FCON, 1, 26, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_DSC, RTL8306_FCOFF, 1 , 13, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_QLEN, RTL8306_FCON, 1, 7, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_QLEN, RTL8306_FCOFF,1, 2, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_DSC, RTL8306_FCON, 1, 13, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_DSC, RTL8306_FCOFF,1 , 6, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_QLEN, RTL8306_FCON, 1, 7, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_QLEN, RTL8306_FCOFF,1, 2, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_DSC, RTL8306_FCON, 1,  13, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_DSC, RTL8306_FCOFF,1 , 6, TRUE);	


    }   
    if (qnum == 1 ) {

        /*set 0 flow control*/
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_QLEN, RTL8306_FCON, 0, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_QLEN, RTL8306_FCOFF,0, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_DSC, RTL8306_FCON, 0,  26, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_DSC, RTL8306_FCOFF,0 , 13, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_QLEN, RTL8306_FCON, 0, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_QLEN, RTL8306_FCOFF, 0, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_DSC, RTL8306_FCON, 0, 13, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_DSC, RTL8306_FCOFF,0 , 6, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_QLEN, RTL8306_FCON, 0, 7, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_QLEN, RTL8306_FCOFF,0, 2, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_DSC, RTL8306_FCON, 0, 13, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_DSC, RTL8306_FCOFF,0 , 6, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_QLEN, RTL8306_FCON, 0, 7, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_QLEN, RTL8306_FCOFF,0, 2, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_DSC, RTL8306_FCON, 0, 13, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_DSC, RTL8306_FCOFF,0 , 6, TRUE);	

        /*set 1 flow control*/
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_QLEN, RTL8306_FCON, 1, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_QLEN, RTL8306_FCOFF,1, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_DSC, RTL8306_FCON, 1, 26, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(0, RTL8306_FCO_DSC, RTL8306_FCOFF,1 , 13, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_QLEN, RTL8306_FCON, 1, 24, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_QLEN, RTL8306_FCOFF, 1, 15, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_DSC, RTL8306_FCON, 1, 13, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(1, RTL8306_FCO_DSC, RTL8306_FCOFF, 1 , 6, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_QLEN, RTL8306_FCON, 1, 7, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_QLEN, RTL8306_FCOFF,1, 2, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_DSC, RTL8306_FCON, 1, 13, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(2, RTL8306_FCO_DSC, RTL8306_FCOFF,1 , 6, TRUE);	

        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_QLEN, RTL8306_FCON, 1, 7, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_QLEN, RTL8306_FCOFF,1, 2, TRUE);		
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_DSC, RTL8306_FCON, 1,  13, TRUE);
        rtl8306_setAsicQosQueueFlowControlThr(3, RTL8306_FCO_DSC, RTL8306_FCOFF,1 , 6, TRUE);	


    }       
    for (port = 0; port < 6; port ++) {
            rtl8306_setAsicQosPortFlowControlMode(port, 0);
            rtl8306_setAsicQosPortFlowControlThr(port, 26, 13, RTL8306_PORT_TX);                            
    }

    /*set schedule parameter for qnum = 1,2 , set 1 for cpu port, set 0 for other ports*/
    if ((qnum == 4) ||(qnum == 3) ) {
        if (rtl8306_setAsicQosTxQueueStrictPriority(RTL8306_QUEUE3, 1, FALSE) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueStrictPriority(RTL8306_QUEUE2, 1, FALSE) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE3, 16, 1) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE2, 4, 1) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE1, 2, 1) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE0, 1, 1) == FAILED)
            return FAILED; 
        if ( rtl8306_setAsicQosTxQueueLeakyBucket(RTL8306_QUEUE3, 1, 48,  1526) == FAILED)
            return FAILED;        
        if ( rtl8306_setAsicQosTxQueueLeakyBucket(RTL8306_QUEUE2, 1, 48,  1526) == FAILED)
            return FAILED;
        
        if (rtl8306_setAsicQosTxQueueStrictPriority(RTL8306_QUEUE3, 0, FALSE) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueStrictPriority(RTL8306_QUEUE2, 0, FALSE) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE3, 1, 0) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE2, 1, 0) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE1, 1, 0) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE0, 1, 0) == FAILED)
            return FAILED;        
        if ( rtl8306_setAsicQosTxQueueLeakyBucket(RTL8306_QUEUE3, 0, 48,  1526) == FAILED)
            return FAILED;        
        if ( rtl8306_setAsicQosTxQueueLeakyBucket(RTL8306_QUEUE2, 0, 48,  1526) == FAILED)
            return FAILED;
    }        
        
    /*set schedule parameter for qnum = 1,2 ,set 1 for cpu port, set 0 for other ports*/
    if ((qnum == 1) ||(qnum == 2) ) {
        if (rtl8306_setAsicQosTxQueueStrictPriority(RTL8306_QUEUE3, 1, FALSE) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueStrictPriority(RTL8306_QUEUE2, 1, FALSE) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE3, 1, 1) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE2, 1, 1) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE1, 16, 1) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE0, 1, 1) == FAILED)
            return FAILED; 
        if ( rtl8306_setAsicQosTxQueueLeakyBucket(RTL8306_QUEUE3, 1, 48,  1526) == FAILED)
            return FAILED;        
        if ( rtl8306_setAsicQosTxQueueLeakyBucket(RTL8306_QUEUE2, 1, 48,  1526) == FAILED)
            return FAILED;
        
        if (rtl8306_setAsicQosTxQueueStrictPriority(RTL8306_QUEUE3, 0, FALSE) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueStrictPriority(RTL8306_QUEUE2, 0, FALSE) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE3, 1, 0) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE2, 1, 0) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE1, 1, 0) == FAILED)
            return FAILED;
        if (rtl8306_setAsicQosTxQueueWeight(RTL8306_QUEUE0, 1, 0) == FAILED)
            return FAILED;        
        if ( rtl8306_setAsicQosTxQueueLeakyBucket(RTL8306_QUEUE3, 0, 48,  1526) == FAILED)
            return FAILED;        
        if ( rtl8306_setAsicQosTxQueueLeakyBucket(RTL8306_QUEUE2, 0, 48,  1526) == FAILED)
            return FAILED;
    }        




    /*default : all ports select schedule set 0*/
      for (port = 0; port < 6; port ++) {
            rtl8306_setAsicQosPortScheduleMode(port, 0, 0);
      }
    /*set default QID mapping */
    switch (qnum) {
    case 1:
        for (prio = 0; prio < 4; prio ++) {
            rtl8306_setAsicQosPrioritytoQIDMapping(prio, 0);
        }
        break;
     case 2:
            rtl8306_setAsicQosPrioritytoQIDMapping(0, 0);
            rtl8306_setAsicQosPrioritytoQIDMapping(1, 0);
            rtl8306_setAsicQosPrioritytoQIDMapping(2, 1);
            rtl8306_setAsicQosPrioritytoQIDMapping(3, 1);
        break;
     case 3:
            rtl8306_setAsicQosPrioritytoQIDMapping(0, 1);
            rtl8306_setAsicQosPrioritytoQIDMapping(1, 0);
            rtl8306_setAsicQosPrioritytoQIDMapping(2, 1);
            rtl8306_setAsicQosPrioritytoQIDMapping(3, 2);            
        break;
     case 4:
            rtl8306_setAsicQosPrioritytoQIDMapping(0, 1);
            rtl8306_setAsicQosPrioritytoQIDMapping(1, 0);
            rtl8306_setAsicQosPrioritytoQIDMapping(2, 2);
            rtl8306_setAsicQosPrioritytoQIDMapping(3, 3);            
        break;
      default:
        return FAILED;                 
    }

    /*default disable all priority source*/
    for (port = 0; port < 6; port ++ ) {
        rtl8306_setAsicQosPriorityEnable(port, RTL8306_DSCP_PRIO, FALSE);           
        rtl8306_setAsicQosPriorityEnable(port, RTL8306_1QBP_PRIO, FALSE); 
        rtl8306_setAsicQosPriorityEnable(port, RTL8306_PBP_PRIO, FALSE);
        rtl8306_setAsicQosPriorityEnable(port, RTL8306_CPUTAG_PRIO, FALSE);
    }
    rtl8306_setAsicQosIPAddress(RTL8306_IPADD_A, 0, 0, FALSE);
    rtl8306_setAsicQosIPAddress(RTL8306_IPADD_B, 0, 0, FALSE);
    rtl8306_setAsicQosIPAddressPriority(RTL8306_PRIO3);
    /*set default priority arbitration*/
    rtl8306_setAsicQosPktPriorityAssign(RTL8306_ACL_PRIO, 1);
    rtl8306_setAsicQosPktPriorityAssign(RTL8306_DSCP_PRIO, 1);
    rtl8306_setAsicQosPktPriorityAssign(RTL8306_1QBP_PRIO, 1); 
    rtl8306_setAsicQosPktPriorityAssign(RTL8306_PBP_PRIO, 1);  
    /*disable all bandwidth control*/
    for (port = 0; port < 6; port ++) {
        rtl8306_setAsicQosPortRate(port, 0x5F6, RTL8306_PORT_RX, TRUE);
        rtl8306_setAsicQosPortRate(port, 0x5F6, RTL8306_PORT_TX, TRUE);        
        rtl8306_setAsicQosPortRate(port, 0x5F6, RTL8306_PORT_RX, FALSE);
        rtl8306_setAsicQosPortRate(port, 0x5F6, RTL8306_PORT_TX, FALSE);
    }
            
    return SUCCESS;    
}




//Tim Wang, init RTL8201CP for WET610N/WAP610N and RTL8306SG for WES610N
#define RTL8201CP_PHY_ADDR		1
void force_port0_100Mbps(void) {
	u32 mac_port_config;
	int i;		

	mac_port_config = GSW_MAC_PORT_0_CONFIG;

	// disable PHY's AN
	mac_port_config &= ~(0x1 << 7);

	// force speed = 100Mbps
	mac_port_config &= ~(0x3 << 8);
	mac_port_config |= (0x1 << 8);

	// force full-duplex
	mac_port_config |= (0x1 << 10);
	GSW_MAC_PORT_0_CONFIG = mac_port_config;

#if 0
	// force Tx/Rx flow-control on
	mac_port_config |= (0x1 << 11) | (0x1 << 12);

	GSW_MAC_PORT_0_CONFIG = mac_port_config;
	udelay(1000);

	for (i = 0; i < 50000; i++) {
		mac_port_config = GSW_MAC_PORT_0_CONFIG;
		if ((mac_port_config & 0x1) && !(mac_port_config & 0x2)) {
			break;
		} else {
			udelay(100);
		}
	}

	if (!(mac_port_config & 0x1) || (mac_port_config & 0x2)) {
		PRINT_INFO("MAC0 PHY Link Status : DOWN!, fail to force init PHY\n");
		return -1;
	} else {
		PRINT_INFO("MAC0 PHY Link Status : UP!\n");
	}

	// enable MAC port 0
	mac_port_config &= ~(0x1 << 18);

	// disable SA learning
	mac_port_config |= (0x1 << 19);

	// forward unknown, multicast and broadcast packets to CPU
	mac_port_config &= ~((0x1 << 25) | (0x1 << 26) | (0x1 << 27));

	// storm rate control for unknown, multicast and broadcast packets
	mac_port_config |= (0x1 << 29) | (0x1 << 30) | ((u32)0x1 << 31);

	GSW_MAC_PORT_0_CONFIG = mac_port_config;

	// disable MAC port 1
	mac_port_config = GSW_MAC_PORT_1_CONFIG;
	mac_port_config |= (0x1 << 18);
	GSW_MAC_PORT_1_CONFIG = mac_port_config;
#endif	
}

#define LNK_STATUS_INTERVAL 1
#define LINK_STATUS_BIT_MASK 0x0004
#define PORT0_LINK_LED_GPIO 7
#define PORT1_LINK_LED_GPIO 6
#define PORT2_LINK_LED_GPIO 5
#define PORT3_LINK_LED_GPIO 4
#define STATUS_REGISTER 1

void checking_link_status(void)
{
	static last_status[4]={GPIO_OFF, GPIO_OFF, GPIO_OFF, GPIO_OFF};
	u16 phy_data;
	int port_cnt;

      if (timer_pending(&link_led_timer))
     {
		/* Stop any pending timer - it might be a previous wait-for-cancel timer */ 
    	del_timer(&link_led_timer); 
       }

	for (port_cnt=0;port_cnt<4;port_cnt++) {
		star_gsw_read_phy(port_cnt, STATUS_REGISTER, &phy_data);

		if (phy_data & LINK_STATUS_BIT_MASK) {
			if (last_status[port_cnt] != GPIO_ON) { //minimize GPIO output to reduce process time
				str9100_gpio_out_bit(GPIO_ON, PORT0_LINK_LED_GPIO - port_cnt);	
				last_status[port_cnt] = GPIO_ON;
			}
		} else {
			if (last_status[port_cnt] != GPIO_OFF) {
				str9100_gpio_out_bit(GPIO_OFF, PORT0_LINK_LED_GPIO - port_cnt);	
				last_status[port_cnt] = GPIO_OFF;
			}
		}
	}
	
	init_timer(&link_led_timer);
	link_led_timer.expires = SECS_TO_JIFFIES(LNK_STATUS_INTERVAL)+jiffies; // 150 msec total.
	link_led_timer.function = checking_link_status;
 	add_timer(&link_led_timer);

}

void start_link_led(void)
{
	str9100_gpio_write_direction_bit(PIN_OUTPUT, PORT0_LINK_LED_GPIO);
	str9100_gpio_write_direction_bit(PIN_OUTPUT, PORT1_LINK_LED_GPIO);
	str9100_gpio_write_direction_bit(PIN_OUTPUT, PORT2_LINK_LED_GPIO);
	str9100_gpio_write_direction_bit(PIN_OUTPUT, PORT3_LINK_LED_GPIO);
	str9100_gpio_out_bit(GPIO_OFF, PORT0_LINK_LED_GPIO);	
	str9100_gpio_out_bit(GPIO_OFF, PORT1_LINK_LED_GPIO);	
	str9100_gpio_out_bit(GPIO_OFF, PORT2_LINK_LED_GPIO);	
	str9100_gpio_out_bit(GPIO_OFF, PORT3_LINK_LED_GPIO);	
      if (timer_pending(&link_led_timer))
     {
		/* Stop any pending timer - it might be a previous wait-for-cancel timer */ 
    	del_timer(&link_led_timer); 
       }
	init_timer(&link_led_timer);
	link_led_timer.expires = SECS_TO_JIFFIES(LNK_STATUS_INTERVAL)+jiffies; // 150 msec total.
	link_led_timer.function = checking_link_status;
  	add_timer(&link_led_timer);
	
}

void setMirror_port(void) {
	rtl8306_mirrorPara_t mir;
	/*Set port 4 as mirror port, mirror Rx/Tx of port 0 , port 1, port 2 and port 3*/
	mir.mirport = 4;
	mir.rxport = 0xf;
	mir.txport = 0xf;
	mir.enMirMac = FALSE;
	rtl8306_setMirror(mir);

	//test
#if 0	
		PRINT_INFO("rtl8306_setAsicFCThrParameter(enabled)\n");
           rtl8306_setAsicPhyReg(0, 16, 0, 0x0BFA);     //disable lock register
           rtl8306_setAsicPhyReg(1, 20, 0, 0x00BC); 
           rtl8306_setAsicPhyReg(5, 18, 2, 0x073F);
           rtl8306_setAsicPhyReg(5, 17, 2, 0x2D2F);
           
           rtl8306_setAsicPhyReg(0, 20, 2, 0x3F7F);
           rtl8306_setAsicPhyReg(1, 20, 2, 0x3F7F);
           rtl8306_setAsicPhyReg(2, 20, 2, 0x3F7F);
           rtl8306_setAsicPhyReg(3, 20, 2, 0x3F7F);

           rtl8306_setAsicPhyReg(0, 22, 3, 0xFE0A);          
           rtl8306_setAsicPhyReg(0, 23, 3, 0xFE25);  
           
           rtl8306_setAsicPhyReg(0, 19, 3, 0x5025);
           rtl8306_setAsicPhyReg(1, 19, 3, 0x5025);
           rtl8306_setAsicPhyReg(2, 19, 3, 0x5025);
           rtl8306_setAsicPhyReg(3, 19, 3, 0x5025);

#else
             
        /*disable all lan port FC capability if have one lan Nic have FC , another Nic not have FC  */
        /* jater test lan port only port1-port3 ,depent on your enviroment */   
#if 1
           rtl8306_setAsicPhyRegBit(0, 4, 10, 0, 0);   //disable FC for port 0
           rtl8306_setAsicPhyRegBit(1, 4, 10, 0, 0);   //disable FC for port 1
           rtl8306_setAsicPhyRegBit(2, 4, 10, 0, 0);   //disable FC for port 2
           rtl8306_setAsicPhyRegBit(3, 4, 10, 0, 0);   //disable FC for port 3
#endif		   
           rtl8306_setAsicPhyRegBit(4, 4, 10, 0, 0);   //disable FC for port 4
#if 1           
           rtl8306_setAsicPhyRegBit(0, 0, 9, 0, 1) ;   //restart nway for port 0
           rtl8306_setAsicPhyRegBit(1, 0, 9, 0, 1) ;   //restart nway for port 1
  	    rtl8306_setAsicPhyRegBit(2, 0, 9, 0, 1) ;   //restart nway for port 2 
  	    rtl8306_setAsicPhyRegBit(3, 0, 9, 0, 1) ;   //restart nway for port 3
#endif		
  	    rtl8306_setAsicPhyRegBit(4, 0, 9, 0, 1) ;   //restart nway for port 4
           
#endif
	
}


#define RTL8306_INPUTDROP			0   /*Input Drop for all types of packet*/
#define RTL8306_OUTPUTDROP		1   /*Output Drop for unicast packet, but not include unknown DA unicast packet*/	
#define RTL8306_BRO_INPUTDROP		2   /*Broadcast packet is input drop, but unicast packet is also output drop*/		
#define RTL8306_BRO_OUTDROP		3   /*Broadcast packet is output drop, and unicastl packet is also output drop*/	
#define RTL8306_MUL_INPUTDROP		4   /*Multicast  packet is input drop, but unicast packet is also output drop*/		  
#define RTL8306_MUL_OUTDROP		5   /*Multicast packet is output drop, and unicast packet is also output drop*/	
#define RTL8306_UDA_INPUTDROP		6   /*Unkown DA  unicast packet is input drop, but unicast packet is also output drop*/		  
#define RTL8306_UDA_OUTPUTDROP	7   /*Unkown DA  uinicast packet is output drop, and unicastl packet is also output drop*/
#define RTL8306_UDA_DISABLEDROP 	8  /* Disable Unkown DA unicast packet Drop*/

#define RTL8306_UNICASTPKT		0   /*Unicast packet, but not include unknown DA unicast packet*/
#define RTL8306_BROADCASTPKT      1   /*Broadcast packet*/
#define RTL8306_MULTICASTPKT		2   /*Multicast packet*/
#define RTL8306_UDAPKT			3   /*Unknown DA unicast packet*/


/*
@func int | rtl8306_setAsicInputOutputDrop | Set asic input/output drop
@parm u32 | type | input/output drop type
@rvalue SUCCESS 
@rvalue FAILED
@comm
There are 9 input/output drop types:<nl>
	RTL8306_INPUTDROP	- input Drop for all types of packet<nl>
	RTL8306_OUTPUTDROP - output Drop for unicast packet, but not include unknown DA unicast packet<nl>	
	RTL8306_BRO_INPUTDROP - broadcast packet is input drop, but unicast packet is also output drop<nl>		
	RTL8306_BRO_OUTDROP - broadcast packet is output drop, and unicastl packet is also output drop<nl>
	RTL8306_MUL_INPUTDROP - multicast  packet is input drop, but unicast packet is also output drop<nl>
	RTL8306_MUL_OUTDROP - multicast packet is output drop, and unicast packet is also output drop<nl>	
	RTL8306_UDA_INPUTDROP - unkown DA  unicast packet is input drop, but unicast packet is also output drop<nl>		  
	RTL8306_UDA_OUTPUTDROP - unkown DA  uinicast packet is output drop, and unicastl packet is also output drop<nl>
	RTL8306_UDA_DISABLEDROP - disable Unkown DA unicast packet Drop<nl>

*/
int rtl8306_setAsicInputOutputDrop(u32 type) {

	switch(type) {
	case RTL8306_INPUTDROP:
		/*Enable EnBrdDrp*/		
		rtl8306_setAsicPhyRegBit(0, 18, 13, 0, 1); 
		break;
	case RTL8306_OUTPUTDROP:
		/*Disable EnBrdDrp*/
		rtl8306_setAsicPhyRegBit(0, 18, 13, 0, 0); 		
		break;
	case RTL8306_BRO_INPUTDROP:
		/*Disable EnBrdDrp*/				
		rtl8306_setAsicPhyRegBit(0, 18, 13, 0, 0);
		/*Select Broadcast packet input drop*/
		rtl8306_setAsicPhyRegBit(2, 23, 13, 3, 1);				
		break;
	case RTL8306_BRO_OUTDROP:
		/*Disable EnBrdDrp*/						
		rtl8306_setAsicPhyRegBit(0, 18, 13, 0, 0);
		/*Select Broadcast packet output drop*/		
		rtl8306_setAsicPhyRegBit(2, 23, 13, 3, 0);						
		break;
	case RTL8306_MUL_INPUTDROP:
		/*Disable EnBrdDrp*/								
		rtl8306_setAsicPhyRegBit(0, 18, 13, 0, 0);
		/*Select Multcast packet input drop*/		
		rtl8306_setAsicPhyRegBit(2, 23, 12, 3, 1);						
		break;
	case RTL8306_MUL_OUTDROP:
		/*Disable EnBrdDrp*/										
		rtl8306_setAsicPhyRegBit(0, 18, 13, 0, 0);
		/*Select Multcast packet input drop*/				
		rtl8306_setAsicPhyRegBit(2, 23, 12, 3, 0);								
		break;
	case RTL8306_UDA_INPUTDROP:
		/*Disable EnBrdDrp*/												
		rtl8306_setAsicPhyRegBit(0, 18, 13, 0, 0);
		/*Enable drop unknown DA unicast*/		
		rtl8306_setAsicPhyRegBit(2, 23, 15, 3, 0);  
		/*Select unkown DA unicast input drop*/
		rtl8306_setAsicPhyRegBit(2, 23, 11, 3, 1);	
		break;
	case RTL8306_UDA_OUTPUTDROP:
		/*Disable EnBrdDrp*/														
		rtl8306_setAsicPhyRegBit(0, 18, 13, 0, 0);
		/*Enable drop unknown DA unicast*/				
		rtl8306_setAsicPhyRegBit(2, 23, 15, 3, 0); 
		/*Select unkown DA unicast output drop*/		
		rtl8306_setAsicPhyRegBit(2, 23, 11, 3, 0);	
		break;
	case RTL8306_UDA_DISABLEDROP:
		/*Disable drop unknown DA unicast*/		
		rtl8306_setAsicPhyRegBit(2, 23, 15, 3, 1); 
		break;
	default:
		return FAILED;		
	}			
	return SUCCESS;
}




/*
@func int | rtl8306_setAsicPortLearningAbility | Enable /Disable physical port learning ability
@parm u32 | port | Specify port number ( 0 ~ 5)
@parm u32 | enabled | TRUE or FALSE
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/

int rtl8306_setAsicPortLearningAbility(u32 port, u32 enabled) {


       if (port > RTL8306_PORT5)
            return FAILED;
       if (port == RTL8306_PORT5 )
            port++;

       rtl8306_setAsicPhyRegBit(port, 24, 9, 0, enabled == TRUE ? 1:0);

       return SUCCESS;
}

/*
@func int | rtl8306_setAsicCPUPort | Specify Asic CPU port.
@parm u32 | port | Specify the port.
@parm u32 | enTag | CPU tag insert or not.
@rvalue SUCCESS 
@rvalue FAILED
@comm
If the port is specified RTL8306_NOCPUPORT, it means that no port is assigned as cpu port
*/

int rtl8306_setAsicCPUPort(u32 port, u32 enTag) {
	u32 regValue;
	
	if (port > RTL8306_NOCPUPORT)
		return FAILED;

      if (port < RTL8306_PORT_NUMBER) {
            /*Enable CPU port Function */
        	rtl8306_setAsicPhyRegBit(2, 21, 15, 3, 0);
        	/*Whether enable inserting CPU tag*/
        	rtl8306_setAsicPhyRegBit(2, 21, 12, 3, enTag == TRUE ? 1 : 0);
        	/*Enable the ability to check cpu tag*/
        	//rtl8306_setAsicPhyRegBit(4, 21, 7, 0, enTag == TRUE ? 1 : 0);
        	rtl8306_setAsicPhyRegBit(4, 21, 7, 0, 1);
        	/*Enable removing CPU tag*/
        	rtl8306_setAsicPhyRegBit(2, 21, 11, 3, 1);
        	rtl8306_getAsicPhyReg(4, 21, 0, &regValue);
        	regValue = (regValue & 0xFFF8) | port;
        	rtl8306_setAsicPhyReg(4, 21, 0, regValue);
        	/*Disable IEEE802.1x function of CPU Port*/	
        	if (port < RTL8306_PORT5) {
	        	rtl8306_setAsicPhyRegBit(port, 17, 9, 2, 0);
        		rtl8306_setAsicPhyRegBit(port, 17, 8, 2, 0);
        	} else {
	        	rtl8306_setAsicPhyRegBit(6, 17, 9, 2, 0);
        		rtl8306_setAsicPhyRegBit(6, 17, 8, 2, 0);
        	}
        	/*Port 5 should be enabled especially*/
        	if (port == RTL8306_PORT5)
	        	rtl8306_setAsicPhyRegBit(6, 22, 15, 0, TRUE);
        }
        else {
            /*Disable CPU port Function */
        	rtl8306_setAsicPhyRegBit(2, 21, 15, 3, 1);	
        	rtl8306_getAsicPhyReg(4, 21, 0, &regValue);
        	regValue = (regValue & 0xFFF8) | port;
        	rtl8306_setAsicPhyReg(4, 21, 0, regValue);
        }   
		
	return SUCCESS;	
}


#define RTL8306_PORT_RX  0
#define RTL8306_PORT_TX  1

#define RTL8306_ETHER_AUTO_100FULL	0x01
#define RTL8306_ETHER_AUTO_100HALF	0x02
#define RTL8306_ETHER_AUTO_10FULL	0x03
#define RTL8306_ETHER_AUTO_10HALF	0x04
#define RTL8306_IDLE_TIMEOUT			10
#define RTL8306_ETHER_SPEED_100 100
#define RTL8306_ETHER_SPEED_10 10

/* PHY control register field definitions 
*/
#define RTL8306_PHY_RESET					(1 << 15)
#define RTL8306_PHY_ENABLE_LOOPBACK		(1 << 14)
#define RTL8306_SPEED_SELECT_100M			(1 << 13)
#define RTL8306_SPEED_SELECT_10M                   0
#define RTL8306_ENABLE_AUTONEGO			(1 << 12)
#define RTL8306_POWER_DOWN					(1 << 11)
#define RTL8306_ISOLATE_PHY					(1 << 10)
#define RTL8306_RESTART_AUTONEGO			(1 << 9)
#define RTL8306_SELECT_FULL_DUPLEX			(1 << 8)
#define RTL8306_SELECT_HALF_DUPLEX			0


/*
@func int | rtl8306_setAsicEthernetPHY | Configure PHY setting.
@parm u32 | phy | Specify the phy to configure.
@parm u32 | autoNegotiation | Specify whether enable auto-negotiation.
@parm u32 | advCapability | When auto-negotiation is enabled, specify the advertised capability.
@parm u32 | speed | When auto-negotiation is disabled, specify the force mode speed.
@parm u32 | fullDuplex | When auto-negotiatoin is disabled, specify the force mode duplex mode.
@rvalue SUCCESS 
@rvalue FAILED
@comm
When auto-negotiation is enabled, the advertisement capability is used to handshaking with link partner.
Wehn auto-negotiation is disabled, the phy is configured into force mode and the speed and duplex mode setting is based on speed and fullDuplex setting.
Port number should be smaller than RTL8306_PHY_NUMBER.
AdverCapability should be ranged between RTL8306_ETHER_AUTO_100FULL and RTL8306_ETHER_AUTO_10HALF.
Speed should be either RTL8306_ETHER_SPEED_100 or RTL8306_ETHER_SPEED_10.

*/
int rtl8306_setAsicEthernetPHY(u32 phy, u32 autoNegotiation, u32 advCapability, u32 speed, u32 fullDuplex) {
	u32 ctrlReg;

	if(phy >= RTL8306_PHY_NUMBER || 
		advCapability < RTL8306_ETHER_AUTO_100FULL 
		|| advCapability > RTL8306_ETHER_AUTO_10HALF 
		||(speed != 100 && speed != 10))
		return FAILED;

	if(advCapability == RTL8306_ETHER_AUTO_100FULL)		
		rtl8306_setAsicPhyReg(phy, 4, 0, RTL8306_CAPABLE_PAUSE | RTL8306_CAPABLE_100BASE_TX_FD 
								| RTL8306_CAPABLE_100BASE_TX_HD | RTL8306_CAPABLE_10BASE_TX_FD 
								| RTL8306_CAPABLE_10BASE_TX_HD | 0x1);
	else if(advCapability == RTL8306_ETHER_AUTO_100HALF)
		rtl8306_setAsicPhyReg(phy, 4, 0, RTL8306_CAPABLE_PAUSE | RTL8306_CAPABLE_100BASE_TX_HD
								| RTL8306_CAPABLE_10BASE_TX_FD | RTL8306_CAPABLE_10BASE_TX_HD | 0x1);
	else	if(advCapability == RTL8306_ETHER_AUTO_10FULL)
		rtl8306_setAsicPhyReg(phy, 4, 0, RTL8306_CAPABLE_PAUSE | RTL8306_CAPABLE_10BASE_TX_FD 
								| RTL8306_CAPABLE_10BASE_TX_HD | 0x1);
	else if(advCapability == RTL8306_ETHER_AUTO_10HALF)
		rtl8306_setAsicPhyReg(phy, 4, 0, RTL8306_CAPABLE_PAUSE | RTL8306_CAPABLE_10BASE_TX_HD | 0x1);
	
	/* Each time the link ability of the RTL8306 is reconfigured, 
		the auto-negotiation process should be executed to allow the configuration to take effect. */
	if(autoNegotiation == TRUE) 
		ctrlReg = RTL8306_ENABLE_AUTONEGO | RTL8306_RESTART_AUTONEGO; 
	else
		ctrlReg = 0;
	if(speed == 100) // 100Mbps, default assume 10Mbps
		ctrlReg |= RTL8306_SPEED_SELECT_100M;
	if(fullDuplex == TRUE)
		ctrlReg |= RTL8306_SELECT_FULL_DUPLEX;
	rtl8306_setAsicPhyReg(phy, 0, RTL8306_REGPAGE0, ctrlReg);
	
	return SUCCESS;
}



/*
@func int | rtl8306_setEthernetPHY | Configure PHY setting.
@parm u32 | phy | Specify the phy to configure (0~6).
@parm u32 | autoNegotiation | Specify whether enable auto-negotiation.
@parm u32 | advCapability | When auto-negotiation is enabled, specify the advertised capability.
@parm u32 | speed | When auto-negotiation is disabled, specify the force mode speed.
@parm u32 | fullDuplex | When auto-negotiatoin is disabled, specify the force mode duplex mode.
@rvalue SUCCESS 
@rvalue FAILED
@comm
phy 0 ~4 correspond port 0~ 4, phy 5 correspond port 4 mac, and phy 6 correspond port5, port 4 mac and port 5 is mii interface
When auto-negotiation is enabled, the advertisement capability is used to handshaking with link partner.
Wehn auto-negotiation is disabled, the phy is configured into force mode and the speed and duplex mode setting is based on speed and fullDuplex setting.
AdverCapability should be ranged between RTL8306_ETHER_AUTO_100FULL and RTL8306_ETHER_AUTO_10HALF.
Speed should be either RTL8306_ETHER_SPEED_100 or RTL8306_ETHER_SPEED_10.

*/

int rtl8306_setEthernetPHY(u32 phy, u32 autoNegotiation, u32 advCapability, u32 speed, u32 fullDuplex) {

    /*phy 5, 6 are mii interface*/
    if ((phy == 5) ||(phy == 6))
        autoNegotiation = FALSE;
    if (rtl8306_setAsicEthernetPHY(phy, autoNegotiation, advCapability, speed, fullDuplex) == FAILED)
        return FAILED;
    return SUCCESS;
}

/*
@func int | rtl8306_getAsicEthernetPHY | Get PHY setting.
@parm u32 | phy | Specify the phy to get setting
@parm u32 * | autoNegotiation | Specify whether auto-negotiation is enabled.
@parm u32 * | advCapability | When auto-negotiation is enabled, get the advertised capability.
@parm u32 * | speed | When auto-negotiation is disabled, get the force mode speed.
@parm u32 * | fullDuplex | When auto-negotiatoin is disabled, get the force mode duplex mode.
@rvalue SUCCESS 
@rvalue FAILED
@comm
Either auto-negotiation advertised capability or force mode speed/duplex setting is valid.
If auto-negotiation is enabled, the advertisement capability is valid.
If auto-negotiation is diabled, the speed and duplex mode setting are valid.
If any of the pointer is NULL, the function will return FAILED.
*/
int rtl8306_getAsicEthernetPHY(u32 phy, u32 *autoNegotiation, u32 *advCapability, u32 *speed, u32 *fullDuplex) {
	u32 regData;
	
	if((phy >= RTL8306_PHY_NUMBER) || (autoNegotiation == NULL) || (advCapability == NULL)
		|| (speed == NULL) || (fullDuplex == NULL))
		return FAILED;
		
	rtl8306_getAsicPhyReg(phy, 0, RTL8306_REGPAGE0, &regData);
	*autoNegotiation = (regData & RTL8306_ENABLE_AUTONEGO)? TRUE: FALSE;
	*speed = (regData & RTL8306_SPEED_SELECT_100M)? 100: 10;
	*fullDuplex = (regData & RTL8306_SELECT_FULL_DUPLEX)? TRUE: FALSE;

	rtl8306_getAsicPhyReg(phy, 4, RTL8306_REGPAGE0, &regData);
	if(regData & RTL8306_CAPABLE_100BASE_TX_FD)
		*advCapability = RTL8306_ETHER_AUTO_100FULL;
	else if(regData & RTL8306_CAPABLE_100BASE_TX_HD)
		*advCapability = RTL8306_ETHER_AUTO_100HALF;
	else if(regData & RTL8306_CAPABLE_10BASE_TX_FD)
		*advCapability = RTL8306_ETHER_AUTO_10FULL;
	else if(regData & RTL8306_CAPABLE_10BASE_TX_HD)
		*advCapability = RTL8306_ETHER_AUTO_10HALF;
	
	return SUCCESS;
}



/*
@func int | rtl8306_getEthernetPHY | Get PHY setting.
@parm u32 | phy | Specify the phy to get setting
@parm u32 * | autoNegotiation | Specify whether auto-negotiation is enabled.
@parm u32 * | advCapability | When auto-negotiation is enabled, get the advertised capability.
@parm u32 * | speed | When auto-negotiation is disabled, get the force mode speed.
@parm u32 * | fullDuplex | When auto-negotiatoin is disabled, get the force mode duplex mode.
@rvalue SUCCESS 
@rvalue FAILED
@comm
Either auto-negotiation advertised capability or force mode speed/duplex setting is valid.
If auto-negotiation is enabled, the advertisement capability is valid.
If auto-negotiation is diabled, the speed and duplex mode setting are valid.
If any of the pointer is NULL, the function will return FAILED.
*/

int rtl8306_getEthernetPHY(u32 phy, u32 *autoNegotiation, u32 *advCapability, u32 *speed, u32 *fullDuplex) {

    if (rtl8306_getAsicEthernetPHY(phy, autoNegotiation,advCapability, speed, fullDuplex) == FAILED)
        return FAILED;
    return SUCCESS;
}

/*
@func int | rtl8306_getAsicPHYLinkStatus | Get PHY Link Status.
@parm u32 | phy | Specify the phy to get setting
@parm u32 * | linkUp | Describe whether link status is up or not.
@rvalue SUCCESS 
@rvalue FAILED
@comm
	Read the link status of PHY register 1. Latched on 0 until read this once again.
*/
int rtl8306_getAsicPHYLinkStatus(u32 phy, u32 *linkUp) {
	u32 bitValue;
	
	if (linkUp == NULL)
		return FAILED;
	rtl8306_getAsicPhyRegBit(phy, 1, 2, RTL8306_REGPAGE0, &bitValue);
	*linkUp = (bitValue == 1? TRUE: FALSE);

	return SUCCESS;
}


/*
@func int | rtl8306_getPHYLinkStatus | Get PHY Link Status.
@parm u32 | phy | Specify the phy to get setting
@parm u32 * | linkUp | Describe whether link status is up or not.
@rvalue SUCCESS 
@rvalue FAILED
@comm
	Read the link status of PHY register 1. Latched on 0 until read this once again.
*/
int rtl8306_getPHYLinkStatus(u32 phy, u32 *linkUp) {

    if (rtl8306_getAsicPHYLinkStatus(phy, linkUp) == FAILED)
        return FAILED;
    return SUCCESS;
}

#define RTL8306_SPAN_DISABLE		0
#define RTL8306_SPAN_BLOCK		1
#define RTL8306_SPAN_LEARN		2
#define RTL8306_SPAN_FORWARD	3


/*
@func int32 | rtl8306_setAsic1dPortState | Set IEEE 802.1d port state
@parm uint32 | port | Specify port number (0 ~ 5)
@parm uint32 | state | Specify port state
@rvalue SUCCESS 
@rvalue FAILED
@comm
There are 4 port state:<nl> 
	RTL8306_SPAN_DISABLE  - Disable state<nl>
	RTL8306_SPAN_BLOCK    - Blocking state<nl>   
	RTL8306_SPAN_LEARN    - Learning state<nl>
	RTL8306_SPAN_FORWARD	- Forwarding state<nl>	
*/
//int32 rtl8306_setAsic1dPortState(uint32 port, uint32 state) {
int rtl8306_setAsic1dPortState(u32 port, u32 state) {
//	printk("rtl8306_setAsic1dPortState: port:%d, status:%d(0:disable, 1:block, 2:learn, 3:forward).\n", port, state);
	//uint32 regValue;
	int regValue;
	
	if ((port > RTL8306_PORT5) || (state > RTL8306_SPAN_FORWARD))
		return FAILED;
	/*Enable BPDU to trap to cpu, BPDU could not be flooded to all port*/
	rtl8306_setAsicPhyRegBit(2, 21, 6, 3, 1);		
	rtl8306_getAsicPhyReg(4, 21, 3, &regValue);
	regValue = (regValue & ~(0x3 << (2*port))) | (state << (2*port));
	rtl8306_setAsicPhyReg(4, 21, 3, regValue);

	return SUCCESS;
}

EXPORT_SYMBOL(rtl8306_setAsic1dPortState);

/*
@func int32 | rtl8306_getAsic1dPortState | Get IEEE 802.1d port state
@parm uint32 | port | Specify port number (0 ~ 5)
@parm uint32* | state | port state
@rvalue SUCCESS 
@rvalue FAILED
@comm
*/
int32 rtl8306_getAsic1dPortState(uint32 port, uint32 *state) {
	uint32 regValue;

	if ((port > RTL8306_PORT5) || (state == NULL))
		return FAILED;
	rtl8306_getAsicPhyReg(4, 21, 3, &regValue);
	*state = (regValue & (0x3 << 2*port)) >> (2*port);	
	return SUCCESS;
}

int rtl_init (void)
{
	u16 phy_data;
	u8 phy_reg=0;	
	int reset_cnt,i;		

	star_gsw_read_phy(RTL8201CP_PHY_ADDR, 3, &phy_data);
	PRINT_INFO("PHY 1, reg 3, PHY Identifier 2=0x%04x\n", phy_data);
	if (phy_data==0xC852) {
		//For RTL8201CP, the reg 3, PHY 	Identifier 2 should be 0xc852, refer the RTL8306SG datasheet
		PRINT_INFO("init RTL8306G\n");
#if 0		
		rtl8306_setAsicCPUPort(5,FALSE);
#else
		star_gsw_read_phy(6, 22, &phy_data);
		PRINT_INFO("default PHY 6, reg 22 value is 0x%04x\n",phy_data);
	    	phy_data = 0x873f;
	    	star_gsw_write_phy(6, 22, phy_data);
		PRINT_INFO("now set PHY 6, reg 22 value as 0x%04x\n",phy_data);
#endif
		
		star_gsw_set_phy_addr(0, 6);
		force_port0_100Mbps();
		start_link_led();
		//rtl8306_initQos(4);
		PRINT_INFO("rtl8306_initQos(4)\n");
#if 0		
		rtl8306_setAsicPortLearningAbility(0, FALSE);
		rtl8306_setAsicPortLearningAbility(1, FALSE);
		rtl8306_setAsicPortLearningAbility(2, FALSE);
		rtl8306_setAsicPortLearningAbility(3, FALSE);
		rtl8306_setAsicPortLearningAbility(4, FALSE);
		PRINT_INFO("rtl8306_setAsicPortLearningAbility(0~4, FALSE)\n");
#endif		
		rtl8306_setAsicInputOutputDrop(RTL8306_OUTPUTDROP);
		PRINT_INFO("rtl8306_setAsicInputOutputDrop(RTL8306_OUTPUTDROP)\n");
		setMirror_port();
		//rtl8306_setAsicQosPortFlowControlMode(4, 1);
		//rtl8306_setAsicQosPortFlowControlThr(4, 127, 63, RTL8306_PORT_TX);
		//rtl8306_setEthernetPHY(4, FALSE, RTL8306_ETHER_AUTO_10HALF, 10, RTL8306_SELECT_HALF_DUPLEX);
		//u32 iphy, *iautoNegotiation, iadvCapability, ispeed, ifullDuplex;
		//rtl8306_getEthernetPHY(4, &iautoNegotiation, &iadvCapability, &ispeed, &ifullDuplex);

		PRINT_INFO("setMirror_port()\n");
		//rtl8306_setAsic1dPortState(0,RTL8306_SPAN_BLOCK);
		//PRINT_INFO("call rtl8306_setAsic1dPortState(0,RTL8306_SPAN_BLOCK)\n");


	} else {
		PRINT_INFO("init RTL8201CP\n");
		//for RTL8201CP, we only need to set the correct PHY address, that's 1.
		star_gsw_read_phy(RTL8201CP_PHY_ADDR, 3, &phy_data);
		if (phy_data==0x8201) {
			star_gsw_set_phy_addr(0, RTL8201CP_PHY_ADDR);
			GSW_MAC_PORT_0_CONFIG |= (0x1 << 7);  //enable AN
			star_gsw_read_phy(RTL8201CP_PHY_ADDR, phy_reg, &phy_data);
			phy_data &= (~(0x1 << 11));  //power on
			phy_data |= (0x1 << 15);  //software reset
			star_gsw_write_phy(RTL8201CP_PHY_ADDR, phy_reg, phy_data);
		} else {
			//Tim Wang, send reset PHY signal from GPIO 3
#define MAX_PHY_RESET_CNT 20
#define SYSTEM_RESET_GPIO 3	
#define PHY_RESET_GPIO 3
	HAL_GPIO_SET_DIRECTION_OUTPUT(1<<PHY_RESET_GPIO);	
			reset_cnt=0;
			while(reset_cnt++ < MAX_PHY_RESET_CNT) {
				printk("detect abnormal PHY status, try to reset PHY RTL8201CP, reset_count=%d\n", reset_cnt);
				HAL_GPIO_SET_DATA_OUT_LOW(1<<PHY_RESET_GPIO);		
				for (i=0;i<200;i++) {			
					udelay(1000);		
				}		
				HAL_GPIO_SET_DATA_OUT_HIGH(1<<PHY_RESET_GPIO);		
				for (i=0;i<100;i++) {			
					udelay(1000);		
				}		
				star_gsw_read_phy(RTL8201CP_PHY_ADDR, 3, &phy_data);
				if (phy_data==0x8201) {
					star_gsw_set_phy_addr(0, RTL8201CP_PHY_ADDR);
					GSW_MAC_PORT_0_CONFIG |= (0x1 << 7);  //enable AN
					star_gsw_read_phy(RTL8201CP_PHY_ADDR, phy_reg, &phy_data);
					phy_data &= (~(0x1 << 11));  //power on
					phy_data |= (0x1 << 15);  //software reset
					star_gsw_write_phy(RTL8201CP_PHY_ADDR, phy_reg, phy_data);
					break;
				} 
			}
			HAL_GPIO_SET_DIRECTION_INPUT(1<<SYSTEM_RESET_GPIO);	
			if (reset_cnt >= MAX_PHY_RESET_CNT) {
				PRINT_INFO("after reset PHY from GPIO 3 %d times, still can not recover it. try workaround by disable AN\n", MAX_PHY_RESET_CNT);
				force_port0_100Mbps();
			}
		}
	}	
		
	return 0;
}


// In GPB 262, don't init anything - device is connected to an unmanaged switch.
// Just set the phy addr.
// TODO: What to init? using UMedia code works well
int gpb262_phy_init (void)
{
	star_gsw_set_phy_addr(0, 1);
	return 0;
}

//  Init the gpb 262 MAC - do the standard MAC init, 
//  and then also disable autoneg and set in full duplex, 100Mbps mode.
int gpb262_mac_init (void)
{
	u32 mac_port_config;
	init_packet_forward(0);

	mac_port_config = GSW_MAC_PORT_0_CONFIG;

	// Disable auto neg
	mac_port_config &= ~(0x1 << 7);
	
	// Force speed 100Mbps, full duplex, Rx/Tx flow control on 
	// (these should be the chip defaults, but clear speed and set just to be safe)
	mac_port_config &= ~(0x11 << 8);
	mac_port_config |= ( (0x01 << 8) | (0x1 << 10) |  (0x1 << 11) |  (0x1 << 12) );
	
	GSW_MAC_PORT_0_CONFIG = mac_port_config;
	return 0;
}


#ifdef CONFIG_LIBRA
void icp_175c_all_phy_power_down(int y)
{
	int i=0;

	for (i=0 ; i < 5 ; ++i)
		std_phy_power_down(i, y);

}

#define CONFIG_PORT0_5

void configure_icplus_175c_phy(void)
{
	u32 volatile	II, jj;
	u32 volatile	mac_port_config;
	u16 volatile reg;
	u32 volatile reg2;
	int i=0;
	u16 idreg1;
	u16 idreg2;

	while (1)
	{
		/* software reset  */
		star_gsw_write_phy(30, 0, 0x175C);
		mdelay(5);		// wait at least 2ms
		
		/* read identifier 1 register */
		star_gsw_read_phy(0, 2, &idreg1);
		
		/* read identifier 2 register */
		star_gsw_read_phy(0, 3, &idreg2);

		if ((idreg1 == 0x0243)&&(idreg2 == 0x0D80))
			break;
	}
	printk("IP175C software reset completed.\n");

	//star_gsw_set_phy_addr(0, 0);
	//star_gsw_set_phy_addr(1, 1);

	printk("\n ICPLUS175C_PHY,enable PORT0 local flow control capability \n");
	/* adjust MAC port 0 /RX/TX clock skew */
	GSW_BIST_RESULT_TEST_0 &= ~((0x3 << 24) | (0x3 << 26));
	GSW_BIST_RESULT_TEST_0 |= ((0x2 << 24) | (0x2 << 26));

// 20061117 descent
	// auto polling default phy address is 0
	// so set phy address to not exist address to avoid auto polling
	// in STAR Libra board if no these code, port 0 link state will get half duplex
	// port 1-4 get full duplex
        if (star_gsw_set_phy_addr(0, 15))
                printk ("star_gsw_set_phy_addr(0,2) is successful\n");
        else
                printk ("star_gsw_set_phy_addr(0,2) is fail\n");

        if (star_gsw_set_phy_addr(1, 16))
                printk ("star_gsw_set_phy_addr(1,3) is successful\n");
        else
                printk ("star_gsw_set_phy_addr(1,3) is fail\n");



	#if 0
	// for PHY_AN_ADVERTISEMENT_REG_ADDR
	// in this case needn't this code, only for reference
		star_gsw_read_phy(i, 4, &phy_data);
		phy_data |= (0x1 << 10);  // Enable PAUSE frame capability of PHY 0
		phy_data |= (0x1 << 5) | (0x1 << 6) | (0x1 << 7) | (0x1 << 8);
		star_gsw_write_phy(i, 4, phy_data);
		star_gsw_read_phy(i, 0, &phy_data);
		phy_data |= (0x1 << 9) | (0x1 << 12);  // Enable AN and Restart-AN
		star_gsw_write_phy(i, 0, phy_data);
	#endif
// 20061117 descent end


#if 0
	for (i=0 ; i < 10; ++i)
	{
	  star_gsw_read_phy(i, 2, &reg);
	  printk("addr: %d, phyid: %x\n", i, reg);
	}




	/*
	 * Configure GSW's MAC port 0
	 * For ASIX's 5-port GbE Switch setting
	 * 1. No SMI (MDC/MDIO) connection between Orion's MAC port 0 and ASIX's MAC port 4
	 * 2. Force Orion's MAC port 0 to be 1000Mbps, and full-duplex, and flow control on
	 */
	mac_port_config = GSW_MAC_PORT_0_CONFIG;



	// enable RGMII-PHY mode
	mac_port_config &= ~(0x1 << 15);

	// force speed = 100Mbps
	mac_port_config &= ~(0x3 << 8);
	mac_port_config |= (0x1 << 8);

	// force full-duplex
	mac_port_config |= (0x1 << 10);

	// force Tx/Rx flow-control on
	mac_port_config |= (0x1 << 11) | (0x1 << 12);

	GSW_MAC_PORT_0_CONFIG = mac_port_config;
	star_gsw_write_phy(29, 31, 0x175C);
	//star_gsw_write_phy(30, 9, 0x1089);
	star_gsw_write_phy(29, 23, 0x2);

	star_gsw_write_phy(29, 24, 0x2);
	star_gsw_write_phy(29, 25, 0x1);
	star_gsw_write_phy(29, 26, 0x1);
	star_gsw_write_phy(29, 27, 0x1);
	star_gsw_write_phy(29, 28, 0x1);
	star_gsw_write_phy(29, 29, 0x2);

	star_gsw_write_phy(30, 1, (0x3e << 8) );
	star_gsw_write_phy(30, 2, 0x21);
	star_gsw_write_phy(30, 9, 0x80);

	udelay(1000);

	mac_port_config = GSW_MAC_PORT_0_CONFIG;

	if (((mac_port_config & 0x1) == 0) || (mac_port_config & 0x2)) {
		printk("Check MAC/PHY 0 Link Status : DOWN!\n");
	} else {
		printk("Check MAC/PHY 0 Link Status : UP!\n");
	}

	/* adjust MAC port 0 /RX/TX clock skew */
	GSW_BIST_RESULT_TEST_0 &= ~((0x3 << 24) | (0x3 << 26));
	GSW_BIST_RESULT_TEST_0 |= ((0x2 << 24) | (0x2 << 26));


#endif



#if 1

	
#ifdef LINUX24
	reg2 = __REG(PWR_PAD_DRV_REG);
	PDEBUG("[PWR_PAD_DRV_REG = %x]\n", reg2);
	reg2 = reg2 | 0x4;

	__REG(PWR_PAD_DRV_REG) = reg2;

	reg2 = __REG(PWR_PAD_DRV_REG);
	PDEBUG("[PWR_PAD_DRV_REG = %x]\n", reg2);
#endif // LINUX24


#ifdef LINUX26
	reg2 = PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG;
	PDEBUG("[PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG = %x]\n", reg2);
	// Set MAC port 0 I/O pad drive strength as 10/100 mode.
	reg2 = reg2 | 0x04;

	PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG = reg2;

	reg2 = PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG;
	PDEBUG("[PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG = %x]\n", reg2);
#endif // LINUX26
	
	star_gsw_write_phy(29, 31, 0x175C);
	

	star_gsw_read_phy(0, 2, &reg);
	PDEBUG("[0,%d,%x]\n", 2, reg);
	star_gsw_read_phy(29, 31, &reg);
	PDEBUG("[29,%d,%x]\n", 31, reg);

	// Tag/un-tag function setup
	star_gsw_write_phy(29, 23, 0x7C2);
	// PVID function setup
	star_gsw_write_phy(29, 24, 0x1); // Define PVID of Port 0
	star_gsw_write_phy(29, 25, 0x1); // Define PVID of Port 1
	star_gsw_write_phy(29, 26, 0x1); // Define PVID of Port 2

#ifdef CONFIG_PORT0_5
	star_gsw_write_phy(29, 27, 0x1); // Define PVID of Port 3
#else
	star_gsw_write_phy(29, 27, 0x2); // Define PVID of Port 3
#endif // CONFIG_PORT0_5

	star_gsw_write_phy(29, 28, 0x2); // Define PVID of Port 4
	star_gsw_write_phy(29, 30, 0x2); // Define PVID of MII0

	// VLAN Mask function setup
	// Tag VLAN0[5:0] / VLAN1[13:8] output mask
#ifdef CONFIG_PORT0_5
	printk("CONFIG_PORT0_5\n");
	// VLAN0 (101111)
	// VLAN1 (110000)
	star_gsw_write_phy(20, 1, 0x0C2F);
#else // port0_4
	printk("not CONFIG_PORT0_5\n");
	// VLAN0 (111000)
	// VLAN1 (100111)
	star_gsw_write_phy(30, 1, 0x09F8);
#endif // CONFIG_PORT0_5

	// Smart MAC function setup
	// 30.9[2:0]  001    Define 1 LAN group
	// 30.9[3]    1      Enable router function
	// 30.9[6:4]  000    Enable ID index as 000
	// 30.9[7]    1      Enable tag VLAN
	// 30.9[12:8] 10000  define port 4 as a WAN port
	star_gsw_write_phy(30, 9, 0x1089);




// 20061115 descent
// If no these code,
// in STAR libra board, will get crc drop
	// configure port 4 (MII 0)
	// configure port 4 (MII 0)

	// P4_FORCE
	star_gsw_read_phy(29, 22, &reg);
	reg |= (0x1 << 15);
        star_gsw_write_phy(29, 22, reg);

	
	// MAC_X_EN, flow control enable of MII0 and MII2
	star_gsw_read_phy(29, 18, &reg);
	reg |= (0x1 << 10);
        star_gsw_write_phy(29, 18, reg);


	// P4_FORCE 100 Mbps
	star_gsw_read_phy(29, 22, &reg);
	reg |= (0x1 << 10);
        star_gsw_write_phy(29, 22, reg);


	// P4_FORCE_FULL duplex
	star_gsw_read_phy(29, 22, &reg);
	reg |= (0x1 << 5);
        star_gsw_write_phy(29, 22, reg);

// 20061115 descent end





	
	#if 0
	// enable flow control
	star_gsw_read_phy(29, 18, &reg);
	reg |= (0x1 < 13);
	star_gsw_write_phy(29, 18, reg);
	star_gsw_read_phy(29, 18, &reg);
	printk("[29,%d,%x]\n", 18, reg);
	#endif
#if 0
	//star_gsw_read_phy(29, 23, &reg);
	//reg = reg & 0xF800;
	//reg = reg | 0x7C2;


	star_gsw_write_phy(29, 24, 0x1);

	star_gsw_read_phy(29, 24, &reg);
	PDEBUG("[  29, 24, %x]\n", reg);

	star_gsw_write_phy(29, 25, 0x2);
	udelay(1000);

	star_gsw_read_phy(29, 25, &reg);
	PDEBUG("[  29, 25, %x]\n", reg);

	star_gsw_write_phy(29, 26, 0x1);
	star_gsw_write_phy(29, 27, 0x1);
	star_gsw_write_phy(29, 28, 0x1);
	star_gsw_write_phy(29, 30, 0x2);

	// tag vlan mask

	star_gsw_write_phy(30, 1, 0x3F3D);

	star_gsw_write_phy(30, 2, 0x3F22);
	star_gsw_write_phy(30, 9, 0x1089);
	// star_gsw_write_phy(30, 1, 0x3D22);
	// star_gsw_write_phy(30, 9, 0x028A);
#endif

#if 0

	for(II=18;II<=30;II++)
	{
		star_gsw_read_phy(29, II, &reg);
		PDEBUG("[29,%d,%x]\n", II, reg);
	}

	for(II=1;II<=10;II++)
	{
		star_gsw_read_phy(30, II, &reg);
		PDEBUG("[30,%d,%x]\n", II, reg);
	}

#endif

	mac_port_config = GSW_MAC_PORT_0_CONFIG;

	// disable PHY's AN
	mac_port_config &= ~(0x1 << 7);

	// disable RGMII-PHY mode
	mac_port_config &= ~(0x1 << 15);

	// force speed = 100Mbps
	mac_port_config &= ~(0x3 << 8);
	mac_port_config |= (0x1 << 8);
	
	// force full-duplex
	mac_port_config |= (0x1 << 10);

	// force Tx/Rx flow-control on
	mac_port_config |= (0x1 << 11) | (0x1 << 12);

	GSW_MAC_PORT_0_CONFIG = mac_port_config;


#if 0
	for (II = 0; II < 0x2000; II++)
	{
		mac_port_config = GSW_MAC_PORT_0_CONFIG;
		
		if ((mac_port_config & 0x1) && !(mac_port_config & 0x2))
		{

			/* enable MAC port 0
			*/
			mac_port_config &= ~(0x1 << 18);

		
			/*
			* enable the forwarding of unknown, multicast and broadcast packets to CPU
			*/
			mac_port_config &= ~((0x1 << 25) | (0x1 << 26) | (0x1 << 27));
		
			/*
			* include unknown, multicast and broadcast packets into broadcast storm
			*/
			mac_port_config |= ((0x1 << 29) | (0x1 << 30) | ((u32)0x1 << 31));
			// mac_port_config |= ( (0x1 << 30) | ((u32)0x1 << 31));
			
			GSW_MAC_PORT_0_CONFIG = mac_port_config;
			
			break;
		}
	}
#endif

	if (!(mac_port_config & 0x1) || (mac_port_config & 0x2))
	{
		/*
		* Port 0 PHY link down or no TXC in Port 0
		*/
		printk("\rCheck MAC/PHY 0 Link Status : DOWN!\n");
		
	}
	else
	{
		printk("\rCheck MAC/PHY 0 Link Status : UP!\n");
	}
#endif

	//printk("Found ICPLUS175C_PHY\n");
}
#endif // CONFIG_LIBRA


/**********************************************************************
 * Orion --- Libra2
 *********************************************************************/
#ifdef CONFIG_LIBRA2_TEST
void icplus_175c_phy_power_down(int port_no, int type)
{
	int i=0;

	if (port_no==0)
	{
		for (i=0 ; i < 4 ; ++i)
			std_phy_power_down(i, type);
	}
	
	if (port_no==1)
	{
		std_phy_power_down(4, type);
	}

}


void configure_icplus_175c_phy0_3(void)
{
	u16 volatile reg;
	u32 volatile reg2;
	
	printk("\n ICPLUS175C_PHY,enable PORT0 local flow control capability \n");
	/* adjust MAC port 0 /RX/TX clock skew */
	GSW_BIST_RESULT_TEST_0 &= ~((0x3 << 24) | (0x3 << 26));
	GSW_BIST_RESULT_TEST_0 |= ((0x2 << 24) | (0x2 << 26));

// 20061117 descent
	// auto polling default phy address is 0
	// so set phy address to not exist address to avoid auto polling
	// in STAR library board if no these code, port 0 link state will get half duplex
	// port 1-4 get full duplex
	// Set MAC0 phy assress to 15 not 0~3 to prevent address parsed
	// by switch.
	if (star_gsw_set_phy_addr(0, 15))
		printk ("star_gsw_set_phy_addr(0,15) is successful\n");
	else
		printk ("star_gsw_set_phy_addr(0,15) is fail\n");

	
#ifdef LINUX24
	reg2 = __REG(PWR_PAD_DRV_REG);
	PDEBUG("[PWR_PAD_DRV_REG = %x]\n", reg2);
	reg2 = reg2 | 0x4;

	__REG(PWR_PAD_DRV_REG) = reg2;

	reg2 = __REG(PWR_PAD_DRV_REG);
	PDEBUG("[PWR_PAD_DRV_REG = %x]\n", reg2);
#endif // LINUX24

#ifdef LINUX26
	reg2 = PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG;
	PDEBUG("[PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG = %x]\n", reg2);
	// Set MAC port 0 I/O pad drive strength as 10/100 mode.
	reg2 = reg2 | 0x04;

	PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG = reg2;

	reg2 = PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG;
	PDEBUG("[PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG = %x]\n", reg2);
#endif // LINUX26

	star_gsw_write_phy(29, 31, 0x175C);
	
// 20061115 descent
// If no these code,
// in STAR libra board, will get crc drop
	// configure port 4 (MII 0)
	// P4_FORCE
	star_gsw_read_phy(29, 22, &reg);
	reg |= (0x1 << 15);
	star_gsw_write_phy(29, 22, reg);

	
	// MAC_X_EN, flow control enable of MII0 and MII2
	star_gsw_read_phy(29, 18, &reg);
	reg |= (0x1 << 10);
	star_gsw_write_phy(29, 18, reg);


	// P4_FORCE 100 Mbps
	star_gsw_read_phy(29, 22, &reg);
	reg |= (0x1 << 10);
	star_gsw_write_phy(29, 22, reg);


	// P4_FORCE_FULL duplex
	star_gsw_read_phy(29, 22, &reg);
	reg |= (0x1 << 5);
	star_gsw_write_phy(29, 22, reg);

// 20061115 descent end

	config_MAC_port0_libra2();

}

void configure_icplus_175c_phy4(void)
{
	u16 volatile reg;
	u32 volatile reg2;

	// Set auto-polling phy address.
	if (star_gsw_set_phy_addr(1, 4))
		printk ("star_gsw_set_phy_addr(1, 4) is successful\n");
	else
		printk ("star_gsw_set_phy_addr(1, 4) is fail\n");
	
#ifdef LINUX24
	reg2 = __REG(PWR_PAD_DRV_REG);
	PDEBUG("[PWR_PAD_DRV_REG = %x]\n", reg2);
	reg2 = reg2 | 0x8;

	__REG(PWR_PAD_DRV_REG) = reg2;

	reg2 = __REG(PWR_PAD_DRV_REG);
	PDEBUG("[PWR_PAD_DRV_REG = %x]\n", reg2);
#endif // LINUX24

#ifdef LINUX26
	reg2 = PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG;
	PDEBUG("[PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG = %x]\n", reg2);
	// Set MAC port 1 I/O pad drive strength as 10/100 mode.
	reg2 = reg2 | 0x08;

	PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG = reg2;

	reg2 = PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG;
	PDEBUG("[PWRMGT_PAD_DRIVE_STRENGTH_CONTROL_REG = %x]\n", reg2);
#endif // LINUX26

	// Read phy4 "Organizationally unique identifier - 0x0243)
	//star_gsw_read_phy(4, 2, &reg);
	//printk("   *** star_gsw_read_phy(4,2) = 0x%04x\n", reg);
	
	// Enable PAUSE frame capability
	star_gsw_read_phy(4, 4, &reg);
	reg |= (0x01 << 10);
	star_gsw_write_phy(4, 4, reg);
	
	// MAC1 - phy4 flow control handshake
	// Auto-negotiation enable(bit12) and Restart auto-negotiation(bit9)
	star_gsw_read_phy(4, 0, &reg);
	reg |= ((0x01 << 9) | (0x01 << 12));
	star_gsw_write_phy(4, 0, reg);

	config_MAC_port1_libra2();
	
}

void config_MAC_port0_libra2(void)
{
	u32 volatile mac_port_config;
	
	mac_port_config = GSW_MAC_PORT_0_CONFIG;
	
	// Disable MAC0 PHY's AN
	mac_port_config &= ~(0x1 << 7);

	// Disable RGMII-PHY mode
	mac_port_config &= ~(0x1 << 15);

	// Force speed = 100Mbps
	mac_port_config &= ~(0x3 << 8);
	mac_port_config |= (0x1 << 8);
	
	// Force full-duplex
	mac_port_config |= (0x1 << 10);

	// Force Tx/Rx flow-control on
	mac_port_config |= (0x1 << 11) | (0x1 << 12);

	GSW_MAC_PORT_0_CONFIG = mac_port_config;
	printk("   ---> MAC Port 0 initial!\n");
		

}


void config_MAC_port1_libra2(void)
{
	u32 volatile mac_port_config;
	
	mac_port_config = GSW_MAC_PORT_1_CONFIG;

	// Enable MAC port 1
	mac_port_config &= ~(0x01 << 18);
	
	mac_port_config &= ~((0x01 << 25) | (0x01 << 26) | (0x01 << 27));
	
	// Eable MAC0 PHY's AN
	mac_port_config |= (0x1 << 7);

	// Disable RGMII-PHY mode
	mac_port_config &= ~(0x1 << 15);

	// Force speed = 100Mbps
	mac_port_config &= ~(0x3 << 8);
	mac_port_config |= (0x1 << 8);
	
	// Force full-duplex
	mac_port_config |= (0x1 << 10);

	// Force Tx/Rx flow-control on
	mac_port_config |= (0x1 << 11) | (0x1 << 12);

	GSW_MAC_PORT_1_CONFIG = mac_port_config;
	printk("   ---> MAC Port 1 initial!\n");



}

#endif // CONFIG_LIBRA2


/* Trigger a PHY reset in the IC+ 101 PHY, using the MII control register */
int icp_101a_phy_reset (int port)
{
        u16 phy_data = 0;

	PRINT_INFO("RESET IC+101A\n");

	if (!star_gsw_read_phy (port, PHY_CONTROL_REG_ADDR, &phy_data)) {
		PDEBUG ("\n Reset PHY%d Failed \n", port);
		return 1;
	}
	// PHY power down 
	phy_data |= (0x1 << 11);

	if (!star_gsw_write_phy (port, PHY_CONTROL_REG_ADDR, phy_data)) {
		PDEBUG ("\n Reset PHY%d Failed \n", port);
		return 1;
	}

	// PHY power up 
	phy_data &= ~(0x1 << 11);

	if (!star_gsw_write_phy (port, PHY_CONTROL_REG_ADDR, phy_data)) {
		PDEBUG ("\n Reset PHY%d Failed \n", port);
		return 1;
	}
	return 0;
}


/* Trigger a phy reset when the user writes 1 to /proc/str9100/phy_reset */
int star_phy_reset_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
	if (count && buffer[0]=='1') {
		PORT0_PHY_RESET
		PORT1_PHY_RESET
	}
	return count;
}

//Jacky.Yang 26-Aug-2009, Begin - Add command line for control port0~3.
int star_switch_set_port_enable(struct file *file, const char *buffer, unsigned long count, void *data)
{
	if (count && (buffer[0]=='0' || buffer[0]=='1' || buffer[0]=='2' || buffer[0]=='3' || buffer[0]=='5')) {
		printk("star_switch_set_port_enable: Switch port %c entering forwarding state\n", buffer[0]);
		if (buffer[0]=='0')
			rtl8306_setAsic1dPortState(0,RTL8306_SPAN_FORWARD);
		else if (buffer[0]=='1')
				rtl8306_setAsic1dPortState(1,RTL8306_SPAN_FORWARD);
		else if (buffer[0]=='2')
				rtl8306_setAsic1dPortState(2,RTL8306_SPAN_FORWARD);
		else if (buffer[0]=='3')
				rtl8306_setAsic1dPortState(3,RTL8306_SPAN_FORWARD);
		else if (buffer[0]=='5')
				rtl8306_setAsic1dPortState(5,RTL8306_SPAN_FORWARD);
	}
	else if (count)
	{
		printk("star_switch_set_port_enable: Error, the switch port number is incorrect!\n");
	}
	return count;
}

int star_switch_set_port_disable(struct file *file, const char *buffer, unsigned long count, void *data)
{
	if (count && (buffer[0]=='0' || buffer[0]=='1' || buffer[0]=='2' || buffer[0]=='3' || buffer[0]=='5')) {
		printk("star_switch_set_port_disable: Switch port %c entering blocking state\n", buffer[0]);
		if (buffer[0]=='0')
			rtl8306_setAsic1dPortState(0,RTL8306_SPAN_BLOCK);
		else if (buffer[0]=='1')
				rtl8306_setAsic1dPortState(1,RTL8306_SPAN_BLOCK);
		else if (buffer[0]=='2')
				rtl8306_setAsic1dPortState(2,RTL8306_SPAN_BLOCK);
		else if (buffer[0]=='3')
				rtl8306_setAsic1dPortState(3,RTL8306_SPAN_BLOCK);
		else if (buffer[0]=='5')
				rtl8306_setAsic1dPortState(5,RTL8306_SPAN_BLOCK);
	}
	else if (count)
	{
		printk("star_switch_set_port_disable: Error, the switch port number is incorrect!\n");
	}
	
	return count;
}

int star_switch_get_port_link_status(char *buffer, char **start, off_t offset,
	int length, int *eof, void *data)
{
	char last_status_trans[4]={'0', '0', '0', '0'};
	char out_last_status[10] = {0};
	u16 phy_data;
	int port_cnt;
	
	int count;
	int num = 0;
	
	for (port_cnt=0;port_cnt<4;port_cnt++) {
		star_gsw_read_phy(port_cnt, STATUS_REGISTER, &phy_data);
		if (phy_data & LINK_STATUS_BIT_MASK) {
			last_status_trans[port_cnt] = '1';
		} else {
			last_status_trans[port_cnt] = '0';
		}
	}
	
	memset(out_last_status, '\0', sizeof(out_last_status));
	snprintf(out_last_status, sizeof(out_last_status), "%c;%c;%c;%c;", last_status_trans[0], last_status_trans[1], last_status_trans[2], last_status_trans[3]);
	
	
	sprintf(buffer, out_last_status, sizeof(out_last_status));
	num += sizeof(out_last_status);

	return num;
}
//Jacky.Yang 26-Aug-2009, End - Add command line for control port0~3.

//june.chen, 2011-01-03, add for get port status (blocked or enabled)
int star_switch_get_port_enable(char *buffer, char **start, off_t offset,
        int length, int *eof, void *data)
{
	int portStatus = 0, tmp = 0, num = 0;
	int i;
	char tmpBuf[5] = {0};

        for(i = 0; i < 4; i++){
		rtl8306_getAsic1dPortState(i, &tmp);
		//portStatus |= (tmp << i);
		tmpBuf[3 - i] = (char) (tmp + '0');
	}

	sprintf(buffer, "%s", tmpBuf);
	
	num += sizeof(tmpBuf);
	return num;
}
//june.chen end
