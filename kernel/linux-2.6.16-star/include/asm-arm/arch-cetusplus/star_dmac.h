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

#ifndef _STAR_DMAC_H_
#define _STAR_DMAC_H_

#include "star_sys_memory_map.h"

#if defined(__UBOOT__)
#define DMAC_MEM_MAP_VALUE(reg_offset)			(*((u32 volatile *)(SYSPA_DMAC_BASE_ADDR + reg_offset)))
#elif defined(__LINUX__)
#define DMAC_MEM_MAP_VALUE(reg_offset)			(*((u32 volatile *)(SYSVA_DMAC_BASE_ADDR + reg_offset)))
#else
#error "NO SYSTEM DEFINED"
#endif

/*
 * define access macros
 */
#define DMAC_INT_STATUS					DMAC_MEM_MAP_VALUE(0x000)
#define DMAC_INT_TC_STATUS				DMAC_MEM_MAP_VALUE(0x004)
#define DMAC_INT_TC_CLR					DMAC_MEM_MAP_VALUE(0x008)
#define DMAC_INT_ERR_ABT_STATUS				DMAC_MEM_MAP_VALUE(0x00C)
#define DMAC_INT_ERR_ABT_CLR				DMAC_MEM_MAP_VALUE(0x010)
#define DMAC_TC_STATUS					DMAC_MEM_MAP_VALUE(0x014)
#define DMAC_ERR_ABT_STATUS				DMAC_MEM_MAP_VALUE(0x018)
#define DMAC_CH_EN_STATUS				DMAC_MEM_MAP_VALUE(0x01C)
#define DMAC_CH_BUSY_STATUS				DMAC_MEM_MAP_VALUE(0x020)
#define DMAC_CSR					DMAC_MEM_MAP_VALUE(0x024)
#define DMAC_SYNC					DMAC_MEM_MAP_VALUE(0x028)
#define DMAC_CH0_CSR					DMAC_MEM_MAP_VALUE(0x100)
#define DMAC_CH0_CFG					DMAC_MEM_MAP_VALUE(0x104)
#define DMAC_CH0_SRC_ADDR				DMAC_MEM_MAP_VALUE(0x108)
#define DMAC_CH0_DST_ADDR				DMAC_MEM_MAP_VALUE(0x10C)
#define DMAC_CH0_LLP					DMAC_MEM_MAP_VALUE(0x110)
#define DMAC_CH0_SIZE					DMAC_MEM_MAP_VALUE(0x114)

#define DMAC_CH1_CSR					DMAC_MEM_MAP_VALUE(0x120)
#define DMAC_CH1_CFG					DMAC_MEM_MAP_VALUE(0x124)
#define DMAC_CH1_SRC_ADDR				DMAC_MEM_MAP_VALUE(0x128)
#define DMAC_CH1_DST_ADDR				DMAC_MEM_MAP_VALUE(0x12C)
#define DMAC_CH1_LLP					DMAC_MEM_MAP_VALUE(0x130)
#define DMAC_CH1_SIZE					DMAC_MEM_MAP_VALUE(0x134)

#define DMAC_CH2_CSR					DMAC_MEM_MAP_VALUE(0x140)
#define DMAC_CH2_CFG					DMAC_MEM_MAP_VALUE(0x144)
#define DMAC_CH2_SRC_ADDR				DMAC_MEM_MAP_VALUE(0x148)
#define DMAC_CH2_DST_ADDR				DMAC_MEM_MAP_VALUE(0x14C)
#define DMAC_CH2_LLP					DMAC_MEM_MAP_VALUE(0x150)
#define DMAC_CH2_SIZE					DMAC_MEM_MAP_VALUE(0x154)

#define DMAC_CH3_CSR					DMAC_MEM_MAP_VALUE(0x160)
#define DMAC_CH3_CFG					DMAC_MEM_MAP_VALUE(0x164)
#define DMAC_CH3_SRC_ADDR				DMAC_MEM_MAP_VALUE(0x168)
#define DMAC_CH3_DST_ADDR				DMAC_MEM_MAP_VALUE(0x16C)
#define DMAC_CH3_LLP					DMAC_MEM_MAP_VALUE(0x170)
#define DMAC_CH3_SIZE					DMAC_MEM_MAP_VALUE(0x174)

#define DMAC_CH4_CSR					DMAC_MEM_MAP_VALUE(0x180)
#define DMAC_CH4_CFG					DMAC_MEM_MAP_VALUE(0x184)
#define DMAC_CH4_SRC_ADDR				DMAC_MEM_MAP_VALUE(0x188)
#define DMAC_CH4_DST_ADDR				DMAC_MEM_MAP_VALUE(0x18C)
#define DMAC_CH4_LLP					DMAC_MEM_MAP_VALUE(0x190)
#define DMAC_CH4_SIZE					DMAC_MEM_MAP_VALUE(0x194)

