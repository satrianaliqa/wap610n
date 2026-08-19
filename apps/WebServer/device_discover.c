#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/wireless.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>

#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include	"webs.h"
#include "uemf.h"
#include "mt_api.h"

#include "xml_tree.h"
#include "expat.h"
#include "hnap_interfaces.h"
#include "hnap_config_metalink.h"

#define DISCOVER_PORT				58000
#define MAX_RECEIVE_BUFFER			76800

#define MAX_DUMP_PER_LINE			16 //For dump data

#define ACTION_LOCATION_DISCOVERY			0x01
#define ACTION_CAPABILITY_DISCOVERY		0x02
#define ACTION_CONFIG_IP_NETWORK			0x04

#define HOST_MAX_LEN	22
#define ST_MAX_LEN		32
#define PHY_MAX_LEN	18
#define PHY_MAX_LEN	18

static int discoverSocket=-1;

// error flag for XML content operate
static int dd_error_flag = 0;
static int pushed = 0; // when parsing and the first tag is seen 
static int rb = 0;

struct device_discover_instance {
	uint8_t action; //0x01:Location Discovery, 0x02:Capability Discovery, 0x04:Conifigure IP Network
	uint8_t host[HOST_MAX_LEN];
	uint8_t st[ST_MAX_LEN];
	uint8_t forward; //0x00:disabled, 0x01:enabled	
	uint8_t sphy[PHY_MAX_LEN];
	uint8_t dphy[PHY_MAX_LEN];
	uint16_t cseq;
	uint8_t authorizated;
	uint8_t legal_content;
	uint16_t content_length;
	uint8_t supported_action;

	uint8_t devphy[PHY_MAX_LEN];
}__attribute__((packed));

int dd_doSystem(char *format, ...)
{
	va_list arg;
	char *cmd;
	int rv=0;
	
	va_start(arg, format);
	if (fmtValloc(&cmd, WEBS_BUFSIZE, format, arg) >= WEBS_BUFSIZE) {
		trace(0, T("doSystem: lost data, buffer overflow\n"));
	}
	va_end(arg);
	
	if (cmd) {
		trace(0, T("%s\n"), cmd);
		rv = system(cmd);
		bfree(B_L, cmd);
	}
	return rv;
}

/*
  * This function dump the memory data, it is for debug.
  */
void dd_dump_data(unsigned char *data, int data_len)
{
	int offset=1;
	unsigned char *dump_buf = (unsigned char *)malloc(data_len);
	
	memset(dump_buf, 0, data_len);
	memcpy(dump_buf, data, data_len);
	
	while((offset-1)<data_len){
		if(!(*(dump_buf+(offset-1))&0xf0)){
			printf("0%1x ", *(dump_buf+(offset-1)));
		}else{
			printf("%2X ", *(dump_buf+(offset-1)));
		}
		
		if(!(offset%MAX_DUMP_PER_LINE)){
			printf("\n");
		}
		offset++;
	}

	printf("\n");
	free(dump_buf);
}

int dd_print_string(const char *format, ...)
{
	va_list ap;
	int retval = 0;
	
	va_start(ap,format);
	printf("Device Discovery: ");
	retval = vprintf(format,ap);
	va_end(ap);
	
	return retval;
}

int dd_print_packet(char *txPacket, int txPacketLen)
{
	char *debug = (char *)malloc(txPacketLen+1);
	memset(debug, 0, txPacketLen+1);
	memcpy(debug, txPacket, txPacketLen);
	dd_print_string("%s\n", debug);
	free(debug);

	return 0;
}

int dd_getIfMacStr(uint8_t *ifname, uint8_t *if_hw)
{
	struct ifreq ifr;
	unsigned char *ptr;
	int skfd;

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("open socket error !!\n");
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, 16);
	if(ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0) {
		printf("get mac address of device fail !!\n");
		close(skfd);
		return -1;
	}

	ptr = (unsigned char *)&ifr.ifr_addr.sa_data;
	sprintf(if_hw, "%02X:%02X:%02X:%02X:%02X:%02X",
			(ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
			(ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377));
	
	close(skfd);
	return 0;
}

int dd_getIfMac(char *ifname, unsigned char *if_hw)
{
	struct ifreq ifr;
	unsigned char *ptr;
	int skfd;

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("open socket error !!\n");
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, 16);
	if(ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0) {
		printf("get mac address of device fail !!\n");
		close(skfd);
		return -1;
	}

	ptr = (unsigned char *)&ifr.ifr_addr.sa_data;
	
	if_hw[0] = ptr[0];
	if_hw[1] = ptr[1];
	if_hw[2] = ptr[2];
	if_hw[3] = ptr[3];
	if_hw[4] = ptr[4];
	if_hw[5] = ptr[5];
	
	close(skfd);
	return 0;
}

int dd_getIfIp(char *ifname, char *if_addr)
{
	struct ifreq ifr;
	int skfd = 0;

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		dd_print_string("getIfIp: open socket error");
		return -1;
	}

	//strncpy(ifr.ifr_name, ifname, IF_NAMESIZE);
	strncpy(ifr.ifr_name, ifname, 16);
	if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0) {
		dd_print_string("getIfIp: ioctl SIOCGIFADDR error for %s", ifname);
		close(skfd);
		return -1;
	}
	strcpy(if_addr, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	close(skfd);
	return 0;
}

int dd_getIfNetmask(char *ifname, char *if_net)
{
	struct ifreq ifr;
	int skfd = 0;

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		dd_print_string("getIfNetmask: open socket error");
		return -1;
	}

	//strncpy(ifr.ifr_name, ifname, IF_NAMESIZE);
	strncpy(ifr.ifr_name, ifname, 16);
	if (ioctl(skfd, SIOCGIFNETMASK, &ifr) < 0) {
		dd_print_string("getIfNetmask: ioctl SIOCGIFNETMASK error for %s\n", ifname);
		close(skfd);
		return -1;
	}
	strcpy(if_net, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	close(skfd);
	return 0;
}

//=====Following functions are implement in hnap_implementation.c or hnap_metalink_config.c=====
extern int hnap_get_config(char *paramName, char *paramValue);
extern int hnap_get_frimware_version_date(char *Version, char *releaseDate);
extern int hnap_doSystem(char *format, ...);
extern char *report_memory_error();
extern void generate_return_msg_header(char *ret);
extern char *xml_encode( char *key );
extern int fill_action_list(xml_element element, xml_tree tree);
//=====Above functions are implement in hnap_implementation.c or hnap_metalink_config.c=====

