/*
 * mini_nat.c -- Lightweight C89 userspace iptables NAT & SPI firewall manager
 * for Linux 2.6.16-star (Cisco Linksys WAP610N, 32 MB RAM / 4 MB NOR Flash).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ip_nat.h>
#include <linux/netfilter_ipv4/ipt_TCPMSS.h>
#include <linux/netfilter/nf_conntrack_common.h>
#include <linux/netfilter/xt_state.h>

#define MN_MAX_FWD 32
#define MN_ENTRY_SIZE XT_ALIGN(sizeof(struct ipt_entry))
#define MN_STD_TGT_SIZE XT_ALIGN(sizeof(struct ipt_entry_target) + XT_ALIGN(sizeof(int)))
#define MN_ERR_TGT_SIZE XT_ALIGN(sizeof(struct ipt_entry_target) + XT_ALIGN(XT_FUNCTION_MAXNAMELEN))
#define MN_MATCH_TCP_SIZE XT_ALIGN(sizeof(struct ipt_entry_match) + XT_ALIGN(sizeof(struct xt_tcp)))
#define MN_MATCH_UDP_SIZE XT_ALIGN(sizeof(struct ipt_entry_match) + XT_ALIGN(sizeof(struct xt_udp)))
#define MN_MATCH_STATE_SIZE XT_ALIGN(sizeof(struct ipt_entry_match) + XT_ALIGN(sizeof(struct xt_state_info)))
#define MN_TGT_NAT_SIZE XT_ALIGN(sizeof(struct ipt_entry_target) + XT_ALIGN(sizeof(struct ip_nat_multi_range_compat)))
#define MN_TGT_MSS_SIZE XT_ALIGN(sizeof(struct ipt_entry_target) + XT_ALIGN(sizeof(struct ipt_tcpmss_info)))

typedef struct {
    unsigned char *data;
    size_t len;
    size_t cap;
} mn_buf_t;

typedef struct {
    int proto; /* 0 = both, IPPROTO_TCP, IPPROTO_UDP */
    unsigned short ext_start;
    unsigned short ext_end;
    struct in_addr lan_ip;
    unsigned short int_port;
} mn_fwd_t;

typedef struct {
    char mode[16];
    char wan_if[IFNAMSIZ];
    char lan_if[IFNAMSIZ];
    int spi;
    int wan_ping;
    int wan_mgmt;
    int wan_ssh;
    int tcpmss;
    int has_dmz;
    struct in_addr dmz_ip;
    mn_fwd_t fwd[MN_MAX_FWD];
    int n_fwd;
} mn_cfg_t;

static int buf_reserve(mn_buf_t *b, size_t extra)
{
    size_t need;
    size_t new_cap;
    unsigned char *p;

    need = b->len + extra;
    if (need <= b->cap) {
        return 0;
    }
    new_cap = b->cap ? b->cap : 4096;
    while (new_cap < need) {
        new_cap *= 2;
    }
    p = (unsigned char *)realloc(b->data, new_cap);
    if (!p) {
        return -1;
    }
    b->data = p;
    b->cap = new_cap;
    return 0;
}

static unsigned char *buf_alloc_zero(mn_buf_t *b, size_t n)
{
    unsigned char *p;

    if (buf_reserve(b, n) != 0) {
        return NULL;
    }
    p = b->data + b->len;
    memset(p, 0, n);
    b->len += n;
    return p;
}

static void init_entry_iface(struct ipt_entry *e, const char *in_if, const char *out_if)
{
    if (in_if && in_if[0] != '\0') {
        size_t l = strlen(in_if);
        if (l >= IFNAMSIZ) {
            l = IFNAMSIZ - 1;
        }
        memcpy(e->ip.iniface, in_if, l);
        memset(e->ip.iniface_mask, 0xff, l + 1);
    }
    if (out_if && out_if[0] != '\0') {
        size_t l = strlen(out_if);
        if (l >= IFNAMSIZ) {
            l = IFNAMSIZ - 1;
        }
        memcpy(e->ip.outiface, out_if, l);
        memset(e->ip.outiface_mask, 0xff, l + 1);
    }
}

static int emit_std_verdict(mn_buf_t *b, const char *in_if, const char *out_if,
                            u_int16_t proto, u_int32_t dst_ip, u_int32_t dst_mask,
                            int verdict)
{
    size_t total = MN_ENTRY_SIZE + MN_STD_TGT_SIZE;
    unsigned char *raw;
    struct ipt_entry *e;
    struct ipt_entry_target *t;
    int *vp;

    raw = buf_alloc_zero(b, total);
    if (!raw) {
        return -1;
    }
    e = (struct ipt_entry *)raw;
    init_entry_iface(e, in_if, out_if);
    e->ip.proto = proto;
    e->ip.dst.s_addr = dst_ip;
    e->ip.dmsk.s_addr = dst_mask;
    e->target_offset = (u_int16_t)MN_ENTRY_SIZE;
    e->next_offset = (u_int16_t)total;

    t = (struct ipt_entry_target *)(raw + MN_ENTRY_SIZE);
    t->u.user.target_size = (u_int16_t)MN_STD_TGT_SIZE;
    strcpy(t->u.user.name, XT_STANDARD_TARGET);
    vp = (int *)t->data;
    *vp = -verdict - 1;
    return 0;
}

