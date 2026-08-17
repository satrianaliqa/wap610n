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

#ifndef __STAR_NIC_CONFIG_H__
#define __STAR_NIC_CONFIG_H__

#if 1
#define DBG_PRINT printk
#else
#define DBG_PRINT(arg...)
#endif

#define DO_PRINT(arg...)

// VSC8601 and WavePlus Phy are the same
#define STAR_NIC_PHY_ADDR	0

#define CONFIG_STAR_NIC_NAPI
#define FREE_TX_SKB_MULTI

#define STAR_NIC_TX_HW_CHECKSUM
#define STAR_NIC_RX_HW_CHECKSUM

#define STAR_NIC_SG

#if defined(STAR_NIC_SG) && !defined(STAR_NIC_TX_HW_CHECKSUM)
#define STAR_NIC_TX_HW_CHECKSUM
#endif

#define STAR_NIC_STATUS_ISR
#define STAR_NIC_RXQF_ISR

//#ifndef CONFIG_STAR_NIC_NAPI
#define STAR_NIC_DELAYED_INTERRUPT
//#endif

#define MAX_PEND_INT_CNT	0x20
#define MAX_PEND_TIME		0x20

//#ifdef CONFIG_STAR_NIC_PHY_VSC8601
//#define CONFIG_STAR_JUMBO
//#endif
//#define CONFIG_STAR_JUMBO
#ifdef CONFIG_STAR_JUMBO
#define MAX_PACKET_LEN		(2038)
//#define MAX_PACKET_LEN		(9038)
#else
#define MAX_PACKET_LEN		(1536)
#endif
#define PKT_MIN_SIZE		60

//#define STAR_NIC_TIMER

/* 
 * Maximum Transmit/Receive Frame Descriptors for NIC's MAC frame
 */
#ifdef FREE_TX_SKB_MULTI
#define STAR_NIC_MAX_TFD_NUM	256
#define STAR_NIC_MAX_RFD_NUM	256
#else
#define STAR_NIC_MAX_TFD_NUM	64
#define STAR_NIC_MAX_RFD_NUM	64
#endif

#endif /* __STAR_NIC_CONFIG_H__ */