//=====Following functions are for process XML content======
#define ISEXT	1
#define NOTEXT	0

// the buffersize to use when generating the return message size
#define RET_MSG_SIZE 76800

// the buffersize to use when generating the return message size, it is contain http header
#define REPLY_MSG_SIZE 76820

/*
* GetDeviceSettings
*/
#define DEV_TYPE_WIFI_GATEWAY    "GatewayWithWiFi"
#define DEV_TYPE_AP "WiFiAccessPoint"
#define DEV_TYPE_WIFI_BRIDGE "WiFiBridge"

#define OFM_ERROR_REPLY "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n\
<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">\
<soap:Body>\
<ErrorOutOfMemoryResponse xmlns=\"http://purenetworks.com/HNAP1/\">\r\n\
</ErrorOutOfMemoryResponse>\r\n\
<Result>ERROR</Result>\
</soap:Body>\
</soap:Envelope>"

// standard tags and attributes
#define ENV_TAG "soap:Envelope"
#define ENV_ATTR1_NAME "xmlns:xsi"
#define ENV_ATTR1_VAL "\"http://www.w3.org/2001/XMLSchema-instance\""
#define ENV_ATTR2_NAME "xmlns:xsd"
#define ENV_ATTR2_VAL "\"http://www.w3.org/2001/XMLSchema\""
#define ENV_ATTR3_NAME "xmlns:soap"
#define ENV_ATTR3_VAL "\"http://schemas.xmlsoap.org/soap/envelope/\""
#define BDY_TAG "soap:Body"
#define RSPNS_ATTR1_NAME "xmlns"
#define RSPNS_ATTR1_VAL "\"http://purenetworks.com/HNAP1/\""
#define RSPNS_ATTR1_VAL2 "\"http://purenetworks.com/HNAPExt/\""

/*
* Common HNAP values
*/
#define DD_OK "OK"
#define DD_ERROR "ERROR"
#define DD_REBOOT "REBOOT"
#define DD_TOO_MANY "TOOMANY"
#define DD_BOOL_TRUE "true"
#define DD_BOOL_FALSE "false"
#define DD_DEFAULT_STRING ""
#define DD_DEFAULT_INT "0"
#define DD_DEFAULT_DATE "0001-01-01T00:00:00"
#define DD_DEFAULT_IP "0.0.0.0"
#define DD_DEFAULT_NETMASK "255.255.255.255"

#define VENDOR_NAME	"Linksys, A Division of Cisco"
#define MODEL_NAME		"WET610N"
#define MODEL_DESC		"Wireless-N Ethernet Bridge with Dual-Band"

#define GEN_HEADER_MEM_CHECK( z ) \
    if ( z == NULL )          \
{                 \
    dd_error_flag = 1;        \
    return;           \
}

#define DD_HANDLER_MEM_CHECK( x, y )      \
    if (x == NULL)                    \
    {                           \
	    dd_error_flag = 1;                  \
	    deep_free_xml_tree( y );                \
	    return report_memory_error();           \
    }

void dd_start(void *userData, const char *elname, const char **attr)
{
	if (dd_error_flag){
       	return;
    	}else{
       	int i;
       	xml_tree tree;
      		xml_element el;

       	tree = ( xml_tree ) userData;
       	el = new_xml_element( elname );

       	if ( !el ){
            		dd_error_flag = 1;
            		return;
        	}

        	for ( i = 0; attr[i]; i += 2 ){
            		attribute curr = new_attribute( attr[i], attr[i + 1] );
            		if ( curr ){
                		add_xml_element_attr( el, curr );
            		}
        	}

        	push_xml_tree( tree, el );
        	pushed = 1;
    	}
}

void dd_end(void *userData, const char *el)
{
	if ( !dd_error_flag ){
       	pop_xml_tree( (xml_tree) userData );
    	}
}

void dd_charhndl( void *userData, const XML_Char *s, int len )
{
	if (dd_error_flag){
		return;
    	}else{
           	xml_tree tree = (xml_tree) userData;
		add_text_xml_tree(tree, s, len);
    	}
}

void dd_generate_common_header( xml_tree tree, char *rspns_tag, int is_ext )
{
    xml_element curr_el;
    attribute curr_attr;

    curr_el = new_xml_element( ENV_TAG );
    GEN_HEADER_MEM_CHECK( curr_el );

    curr_attr = new_attribute( ENV_ATTR1_NAME, ENV_ATTR1_VAL );
    GEN_HEADER_MEM_CHECK( curr_attr );
    add_xml_element_attr( curr_el, curr_attr );

    curr_attr = new_attribute( ENV_ATTR2_NAME, ENV_ATTR2_VAL );
    GEN_HEADER_MEM_CHECK( curr_attr );
    add_xml_element_attr( curr_el, curr_attr );

    curr_attr = new_attribute( ENV_ATTR3_NAME, ENV_ATTR3_VAL );
    GEN_HEADER_MEM_CHECK( curr_attr );
    add_xml_element_attr( curr_el, curr_attr );
    push_xml_tree( tree, curr_el );

    curr_el = new_xml_element( BDY_TAG );
    GEN_HEADER_MEM_CHECK( curr_el );
    push_xml_tree( tree, curr_el );

    curr_el = new_xml_element( rspns_tag );
    GEN_HEADER_MEM_CHECK( curr_el );
    curr_attr = new_attribute( RSPNS_ATTR1_NAME, is_ext?RSPNS_ATTR1_VAL2: RSPNS_ATTR1_VAL );
    GEN_HEADER_MEM_CHECK( curr_attr );
    add_xml_element_attr( curr_el, curr_attr );
    push_xml_tree( tree, curr_el );
}

char *dd_report_error_owntext( xml_tree tree, xml_element el, char* textToError )
{
    char *ret;
    dd_error_flag = 1;
    set_xml_element_text( el, textToError );
    DD_HANDLER_MEM_CHECK( el->text, tree  );
    push_pop_xml_tree( tree, el );
    pop_all_xml_tree( tree );
    ret = (char *) malloc( sizeof( char ) * RET_MSG_SIZE );
    DD_HANDLER_MEM_CHECK( ret, tree  );
    memset( ret, 0, sizeof( char ) * RET_MSG_SIZE );
    generate_return_msg_header( ret );
    generate_xml( tree, ret );
    deep_free_xml_tree( tree );

    return ret;
}

char *dd_report_error(xml_tree tree, xml_element el)
{
    return dd_report_error_owntext(tree, el, DD_ERROR);
}