static int emit_error_term(mn_buf_t *b)
{
    size_t total = MN_ENTRY_SIZE + MN_ERR_TGT_SIZE;
    unsigned char *raw;
    struct ipt_entry *e;
    struct ipt_entry_target *t;

    raw = buf_alloc_zero(b, total);
    if (!raw) {
        return -1;
    }
    e = (struct ipt_entry *)raw;
    e->target_offset = (u_int16_t)MN_ENTRY_SIZE;
    e->next_offset = (u_int16_t)total;

    t = (struct ipt_entry_target *)(raw + MN_ENTRY_SIZE);
    t->u.user.target_size = (u_int16_t)MN_ERR_TGT_SIZE;
    strcpy(t->u.user.name, XT_ERROR_TARGET);
    strcpy((char *)t->data, "ERROR");
    return 0;
}

static int emit_state_std(mn_buf_t *b, const char *in_if, const char *out_if,
                          unsigned int statemask, int verdict)
{
    size_t total = MN_ENTRY_SIZE + MN_MATCH_STATE_SIZE + MN_STD_TGT_SIZE;
    unsigned char *raw;
    struct ipt_entry *e;
    struct ipt_entry_match *m;
    struct xt_state_info *si;
    struct ipt_entry_target *t;
    int *vp;

    raw = buf_alloc_zero(b, total);
    if (!raw) {
        return -1;
    }
    e = (struct ipt_entry *)raw;
    init_entry_iface(e, in_if, out_if);
    e->target_offset = (u_int16_t)(MN_ENTRY_SIZE + MN_MATCH_STATE_SIZE);
    e->next_offset = (u_int16_t)total;

    m = (struct ipt_entry_match *)(raw + MN_ENTRY_SIZE);
    m->u.user.match_size = (u_int16_t)MN_MATCH_STATE_SIZE;
    strcpy(m->u.user.name, "state");
    si = (struct xt_state_info *)m->data;
    si->statemask = statemask;

    t = (struct ipt_entry_target *)(raw + e->target_offset);
    t->u.user.target_size = (u_int16_t)MN_STD_TGT_SIZE;
    strcpy(t->u.user.name, XT_STANDARD_TARGET);
    vp = (int *)t->data;
    *vp = -verdict - 1;
    return 0;
}

static void fill_port_match(unsigned char *dst, u_int16_t proto,
                            unsigned short dpt_min, unsigned short dpt_max)
{
    struct ipt_entry_match *m = (struct ipt_entry_match *)dst;

    if (proto == IPPROTO_TCP) {
        struct xt_tcp *tp = (struct xt_tcp *)m->data;
        m->u.user.match_size = (u_int16_t)MN_MATCH_TCP_SIZE;
        strcpy(m->u.user.name, "tcp");
        tp->spts[0] = 0;
        tp->spts[1] = 65535;
        tp->dpts[0] = dpt_min;
        tp->dpts[1] = dpt_max;
    } else {
        struct xt_udp *up = (struct xt_udp *)m->data;
        m->u.user.match_size = (u_int16_t)MN_MATCH_UDP_SIZE;
        strcpy(m->u.user.name, "udp");
        up->spts[0] = 0;
        up->spts[1] = 65535;
        up->dpts[0] = dpt_min;
        up->dpts[1] = dpt_max;
    }
}

static int emit_port_std(mn_buf_t *b, const char *in_if, const char *out_if,
                         u_int32_t dst_ip, u_int32_t dst_mask, u_int16_t proto,
                         unsigned short dpt_min, unsigned short dpt_max, int verdict)
{
    size_t msz = (proto == IPPROTO_TCP) ? MN_MATCH_TCP_SIZE : MN_MATCH_UDP_SIZE;
    size_t total = MN_ENTRY_SIZE + msz + MN_STD_TGT_SIZE;
    unsigned char *raw;
    struct ipt_entry *e;
    struct ipt_entry_target *t;
    int *vp;

    raw = buf_alloc_zero(b, total);
    if (!raw) {
        return -1;
    }
    e = (struct ipt_entry *)raw;
    init_entry_iface(e, in_if, out_if);
    e->ip.proto = proto;
    e->ip.dst.s_addr = dst_ip;
    e->ip.dmsk.s_addr = dst_mask;
    e->target_offset = (u_int16_t)(MN_ENTRY_SIZE + msz);
    e->next_offset = (u_int16_t)total;

    fill_port_match(raw + MN_ENTRY_SIZE, proto, dpt_min, dpt_max);

    t = (struct ipt_entry_target *)(raw + e->target_offset);
    t->u.user.target_size = (u_int16_t)MN_STD_TGT_SIZE;
    strcpy(t->u.user.name, XT_STANDARD_TARGET);
    vp = (int *)t->data;
    *vp = -verdict - 1;
    return 0;
}