#define DMAC_CH5_CSR					DMAC_MEM_MAP_VALUE(0x1A0)
#define DMAC_CH5_CFG					DMAC_MEM_MAP_VALUE(0x1A4)
#define DMAC_CH5_SRC_ADDR				DMAC_MEM_MAP_VALUE(0x1A8)
#define DMAC_CH5_DST_ADDR				DMAC_MEM_MAP_VALUE(0x1AC)
#define DMAC_CH5_LLP					DMAC_MEM_MAP_VALUE(0x1B0)
#define DMAC_CH5_SIZE					DMAC_MEM_MAP_VALUE(0x1B4)

#define DMAC_CH6_CSR					DMAC_MEM_MAP_VALUE(0x1C0)
#define DMAC_CH6_CFG					DMAC_MEM_MAP_VALUE(0x1C4)
#define DMAC_CH6_SRC_ADDR				DMAC_MEM_MAP_VALUE(0x1C8)
#define DMAC_CH6_DST_ADDR				DMAC_MEM_MAP_VALUE(0x1CC)
#define DMAC_CH6_LLP					DMAC_MEM_MAP_VALUE(0x1D0)
#define DMAC_CH6_SIZE					DMAC_MEM_MAP_VALUE(0x1D4)

#define DMAC_CH7_CSR					DMAC_MEM_MAP_VALUE(0x1E0)
#define DMAC_CH7_CFG					DMAC_MEM_MAP_VALUE(0x1E4)
#define DMAC_CH7_SRC_ADDR				DMAC_MEM_MAP_VALUE(0x1E8)
#define DMAC_CH7_DST_ADDR				DMAC_MEM_MAP_VALUE(0x1EC)
#define DMAC_CH7_LLP					DMAC_MEM_MAP_VALUE(0x1F0)
#define DMAC_CH7_SIZE					DMAC_MEM_MAP_VALUE(0x1F4)
/*
 * define constants macros
 */
#define DMAC_CH0_TC_BIT_INDEX				(1 << 0)
#define DMAC_CH1_TC_BIT_INDEX				(1 << 1)
#define DMAC_CH2_TC_BIT_INDEX				(1 << 2)
#define DMAC_CH3_TC_BIT_INDEX				(1 << 3)
#define DMAC_CH4_TC_BIT_INDEX				(1 << 4)
#define DMAC_CH5_TC_BIT_INDEX				(1 << 5)
#define DMAC_CH6_TC_BIT_INDEX				(1 << 6)
#define DMAC_CH7_TC_BIT_INDEX				(1 << 7)

#define DMAC_CH0_ERR_BIT_INDEX				(1 << 0)
#define DMAC_CH1_ERR_BIT_INDEX				(1 << 1)
#define DMAC_CH2_ERR_BIT_INDEX				(1 << 2)
#define DMAC_CH3_ERR_BIT_INDEX				(1 << 3)
#define DMAC_CH4_ERR_BIT_INDEX				(1 << 4)
#define DMAC_CH5_ERR_BIT_INDEX				(1 << 5)
#define DMAC_CH6_ERR_BIT_INDEX				(1 << 6)
#define DMAC_CH7_ERR_BIT_INDEX				(1 << 7)

#define DMAC_CH0_ABT_BIT_INDEX				(1 << 16)
#define DMAC_CH1_ABT_BIT_INDEX				(1 << 17)
#define DMAC_CH2_ABT_BIT_INDEX				(1 << 18)
#define DMAC_CH3_ABT_BIT_INDEX				(1 << 19)
#define DMAC_CH4_ABT_BIT_INDEX				(1 << 20)
#define DMAC_CH5_ABT_BIT_INDEX				(1 << 21)
#define DMAC_CH6_ABT_BIT_INDEX				(1 << 22)
#define DMAC_CH7_ABT_BIT_INDEX				(1 << 23)

#define DMAC_ENABLE_BIT_INDEX				(1 << 0)
#define DMAC_M0_ENDIAN_BIG_BIT_INDEX			(1 << 1)
#define DMAC_M1_ENDIAN_BIG_BIT_INDEX			(1 << 2)

/*
 * defines for each channel
 */
#define DMAC_CH_DISABLE					0
#define DMAC_CH_ENABLE					1

#define DMAC_CH_DST_SEL_M0				0
#define DMAC_CH_DST_SEL_M1				1

#define DMAC_CH_SRC_SEL_M0				0
#define DMAC_CH_SRC_SEL_M1				1

#define DMAC_CH_DSTAD_CTL_INC				0
#define DMAC_CH_DSTAD_CTL_DEC				1
#define DMAC_CH_DSTAD_CTL_FIX				2

