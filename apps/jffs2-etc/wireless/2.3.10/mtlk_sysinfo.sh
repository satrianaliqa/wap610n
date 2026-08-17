#!/bin/sh




LOG_file=/tmp/mtlk_sysinfo.log


# ifconfig output (packets lost in driver)
echo ==================== > $LOG_file
echo ifconfig statistics: >> $LOG_file
echo ==================== >> $LOG_file

ifconfig >> $LOG_file

# iwconfig output (packets lost in driver)
echo ==================== > $LOG_file
echo iwconfig statistics: >> $LOG_file
echo ==================== >> $LOG_file

iwconfig wlan0 >> $LOG_file

#tc outprut
echo "===============================" >> $LOG_file
echo tc output "\(packets lost in OS\)": >> $LOG_file
echo "===============================" >> $LOG_file

tc -s qdisc >> $LOG_file


# Driver Statistics (DebugAPI)

cat /proc/net/mtlk/wlan0/Debug/General >> $LOG_file

cat /proc/net/mtlk/wlan0/Debug/ReorderingStats >> $LOG_file

echo >> $LOG_file

# MAC statistics (DebugAPI)

echo =============================== >> $LOG_file
cat /proc/net/mtlk/wlan0/Debug/MACStats >> $LOG_file

if [ "$1" = "-v" ] 
then
 		cat $LOG_file
fi