static int emit_tcpmss_clamp(mn_buf_t *b)
{
    size_t total = MN_ENTRY_SIZE + MN_MATCH_TCP_SIZE + MN_TGT_MSS_SIZE;
    unsigned char *raw;
    struct ipt_entry *e;
    struct ipt_entry_match *m;
    struct xt_tcp *tp;
    struct ipt_entry_target *t;
    struct ipt_tcpmss_info *mi;

    raw = buf_alloc_zero(b, total);
    if (!raw) {
        return -1;
    }
    e = (struct ipt_entry *)raw;
    e->ip.proto = IPPROTO_TCP;
    e->target_offset = (u_int16_t)(MN_ENTRY_SIZE + MN_MATCH_TCP_SIZE);
    e->next_offset = (u_int16_t)total;

    m = (struct ipt_entry_match *)(raw + MN_ENTRY_SIZE);
    m->u.user.match_size = (u_int16_t)MN_MATCH_TCP_SIZE;
    strcpy(m->u.user.name, "tcp");
    tp = (struct xt_tcp *)m->data;
    tp->spts[0] = 0;
    tp->spts[1] = 65535;
    tp->dpts[0] = 0;
    tp->dpts[1] = 65535;
    tp->flg_mask = 0x06; /* SYN | RST */
    tp->flg_cmp = 0x02;  /* SYN */

    t = (struct ipt_entry_target *)(raw + e->target_offset);
    t->u.user.target_size = (u_int16_t)MN_TGT_MSS_SIZE;
    strcpy(t->u.user.name, "TCPMSS");
    mi = (struct ipt_tcpmss_info *)t->data;
    mi->mss = IPT_TCPMSS_CLAMP_PMTU;
    return 0;
}

static int emit_masquerade(mn_buf_t *b, const char *wan_if)
{
    size_t total = MN_ENTRY_SIZE + MN_TGT_NAT_SIZE;
    unsigned char *raw;
    struct ipt_entry *e;
    struct ipt_entry_target *t;
    struct ip_nat_multi_range_compat *mr;

    raw = buf_alloc_zero(b, total);
    if (!raw) {
        return -1;
    }
    e = (struct ipt_entry *)raw;
    init_entry_iface(e, NULL, wan_if);
    e->target_offset = (u_int16_t)MN_ENTRY_SIZE;
    e->next_offset = (u_int16_t)total;

    t = (struct ipt_entry_target *)(raw + MN_ENTRY_SIZE);
    t->u.user.target_size = (u_int16_t)MN_TGT_NAT_SIZE;
    strcpy(t->u.user.name, "MASQUERADE");
    mr = (struct ip_nat_multi_range_compat *)t->data;
    mr->rangesize = 1;
    mr->range[0].flags = 0;
    return 0;
}

static int emit_dnat(mn_buf_t *b, const char *wan_if, u_int16_t proto,
                     unsigned short dpt_min, unsigned short dpt_max,
                     int use_port_match, u_int32_t to_ip, unsigned short to_port)
{
    size_t msz = 0;
    size_t total;
    unsigned char *raw;
    struct ipt_entry *e;
    struct ipt_entry_target *t;
    struct ip_nat_multi_range_compat *mr;

    if (use_port_match) {
        msz = (proto == IPPROTO_TCP) ? MN_MATCH_TCP_SIZE : MN_MATCH_UDP_SIZE;
    }
    total = MN_ENTRY_SIZE + msz + MN_TGT_NAT_SIZE;
    raw = buf_alloc_zero(b, total);
    if (!raw) {
        return -1;
    }
    e = (struct ipt_entry *)raw;
    init_entry_iface(e, wan_if, NULL);
    e->ip.proto = proto;
    e->target_offset = (u_int16_t)(MN_ENTRY_SIZE + msz);
    e->next_offset = (u_int16_t)total;

    if (use_port_match) {
        fill_port_match(raw + MN_ENTRY_SIZE, proto, dpt_min, dpt_max);
    }

    t = (struct ipt_entry_target *)(raw + e->target_offset);
    t->u.user.target_size = (u_int16_t)MN_TGT_NAT_SIZE;
    strcpy(t->u.user.name, "DNAT");
    mr = (struct ip_nat_multi_range_compat *)t->data;
    mr->rangesize = 1;
    mr->range[0].flags = IP_NAT_RANGE_MAP_IPS;
    mr->range[0].min_ip = to_ip;
    mr->range[0].max_ip = to_ip;
    if (to_port > 0) {
        mr->range[0].flags |= IP_NAT_RANGE_PROTO_SPECIFIED;
        mr->range[0].min.tcp.port = htons(to_port);
        mr->range[0].max.tcp.port = htons(to_port);
    }
    return 0;
}

