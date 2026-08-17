
#define CONFIG_LANGUAGES		"Language"

#define CONFIG_DEVICE_NAME	"HostName"
#define CONFIG_VENDOR_NAME	"VendorName"
#define CONFIG_MODEL_NAME		"ModelName"
#define CONFIG_MODEL_DESC		"ModelDescription"
#define CONFIG_FIRMWARE_VER	"FirmwareVersion"

#define CONFIG_COUNTRY	"Country" //"0"=Infrastructure Station, "2"=Access Point

#define CONFIG_NETWORK_TYPE	"network_type" //"0"=Infrastructure Station, "2"=Access Point

#define CONFIG_WAN_TYPE	"wan_type"

#define CONFIG_SYS_ADMINPASSWORD		"AdminPassword"

#define CONFIG_SYS_BRIDGE_MODE				"BridgeMode" //For AP, "1":Four Addresses Support, "4":Bridge
#define CONFIG_SYS_LAN_IP_CONFIG_METHOD		"ip_config_method" //"0":dhcp, "1":static
#define CONFIG_SYS_LAN_IP						"ip_lan"
#define CONFIG_SYS_LAN_NETMASK				"subnet_lan"
#define CONFIG_SYS_LAN_GATEWAY				"default_gw"
#define CONFIG_SYS_LAN_DHCPSERVER_ENABLED	"DHCPDEnabled" //"0":VideoBridge, "1":LAN Party
#define CONFIG_SYS_LAN_DHCP_START			"DHCPDStartAddress"
#define CONFIG_SYS_LAN_DHCP_END				"DHCPDEndAddress"
#define CONFIG_SYS_LAN_DHCP_SUBNET			"DHCPDSubnet"
#define CONFIG_SYS_LAN_DHCP_WINS				"DHCPDWINS"

#define DHCP_LAN_IP		"0"
#define STATIC_LAN_IP	"1"
#define ENABLE_DHCPSERVER	"1"
#define DISABLE_DHCPSERVER	"0"

#define CONFIG_WLAN_DISCONNECT			"forceWlanDisconnect"

#define CONFIG_WLAN_ENABLED				"RadioEnabled"
#define CONFIG_WLAN_NETWORK_MODE		"NetworkMode"
#define CONFIG_WLAN_FREQUENCY_BAND		"FrequencyBand"
#define CONFIG_WLAN_MAC					"MAC"
#define CONFIG_WLAN_SSID					"NonProc_ESSID"
#define CONFIG_WLAN_WILDCARD_SSID		"Wildcard_ESSID"
#define CONFIG_WLAN_SSID_VISIBLE			"HiddenSSID"
#define CONFIG_WLAN_CHANNEL_WIDTH		"ChannelBonding"
#define CONFIG_WLAN_AUTO_CHANNEL_WIDTH	"AutoChannelWidth" //This veriable only for pass the test of HNAP TestDevice
#define CONFIG_WLAN_CHANNEL				"Channel"
#define CONFIG_WLAN_CHANNEL_UPPERLOWER	"UpperLowerChannelBonding"