// reports an error if there's an initial parsing error
int dd_initial_error(char **packetBuf, char *str, int is_ext)
{
	char ret[5000];
    	xml_tree err_tree;
    	xml_element el;
	char replyHeader[]={"HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/xml; charset=utf-8\r\n\r\n"};
	int dataLen=0;

    	memset( ret, 0, sizeof( ret ) );
    	err_tree = new_xml_tree();
    	if ( !err_tree ){
       	goto send;
    	}

    	dd_generate_common_header( err_tree, "ErrorResponse", is_ext );
    	if ( dd_error_flag ){
		goto send;
    	}

    	el = new_xml_element( "ErrorResult" );
    	if ( !el ){
       	goto send;
    	}
    	//set_xml_element_text( el, HNAP_ERROR );
    	set_xml_element_text( el, str );
    	push_pop_xml_tree( err_tree, el );
    	pop_all_xml_tree( err_tree );

    	dd_error_flag = 1;

send:
    	generate_return_msg_header( ret );
    	generate_xml( err_tree, ret );
    	deep_free_xml_tree( err_tree );

	dataLen = strlen(replyHeader)+strlen(ret);
	printf("dataLen=%d\n", dataLen);
	
	*packetBuf = (char *)realloc(*packetBuf, dataLen);
	memcpy(*packetBuf, replyHeader, strlen(replyHeader));
	memcpy(*packetBuf+strlen(replyHeader), ret, strlen(ret));

	return dataLen;
}
//=====Above functions are for process XML content=====

int dd_send_reply(socket_t *sp, uint16_t rport, char *txPacket, int txPacketLen)
{
	int result = 0;
	struct sockaddr_in bc_addr;
	int bc_addr_len;
	uint16_t org_port = sp->port;

	bc_addr.sin_family = AF_INET;
	bc_addr.sin_port = htons(rport);
	bc_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
	bzero(bc_addr.sin_zero, 8);
	bc_addr_len = sizeof(bc_addr);

	system("route add 255.255.255.255 br0");
	sp->port = rport; //Because Goahead will use listen port for destination, so I need chenge destination port by myself
	dd_print_string("sending %d bytes data ..\n", txPacketLen);
	if((result = socketWrite(discoverSocket, txPacket, txPacketLen))<0){
		dd_print_string("socketWrite failure\n");
	}else{
		if((result = socketFlush(discoverSocket)) == -1){
			dd_print_string("socketFlush failure\n");
		}
	}
	sp->port = org_port; //after sent packets, revised sp->port to listen port
	dd_doSystem("route del 255.255.255.255 br0");
	
	return result;
}

int dd_extract_header_value(uint8_t *hvPair, uint8_t **header, uint8_t **value, char delimiter)
{
	uint8_t *sPtr, *ePtr;
	
	//dd_print_string("hvPair=%s\n", hvPair);
	//1. extract header
	sPtr = hvPair;
	ePtr = strchr(hvPair, ':');
	//Remove all space from tail
	while((sPtr!=ePtr) && (*(ePtr-1)==0x20 || *(ePtr-1)=='\t')){
		ePtr = ePtr - 1;
	}
	if(sPtr==ePtr){
		*header = malloc(1);
		memset(*header, 0, 1);
		dd_print_string("WARNING: The header is NULL..\n");		
	}else{
		*header = malloc(ePtr-sPtr+1);
		memset(*header, 0, ePtr-sPtr+1);
		memcpy(*header, sPtr, ePtr-sPtr);
		//dd_print_string("header=%s\n", *header);
	}
	
	//2. extract value
	sPtr = strchr(hvPair, delimiter)+1;
	//Remove all space from head
	while(*sPtr == 0x20 || *sPtr == '\t'){
		sPtr = sPtr + 1;
	}
	if(*sPtr != '\0'){
		ePtr = strrchr(hvPair, '\0');
		//Remove all space from tail
		while(*(ePtr-1)==0x20 || *(ePtr-1)=='\t'){
			ePtr = ePtr - 1;
		}
		*ePtr = '\0';
		*value = malloc(ePtr-sPtr+1);
		memset(*value, 0, ePtr-sPtr+1);	
		memcpy(*value, sPtr, ePtr-sPtr);
		//dd_print_string("value=%s\n", *value);
	}else{
		*value = malloc(1);
		memset(*value, 0, 1);
		dd_print_string("WARNING: The value is NULL..\n");		
	}

	return 0;
}

int dd_verify_authorization(char *value)
{
	char *sPtr, *ePtr;
	uint8_t *authType, *authCert, *b64decode_authCert, *username=NULL, *password=NULL;
	char sysUserName[MT_MAX_PARAM_VALUE_LENGTH]={0}, sysPassword[MT_MAX_PARAM_VALUE_LENGTH]={0};

	//dd_print_string("string for authorization is \"%s\"\n", value);

	sPtr=value;
	ePtr=strchr(value, 0x20);
	authType = (char *)malloc(ePtr-sPtr+1);
	memset(authType, 0, ePtr-sPtr+1);
	memcpy(authType, sPtr, ePtr-sPtr);

	sPtr=ePtr+1;
	while(*sPtr==0x20 || *sPtr=='\t'){
		sPtr=sPtr+1;
	}
	authCert=(char *)malloc(strlen(sPtr)+1);
	memset(authCert, 0, strlen(sPtr)+1);
	memcpy(authCert, sPtr, strlen(sPtr));

	dd_print_string("authorization type: %s\n", authType);
	if(strcmp(authType, "Basic")){
		dd_print_string("Unsupport authorization type %s\n", authType);
		free(authType);
		free(authCert);		
		return -1;
	}

	b64decode_authCert = (char *)malloc(strlen(authCert)+1);
	memset(b64decode_authCert, 0, strlen(authCert)+1);
	b64_decode(authCert, b64decode_authCert, strlen(authCert));
	//dd_print_string("authCert after b64 decode: \"%s\"\n", b64decode_authCert);
	free(authType);
	free(authCert);

	if(strchr(b64decode_authCert, ':')){
		dd_extract_header_value(b64decode_authCert, &username, &password, ':');
		MT_Get_Param(MTSECURITY_USER_NAME_VAR_NAME, sysUserName,MT_MAX_PARAM_VALUE_LENGTH-1);
		MT_Get_Param(MTSECURITY_USER_PASSWD_VAR_NAME, sysPassword,MT_MAX_PARAM_VALUE_LENGTH-1);
		dd_print_string("username:%s, password:%s, sysPassword:%s, sysUserName:%s\n", username, password, sysPassword, sysUserName);	
		//WET610N doesn't check username
		if(strcmp(password, sysPassword)){
			free(b64decode_authCert);
			free(username);
			free(password);		
			return -1;
		}
	}else{
		free(b64decode_authCert);		
		return -1;
	}

	free(b64decode_authCert);			
	free(username);
	free(password);
	
	return 0;
}