static int replace_table(int sock, const char *name, const mn_buf_t *b,
                         const unsigned int hook_entry[NF_IP_NUMHOOKS],
                         const unsigned int underflow[NF_IP_NUMHOOKS],
                         unsigned int num_entries)
{
    struct ipt_getinfo info;
    socklen_t infolen = (socklen_t)sizeof(info);
    struct xt_counters *old_cnt;
    struct ipt_replace *repl;
    int rc;

    memset(&info, 0, sizeof(info));
    strncpy(info.name, name, IPT_TABLE_MAXNAMELEN - 1);
    if (getsockopt(sock, SOL_IP, IPT_SO_GET_INFO, &info, &infolen) != 0) {
        fprintf(stderr, "mini-nat: getsockopt(%s) failed: %s\n", name, strerror(errno));
        return -1;
    }

    old_cnt = (struct xt_counters *)calloc(info.num_entries ? info.num_entries : 1,
                                           sizeof(struct xt_counters));
    if (!old_cnt) {
        return -1;
    }

    repl = (struct ipt_replace *)calloc(1, sizeof(struct ipt_replace) + b->len);
    if (!repl) {
        free(old_cnt);
        return -1;
    }

    strncpy(repl->name, name, IPT_TABLE_MAXNAMELEN - 1);
    repl->valid_hooks = info.valid_hooks;
    repl->num_entries = num_entries;
    repl->size = (unsigned int)b->len;
    memcpy(repl->hook_entry, hook_entry, sizeof(repl->hook_entry));
    memcpy(repl->underflow, underflow, sizeof(repl->underflow));
    repl->num_counters = info.num_entries;
    repl->counters = old_cnt;
    memcpy(repl->entries, b->data, b->len);

    rc = setsockopt(sock, SOL_IP, IPT_SO_SET_REPLACE, repl,
                    (socklen_t)(sizeof(struct ipt_replace) + b->len));
    if (rc != 0) {
        fprintf(stderr, "mini-nat: setsockopt(%s) failed: %s\n", name, strerror(errno));
    }

    free(repl);
    free(old_cnt);
    return rc;
}

static int apply_empty_table(int sock, const char *name, const int hooks[3])
{
    mn_buf_t b;
    unsigned int he[NF_IP_NUMHOOKS];
    unsigned int uf[NF_IP_NUMHOOKS];
    int i;
    int rc;

    memset(&b, 0, sizeof(b));
    memset(he, 0, sizeof(he));
    memset(uf, 0, sizeof(uf));

    for (i = 0; i < 3; i++) {
        he[hooks[i]] = (unsigned int)b.len;
        uf[hooks[i]] = (unsigned int)b.len;
        if (emit_std_verdict(&b, NULL, NULL, 0, 0, 0, NF_ACCEPT) != 0) {
            free(b.data);
            return -1;
        }
    }
    if (emit_error_term(&b) != 0) {
        free(b.data);
        return -1;
    }
    rc = replace_table(sock, name, &b, he, uf, 4);
    free(b.data);
    return rc;
}

static int flush_all(int sock)
{
    static const int nat_hooks[3] = { NF_IP_PRE_ROUTING, NF_IP_POST_ROUTING, NF_IP_LOCAL_OUT };
    static const int flt_hooks[3] = { NF_IP_LOCAL_IN, NF_IP_FORWARD, NF_IP_LOCAL_OUT };
    int r1 = apply_empty_table(sock, "nat", nat_hooks);
    int r2 = apply_empty_table(sock, "filter", flt_hooks);
    return (r1 == 0 && r2 == 0) ? 0 : -1;
}

