#ifndef DROPBEAR_LOCALOPTIONS_H_
#define DROPBEAR_LOCALOPTIONS_H_

/* Optimize for small flash footprint */
#define DROPBEAR_SMALL_CODE 1

/* Modern hostkey & pubkey algorithms (OpenSSH 8.8+/9.x compatible) */
#define DROPBEAR_ED25519 1
#define DROPBEAR_RSA 1
#define DROPBEAR_RSA_SHA1 0
#define DROPBEAR_DSS 0
#define DROPBEAR_ECDSA 0
#define DROPBEAR_SK_KEYS 0

/* Modern KEX (fast on ARM922T 200MHz) */
#define DROPBEAR_CURVE25519 1
#define DROPBEAR_DH_GROUP14_SHA256 1
#define DROPBEAR_DH_GROUP14_SHA1 0
#define DROPBEAR_DH_GROUP1 0
#define DROPBEAR_DH_GROUP16 0
#define DROPBEAR_ECDH 0

/* Modern ciphers & MACs (ChaCha20-Poly1305 is fast on ARM without AES-NI) */
#define DROPBEAR_CHACHA20POLY1305 1
#define DROPBEAR_AES128 1
#define DROPBEAR_AES256 1
#define DROPBEAR_3DES 0
#define DROPBEAR_ENABLE_CTR_MODE 1
#define DROPBEAR_ENABLE_CBC_MODE 0
#define DROPBEAR_SHA2_256_HMAC 1
#define DROPBEAR_SHA1_HMAC 0
#define DROPBEAR_SHA1_96_HMAC 0

/* Disable unnecessary features to save flash & RAM */
#define DROPBEAR_X11FWD 0
#define DROPBEAR_SVR_AGENTFWD 0
#define DROPBEAR_CLI_AGENTFWD 0
#define DO_HOST_LOOKUP 0
#define DROPBEAR_SVR_PAM_AUTH 0
#define DROPBEAR_SFTPSERVER 0
#define DROPBEAR_CLI_PROXYCMD 0
#define DROPBEAR_CLI_NETCAT 0

/* Keep TCP forwarding (local & remote) for dbclient reverse tunneling */
#define DROPBEAR_SVR_LOCALTCPFWD 1
#define DROPBEAR_SVR_REMOTETCPFWD 1
#define DROPBEAR_CLI_LOCALTCPFWD 1
#define DROPBEAR_CLI_REMOTETCPFWD 1

/* Keep password + pubkey auth */
#define DROPBEAR_SVR_PASSWORD_AUTH 1
#define DROPBEAR_SVR_PUBKEY_AUTH 1
#define DROPBEAR_SVR_PUBKEY_OPTIONS 1

/* 32 MB RAM DoS protection (tuned for reverse SSH tunnel via 127.0.0.1) */
#define MAX_UNAUTH_PER_IP 6
#define MAX_UNAUTH_CLIENTS 12
#define MAX_AUTH_TRIES 5
#define AUTH_TIMEOUT 60

/* Default paths for WAP610N rootfs */
#define DEFAULT_PATH "/usr/sbin:/usr/bin:/sbin:/bin"
#define DEFAULT_ROOT_PATH "/usr/sbin:/usr/bin:/sbin:/bin"

#endif /* DROPBEAR_LOCALOPTIONS_H_ */