int dd_check_content_type(char *value)
{
	dd_print_string("Doesn't check content-type at present, just skip it..\n");

	return 0;
}

int dd_check_action_name(char *value)
{
	char *tmpPtr=value;
	
	while(*tmpPtr=='"')
		tmpPtr++;
	
	if(strchr(tmpPtr, '"')){
		*(strchr(tmpPtr, '"'))='\0';
	}

	if(strcmp(tmpPtr, "http://purenetworks.com/HNAP1/SetIpSettings")){
		dd_print_string("WARNING: Unknow action \"%s\"\n", tmpPtr);
		return -1;
	}

	return 0;
}

int dd_parse_header(uint8_t *hvPair, struct device_discover_instance *ddi)
{
	uint8_t *header, *value;

	if(!strcmp(hvPair, "SEARCH * HTTP/1.1")){
		ddi->action |= ACTION_LOCATION_DISCOVERY;
		dd_print_string("Got a Location Discovery request .. \n");
	}else if(!strcmp(hvPair, "GET /HNAP1/CAP HTTP/1.1")){
		ddi->action |= ACTION_CAPABILITY_DISCOVERY;
		dd_print_string("Got a Capability Discovery request .. \n");
	}else if(!strcmp(hvPair, "POST /HNAP1/ HTTP/1.1")){
		ddi->action |= ACTION_CONFIG_IP_NETWORK;
		dd_print_string("Got a Config IP Network request .. \n");
	}else if(strchr(hvPair, ':')){
		dd_extract_header_value(hvPair, &header, &value, ':');
		//dd_print_string("header=%s\tvalue=%s\n", header, value);

		if(!strcmp(header, "HOST")){
			if(strlen(value)<HOST_MAX_LEN)
				strcpy(ddi->host, value);
			else
				dd_print_string("WARNING: the value size (%d) for HOST (%d) is too large..\n", strlen(value), HOST_MAX_LEN);
		}else if(!strcmp(header, "ST")){
			if(strlen(value)<ST_MAX_LEN)
				strcpy(ddi->st, value);
			else
				dd_print_string("WARNING: the value size (%d) for ST (%d) is too large..\n", strlen(value), ST_MAX_LEN);
		}else if(!strcmp(header, "Forward")){
			if(!strcasecmp(value, "true")){
				ddi->forward=1;
			}else if(!strcasecmp(value, "false")){
				ddi->forward=0;
			}else{
				dd_print_string("Invalid value %s for Forward in header", value);
			}
		}else if(!strcmp(header, "SPhy")){
			if(strlen(value)<PHY_MAX_LEN)
				strcpy(ddi->sphy, value);
			else
				dd_print_string("WARNING: the value size (%d) for SPhy (%d) is too large..\n", strlen(value), PHY_MAX_LEN);
		}else if(!strcmp(header, "DPhy")){
			if(strlen(value)<PHY_MAX_LEN)
				strcpy(ddi->dphy, value);
			else
				dd_print_string("WARNING: the value size (%d) for DPhy (%d) is too large..\n", strlen(value), PHY_MAX_LEN);
		}else if(!strcmp(header, "CSeq")){
			int cseq_val = atoi(value);
			if(cseq_val >= 1 && cseq_val <= 65535)
				ddi->cseq = (uint16_t)cseq_val;
			else
				dd_print_string("WARNING: Invalid value %d for CSeq in header..\n", atoi(value));
		}else if(!strcmp(header, "Authorization")){
			if(strlen(value)==0 || dd_verify_authorization(value) == -1){
				ddi->authorizated=0;
			}else{
				ddi->authorizated=1;
			}
		}else if(!strcmp(header, "Content-Type")){
			if(dd_check_content_type(value) == -1){
				ddi->legal_content=0;
			}else{
				ddi->legal_content=1;
			}
		}else if(!strcmp(header, "Content-Length")){
			ddi->content_length=(uint16_t)atoi(value);
		}else if(!strcmp(header, "SOAPAction")){
			if(dd_check_action_name(value) == -1){
				ddi->supported_action=0;
			}else{
				ddi->supported_action=1;
			}
		}
		else{
			dd_print_string("Got an unknow header \"%s\", just skip it..\n", header);
		}
		
		free(header);
		free(value);
	}

	return 0;
}