#define CONFIG_WLAN_SECURITY_MODE					"NonProcSecurityMode" //Prossible value: "1"=OPEN, "2"=WEP, "3"=WPA/WPA2 Personal, "4"=WPA/WPA2 Enterprise
#define CONFIG_WLAN_WEP_USED_KEY_INDEX				"WepTxKeyIdx"  //Possible value: "0", "1", "2", "3"
#define CONFIG_WLAN_WEP_KEY0							"WepKeys_DefaultKey0"
#define CONFIG_WLAN_WEP_KEY1							"WepKeys_DefaultKey1"
#define CONFIG_WLAN_WEP_KEY2							"WepKeys_DefaultKey2"
#define CONFIG_WLAN_WEP_KEY3							"WepKeys_DefaultKey3"
#define CONFIG_WLAN_WEP_AUTHENTICATION_AP			"NonProc_Authentication" //Possible value:"0"=OPEN, "1"=SHARED
#define CONFIG_WLAN_WEP_AUTHENTICATION_STA			"NonProc_Authentication" //Possible value:"0"=OPEN, "1"=SHARED, "2"=AUTO, Jacky.Yang 22-May-2009, METALINK has changed this difference, "1"=OPEN, "2"=SHARED, "3"=AUTO
#define CONFIG_WLAN_WEP_KEY_LENGTH					"NonProc_WepKeyLength" //Possible value: "64", "128"
#define CONFIG_WLAN_WPAPSK_TYPE 						"NonProc_WPA_Personal_Mode" //Possible value: "1"=WPA, "2"=WPA2, "3"=WPA+WPA2
#define CONFIG_WLAN_WPAPSK_KEY 						"NonProc_WPA_Personal_PSK"
#define CONFIG_WLAN_WPAPSK_ENCAP					"NonProc_WPA_Personal_Encapsulation" //Possible value: "0"=TKIP, "1"=CCMP, "2"=TKIP+CCMP
#define CONFIG_WLAN_WPAPSK_REKEY_INTERVAL			"NonProc_WPA_Personal_ReKey_Interval"
#define CONFIG_WLAN_WPAENTERPRISE_TYPE 				"NonProc_WPA_Enterprise_Mode" //Possible value: "1"=WPA, "2"=WPA2, "3"=WPA+WPA2
#define CONFIG_WLAN_WPAENTERPRISE_ENCAP			"NonProc_WPA_Enterprise_Encapsulation" //Possible value: "0"=TKIP, "1"=CCMP, "2"=TKIP+CCMP
#define CONFIG_WLAN_WPAENTERPRISE_REKEY_INTERVAL	"NonProc_WPA_Enterprise_Radius_ReKey_Interval"
#define CONFIG_WLAN_WPAENTERPRISE_RADIUS1_IP		"NonProc_WPA_Enterprise_Radius_IP"
#define CONFIG_WLAN_WPAENTERPRISE_RADIUS1_PORT	"NonProc_WPA_Enterprise_Radius_Port"
#define CONFIG_WLAN_WPAENTERPRISE_RADIUS1_SECRET 	"NonProc_WPA_Enterprise_Radius_Key"
//Metalink doesn't support secondary RADIUS server, this configuration only for pass the test of HNAP testDevice
#define CONFIG_WLAN_WPAENTERPRISE_RADIUS2_IP		"NonProc_WPA_Enterprise_Radius2_IP"
#define CONFIG_WLAN_WPAENTERPRISE_RADIUS2_PORT	"NonProc_WPA_Enterprise_Radius2_Port"
#define CONFIG_WLAN_WPAENTERPRISE_RADIUS2_SECRET 	"NonProc_WPA_Enterprise_Radius2_Key"
//Metalink doesn't support Qos enable/disable, this configuration only for pass the test of HNAP testDevice
#define CONFIG_WLAN_ENABLE_QOS						"EnableQos"

#define CONFIG_WLAN_ACTIVE_WPS 						"NonProc_WPS_ActivateWPS"

#define CONFIG_WLAN_MACFILTER_MODE	"AclMode"
#define CONFIG_WLAN_MACFILTER_LIST	"ACL"

#define CONFIG_WLAN_STATION_NETWORKTYPE	"Scan_Infrastructure_Adhoc"

#define HNAP_SECURITY_NONE				"1"
#define HNAP_SECURITY_WEP					"2"
#define HNAP_SECURITY_WPA_PERSIONAL		"3"
#define HNAP_SECURITY_WPA_ENTERPRISE		"4"

#define HNAP_TKIP					"0"
#define HNAP_AES					"1"
#define HNAP_TKIPORAES				"2"

//Jacky.Yang 22-May-2009, METALINK has changed this difference, "1"=OPEN, "2"=SHARED, "3"=AUTO
/*#define HNAP_WEP_OPEN				"0"
#define HNAP_WEP_SHARED			"1"
#define HNAP_WEP_AUTO				"2"*/
#define HNAP_WEP_OPEN				"1"
#define HNAP_WEP_SHARED			"2"
#define HNAP_WEP_AUTO				"3"

