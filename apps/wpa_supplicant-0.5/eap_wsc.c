/*
 * WPA Supplicant / Wi-Fi Simple Configuration 7C Proposal
 * Copyright (c) 2004-2005, Jouni Malinen <jkmaline@cc.hut.fi>
 * Copyright (c) 2005 Intel Corporation. All rights reserved.
 * Contact Information: Harsha Hegde  <harsha.hegde@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README, README_WSC and COPYING for more details.
 * 
 * $Id: eap_wsc.c 1711 2007-10-19 06:56:28Z oleksandrb $
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __linux__
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/dh.h>
#include <openssl/bn.h>
#include <openssl/rand.h>

#include "common.h"
#include "eloop.h"
#include "eap_i.h"
#include "wpa_supplicant.h"
#include "eap_wsc.h"
#include "UdpLib.h"

#define WSC_EAP_UDP_PORT            37000
#define WSC_EAP_UDP_ADDR            "127.0.0.1"

// This function is called from wpa_supplicant_init; it is responsible
// to initiate the connection with Intel WPS application
struct eap_wsc_data *eap_wsc_init(void)
{
    struct eap_wsc_data *data;

    wpa_printf(MSG_DEBUG,"@#*@#*@#*EAP-WSC: Entered eap_wsc_init *#@*#@*#@");

    data = malloc(sizeof(*data));
    if (data == NULL)
        return data;
    memset(data, 0, sizeof(*data));

    data->udpFdEap = udp_open();

    return data;
}

void eap_wsc_deinit(struct eap_wsc_data **priv)
{
    struct eap_wsc_data *data = *priv;

    wpa_printf(MSG_DEBUG,"@#*@#*@#*EAP-WSC: Entered eap_wsc_reset *#@*#@*#@");


    if (data == NULL)
        return;

    if (data->udpFdEap != -1)
    {
        udp_close(data->udpFdEap);
        data->udpFdEap = -1;
        free(data);
        *priv = NULL;
   }
}

u8 *eap_wsc_process(struct eap_wsc_data *data, const u8 *reqData, size_t reqDataLen, size_t *respDataLen)
{
    struct eap_hdr *req;
    int recvBytes;
    u8 *resp;
    u8 *sendBuf;
    u32 sendBufLen;
    struct sockaddr_in from;
    struct sockaddr_in to;
    WSC_NOTIFY_DATA notifyData;

    wpa_printf(MSG_DEBUG,"@#*@#*@#*EAP-WSC: Entered eap_wsc_process *#@*#@*#@");

    req = (struct eap_hdr *) reqData;
    wpa_printf(MSG_DEBUG, "EAP-WSC : Received packet(len=%lu) ",
               (unsigned long) reqDataLen);
    if(ntohs(req->length) != reqDataLen)
    {
        wpa_printf(MSG_INFO, "EAP-WSC: Pkt length in pkt(%d) differs from" 
            " supplied (%d)\n", ntohs(req->length), reqDataLen);
        return NULL;
    }

    notifyData.type = WSC_NOTIFY_TYPE_PROCESS_REQ;
    notifyData.length = reqDataLen;
    notifyData.u.process.state = data->state;

    sendBuf = (u8 *) malloc(sizeof(WSC_NOTIFY_DATA) + reqDataLen);
    if ( ! sendBuf)
    {
        wpa_printf(MSG_INFO, "EAP-WSC: Memory allocation "
                "for the sendBuf failed\n");
        return NULL;
    }

    memcpy(sendBuf, &notifyData, sizeof(WSC_NOTIFY_DATA));
    memcpy(sendBuf + sizeof(WSC_NOTIFY_DATA), reqData, reqDataLen);
    sendBufLen = sizeof(WSC_NOTIFY_DATA) + reqDataLen;

    to.sin_addr.s_addr = inet_addr(WSC_EAP_UDP_ADDR);
    to.sin_family = AF_INET;
    to.sin_port = htons(WSC_EAP_UDP_PORT);

    if (udp_write(data->udpFdEap, (char *) sendBuf, sendBufLen, &to) < 
            sendBufLen)
    {
        wpa_printf(MSG_INFO, "EAP-WSC: Sending Eap message to "
                "upper Layer failed\n");
        free(sendBuf);
        return NULL;
    }

    free(sendBuf);

    recvBytes = udp_read_timed(data->udpFdEap, (char *) data->recvBuf, 
            WSC_RECVBUF_SIZE, &from, 5);

    if (recvBytes == -1)
    {
        wpa_printf(MSG_INFO, "EAP-WSC: Reading EAP message "
                "from upper layer failed\n");
        return NULL;
    }

    data->recvNotify = (WSC_NOTIFY_DATA *) data->recvBuf;
    if ((data->recvNotify->type != WSC_NOTIFY_TYPE_PROCESS_RESULT) ||
    (data->recvNotify->u.processResult.result != WSC_NOTIFY_RESULT_SUCCESS))
    {
        wpa_printf(MSG_INFO, "EAP-WSC: Process Message failed "
            "somewhere\n");
        return NULL;
    }
    
    resp = (u8 *) malloc(data->recvNotify->length);
    if ( ! resp)
    {
        wpa_printf(MSG_INFO, "EAP-WSC: Memory allocation "
                "for the resp failed\n");
        return NULL;
    }

    memcpy(resp, data->recvNotify + 1, data->recvNotify->length);
    *respDataLen = data->recvNotify->length;

    return resp;
}


static void * eap_wsc_mthd_init(struct eap_sm *sm)
{
    struct eap_wsc_data *data;

    wpa_printf(MSG_DEBUG,"@#*@#*@#*EAP-WSC: Entered eap_wsc_mthd_init *#@*#@*#@");

    data = eap_wsc_init();

    sm->eap_method_priv = data;

    return data;
}


static void eap_wsc_mthd_deinit(struct eap_sm *sm, void *priv)
{
    struct eap_wsc_data *p_data = priv;
    wpa_printf(MSG_DEBUG,"@#*@#*@#*EAP-WSC: Entered eap_wsc_mthd_deinit *#@*#@*#@"); 

    eap_wsc_deinit(&p_data);
}

static u8 * eap_wsc_mthd_process(struct eap_sm *sm, void *priv,
                                 struct eap_method_ret *ret,
                                 const u8 *reqData, size_t reqDataLen,
                                 size_t *respDataLen)
{
    u8 *resp;
    WSC_NOTIFY_DATA *recvNotify;

    wpa_printf(MSG_DEBUG,"@#*@#*@#*EAP-WSC: Entered eap_wsc_mthd_process *#@*#@*#@");

    resp = eap_wsc_process(priv, reqData, reqDataLen, respDataLen);
    recvNotify = ((struct eap_wsc_data *)priv)->recvNotify;

    if (resp == NULL)
    {
        ret->ignore = TRUE;
        return NULL;
    }
    else
    {
        ret->ignore = FALSE;
        ret->decision = DECISION_COND_SUCC;
        ret->allowNotifications = FALSE;
        
        /*check if we're done*/
        if (recvNotify->u.processResult.done)
        {
            ret->methodState = METHOD_DONE;
        }
        else
        {
            wpa_printf(MSG_INFO, "Always setting it to METHOD_CONT\n");
            ret->methodState = METHOD_CONT;
        }
        return resp;
    }
}

/*
const struct eap_method eap_method_wsc =
{
    .method = EAP_TYPE_WSC ,
    .name = "WSC",
    .init = eap_wsc_mthd_init,
    .deinit = eap_wsc_mthd_deinit,
    .process = eap_wsc_mthd_process,
};
*/

int eap_peer_wsc_register(void)
{
    struct eap_method *eap;
    int ret;

    eap = eap_peer_method_alloc(EAP_PEER_METHOD_INTERFACE_VERSION,
                    EAP_VENDOR_IETF, EAP_TYPE_WSC, "WSC");
    if (eap == NULL)
        return -1;

    eap->init = eap_wsc_mthd_init;
    eap->deinit = eap_wsc_mthd_deinit;
    eap->process = eap_wsc_mthd_process;

    ret = eap_peer_method_register(eap);
    if (ret)
        eap_peer_method_free(eap);
    return ret;
}