static int apply_config(int sock, const mn_cfg_t *cfg)
{
    mn_buf_t nb;
    mn_buf_t fb;
    unsigned int n_he[NF_IP_NUMHOOKS];
    unsigned int n_uf[NF_IP_NUMHOOKS];
    unsigned int f_he[NF_IP_NUMHOOKS];
    unsigned int f_uf[NF_IP_NUMHOOKS];
    unsigned int n_cnt = 0;
    unsigned int f_cnt = 0;
    unsigned int est_rel = XT_STATE_BIT(IP_CT_ESTABLISHED) | XT_STATE_BIT(IP_CT_RELATED);
    u_int32_t full_mask = htonl(0xffffffffUL);
    int i;
    int rc = 0;

    if (strcmp(cfg->mode, "bridge") == 0 || cfg->wan_if[0] == '\0') {
        return flush_all(sock);
    }

    memset(&nb, 0, sizeof(nb));
    memset(&fb, 0, sizeof(fb));
    memset(n_he, 0, sizeof(n_he));
    memset(n_uf, 0, sizeof(n_uf));
    memset(f_he, 0, sizeof(f_he));
    memset(f_uf, 0, sizeof(f_uf));

    /* --- NAT table: PRE_ROUTING -> POST_ROUTING -> LOCAL_OUT -> ERROR --- */
    n_he[NF_IP_PRE_ROUTING] = (unsigned int)nb.len;
    for (i = 0; i < cfg->n_fwd; i++) {
        const mn_fwd_t *fw = &cfg->fwd[i];
        unsigned short to_port = (fw->ext_start == fw->ext_end) ? fw->int_port : 0;
        if (fw->proto == 0 || fw->proto == IPPROTO_TCP) {
            if (emit_dnat(&nb, cfg->wan_if, IPPROTO_TCP, fw->ext_start, fw->ext_end,
                          1, fw->lan_ip.s_addr, to_port) != 0) {
                goto fail;
            }
            n_cnt++;
        }
        if (fw->proto == 0 || fw->proto == IPPROTO_UDP) {
            if (emit_dnat(&nb, cfg->wan_if, IPPROTO_UDP, fw->ext_start, fw->ext_end,
                          1, fw->lan_ip.s_addr, to_port) != 0) {
                goto fail;
            }
            n_cnt++;
        }
    }
    if (cfg->has_dmz) {
        if (cfg->wan_mgmt) {
            if (emit_port_std(&nb, cfg->wan_if, NULL, 0, 0, IPPROTO_TCP, 80, 80, NF_ACCEPT) != 0) {
                goto fail;
            }
            n_cnt++;
        }
        if (cfg->wan_ssh) {
            if (emit_port_std(&nb, cfg->wan_if, NULL, 0, 0, IPPROTO_TCP, 22, 22, NF_ACCEPT) != 0) {
                goto fail;
            }
            n_cnt++;
        }
        if (emit_dnat(&nb, cfg->wan_if, IPPROTO_TCP, 0, 0, 0, cfg->dmz_ip.s_addr, 0) != 0) {
            goto fail;
        }
        n_cnt++;
        if (emit_dnat(&nb, cfg->wan_if, IPPROTO_UDP, 0, 0, 0, cfg->dmz_ip.s_addr, 0) != 0) {
            goto fail;
        }
        n_cnt++;
    }
    n_uf[NF_IP_PRE_ROUTING] = (unsigned int)nb.len;
    if (emit_std_verdict(&nb, NULL, NULL, 0, 0, 0, NF_ACCEPT) != 0) {
        goto fail;
    }
    n_cnt++;

    n_he[NF_IP_POST_ROUTING] = (unsigned int)nb.len;
    if (emit_masquerade(&nb, cfg->wan_if) != 0) {
        goto fail;
    }
    n_cnt++;
    n_uf[NF_IP_POST_ROUTING] = (unsigned int)nb.len;
    if (emit_std_verdict(&nb, NULL, NULL, 0, 0, 0, NF_ACCEPT) != 0) {
        goto fail;
    }
    n_cnt++;

    n_he[NF_IP_LOCAL_OUT] = (unsigned int)nb.len;
    n_uf[NF_IP_LOCAL_OUT] = (unsigned int)nb.len;
    if (emit_std_verdict(&nb, NULL, NULL, 0, 0, 0, NF_ACCEPT) != 0) {
        goto fail;
    }
    n_cnt++;

    if (emit_error_term(&nb) != 0) {
        goto fail;
    }
    n_cnt++;

    /* --- FILTER table: LOCAL_IN -> FORWARD -> LOCAL_OUT -> ERROR --- */
    f_he[NF_IP_LOCAL_IN] = (unsigned int)fb.len;
    if (emit_std_verdict(&fb, "lo", NULL, 0, 0, 0, NF_ACCEPT) != 0) {
        goto fail;
    }
    f_cnt++;
    if (emit_std_verdict(&fb, cfg->lan_if, NULL, 0, 0, 0, NF_ACCEPT) != 0) {
        goto fail;
    }
    f_cnt++;
    if (emit_state_std(&fb, NULL, NULL, est_rel, NF_ACCEPT) != 0) {
        goto fail;
    }
    f_cnt++;
    if (cfg->spi) {
        if (emit_state_std(&fb, NULL, NULL, XT_STATE_INVALID, NF_DROP) != 0) {
            goto fail;
        }
        f_cnt++;
    }
    if (cfg->wan_ping) {
        if (emit_std_verdict(&fb, cfg->wan_if, NULL, IPPROTO_ICMP, 0, 0, NF_ACCEPT) != 0) {
            goto fail;
        }
        f_cnt++;
    }
    if (cfg->wan_mgmt) {
        if (emit_port_std(&fb, cfg->wan_if, NULL, 0, 0, IPPROTO_TCP, 80, 80, NF_ACCEPT) != 0) {
            goto fail;
        }
        f_cnt++;
    }
    if (cfg->wan_ssh) {
        if (emit_port_std(&fb, cfg->wan_if, NULL, 0, 0, IPPROTO_TCP, 22, 22, NF_ACCEPT) != 0) {
            goto fail;
        }
        f_cnt++;
    }
    if (cfg->spi) {
        if (emit_std_verdict(&fb, cfg->wan_if, NULL, 0, 0, 0, NF_DROP) != 0) {
            goto fail;
        }
        f_cnt++;
    }
    f_uf[NF_IP_LOCAL_IN] = (unsigned int)fb.len;
    if (emit_std_verdict(&fb, NULL, NULL, 0, 0, 0, NF_ACCEPT) != 0) {
        goto fail;
    }
    f_cnt++;

    f_he[NF_IP_FORWARD] = (unsigned int)fb.len;
    if (cfg->tcpmss) {
        if (emit_tcpmss_clamp(&fb) != 0) {
            goto fail;
        }
        f_cnt++;
    }
    if (emit_state_std(&fb, NULL, NULL, est_rel, NF_ACCEPT) != 0) {
        goto fail;
    }
    f_cnt++;
    if (cfg->spi) {
        if (emit_state_std(&fb, NULL, NULL, XT_STATE_INVALID, NF_DROP) != 0) {
            goto fail;
        }
        f_cnt++;
    }
    if (emit_std_verdict(&fb, cfg->lan_if, cfg->wan_if, 0, 0, 0, NF_ACCEPT) != 0) {
        goto fail;
    }
    f_cnt++;

    for (i = 0; i < cfg->n_fwd; i++) {
        const mn_fwd_t *fw = &cfg->fwd[i];
        unsigned short p0 = fw->ext_start;
        unsigned short p1 = fw->ext_end;
        if (fw->ext_start == fw->ext_end && fw->int_port > 0) {
            p0 = fw->int_port;
            p1 = fw->int_port;
        }
        if (fw->proto == 0 || fw->proto == IPPROTO_TCP) {
            if (emit_port_std(&fb, cfg->wan_if, cfg->lan_if, fw->lan_ip.s_addr,
                              full_mask, IPPROTO_TCP, p0, p1, NF_ACCEPT) != 0) {
                goto fail;
            }
            f_cnt++;
        }
        if (fw->proto == 0 || fw->proto == IPPROTO_UDP) {
            if (emit_port_std(&fb, cfg->wan_if, cfg->lan_if, fw->lan_ip.s_addr,
                              full_mask, IPPROTO_UDP, p0, p1, NF_ACCEPT) != 0) {
                goto fail;
            }
            f_cnt++;
        }
    }
    if (cfg->has_dmz) {
        if (emit_std_verdict(&fb, cfg->wan_if, cfg->lan_if, 0,
                             cfg->dmz_ip.s_addr, full_mask, NF_ACCEPT) != 0) {
            goto fail;
        }
        f_cnt++;
    }
    if (cfg->spi) {
        if (emit_std_verdict(&fb, cfg->wan_if, NULL, 0, 0, 0, NF_DROP) != 0) {
            goto fail;
        }
        f_cnt++;
    }
    f_uf[NF_IP_FORWARD] = (unsigned int)fb.len;
    if (emit_std_verdict(&fb, NULL, NULL, 0, 0, 0, NF_ACCEPT) != 0) {
        goto fail;
    }
    f_cnt++;

    f_he[NF_IP_LOCAL_OUT] = (unsigned int)fb.len;
    f_uf[NF_IP_LOCAL_OUT] = (unsigned int)fb.len;
    if (emit_std_verdict(&fb, NULL, NULL, 0, 0, 0, NF_ACCEPT) != 0) {
        goto fail;
    }
    f_cnt++;

    if (emit_error_term(&fb) != 0) {
        goto fail;
    }
    f_cnt++;

    if (replace_table(sock, "nat", &nb, n_he, n_uf, n_cnt) != 0 ||
        replace_table(sock, "filter", &fb, f_he, f_uf, f_cnt) != 0) {
        rc = -1;
    }
    free(nb.data);
    free(fb.data);
    return rc;

fail:
    free(nb.data);
    free(fb.data);
    return -1;
}

