/* $Id: eap_wsc.h 1668 2007-09-19 08:20:05Z assafh $ */

#ifndef EAP_WSC_H
#define EAP_WSC_H

/*#pragma pack(push, 1)*/

#define WSC_RECVBUF_SIZE    2048


#define WSC_NOTIFY_TYPE_BUILDREQ              1
#define WSC_NOTIFY_TYPE_BUILDREQ_RESULT       2
#define WSC_NOTIFY_TYPE_PROCESS_REQ           3
#define WSC_NOTIFY_TYPE_PROCESS_RESP          4
#define WSC_NOTIFY_TYPE_PROCESS_RESULT        5

#define WSC_NOTIFY_RESULT_SUCCESS          0x00
#define WSC_NOTIFY_RESULT_FAILURE          0xFF

typedef struct wsc_notify_buildreq_tag {
    u32    id;
    u32 state;
}__attribute__((packed)) WSC_NOTIFY_BUILDREQ;

typedef struct wsc_notify_process_buildreq_result_tag {
    u8 result;
}__attribute__((packed)) WSC_NOTIFY_BUILDREQ_RESULT;

typedef struct wsc_notify_process_tag {
    u32 state;
}__attribute__((packed)) WSC_NOTIFY_PROCESS;

typedef struct wsc_notify_process_result_tag {
    u8 result;
    u8 done;
}__attribute__((packed)) WSC_NOTIFY_PROCESS_RESULT;

typedef struct wsc_notify_data_tag {
    u8 type;
    u8 addr[6]; //MAC ADDR of the peer station
    union {
        WSC_NOTIFY_BUILDREQ bldReq;
        WSC_NOTIFY_BUILDREQ_RESULT bldReqResult;
        WSC_NOTIFY_PROCESS process;
        WSC_NOTIFY_PROCESS_RESULT processResult;
    } u;
    u32 length; // length of the data that follows
}__attribute__((packed)) WSC_NOTIFY_DATA;


struct eap_wsc_data {
    enum { START, CONTINUE, SUCCESS, FAILURE } state;
    int udpFdEap;
    int udpFdCom;
    u8 recvBuf[WSC_RECVBUF_SIZE];
    WSC_NOTIFY_DATA *recvNotify;
};

struct eap_wsc_data *eap_wsc_init(void);
void eap_wsc_deinit(struct eap_wsc_data **priv);
u8 *eap_wsc_process(struct eap_wsc_data *data, const u8 *reqData, size_t reqDataLen, size_t *respDataLen);

/*#pragma pack(pop)*/

#endif /*EAP_WSC_H*/