#define DMAC_CH_SRCAD_CTL_INC				0
#define DMAC_CH_SRCAD_CTL_DEC				1
#define DMAC_CH_SRCAD_CTL_FIX				2

#define DMAC_CH_MODE_NORMAL				0
#define DMAC_CH_MODE_HW_HANDSHAKE			1

#define DMAC_CH_SRC_WIDTH_8_BITS			0
#define DMAC_CH_SRC_WIDTH_16_BITS			1
#define DMAC_CH_SRC_WIDTH_32_BITS			2

#define DMAC_CH_DST_WIDTH_8_BITS			0
#define DMAC_CH_DST_WIDTH_16_BITS			1
#define DMAC_CH_DST_WIDTH_32_BITS			2

#define DMAC_CH_ABT_TRANSFER				1

#define DMAC_CH_SRC_BURST_SIZE_1			0
#define DMAC_CH_SRC_BURST_SIZE_4			1
#define DMAC_CH_SRC_BURST_SIZE_8			2
#define DMAC_CH_SRC_BURST_SIZE_16			3
#define DMAC_CH_SRC_BURST_SIZE_32			4
#define DMAC_CH_SRC_BURST_SIZE_64			5
#define DMAC_CH_SRC_BURST_SIZE_128			6
#define DMAC_CH_SRC_BURST_SIZE_256			7

#define DMAC_CH_PROT1_PRIVILEGED_MODE			1
#define DMAC_CH_PROT1_USER_MODE				0

#define DMAC_CH_PROT2_BUFFERABLE			1
#define DMAC_CH_PROT2_NON_BUFFERABLE			0

#define DMAC_CH_PROT3_CACHEABLE				1
#define DMAC_CH_PROT3_NON_CACHEABLE			0

#define DMAC_CH_PRI_LEVEL_0				0
#define DMAC_CH_PRI_LEVEL_1				1
#define DMAC_CH_PRI_LEVEL_2				2
#define DMAC_CH_PRI_LEVEL_3				3

#define DMAC_CH_TC_MASK_DISABLE				0
#define DMAC_CH_TC_MASK_ENABLE				1

#define DMAC_CH_INT_TC_BIT_INDEX			(1 << 0)
#define DMAC_CH_INT_ERR_BIT_INDEX			(1 << 1)
#define DMAC_CH_INT_ABT_BIT_INDEX			(1 << 2)
#define DMAC_CH_BUSY_BIT_INDEX				(1 << 8)


#define DMAC_MAX_CHANNEL_NUM				(8)

#define DMAC_CH0_ID					(1 << 0)
#define DMAC_CH1_ID					(1 << 1)
#define DMAC_CH2_ID					(1 << 2)
#define DMAC_CH3_ID					(1 << 3)
#define DMAC_CH4_ID					(1 << 4)
#define DMAC_CH5_ID					(1 << 5)
#define DMAC_CH6_ID					(1 << 6)
#define DMAC_CH7_ID					(1 << 7)
#define DMAC_CH_ID(idx)					(1 << idx)

#define DMAC_LITTLE_ENDIAN				(0)
#define DMAC_BIG_ENDIAN					(1)

/*
 * macro declarations
 */
#define HAL_DMAC_READ_INTERRUPT_STATUS(interrupt_status) \
    ((interrupt_status) = (DMAC_INT_STATUS) & 0xFF)

#define HAL_DMAC_READ_TERMINAL_COUNT_INTERRUPT_STATUS(tc_status) \
    ((tc_status) = (DMAC_INT_TC_STATUS) & 0xFF)

#define HAL_DMAC_CLEAR_TERMINAL_COUNT_INTERRUPT_STATUS(tc_status) \
    ((DMAC_INT_TC_CLR) = (tc_status) & 0xFF)

#define HAL_DMAC_READ_ERROR_ABORT_INTERRUPT_STATUS(err_abt_status) \
    ((err_abt_status) = (DMAC_INT_ERR_ABT_STATUS))

#define HAL_DMAC_CLEAR_ERROR_ABORT_INTERRUPT_STATUS(err_abt_status) \
    ((DMAC_INT_ERR_ABT_CLR) = (err_abt_status))

#define HAL_DMAC_READ_TERMINAL_COUNT_STATUS(tc_status) \
    ((tc_status) = (DMAC_TC_STATUS) & 0xFF)

#define HAL_DMAC_READ_ERROR_ABORT_STATUS(err_abt_status) \
    ((err_abt_status) = (DMAC_ERR_ABT_STATUS))

#define HAL_DMAC_READ_CHANNEL_ENABLE_STATUS(channel_enable_status) \
    ((channel_enable_status) = (DMAC_CH_EN_STATUS) & 0xFF)

#define HAL_DMAC_READ_CHANNEL_BUSY_STATUS(channel_busy_status) \
    ((channel_busy_status) = (DMAC_CH_BUSY_STATUS) & 0xFF)