static int parse_fwd_line(const char *s, mn_fwd_t *fw)
{
    char buf[128];
    char *p1, *p2, *p3, *p4;
    long v1, v2, v3;

    memset(fw, 0, sizeof(*fw));
    snprintf(buf, sizeof(buf), "%s", s);
    buf[sizeof(buf) - 1] = '\0';

    p1 = strchr(buf, '|');
    if (!p1) return -1;
    *p1++ = '\0';
    p2 = strchr(p1, '|');
    if (!p2) return -1;
    *p2++ = '\0';
    p3 = strchr(p2, '|');
    if (!p3) return -1;
    *p3++ = '\0';
    p4 = strchr(p3, '|');
    if (!p4) return -1;
    *p4++ = '\0';

    if (strcmp(buf, "tcp") == 0) fw->proto = IPPROTO_TCP;
    else if (strcmp(buf, "udp") == 0) fw->proto = IPPROTO_UDP;
    else if (strcmp(buf, "both") == 0) fw->proto = 0;
    else return -1;

    v1 = strtol(p1, NULL, 10);
    v2 = strtol(p2, NULL, 10);
    v3 = strtol(p4, NULL, 10);
    if (v1 < 1 || v1 > 65535 || v2 < v1 || v2 > 65535 || v3 < 0 || v3 > 65535) {
        return -1;
    }
    fw->ext_start = (unsigned short)v1;
    fw->ext_end = (unsigned short)v2;
    fw->int_port = (unsigned short)v3;
    if (inet_aton(p3, &fw->lan_ip) == 0) {
        return -1;
    }
    return 0;
}

