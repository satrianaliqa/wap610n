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

#ifdef CONFIG_NETFILTER
#include <linux/types.h>
#include <linux/inetdevice.h>
#include <linux/ip.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <net/protocol.h>
#include <net/ip.h>
#include <net/checksum.h>
#include <net/route.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv4/ip_nat_rule.h>
#include <linux/netfilter_ipv4/ip_tables.h>

#include <linux/str9100/star9100_shnat.h>
#include <linux/str9100/str9100_shnat_hook.h>

//#ifdef CONFIG_STAR9100_SHNAT_PCI_FASTPATH
struct net_device *pci_netdev[MAX_FP_PCIDEV];
EXPORT_SYMBOL(pci_netdev);
int (*star9100_shnat_pci_fp_forward_skb_ptr)(struct sk_buff *skb);
//#endif

int star9100_shnat_hook_ready;
int (*star9100_shnat_preadd_hnatable_hook)(u32 sip,u16 sport,u16 dport, u32 proto);
int (*star9100_shnat_check_shnat_enable_hook)(void);
int (*star9100_shnat_nf_nat_preadd_hnatable_hook)(const struct ip_conntrack *ct, int dir, const u16 port);
int (*star9100_shnat_nf_remove_hnatable_hook)(struct ip_conntrack *);
int (*star9100_shnat_nf_add_hnatable_hook)(const struct ip_conntrack *ct,const struct iphdr *iph, u16 proto);
int (*star9100_shnat_add_arptable_hook)(u32 myip, u32 targetip);
int (*star9100_shnat_fix_arptable_hook)(u32 myip, u32 targetip);
int (*star9100_shnat_nf_preadd_hnatable_hook)(const struct sk_buff **pskb);
int (*star9100_shnat_check_ftponly_enable_hook)(void);
int (*star9100_shnat_pci_fp_getdev_hook)(struct sk_buff *skb_ptr);
star9100_arp_table *(*star9100_shnat_getarptable_hook)( u32 ip_addr);


EXPORT_SYMBOL(star9100_shnat_hook_ready);
EXPORT_SYMBOL(star9100_shnat_preadd_hnatable_hook);
EXPORT_SYMBOL(star9100_shnat_check_shnat_enable_hook);
EXPORT_SYMBOL(star9100_shnat_nf_nat_preadd_hnatable_hook);
EXPORT_SYMBOL(star9100_shnat_nf_remove_hnatable_hook);
EXPORT_SYMBOL(star9100_shnat_nf_add_hnatable_hook);
EXPORT_SYMBOL(star9100_shnat_add_arptable_hook);
EXPORT_SYMBOL(star9100_shnat_fix_arptable_hook);
EXPORT_SYMBOL(star9100_shnat_nf_preadd_hnatable_hook);
EXPORT_SYMBOL(star9100_shnat_check_ftponly_enable_hook);
EXPORT_SYMBOL(star9100_shnat_getarptable_hook);
EXPORT_SYMBOL(star9100_shnat_pci_fp_getdev_hook);
EXPORT_SYMBOL(star9100_shnat_pci_fp_forward_skb_ptr);

#endif