#define HAL_DMAC_ENABLE() \
    ((DMAC_CSR) |= (DMAC_ENABLE_BIT_INDEX))

#define HAL_DMAC_DISABLE() \
    ((DMAC_CSR) &= ~(DMAC_ENABLE_BIT_INDEX))

#define HAL_DMAC_SET_MASTER0_BIG_ENDIAN() \
    ((DMAC_CSR) |= (DMAC_M0_ENDIAN_BIG_BIT_INDEX))

#define HAL_DMAC_SET_MASTER0_LITTLE_ENDIAN() \
    ((DMAC_CSR) &= ~(DMAC_M0_ENDIAN_BIG_BIT_INDEX))

#define HAL_DMAC_SET_MASTER1_BIG_ENDIAN() \
    ((DMAC_CSR) |= (DMAC_M1_ENDIAN_BIG_BIT_INDEX))

#define HAL_DMAC_SET_MASTER1_LITTLE_ENDIAN() \
    ((DMAC_CSR) &= ~(DMAC_M1_ENDIAN_BIG_BIT_INDEX))

#define HAL_DMAC_READ_SYNC_CONFIG(sync_config) \
    ((sync_config) = (DMAC_SYNC))

#define HAL_DMAC_WRITE_SYNC_CONFIG(sync_config) \
    ((DMAC_SYNC) = (sync_config))

/*
 * Macros for Channel 0
 */
#define HAL_DMAC_WRITE_CHANNEL0_CONTROL(control_setting) \
    ((DMAC_CH0_CSR) = (control_setting))

#define HAL_DMAC_READ_CHANNEL0_CONTROL(control_setting) \
    ((control_setting) = (DMAC_CH0_CSR))

#define HAL_DMAC_ENABLE_CHANNEL0() \
    ((DMAC_CH0_CSR) |= (0x1))

#define HAL_DMAC_DISABLE_CHANNEL0() \
    ((DMAC_CH0_CSR) &= ~(0x1))

#define HAL_DMAC_CHANNEL0_TRANSACTION_ABORT() \
    ((DMAC_CH0_CSR) |= (DMAC_CH_ABT_TRANSFER << 15))