char *dd_capability_discovery(xml_tree request)
{
    	char *ret=NULL;
    	xml_tree get_dev_s;
    	xml_element curr_el;
	char paramValue[MT_MAX_PARAM_VALUE_LENGTH]={0};
	char tempBuf[MT_MAX_PARAM_VALUE_LENGTH]={0};
	char *xml_encode_tempbuf, *tempBuf_ptr;
	char firmwareVersion[128]={0}, releaseDate[128]={0};

	hnap_get_frimware_version_date(firmwareVersion, releaseDate);

    	get_dev_s = new_xml_tree();
    	DD_HANDLER_MEM_CHECK( get_dev_s, get_dev_s );
    	dd_generate_common_header( get_dev_s, "GetDeviceSettingsResponse", NOTEXT );
    	if ( dd_error_flag ){
       	deep_free_xml_tree( get_dev_s );
       	return report_memory_error();
    	}
		
    	// result
    	curr_el = new_xml_element( "GetDeviceSettingsResult" );
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	set_xml_element_text( curr_el, DD_OK );
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );

    	// device type
    	curr_el = new_xml_element( "Type" );
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
	hnap_get_config(CONFIG_NETWORK_TYPE, paramValue);
    	if (strncmp(paramValue, "0", 1) == 0){
       	set_xml_element_text( curr_el, DEV_TYPE_WIFI_BRIDGE );
    	}else{
       	set_xml_element_text( curr_el, DEV_TYPE_AP );
    	}
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );

    	// device name
    	curr_el = new_xml_element( "DeviceName" );
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	hnap_get_config(CONFIG_DEVICE_NAME, paramValue);
	strcpy(tempBuf, paramValue);
	xml_encode_tempbuf = xml_encode(tempBuf);
	if(xml_encode_tempbuf!=NULL && strcmp(xml_encode_tempbuf, tempBuf)){
		tempBuf_ptr = xml_encode_tempbuf;
	}else{
		tempBuf_ptr = tempBuf;
	}
    	set_xml_element_text( curr_el, tempBuf_ptr );
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );

    	// vendor name
    	curr_el = new_xml_element( "VendorName" );
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	set_xml_element_text( curr_el, VENDOR_NAME);
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );

    	//model desc
    	curr_el = new_xml_element( "ModelDescription" );
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	set_xml_element_text( curr_el, MODEL_DESC);
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );

   	// model name
    	curr_el = new_xml_element( "ModelName" );
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	set_xml_element_text( curr_el, MODEL_NAME);
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );

    	// firmware
    	curr_el = new_xml_element( "FirmwareVersion" );
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
	memset(paramValue, 0, MT_MAX_PARAM_VALUE_LENGTH);
    	set_xml_element_text( curr_el, firmwareVersion);
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );

    	// presentation url
    	curr_el = new_xml_element( "PresentationURL" );
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	set_xml_element_text( curr_el, "/station/wireless_basic.asp" );
    	//set_xml_element_text( curr_el, "/default.asp" ); //This is only for test, it need be revised to /basic/mode.asp when formal release
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );

	if(fill_action_list(curr_el, get_dev_s)==-1){
    		DD_HANDLER_MEM_CHECK( ret, get_dev_s );
	}

    	// sub dev urls
    	curr_el = new_xml_element( "SubDeviceURLs" );
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	push_xml_tree( get_dev_s, curr_el );
    	pop_xml_tree( get_dev_s );

    	// set task extentions
    	curr_el = new_xml_element( "Tasks" );
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	push_xml_tree( get_dev_s, curr_el );

    	// Task1 - browse status page
    	curr_el = new_xml_element( "TaskExtension" );
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	push_xml_tree( get_dev_s, curr_el );

    	curr_el = new_xml_element( "Name");
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	set_xml_element_text( curr_el, "Status Page" );
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );

    	curr_el = new_xml_element( "URL");
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	set_xml_element_text( curr_el, "/status/device_status.asp" );
	//set_xml_element_text( curr_el, "/Status_And_Statistics.asp" );//This is only for test, it need be revised to /basic/mode.asp when formal release
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );

    	curr_el = new_xml_element( "Type");
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	set_xml_element_text( curr_el, "Browser" );
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );
		
    	pop_xml_tree( get_dev_s ); // pop the TaskExtension

    	// task 2 - browse wireless setting page
    	curr_el = new_xml_element( "TaskExtension" );
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	push_xml_tree( get_dev_s, curr_el );

    	curr_el = new_xml_element( "Name");
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	set_xml_element_text( curr_el, "Basic Wireless Settings" );
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );

    	curr_el = new_xml_element( "URL");
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	set_xml_element_text( curr_el, "/station/site_survey.asp" );
	//set_xml_element_text( curr_el, "/Wireless_Settings.asp" );//This is only for test, it need be revised to /basic/mode.asp when formal release
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );

    	curr_el = new_xml_element( "Type");
    	DD_HANDLER_MEM_CHECK( curr_el, get_dev_s );
    	set_xml_element_text( curr_el, "Browser" );
    	DD_HANDLER_MEM_CHECK( curr_el->text, get_dev_s );
    	push_pop_xml_tree( get_dev_s, curr_el );

    	pop_all_xml_tree( get_dev_s );
		
    	ret = (char *) malloc( sizeof( char ) * RET_MSG_SIZE );
    	DD_HANDLER_MEM_CHECK( ret, get_dev_s );
    	memset( ret, 0, sizeof( char ) * RET_MSG_SIZE );
    	generate_return_msg_header( ret );
    	generate_xml( get_dev_s, ret );
    	deep_free_xml_tree( get_dev_s );

    	return ret;
}

int dd_process_capability_discovery(struct device_discover_instance *ddi, char **packetBuf, xml_tree request)
{
	char content[]="HTTP/1.1 200 OK\r\nCSeq: %d GET\r\nSPhy: %s\r\nDPhy: %s\r\nContent-Type: text/xml; charset=utf-8\r\nContent-Length: %d\r\n\r\n";
	char *ret;
	char tmpHeaderBuf[256]={0};
	int dataLen=0;

	ret = dd_capability_discovery(request);
	printf("ret=\n%s\n", ret);
	
	sprintf(tmpHeaderBuf, content, ddi->cseq, ddi->devphy, ddi->sphy, strlen(ret));
	printf("header=\n%s\n", tmpHeaderBuf);
	
	dataLen = strlen(tmpHeaderBuf)+strlen(ret);
	printf("dataLen=%d\n", dataLen);
	
	*packetBuf = (char *)realloc(*packetBuf, dataLen);
	memcpy(*packetBuf, tmpHeaderBuf, strlen(tmpHeaderBuf));
	memcpy(*packetBuf+strlen(tmpHeaderBuf), ret, strlen(ret));

	free(ret);
	return dataLen;
}

