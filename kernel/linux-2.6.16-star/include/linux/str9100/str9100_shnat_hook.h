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

#ifndef __STR9100_SHNAT_HOOK_H__
#define __STR9100_SHNAT_HOOK_H__

#include <linux/config.h>

#ifdef CONFIG_NETFILTER

#if defined(CONFIG_STR9100_SHNAT) || defined(CONFIG_STR9100_SHNAT_MODULE)
#include <linux/str9100/star9100_shnat.h>

#define MAX_FP_PCIDEV 8
extern struct net_device *pci_netdev[MAX_FP_PCIDEV];
extern int (*star9100_shnat_pci_fp_forward_skb_ptr)(struct sk_buff *skb);

#define HNAT_SOURCE_PORT_BASE 50000

#define DUMP_TUPLE2(tp) \
	printk("tuple %p: %u %u.%u.%u.%u:%hu -> %u.%u.%u.%u:%hu\n", \
		(tp), (tp)->dst.protonum, \
		NIPQUAD((tp)->src.ip), ntohs((tp)->src.u.all), \
		NIPQUAD((tp)->dst.ip), ntohs((tp)->dst.u.all))

extern int star9100_shnat_hook_ready;
extern int (*star9100_shnat_preadd_hnatable_hook)(u32 sip,u16 sport,u16 dport, u32 proto);
extern int (*star9100_shnat_check_shnat_enable_hook)(void);
extern int (*star9100_shnat_nf_nat_preadd_hnatable_hook)(const struct ip_conntrack *ct, int dir, const u16 port);
extern int (*star9100_shnat_nf_remove_hnatable_hook)(struct ip_conntrack *);
extern int (*star9100_shnat_nf_add_hnatable_hook)(const struct ip_conntrack *ct,const struct iphdr *iph, u16 proto);
extern int (*star9100_shnat_add_arptable_hook)(u32 myip, u32 targetip);
extern int (*star9100_shnat_fix_arptable_hook)(u32 myip, u32 targetip);
extern int (*star9100_shnat_nf_preadd_hnatable_hook)(const struct sk_buff **pskb);
extern int (*star9100_shnat_check_ftponly_enable_hook)(void);
extern int (*star9100_shnat_pci_fp_getdev_hook)(struct sk_buff *skb_ptr);
extern star9100_arp_table *(*star9100_shnat_getarptable_hook)( u32 ip_addr);
#endif /* defined(CONFIG_STR9100_SHNAT) || defined(CONFIG_STR9100_SHNAT_MODULE) */


#endif /* CONFIG_NETFILTER */

#endif /* __STR9100_SHNAT_HOOK_H__ */