static int load_config(const char *path, mn_cfg_t *cfg)
{
    FILE *fp;
    char line[256];

    memset(cfg, 0, sizeof(*cfg));
    strcpy(cfg->mode, "bridge");
    strcpy(cfg->lan_if, "br0");
    cfg->spi = 1;
    cfg->tcpmss = 1;

    fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "mini-nat: cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }
    while (fgets(line, sizeof(line), fp) != NULL) {
        size_t l = strlen(line);
        char *eq, *k, *v;

        while (l > 0 && (line[l - 1] == '\n' || line[l - 1] == '\r' ||
                         line[l - 1] == ' ' || line[l - 1] == '\t')) {
            line[--l] = '\0';
        }
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        eq = strchr(line, '=');
        if (!eq) {
            continue;
        }
        *eq = '\0';
        k = line;
        v = eq + 1;

        if (strcmp(k, "MODE") == 0) {
            snprintf(cfg->mode, sizeof(cfg->mode), "%s", v);
        } else if (strcmp(k, "WAN_IF") == 0) {
            snprintf(cfg->wan_if, sizeof(cfg->wan_if), "%s", v);
        } else if (strcmp(k, "LAN_IF") == 0 && v[0] != '\0') {
            snprintf(cfg->lan_if, sizeof(cfg->lan_if), "%s", v);
        } else if (strcmp(k, "SPI") == 0) {
            cfg->spi = (atoi(v) != 0);
        } else if (strcmp(k, "WAN_PING") == 0) {
            cfg->wan_ping = (atoi(v) != 0);
        } else if (strcmp(k, "WAN_MGMT") == 0) {
            cfg->wan_mgmt = (atoi(v) != 0);
        } else if (strcmp(k, "WAN_SSH") == 0) {
            cfg->wan_ssh = (atoi(v) != 0);
        } else if (strcmp(k, "TCPMSS") == 0) {
            cfg->tcpmss = (atoi(v) != 0);
        } else if (strcmp(k, "DMZ_IP") == 0 && v[0] != '\0') {
            if (inet_aton(v, &cfg->dmz_ip) != 0 && cfg->dmz_ip.s_addr != 0) {
                cfg->has_dmz = 1;
            }
        } else if (strcmp(k, "FWD") == 0 && cfg->n_fwd < MN_MAX_FWD) {
            if (parse_fwd_line(v, &cfg->fwd[cfg->n_fwd]) == 0) {
                cfg->n_fwd++;
            }
        }
    }
    fclose(fp);
    return 0;
}

static const char *hook_name(unsigned int h)
{
    switch (h) {
        case NF_IP_PRE_ROUTING:  return "PREROUTING";
        case NF_IP_LOCAL_IN:     return "INPUT";
        case NF_IP_FORWARD:      return "FORWARD";
        case NF_IP_LOCAL_OUT:    return "OUTPUT";
        case NF_IP_POST_ROUTING: return "POSTROUTING";
        default:                 return "CHAIN";
    }
}

static const char *verdict_name(int v)
{
    int dec = -v - 1;
    if (dec == NF_ACCEPT) return "ACCEPT";
    if (dec == NF_DROP) return "DROP";
    if (dec == NF_QUEUE) return "QUEUE";
    if (v == XT_RETURN) return "RETURN";
    return "JUMP";
}