char *dd_set_ip_settings(xml_tree request)
{
    	char *lanIPConfigureMode, *lanIP, *netmask, *lanGateway, *ret;
    	xml_element curr_el, curr_rqst;
    	xml_tree set_wireless_client_lan_settings;
    	unsigned int argFlags = 0;
	char paramValue[MT_MAX_PARAM_VALUE_LENGTH]={0}; //for get current setting from configuration
	char_t *argp[] = {CONFIG_SYS_LAN_IP_CONFIG_METHOD,CONFIG_SYS_LAN_IP,CONFIG_SYS_LAN_NETMASK,CONFIG_SYS_LAN_GATEWAY};
	int argc = 4;
	int isGatewayConfigured=0, OrgIsDhcpClient=0;

    	lanIPConfigureMode = lanIP = netmask = lanGateway = DD_DEFAULT_STRING;
    	set_wireless_client_lan_settings = new_xml_tree();
    	DD_HANDLER_MEM_CHECK( set_wireless_client_lan_settings , set_wireless_client_lan_settings );
    	dd_generate_common_header( set_wireless_client_lan_settings, "SetIpSettingsResponse", NOTEXT );
    	if ( dd_error_flag ){
       	deep_free_xml_tree( set_wireless_client_lan_settings );
		dd_print_string("report memory error !!\n");//Ricky Trace
        	return report_memory_error();
    	}
    	curr_el = new_xml_element( "SetIpSettingsResult" );
    	DD_HANDLER_MEM_CHECK( curr_el, set_wireless_client_lan_settings );
    
    	curr_rqst = request->root; // soap:Envelope
    	if ( curr_rqst == NULL ){
		dd_print_string("report HNAP error 1 !!\n");//Ricky Trace
       	return dd_report_error( set_wireless_client_lan_settings, curr_el );
    	}

    	curr_rqst = curr_rqst->first_child; // soap:Body
    	if ( curr_rqst == NULL ){
		dd_print_string("report HNAP error 2 !!\n");//Ricky Trace
       	return dd_report_error( set_wireless_client_lan_settings, curr_el );
    	}

    	curr_rqst = curr_rqst->first_child; // SetIpSettings
    	if ( curr_rqst == NULL ){
		dd_print_string("report HNAP error 3 !!\n");//Ricky Trace
       	return dd_report_error( set_wireless_client_lan_settings, curr_el );
    	}

    	curr_rqst = curr_rqst->first_child; // RouterIPAddress
    	while( argFlags ^ 0x0f ){
       	if ( curr_rqst == NULL ){
			dd_print_string("report HNAP error 4 !!\n");//Ricky Trace
            		return dd_report_error( set_wireless_client_lan_settings, curr_el );
        	}

		if ( !strcmp( curr_rqst->name, "Type" )){
	    		if( curr_rqst->text == NULL ){
				dd_print_string("report HNAP error 5 !!\n");//Ricky Trace
				return dd_report_error( set_wireless_client_lan_settings, curr_el );
	    		}
            		lanIPConfigureMode = curr_rqst->text;
            		argFlags |= 0x01;
        	}else if (!strcmp( curr_rqst->name, "IpAddress" )){
	    		if(curr_rqst->text == NULL && !strcmp(lanIPConfigureMode, "Static")){
				dd_print_string("report HNAP error 5 !!\n");//Ricky Trace
				return dd_report_error( set_wireless_client_lan_settings, curr_el );
	    		}
            		lanIP = curr_rqst->text;
            		argFlags |= 0x02;
        	}else if (!strcmp( curr_rqst->name, "NetMask" )){
	    		if(curr_rqst->text == NULL && !strcmp(lanIPConfigureMode, "Static")){
				dd_print_string("report HNAP error 6 !!\n");//Ricky Trace
		    		return dd_report_error( set_wireless_client_lan_settings, curr_el );
	    		}
            		netmask = curr_rqst->text;
            		argFlags |= 0x04;
        	}else if (!strcmp( curr_rqst->name, "Gateway" )){
			if(curr_rqst->text != NULL ){
				isGatewayConfigured=1;
            			lanGateway = curr_rqst->text;
				dd_print_string("LanGateway=%s\n", lanGateway);
			}
            		argFlags |= 0x08;
        	}else{
        		dd_print_string("report HNAP error 7 !!\n");//Ricky Trace
            		return dd_report_error( set_wireless_client_lan_settings, curr_el );
        	}
        	curr_rqst = curr_rqst->next_sibling;
    	}

	if(strcmp( lanIPConfigureMode, "DHCP" ) && strcmp( lanIPConfigureMode, "Static")){
		dd_print_string("Unknow IP mode (%s) !!\n", lanIPConfigureMode);
		return dd_report_error( set_wireless_client_lan_settings, curr_el );
    	}

	if(!strcmp( lanIPConfigureMode, "Static") && 
		(!hnap_validate_ipaddr(lanIP) ||
		!hnap_validate_ipaddr(netmask) ||
		(isGatewayConfigured &&!hnap_validate_ipaddr(lanGateway)) ||
		!hnap_valid_host_ip_with_subnet(lanIP, netmask) ||
		(isGatewayConfigured && !hnap_validate_gateway_with_ip(lanIP, lanGateway, netmask)))){
		dd_print_string("Invalid IP configuration !!\n");
		return dd_report_error( set_wireless_client_lan_settings, curr_el );
	}

	hnap_get_config(CONFIG_SYS_LAN_IP_CONFIG_METHOD, paramValue);
	if(!strncmp(paramValue, STATIC_LAN_IP, 2)){
		OrgIsDhcpClient = 0;
	}else{
		OrgIsDhcpClient = 1;
	}

	//dd_print_string("Set lan ip to %s, netmask to %s, gateway to %s\n", lanIP, netmask, lanGateway);//Ricky Trace

	device_set_string_value(CONFIG_SYS_LAN_IP_CONFIG_METHOD, strcmp( lanIPConfigureMode, "DHCP" )?STATIC_LAN_IP: DHCP_LAN_IP);
	if(!strcmp( lanIPConfigureMode, "Static" )){
		device_set_string_value(CONFIG_SYS_LAN_IP, lanIP);
		device_set_string_value(CONFIG_SYS_LAN_NETMASK, netmask);
		device_set_string_value(CONFIG_SYS_LAN_GATEWAY, lanGateway);
	}
	
	hnap_commit_changes(argc, argp, COMMIT_SYS);

	hnap_doSystem("killall udhcpc");
	hnap_doSystem("/root/mtlk/web/post_apply.tcl");
	if(!strcmp( lanIPConfigureMode, "Static" )){
		hnap_doSystem("ifconfig br0 %s netmask %s", lanIP, netmask);
		hnap_doSystem("route del default");
		hnap_doSystem("route add default gw %s", lanGateway);
	}else{
		hnap_doSystem("/etc/udhcpc/dhcp.tcl startup &");
	}

    	set_xml_element_text(curr_el, DD_OK);	

    	DD_HANDLER_MEM_CHECK( curr_el->text, set_wireless_client_lan_settings );
    	push_pop_xml_tree( set_wireless_client_lan_settings, curr_el );
		
    	pop_all_xml_tree( set_wireless_client_lan_settings );
    	ret = (char *) malloc( sizeof( char ) * RET_MSG_SIZE );
    	DD_HANDLER_MEM_CHECK( ret, set_wireless_client_lan_settings );
    	memset( ret, 0, sizeof( char ) * RET_MSG_SIZE );
    	generate_return_msg_header( ret );
    	generate_xml( set_wireless_client_lan_settings, ret );
    	deep_free_xml_tree( set_wireless_client_lan_settings );
		
    	return ret;
}