#define HAL_DMAC_ENABLE_CHANNEL0_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH0_CFG) &= ~(DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL0_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH0_CFG) |= (DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL0_ERROR_INTERRUPT() \
    ((DMAC_CH0_CFG) &= ~(DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL0_ERROR_INTERRUPT() \
    ((DMAC_CH0_CFG) |= (DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL0_ABORT_INTERRUPT() \
    ((DMAC_CH0_CFG) &= ~(DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL0_ABORT_INTERRUPT() \
    ((DMAC_CH0_CFG) |= (DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_WRITE_CHANNEL0_SOURCE_ADDRESS(src_address) \
    ((DMAC_CH0_SRC_ADDR) = (src_address))

#define HAL_DMAC_READ_CHANNEL0_SOURCE_ADDRESS(src_address) \
    ((src_address) = (DMAC_CH0_SRC_ADDR))

#define HAL_DMAC_WRITE_CHANNEL0_DESTINATION_ADDRESS(dst_address) \
    ((DMAC_CH0_DST_ADDR) = (dst_address))

#define HAL_DMAC_READ_CHANNEL0_DESTINATION_ADDRESS(dst_address) \
    ((dst_address) = (DMAC_CH0_DST_ADDR))

#define HAL_DMAC_WRITE_CHANNEL0_TRANSFER_SIZE(transfer_size) \
    ((DMAC_CH0_SIZE) = (transfer_size) & 0x0FFF)

#define HAL_DMAC_READ_CHANNEL0_TRANSFER_SIZE(transfer_size) \
    ((transfer_size) = (DMAC_CH0_SIZE) & 0x0FFF)

/*
 * Macros for Channel 1
 */
#define HAL_DMAC_WRITE_CHANNEL1_CONTROL(control_setting) \
    ((DMAC_CH1_CSR) = (control_setting))

#define HAL_DMAC_READ_CHANNEL1_CONTROL(control_setting) \
    ((control_setting) = (DMAC_CH1_CSR))

#define HAL_DMAC_ENABLE_CHANNEL1() \
    ((DMAC_CH1_CSR) |= (1))

#define HAL_DMAC_DISABLE_CHANNEL1() \
    ((DMAC_CH1_CSR) &= ~(1))

#define HAL_DMAC_CHANNEL1_TRANSACTION_ABORT() \
    ((DMAC_CH1_CSR) |= (DMAC_CH_ABT_TRANSFER << 15))

#define HAL_DMAC_ENABLE_CHANNEL1_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH1_CFG) &= ~(DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL1_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH1_CFG) |= (DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL1_ERROR_INTERRUPT() \
    ((DMAC_CH1_CFG) &= ~(DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL1_ERROR_INTERRUPT() \
    ((DMAC_CH1_CFG) |= (DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL1_ABORT_INTERRUPT() \
    ((DMAC_CH1_CFG) &= ~(DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL1_ABORT_INTERRUPT() \
    ((DMAC_CH1_CFG) |= (DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_WRITE_CHANNEL1_SOURCE_ADDRESS(src_address) \
    ((DMAC_CH1_SRC_ADDR) = (src_address))

#define HAL_DMAC_READ_CHANNEL1_SOURCE_ADDRESS(src_address) \
    ((src_address) = (DMAC_CH1_SRC_ADDR))

#define HAL_DMAC_WRITE_CHANNEL1_DESTINATION_ADDRESS(dst_address) \
    ((DMAC_CH1_DST_ADDR) = (dst_address))

#define HAL_DMAC_READ_CHANNEL1_DESTINATION_ADDRESS(dst_address) \
    ((dst_address) = (DMAC_CH1_DST_ADDR))

#define HAL_DMAC_WRITE_CHANNEL1_TRANSFER_SIZE(transfer_size) \
    ((DMAC_CH1_SIZE) = (transfer_size) & 0x0FFF)

#define HAL_DMAC_READ_CHANNEL1_TRANSFER_SIZE(transfer_size) \
    ((transfer_size) = (DMAC_CH1_SIZE) & 0x0FFF)

/*
 * Macros for Channel 2
 */
#define HAL_DMAC_WRITE_CHANNEL2_CONTROL(control_setting) \
    ((DMAC_CH2_CSR) = (control_setting))

#define HAL_DMAC_READ_CHANNEL2_CONTROL(control_setting) \
    ((control_setting) = (DMAC_CH2_CSR))

#define HAL_DMAC_ENABLE_CHANNEL2() \
    ((DMAC_CH2_CSR) |= (1))

#define HAL_DMAC_DISABLE_CHANNEL2() \
    ((DMAC_CH2_CSR) &= ~(1))

#define HAL_DMAC_CHANNEL2_TRANSACTION_ABORT() \
    ((DMAC_CH2_CSR) |= (DMAC_CH_ABT_TRANSFER << 15))

#define HAL_DMAC_ENABLE_CHANNEL2_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH2_CFG) &= ~(DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL2_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH2_CFG) |= (DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL2_ERROR_INTERRUPT() \
    ((DMAC_CH2_CFG) &= ~(DMAC_CH_INT_ERR_BIT_INDEX))


#define HAL_DMAC_DISABLE_CHANNEL2_ERROR_INTERRUPT() \
    ((DMAC_CH2_CFG) |= (DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL2_ABORT_INTERRUPT() \
    ((DMAC_CH2_CFG) &= ~(DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL2_ABORT_INTERRUPT() \
    ((DMAC_CH2_CFG) |= (DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_WRITE_CHANNEL2_SOURCE_ADDRESS(src_address) \
    ((DMAC_CH2_SRC_ADDR) = (src_address))

#define HAL_DMAC_READ_CHANNEL2_SOURCE_ADDRESS(src_address) \
    ((src_address) = (DMAC_CH2_SRC_ADDR))

#define HAL_DMAC_WRITE_CHANNEL2_DESTINATION_ADDRESS(dst_address) \
    ((DMAC_CH2_DST_ADDR) = (dst_address))

#define HAL_DMAC_READ_CHANNEL2_DESTINATION_ADDRESS(dst_address) \
    ((dst_address) = (DMAC_CH2_DST_ADDR))

#define HAL_DMAC_WRITE_CHANNEL2_TRANSFER_SIZE(transfer_size) \
    ((DMAC_CH2_SIZE) = (transfer_size) & 0x0FFF)

#define HAL_DMAC_READ_CHANNEL2_TRANSFER_SIZE(transfer_size) \
    ((transfer_size) = (DMAC_CH2_SIZE) & 0x0FFF)

/*
 * Macros for Channel 3
 */
#define HAL_DMAC_WRITE_CHANNEL3_CONTROL(control_setting) \
    ((DMAC_CH3_CSR) = (control_setting))

#define HAL_DMAC_READ_CHANNEL3_CONTROL(control_setting) \
    ((control_setting) = (DMAC_CH3_CSR))

#define HAL_DMAC_ENABLE_CHANNEL3() \
    ((DMAC_CH3_CSR) |= (1))

#define HAL_DMAC_DISABLE_CHANNEL3() \
    ((DMAC_CH3_CSR) &= ~(1))

#define HAL_DMAC_CHANNEL3_TRANSACTION_ABORT() \
    ((DMAC_CH3_CSR) |= (DMAC_CH_ABT_TRANSFER << 15))

#define HAL_DMAC_ENABLE_CHANNEL3_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH3_CFG) &= ~(DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL3_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH3_CFG) |= (DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL3_ERROR_INTERRUPT() \
    ((DMAC_CH3_CFG) &= ~(DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL3_ERROR_INTERRUPT() \
    ((DMAC_CH3_CFG) |= (DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL3_ABORT_INTERRUPT() \
    ((DMAC_CH3_CFG) &= ~(DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL3_ABORT_INTERRUPT() \
    ((DMAC_CH3_CFG) |= (DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_WRITE_CHANNEL3_SOURCE_ADDRESS(src_address) \
    ((DMAC_CH3_SRC_ADDR) = (src_address))

#define HAL_DMAC_READ_CHANNEL3_SOURCE_ADDRESS(src_address) \
    ((src_address) = (DMAC_CH3_SRC_ADDR))

#define HAL_DMAC_WRITE_CHANNEL3_DESTINATION_ADDRESS(dst_address) \
    ((DMAC_CH3_DST_ADDR) = (dst_address))

#define HAL_DMAC_READ_CHANNEL3_DESTINATION_ADDRESS(dst_address) \
    ((dst_address) = (DMAC_CH3_DST_ADDR))

#define HAL_DMAC_WRITE_CHANNEL3_TRANSFER_SIZE(transfer_size) \
    ((DMAC_CH3_SIZE) = (transfer_size) & 0x0FFF)

#define HAL_DMAC_READ_CHANNEL3_TRANSFER_SIZE(transfer_size) \
    ((transfer_size) = (DMAC_CH3_SIZE) & 0x0FFF)

/*
 * Macros for Channel 4
 */
#define HAL_DMAC_WRITE_CHANNEL4_CONTROL(control_setting) \
    ((DMAC_CH4_CSR) = (control_setting))

#define HAL_DMAC_READ_CHANNEL4_CONTROL(control_setting) \
    ((control_setting) = (DMAC_CH4_CSR))

#define HAL_DMAC_ENABLE_CHANNEL4() \
    ((DMAC_CH4_CSR) |= (1))

#define HAL_DMAC_DISABLE_CHANNEL4() \
    ((DMAC_CH4_CSR) &= ~(1))

#define HAL_DMAC_CHANNEL4_TRANSACTION_ABORT() \
    ((DMAC_CH4_CSR) |= (DMAC_CH_ABT_TRANSFER << 15))

#define HAL_DMAC_ENABLE_CHANNEL4_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH4_CFG) &= ~(DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL4_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH4_CFG) |= (DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL4_ERROR_INTERRUPT() \
    ((DMAC_CH4_CFG) &= ~(DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL4_ERROR_INTERRUPT() \
    ((DMAC_CH4_CFG) |= (DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL4_ABORT_INTERRUPT() \
    ((DMAC_CH4_CFG) &= ~(DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL4_ABORT_INTERRUPT() \
    ((DMAC_CH4_CFG) |= (DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_WRITE_CHANNEL4_SOURCE_ADDRESS(src_address) \
    ((DMAC_CH4_SRC_ADDR) = (src_address))

#define HAL_DMAC_READ_CHANNEL4_SOURCE_ADDRESS(src_address) \
    ((src_address) = (DMAC_CH4_SRC_ADDR))

#define HAL_DMAC_WRITE_CHANNEL4_DESTINATION_ADDRESS(dst_address) \
    ((DMAC_CH4_DST_ADDR) = (dst_address))

#define HAL_DMAC_READ_CHANNEL4_DESTINATION_ADDRESS(dst_address) \
    ((dst_address) = (DMAC_CH4_DST_ADDR))

#define HAL_DMAC_WRITE_CHANNEL4_TRANSFER_SIZE(transfer_size) \
    ((DMAC_CH4_SIZE) = (transfer_size) & 0x0FFF)

#define HAL_DMAC_READ_CHANNEL4_TRANSFER_SIZE(transfer_size) \
    ((transfer_size) = (DMAC_CH4_SIZE) & 0x0FFF)

/*
 * Macros for Channel 5
 */
#define HAL_DMAC_WRITE_CHANNEL5_CONTROL(control_setting) \
    ((DMAC_CH5_CSR) = (control_setting))

#define HAL_DMAC_READ_CHANNEL5_CONTROL(control_setting) \
    ((control_setting) = (DMAC_CH5_CSR))

#define HAL_DMAC_ENABLE_CHANNEL5() \
    ((DMAC_CH5_CSR) |= (1))

#define HAL_DMAC_DISABLE_CHANNEL5() \
    ((DMAC_CH5_CSR) &= ~(1))

#define HAL_DMAC_CHANNEL5_TRANSACTION_ABORT() \
    ((DMAC_CH5_CSR) |= (DMAC_CH_ABT_TRANSFER << 15))

#define HAL_DMAC_ENABLE_CHANNEL5_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH5_CFG) &= ~(DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL5_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH5_CFG) |= (DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL5_ERROR_INTERRUPT() \
    ((DMAC_CH5_CFG) &= ~(DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL5_ERROR_INTERRUPT() \
    ((DMAC_CH5_CFG) |= (DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL5_ABORT_INTERRUPT() \
    ((DMAC_CH5_CFG) &= ~(DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL5_ABORT_INTERRUPT() \
    ((DMAC_CH5_CFG) |= (DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_WRITE_CHANNEL5_SOURCE_ADDRESS(src_address) \
    ((DMAC_CH5_SRC_ADDR) = (src_address))

#define HAL_DMAC_READ_CHANNEL5_SOURCE_ADDRESS(src_address) \
    ((src_address) = (DMAC_CH5_SRC_ADDR))

#define HAL_DMAC_WRITE_CHANNEL5_DESTINATION_ADDRESS(dst_address) \
    ((DMAC_CH5_DST_ADDR) = (dst_address))

#define HAL_DMAC_READ_CHANNEL5_DESTINATION_ADDRESS(dst_address) \
    ((dst_address) = (DMAC_CH5_DST_ADDR))

#define HAL_DMAC_WRITE_CHANNEL5_TRANSFER_SIZE(transfer_size) \
    ((DMAC_CH5_SIZE) = (transfer_size) & 0x0FFF)

#define HAL_DMAC_READ_CHANNEL5_TRANSFER_SIZE(transfer_size) \
    ((transfer_size) = (DMAC_CH5_SIZE) & 0x0FFF)

/*
 * Macros for Channel 6
 */
#define HAL_DMAC_WRITE_CHANNEL6_CONTROL(control_setting) \
    ((DMAC_CH6_CSR) = (control_setting))

#define HAL_DMAC_READ_CHANNEL6_CONTROL(control_setting) \
    ((control_setting) = (DMAC_CH6_CSR))

#define HAL_DMAC_ENABLE_CHANNEL6() \
    ((DMAC_CH6_CSR) |= (1))

#define HAL_DMAC_DISABLE_CHANNEL6() \
    ((DMAC_CH6_CSR) &= ~(1))

#define HAL_DMAC_CHANNEL6_TRANSACTION_ABORT() \
    ((DMAC_CH6_CSR) |= (DMAC_CH_ABT_TRANSFER << 16))

#define HAL_DMAC_ENABLE_CHANNEL6_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH6_CFG) &= ~(DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL6_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH6_CFG) |= (DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL6_ERROR_INTERRUPT() \
    ((DMAC_CH6_CFG) &= ~(DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL6_ERROR_INTERRUPT() \
    ((DMAC_CH6_CFG) |= (DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL6_ABORT_INTERRUPT() \
    ((DMAC_CH6_CFG) &= ~(DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL6_ABORT_INTERRUPT() \
    ((DMAC_CH6_CFG) |= (DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_WRITE_CHANNEL6_SOURCE_ADDRESS(src_address) \
    ((DMAC_CH6_SRC_ADDR) = (src_address))

#define HAL_DMAC_READ_CHANNEL6_SOURCE_ADDRESS(src_address) \
    ((src_address) = (DMAC_CH6_SRC_ADDR))

#define HAL_DMAC_WRITE_CHANNEL6_DESTINATION_ADDRESS(dst_address) \
    ((DMAC_CH6_DST_ADDR) = (dst_address))

#define HAL_DMAC_READ_CHANNEL6_DESTINATION_ADDRESS(dst_address) \
    ((dst_address) = (DMAC_CH6_DST_ADDR))

#define HAL_DMAC_WRITE_CHANNEL6_TRANSFER_SIZE(transfer_size) \
    ((DMAC_CH6_SIZE) = (transfer_size) & 0x0FFF)

#define HAL_DMAC_READ_CHANNEL6_TRANSFER_SIZE(transfer_size) \
    ((transfer_size) = (DMAC_CH6_SIZE) & 0x0FFF)

/*
 * Macros for Channel 7
 */
#define HAL_DMAC_WRITE_CHANNEL7_CONTROL(control_setting) \
    ((DMAC_CH7_CSR) = (control_setting))

#define HAL_DMAC_READ_CHANNEL7_CONTROL(control_setting) \
    ((control_setting) = (DMAC_CH7_CSR))

#define HAL_DMAC_ENABLE_CHANNEL7() \
    ((DMAC_CH7_CSR) |= (1))

#define HAL_DMAC_DISABLE_CHANNEL7() \
    ((DMAC_CH7_CSR) &= ~(1))

#define HAL_DMAC_CHANNEL7_TRANSACTION_ABORT() \
    ((DMAC_CH7_CSR) |= (DMAC_CH_ABT_TRANSFER << 17))

#define HAL_DMAC_ENABLE_CHANNEL7_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH7_CFG) &= ~(DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL7_TERMINAL_COUNT_INTERRUPT() \
    ((DMAC_CH7_CFG) |= (DMAC_CH_INT_TC_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL7_ERROR_INTERRUPT() \
    ((DMAC_CH7_CFG) &= ~(DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL7_ERROR_INTERRUPT() \
    ((DMAC_CH7_CFG) |= (DMAC_CH_INT_ERR_BIT_INDEX))

#define HAL_DMAC_ENABLE_CHANNEL7_ABORT_INTERRUPT() \
    ((DMAC_CH7_CFG) &= ~(DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_DISABLE_CHANNEL7_ABORT_INTERRUPT() \
    ((DMAC_CH7_CFG) |= (DMAC_CH_INT_ABT_BIT_INDEX))

#define HAL_DMAC_WRITE_CHANNEL7_SOURCE_ADDRESS(src_address) \
    ((DMAC_CH7_SRC_ADDR) = (src_address))

#define HAL_DMAC_READ_CHANNEL7_SOURCE_ADDRESS(src_address) \
    ((src_address) = (DMAC_CH7_SRC_ADDR))

#define HAL_DMAC_WRITE_CHANNEL7_DESTINATION_ADDRESS(dst_address) \
    ((DMAC_CH7_DST_ADDR) = (dst_address))

#define HAL_DMAC_READ_CHANNEL7_DESTINATION_ADDRESS(dst_address) \
    ((dst_address) = (DMAC_CH7_DST_ADDR))

#define HAL_DMAC_WRITE_CHANNEL7_TRANSFER_SIZE(transfer_size) \
    ((DMAC_CH7_SIZE) = (transfer_size) & 0x0FFF)

#define HAL_DMAC_READ_CHANNEL7_TRANSFER_SIZE(transfer_size) \
    ((transfer_size) = (DMAC_CH7_SIZE) & 0x0FFF)

#define DMAC_CH_CSR(idx)			DMAC_MEM_MAP_VALUE(0x100+0x20*idx)
#define DMAC_CH_CFG(idx)			DMAC_MEM_MAP_VALUE(0x104+0x20*idx)
#define DMAC_CH_SRC_ADDR(idx)			DMAC_MEM_MAP_VALUE(0x108+0x20*idx)
#define DMAC_CH_DST_ADDR(idx)			DMAC_MEM_MAP_VALUE(0x10C+0x20*idx)
#define DMAC_CH_LLP(idx)			DMAC_MEM_MAP_VALUE(0x110+0x20*idx)
#define DMAC_CH_SIZE(idx)			DMAC_MEM_MAP_VALUE(0x114+0x20*idx)

#define HAL_DMAC_ENABLE_CHANNEL(idx)		((DMAC_CH_CSR(idx)) |= (0x1))

#define HAL_DMAC_DISABLE_CHANNEL(idx)		((DMAC_CH_CSR(idx)) &= ~(0x1))

#define HAL_GET_DMAC_CHANNEL_LLP_COUNTER(ch)	((DMAC_CH_CFG(ch)>>16)&0xF)

#define DMAC_PER_CHANNEL_DESC_COUNT		4


/*DMAC HW Hand-shake target ID*/
#define DMAC_HW_HAND_SHAKE_PCM_TX0_ID		0
#define DMAC_HW_HAND_SHAKE_PCM_RX0_ID		1

#define DMAC_HW_HAND_SHAKE_SPI_TX_ID		2
#define DMAC_HW_HAND_SHAKE_SPI_RX_ID		3

#define DMAC_HW_HAND_SHAKE_I2S_TX_LEFT_ID	4
#define DMAC_HW_HAND_SHAKE_I2S_TX_RIGHT_ID	5

#define DMAC_HW_HAND_SHAKE_UART0_TX_ID		6
#define DMAC_HW_HAND_SHAKE_UART0_RX_ID		7

#define DMAC_HW_HAND_SHAKE_UART1_TX_ID		8
#define DMAC_HW_HAND_SHAKE_UART1_RX_ID		9

#define DMAC_HW_HAND_SHAKE_USBDEV_ID		0xA

#define DMAC_HW_HAND_SHAKE_I2S_RX_LEFT_ID	0xB
#define DMAC_HW_HAND_SHAKE_I2S_RX_RIGHT_ID	0xC

#define DMAC_HW_HAND_SHAKE_PCM_TX1_ID		0xD
#define DMAC_HW_HAND_SHAKE_PCM_RX1_ID		0xE

#endif  // end of #ifndef _STAR_DMAC_H_