#define HNAP_WEP_KEYLEN_64		"64"
#define HNAP_WEP_KEYLEN_128		"128"

#define HNAP_WPA			"1"
#define HNAP_WPA2			"2"
#define HNAP_WPAORWPA2	"3"

#define HNAP_5G_BAND	"0"
#define HNAP_24G_BAND	"1"

#define HNAP_MAC_FILTER_DISABLED	"0"
#define HNAP_MAC_FILTER_ALLOW		"1"
#define HNAP_MAC_FILTER_DENY		"2"

#define HNAP_STATION_NETWORKTYPE_INFRASTRUCTURE	"0"
#define HNAP_STATION_NETWORKTYPE_ADHOC				"2"
#define HNAP_STATION_NETWORKTYPE_AUTO				"3"

#define HNAP_NETWORK_TYPE_STATION	"0"
#define HNAP_NETWORK_TYPE_AP			"2"

#define LAN_IF_NAME		"br0"

#define ENABLE_FEATURE		"1"
#define DISABLE_FEATURE	"0"

struct netStats {
	char if_name[32];
	unsigned long rx_bytes;
	unsigned long rx_packets;
	unsigned long rx_errs;
	unsigned long rx_drop;
	unsigned long rx_fifo;
	unsigned long rx_frame;
	unsigned long rx_compressed;
	unsigned long rx_multicast;
	unsigned long tx_bytes;
	unsigned long tx_packets;
	unsigned long tx_errs;
	unsigned long tx_drop;
	unsigned long tx_fifo;
	unsigned long tx_frame;
	unsigned long tx_compressed;
	unsigned long tx_multicast;		
};

extern int hnap_getNthValueSafe(int index, char *value, char delimit, char *result, int len);
extern int get_signal_strength_percent(int signalLevel, char *signalStrengthPercent);
extern int get_signal_noise_percent(int noiseLevel, char *noisePercent);
extern int hnap_prepare_ap_list(char *listBuf);
extern void hnap_active_lan_basic(char *lanIP, char *lanNetmask);
extern void hnap_active_wlan_basic(int isRadioOn);
extern void hnap_active_wlan_security(void);
extern void hnap_active_station_wlan_security(void);
extern int hnap_valid_mac(char *str);
extern int hnap_validate_ipaddr(char* ipaddr);
extern int hnap_getIfIp(char *ifname, char *if_addr);
extern int hnap_getIfMac(char *ifname, char *if_hw);
extern int hnap_getIfNetmask(char *ifname, char *if_net);
extern int hnap_getClientGateway(char *gateway);
extern int hnap_get_config(char *paramName, char *paramValue);
extern int hnap_valid_host_ip_with_subnet(char *ip, char *mask);
extern int hnap_validate_gateway_with_ip(char *ipStr, char *gatewayStr, char *netmaskStr);
extern int hnap_ap_wbridge_upgrade( webs_t wp, int len, char *msg );
extern int hnap_get_frimware_version_date(char *Version, char *releaseDate);
extern int hnap_configure_dhcp(char *lanIp, char *lanNetMask);
extern int hnap_do_wps_pbc_direct_tv(void);
extern int hnap_do_wps_pbc(void);
extern int hnap_do_wps_pin(char *pinCode, char *bssid);
extern int hnap_stop_wps(void);
extern int hnap_get_wps_status(char *status);
extern int hanp_get_value_from_eeprom(char *tag, char *value);
extern int hnap_get_serial_number(char *serialNo);
extern int hnap_get_interface_statistics(char *ifName, void *data);
extern int hnap_get_station_layer2_link_status(void);
extern int hnap_get_station_wireless_info(char *request, char *result);
extern int hnap_clear_unconfigured_flag(void);
extern int hnap_active_wps_monitor(void);