int dd_process_config_ip_network(struct device_discover_instance *ddi, char **packetBuf, xml_tree request)
{
	char content[]="HTTP/1.1 200 OK\r\nCSeq: %d POST\r\nSPhy: %s\r\nDPhy: %s\r\nContent-Type: text/xml; charset=utf-8\r\nContent-Length: %d\r\n\r\n";
	char *ret;
	char tmpHeaderBuf[256]={0};
	int dataLen=0;

	ret = dd_set_ip_settings(request);
	printf("ret=\n%s\n", ret);
	
	sprintf(tmpHeaderBuf, content, ddi->cseq, ddi->devphy, ddi->sphy, strlen(ret));
	printf("header=\n%s\n", tmpHeaderBuf);
	
	dataLen = strlen(tmpHeaderBuf)+strlen(ret);
	printf("dataLen=%d\n", dataLen);
	
	*packetBuf = (char *)realloc(*packetBuf, dataLen);
	memcpy(*packetBuf, tmpHeaderBuf, strlen(tmpHeaderBuf));
	memcpy(*packetBuf+strlen(tmpHeaderBuf), ret, strlen(ret));

	free(ret);
	return dataLen;
}

int dd_process_unauthenticated_request(struct device_discover_instance *ddi, char **packetBuf)
{
	char contentHeader[]="HTTP/1.1 401 Unauthorized\r\nCSeq: %d POST\r\nSPhy: %s\r\nDPhy: %s\r\nContent-Type: text/xml; charset=utf-8\r\nContent-Length: %d\r\n\r\n";
	char contentBody[]="<html><head><title>Document Error: Unauthorized</title></head>\r\n\
		<body><h2>Access Error: Unauthorized</h2>\r\n\
		<p>Please provide correct authorization information !!!</p></body></html>\r\n";
	char tmpHeaderBuf[256]={0};
	int dataLen=0;

	sprintf(tmpHeaderBuf, contentHeader, ddi->cseq, ddi->devphy, ddi->sphy, strlen(contentBody));
	dataLen=strlen(tmpHeaderBuf)+strlen(contentBody);

	*packetBuf = (char *)realloc(*packetBuf, dataLen);
	memcpy(*packetBuf, tmpHeaderBuf, strlen(tmpHeaderBuf));
	memcpy(*packetBuf+strlen(tmpHeaderBuf), contentBody, strlen(contentBody));

	return dataLen;
}

int dd_process_none_location_discovery(struct device_discover_instance *ddi, char **packetBuf, char *bodyContent, int bodyLen)
{
	xml_tree request;
    	XML_Parser p = NULL;
	int dataLen=0;
	int content_len=ddi->content_length;
	char line[10000]={0};
    	int buffersize = 10000;
	int xml_start=0;

	 pushed = dd_error_flag = rb = 0;
		
	// create the expat parser
    	p = XML_ParserCreate( NULL );
	if (!p){
       	dd_print_string("XML PARSE CREATE ERROR\n");
		return dd_initial_error(packetBuf, "XML PARSE CREATE ERROR", NOTEXT);
	}
		
    	// start the request tree
    	request = new_xml_tree();
    	if (!request){
		XML_ParserFree(p);
		dd_print_string("NEW XML TREE ERROR\n");
		return dd_initial_error(packetBuf, "NEW XML TREE ERROR", NOTEXT);
    	}
    	XML_SetUserData(p, request);
    	XML_SetElementHandler(p, dd_start, dd_end);
    	XML_SetCharacterDataHandler(p, dd_charhndl);

	if(bodyLen){
        	int body_read = 0;
       	while ( body_read < content_len ){
            		char *start; char moretoread[10];
            		int justread, toread;

            		toread = content_len - body_read;

            		if (toread > ( buffersize - 1 )){
         			toread = buffersize - 1;
            		}

            		for (justread = 0; justread < toread; ){
                		char cur = *(bodyContent + body_read + justread);

				line[justread] = cur;
                		justread++;
                		line[justread] = '\0';
            		}
            		body_read += justread;

            		start = line;
            		sprintf(moretoread, "%d", toread);

            		if ( !xml_start ){
                		while ( *start != '\0' ){
                    			if ( *start == '<' ){
                        			xml_start = 1;
                        			break;
                    			}
                    				start++;
                		}
            		}

            		if (xml_start){
                		if (XML_Parse(p, start, (int) strlen( start), 0 )  != XML_STATUS_OK){
					dd_print_string("XML PARSE STATUS NOT OK ERROR\n"); //Ricky Trace
                    			XML_ParserFree(p);
                    			deep_free_xml_tree(request);
                    			dataLen = dd_initial_error(packetBuf, "XML PARSE STATUS NOT OK ERROR", NOTEXT);
                    			return dataLen;
                		}
            		}
        	}	
	}

	if(ddi->action == ACTION_CAPABILITY_DISCOVERY){
		dataLen = dd_process_capability_discovery(ddi, packetBuf, request);
	}else if(ddi->action == ACTION_CONFIG_IP_NETWORK){
		if(ddi->authorizated==1){
			dataLen = dd_process_config_ip_network(ddi, packetBuf, request);
		}else{
			dataLen = dd_process_unauthenticated_request(ddi, packetBuf);
		}
	}

    	deep_free_xml_tree( request );
    	XML_ParserFree( p );

	return dataLen;
}

int dd_process_location_discovery(struct device_discover_instance *ddi, char **packetBuf)
{
	char nmask[16]={0};
	char tmpBuf[256]={0};
	char content[]="HTTP/1.1 200 OK\r\nCSeq: %d SEARCH\r\nSPhy: %s\r\nDPhy: %s\r\nST: hnap:WifiBridge\r\nNMask: %s\r\n\r\n";

	dd_getIfNetmask("br0", nmask);

	sprintf(tmpBuf, content, ddi->cseq, ddi->devphy, ddi->sphy, nmask);
	*packetBuf = (char *)realloc(*packetBuf, strlen(tmpBuf));
	memcpy(*packetBuf, tmpBuf, strlen(tmpBuf));

	return strlen(tmpBuf);
}

