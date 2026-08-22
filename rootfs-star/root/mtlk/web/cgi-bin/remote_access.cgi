#!/bin/sh
# Real-time Remote Access (SSH / Telnet) Manager for Cisco WAP610N Web GUI

QUERY="${QUERY_STRING}"

# Default values
SSH_VAL=1
TELNET_VAL=1

case "$QUERY" in
    *ssh=0*)
        SSH_VAL=0
        ;;
    *ssh=1*)
        SSH_VAL=1
        ;;
esac

case "$QUERY" in
    *telnet=0*)
        TELNET_VAL=0
        ;;
    *telnet=1*)
        TELNET_VAL=1
        ;;
esac

# Persist to /mnt/jffs2/sys.conf
mkdir -p /mnt/jffs2
touch /mnt/jffs2/sys.conf
sed -i '/^SSH_Enabled/d' /mnt/jffs2/sys.conf 2>/dev/null || true
echo "SSH_Enabled = $SSH_VAL" >> /mnt/jffs2/sys.conf

sed -i '/^Telnet_Enabled/d' /mnt/jffs2/sys.conf 2>/dev/null || true
echo "Telnet_Enabled = $TELNET_VAL" >> /mnt/jffs2/sys.conf

# Synchronize to /tmp/sys.conf
cp -af /mnt/jffs2/sys.conf /tmp/sys.conf 2>/dev/null || true

# Dynamic process management
if [ "$SSH_VAL" = "0" ]; then
    killall dropbear 2>/dev/null || true
else
    if ! ps | grep -v grep | grep -q dropbear; then
        /usr/sbin/dropbear -B &
    fi
fi

if [ "$TELNET_VAL" = "0" ]; then
    killall telnetd 2>/dev/null || true
else
    if ! ps | grep -v grep | grep -q telnetd; then
        telnetd -l /bin/sh &
    fi
fi

printf "HTTP/1.0 200 OK\r\n"
printf "Content-Type: text/html\r\n"
printf "Pragma: no-cache\r\n"
printf "Cache-Control: no-cache\r\n\r\n"

cat << 'HTML'
<!DOCTYPE html>
<html>
<head>
<meta http-equiv="refresh" content="2;url=/admin/management.asp">
<title>Settings Saved</title>
<style>
body { font-family: Arial, sans-serif; text-align: center; margin-top: 60px; background: #f4f4f4; }
.card { background: #fff; border: 1px solid #ccc; border-radius: 6px; display: inline-block; padding: 25px 40px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
h2 { color: #005a9c; margin-top: 0; }
</style>
</head>
<body>
<div class="card">
    <h2>Settings Saved Successfully!</h2>
    <p>Remote Access configuration has been updated.</p>
    <p style="color:#666; font-size:13px;">Redirecting back to Management page in 2 seconds...</p>
</div>
</body>
</html>
HTML