static void print_table_status(int sock, const char *name)
{
    struct ipt_getinfo info;
    socklen_t infolen = (socklen_t)sizeof(info);
    struct ipt_get_entries *ge;
    socklen_t glen;
    unsigned int off = 0;

    memset(&info, 0, sizeof(info));
    strncpy(info.name, name, IPT_TABLE_MAXNAMELEN - 1);
    if (getsockopt(sock, SOL_IP, IPT_SO_GET_INFO, &info, &infolen) != 0) {
        printf("=== table %s: unavailable (%s) ===\n", name, strerror(errno));
        return;
    }

    glen = (socklen_t)(sizeof(struct ipt_get_entries) + info.size);
    ge = (struct ipt_get_entries *)calloc(1, glen);
    if (!ge) {
        return;
    }
    strncpy(ge->name, name, IPT_TABLE_MAXNAMELEN - 1);
    ge->size = info.size;
    if (getsockopt(sock, SOL_IP, IPT_SO_GET_ENTRIES, ge, &glen) != 0) {
        free(ge);
        return;
    }

    printf("=== table %s (%u entries) ===\n", name, info.num_entries);
    while (off < ge->size) {
        struct ipt_entry *e = (struct ipt_entry *)((unsigned char *)ge->entrytable + off);
        struct ipt_entry_target *t = (struct ipt_entry_target *)((unsigned char *)e + e->target_offset);
        char sbuf[24], dbuf[24], tgt[64];
        const char *pstr = "all";
        unsigned int h;

        for (h = 0; h < NF_IP_NUMHOOKS; h++) {
            if ((info.valid_hooks & (1U << h)) && info.hook_entry[h] == off) {
                printf("Chain %s:\n", hook_name(h));
            }
        }
        if (strcmp(t->u.user.name, XT_ERROR_TARGET) == 0) {
            off += e->next_offset;
            continue;
        }
        if (e->ip.proto == IPPROTO_TCP) pstr = "tcp";
        else if (e->ip.proto == IPPROTO_UDP) pstr = "udp";
        else if (e->ip.proto == IPPROTO_ICMP) pstr = "icmp";

        snprintf(sbuf, sizeof(sbuf), "%s", e->ip.src.s_addr ? inet_ntoa(e->ip.src) : "0/0");
        snprintf(dbuf, sizeof(dbuf), "%s", e->ip.dst.s_addr ? inet_ntoa(e->ip.dst) : "0/0");

        if (strcmp(t->u.user.name, XT_STANDARD_TARGET) == 0) {
            int *vp = (int *)t->data;
            snprintf(tgt, sizeof(tgt), "%s", verdict_name(*vp));
        } else if (strcmp(t->u.user.name, "DNAT") == 0) {
            struct ip_nat_multi_range_compat *mr = (struct ip_nat_multi_range_compat *)t->data;
            struct in_addr ina;
            ina.s_addr = mr->range[0].min_ip;
            if (mr->range[0].flags & IP_NAT_RANGE_PROTO_SPECIFIED) {
                snprintf(tgt, sizeof(tgt), "DNAT->%s:%u", inet_ntoa(ina), ntohs(mr->range[0].min.tcp.port));
            } else {
                snprintf(tgt, sizeof(tgt), "DNAT->%s", inet_ntoa(ina));
            }
        } else {
            snprintf(tgt, sizeof(tgt), "%s", t->u.user.name);
        }

        printf("  %-11s pkts=%-6llu bytes=%-8llu prot=%-4s in=%-6s out=%-6s src=%-15s dst=%-15s\n",
               tgt,
               (unsigned long long)e->counters.pcnt,
               (unsigned long long)e->counters.bcnt,
               pstr,
               e->ip.iniface[0] ? e->ip.iniface : "*",
               e->ip.outiface[0] ? e->ip.outiface : "*",
               sbuf, dbuf);
        off += e->next_offset;
    }
    free(ge);
}

static void print_status(int sock)
{
    FILE *fp;
    unsigned long cnt = 0;
    char line[256];

    print_table_status(sock, "filter");
    print_table_status(sock, "nat");
    fp = fopen("/proc/net/ip_conntrack", "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp) != NULL) {
            cnt++;
        }
        fclose(fp);
        printf("=== ip_conntrack active entries: %lu ===\n", cnt);
    } else {
        printf("=== /proc/net/ip_conntrack: unavailable ===\n");
    }
}

int main(int argc, char **argv)
{
    int sock;
    int rc = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s --flush | --apply <conf> | --status\n", argv[0]);
        return 1;
    }

    sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) {
        fprintf(stderr, "mini-nat: raw socket failed: %s\n", strerror(errno));
        return 1;
    }

    if (strcmp(argv[1], "--flush") == 0) {
        rc = flush_all(sock);
    } else if (strcmp(argv[1], "--apply") == 0 && argc >= 3) {
        mn_cfg_t cfg;
        if (load_config(argv[2], &cfg) != 0) {
            rc = 1;
        } else {
            rc = apply_config(sock, &cfg);
        }
    } else if (strcmp(argv[1], "--status") == 0) {
        print_status(sock);
    } else {
        fprintf(stderr, "Usage: %s --flush | --apply <conf> | --status\n", argv[0]);
        rc = 1;
    }

    close(sock);
    return rc ? 1 : 0;
}