int dd_process_packet(socket_t	*sp, uint16_t rport, char *data, int dataLen)
{
	struct device_discover_instance *ddi;
	uint8_t *sPtr, *tmpPtr, *ePtr, *tmpBuf;
	int hvLen=0;
	int headerEnd=0;
	uint8_t devMac[17]={0};
	char *txPacket = NULL;
	int result=0;
	int dataRead=0;
	int bodyLen=0;
	char *bodyContent=NULL;

	ddi = (struct device_discover_instance *)malloc(sizeof(struct device_discover_instance));
	memset(ddi, 0, sizeof(struct device_discover_instance));

	dd_getIfMacStr("br0", devMac);
	strcpy(ddi->devphy, devMac);	
	//dd_print_string("MAC address for br0 is %s\n", ddi->devphy);
	//printf("===== Start to parse data header =====\n");
	//Parse the data header and extract header value
	sPtr = tmpPtr = data;
	while((ePtr=strchr(tmpPtr, '\r'))!=0){
		if(*(ePtr+1)=='\n'){
			if(*(ePtr+2)=='\r' && *(ePtr+3)=='\n'){
				headerEnd=1;
			}
			ePtr = ePtr+1;
			sPtr = tmpPtr;	
		}else{
			tmpPtr = ePtr+1;
			continue;
		}

		hvLen = ePtr - sPtr - 1; //eliminate the character '\r'
		dataRead = dataRead + hvLen + 2;
		tmpBuf = (char *)malloc(hvLen+1);
		memset(tmpBuf, 0, hvLen+1);
		memcpy(tmpBuf, sPtr, hvLen);
		//dd_print_string("%s\n", tmpBuf);
		dd_parse_header(tmpBuf, ddi);
		sPtr = tmpPtr = ePtr + 1;
		free(tmpBuf);
		
		if(headerEnd)
			break;
	}

	dataRead = dataRead + 2;
	printf("dataRead=%d, dataLen=%d\n", dataRead, dataLen);
	bodyLen = dataLen-dataRead;
	if(bodyLen){
		bodyContent=(char *)malloc(bodyLen);
		memset(bodyContent, 0, bodyLen);
		memcpy(bodyContent, data+dataRead, bodyLen);
	}
	
	dd_print_string("ddi->action: %x\n", ddi->action);
	dd_print_string("ddi->host: %s\n", ddi->host);
	dd_print_string("ddi->st: %s\n", ddi->st);
	dd_print_string("ddi->forward: %s\n", ddi->forward?"true":"false");
	dd_print_string("ddi->sphy: %s\n", ddi->sphy);
	dd_print_string("ddi->dphy: %s\n", ddi->dphy);
	dd_print_string("ddi->cseq: %u\n", ddi->cseq);
	dd_print_string("dd->authorizated: %u\n", ddi->authorizated);
	dd_print_string("dd->legal_content: %u\n", ddi->legal_content);
	dd_print_string("dd->content_length: %u\n", ddi->content_length);
	dd_print_string("dd->supported_action: %u\n", ddi->supported_action);

	//dd_print_string("===== End to parse data header =====\n");
	//Valid the values in header 
	//1. Check the action from this request
	//    so far we only support location discovery, capability discover and config ip network
	if(ddi->action != ACTION_LOCATION_DISCOVERY &&
		ddi->action != ACTION_CAPABILITY_DISCOVERY &&
		ddi->action != ACTION_CONFIG_IP_NETWORK){
		dd_print_string("WARNING: Unknow action (0x%2x), drop this packet silently..\n", ddi->action);
		goto end;
	}

	//2. Check the ST from this request
	if(ddi->action == ACTION_LOCATION_DISCOVERY && 
		strcmp(ddi->st, "hnap:WifiBridge") && strcmp(ddi->st, "hnap:all")){
		dd_print_string("WARNING: The device type (%s) in ST header does not agree with this device..\n", ddi->st);
		goto end;
	}

	//3. Check the DPhy from this request
	if(strcasecmp(ddi->dphy, "FF:FF:FF:FF:FF:FF") && strcasecmp(ddi->dphy, devMac)){
		dd_print_string("WARNING: The MAC (%s) in DPhy header does not agree with this device..\n", ddi->dphy);
		goto end;		
	}

	if(!strcasecmp(ddi->dphy, "FF:FF:FF:FF:FF:FF") && ddi->action == ACTION_CONFIG_IP_NETWORK){
		dd_print_string("WARNING: The MAC (%s) in DPhy header is broadcast address, for \"Config IP Network\" only accept the device's MAC %s for DPhy..\n", ddi->dphy, devMac);
		goto end;				
	}

	if(ddi->action == ACTION_LOCATION_DISCOVERY){
		result = dd_process_location_discovery(ddi, &txPacket);
		dd_print_packet(txPacket, result);
		dd_send_reply(sp, rport, txPacket, result);
	}else if(ddi->action == ACTION_CAPABILITY_DISCOVERY){
		result = dd_process_none_location_discovery(ddi, &txPacket, NULL, 0);
		dd_print_packet(txPacket, result);
		dd_send_reply(sp, rport, txPacket, result);		
	}else if(ddi->action == ACTION_CONFIG_IP_NETWORK){
		result = dd_process_none_location_discovery(ddi, &txPacket, bodyContent, bodyLen);
		dd_print_packet(txPacket, result);
		dd_send_reply(sp, rport, txPacket, result);				
	}
	
end:
	if(bodyLen)
		free(bodyContent);
	if(txPacket)
		free(txPacket);
	free(ddi);

	return 0;
}

static void dd_socket_Event(int sid, int mask, int iwp)
{
	socket_t	*sp;
	uint8_t receive_buffer[MAX_RECEIVE_BUFFER] = {0}; //buffer for receive data
	struct sockaddr_in host_addr; //keep bind address and address information of received packets
	socklen_t address_len; //size of address
	int result;

	address_len = sizeof(host_addr);

	if ((sp = socketPtr(sid)) == NULL) {
		return;
	}

	if (mask & SOCKET_READABLE) {
		//result=socketRead(sid, receive_buffer, sizeof(receive_buffer));
		result = recvfrom(sp->sock, receive_buffer, sizeof(receive_buffer), 0, (struct sockaddr *)&host_addr, &address_len);
		if(result>0){
			dd_print_string("Got %d bytes data for device discovery or IP configuration as below..\n", result);
			dd_print_string("%s", receive_buffer);
			dd_process_packet(sp, ntohs(host_addr.sin_port), receive_buffer, result);
		}
	}

	return;
}

int device_discover_open(void)
{
	printf("Open socket on port %d for receive broadcast discover packet\n", DISCOVER_PORT);

	discoverSocket = socketOpenConnection(NULL, DISCOVER_PORT, NULL, SOCKET_BROADCAST);
	if (discoverSocket < 0) {
		printf("Unable to open discover socket on port <%d>!\n", DISCOVER_PORT);
		return -1;
	}else
		printf("Create a socket for device discover successfully, the socket descript is %d\n", discoverSocket);
	socketSetBufferSize(discoverSocket, -1, -1, 5120); //Expand the out buffer to 5120 bytes
	socketCreateHandler(discoverSocket, SOCKET_READABLE, dd_socket_Event, 0);

	return 0;
}


