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

#ifndef _STAR_HSDMAC_H_
#define _STAR_HSDMAC_H_

#include "star_sys_memory_map.h"

#if defined(__UBOOT__)
#define HSDMAC_MEM_MAP_VALUE(reg_offset)		(*((u32 volatile *)(SYSPA_HSDMAC_BASE_ADDR + reg_offset)))
#elif defined(__LINUX__)
#define HSDMAC_MEM_MAP_VALUE(reg_offset)		(*((u32 volatile *)(SYSVA_HSDMAC_BASE_ADDR + reg_offset)))
#else
#error "NO SYSTEM DEFINED"
#endif

/*
 * define access macros
 */
#define HSDMAC_CONTROL_STATUS_REG			HSDMAC_MEM_MAP_VALUE(0x040)
#define HSDMAC_MASTER0_ADDR_REG				HSDMAC_MEM_MAP_VALUE(0x050)
#define HSDMAC_MASTER1_ADDR_REG				HSDMAC_MEM_MAP_VALUE(0x054)
#define HSDMAC_LLP_REG					HSDMAC_MEM_MAP_VALUE(0x058)
#define HSDMAC_TOT_SIZE_REG				HSDMAC_MEM_MAP_VALUE(0x05C)



#define HSDMAC_TRANSFER_M0_TO_M1			(0)
#define HSDMAC_TRANSFER_M1_TO_M0			(1)
#define HSDMAC_M0_ADDRESS_CTRL_INC			(0)
#define HSDMAC_M0_ADDRESS_CTRL_FIX			(1)
#define HSDMAC_ENABLE_INTERRUPT				(0)
#define HSDMAC_DISABLE_INTERRUPT			(1)

/*
 * macro declarations
 */
#define HAL_HSDMAC_ENABLE_HSDMAC() \
{ \
    (HSDMAC_CONTROL_STATUS_REG) |= (0x1); \
}

#define HAL_HSDMAC_DISABLE_HSDMAC() \
{ \
    (HSDMAC_CONTROL_STATUS_REG) &= ~(0x1); \
}

#define HAL_HSDMAC_CONFIGURE_M0_ADDRESS_CONTROL_INCREMENTAL() \
{ \
    (HSDMAC_CONTROL_STATUS_REG) &= ~(0x1 << 2); \
}

#define HAL_HSDMAC_CONFIGURE_M0_ADDRESS_CONTROL_FIX() \
{ \
    (HSDMAC_CONTROL_STATUS_REG) |= (0x1 << 2); \
}

#define HAL_HSDMAC_ENABLE_HARDWARE_HANDSHAKE_MODE() \
{ \
    (HSDMAC_CONTROL_STATUS_REG) |= (0x1 << 3); \
}

#define HAL_HSDMAC_ENABLE_NORMAL_MODE() \
{ \
    (HSDMAC_CONTROL_STATUS_REG) &= ~(0x1 << 3); \
}

#define HAL_HSDMAC_GET_LLP_COUNTER(llp_counter) \
{ \
    llp_counter = ((HSDMAC_CONTROL_STATUS_REG >> 8) & 0xF); \
}

#define HAL_HSDMAC_CLEAR_MASTER0_ERROR_RESPONSE() \
{ \
    (HSDMAC_CONTROL_STATUS_REG) &= ~(0x1); \
}

#define HAL_HSDMAC_SET_MASTER0_ADDRESS(master0_address) \
{ \
    (HSDMAC_MASTER0_ADDR_REG) = (master0_address); \
}

#define HAL_HSDMAC_SET_MASTER1_ADDRESS(master1_address) \
{ \
    (HSDMAC_MASTER1_ADDR_REG) = (master1_address); \
}

#define HAL_HSDMAC_SET_LLP_ADDRESS(llp_address) \
{ \
    (HSDMAC_LLP_REG) = (llp_address); \
}

#define HAL_HSDMAC_DISABLE_LLP() \
{ \
    (HSDMAC_LLP_REG) = (0x0); \
}

#define HAL_HSDMAC_CONFIGURE_TRANSFER_M0_TO_M1() \
{ \
    (HSDMAC_TOT_SIZE_REG) &= ~(0x1 << 29); \
}

#define HAL_HSDMAC_CONFIGURE_TRANSFER_M1_TO_M0() \
{ \
    (HSDMAC_TOT_SIZE_REG) |= (0x1 << 29); \
}

#endif  // end of #ifndef _STAR_DMAC_H_
