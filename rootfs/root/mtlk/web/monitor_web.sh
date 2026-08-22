#!/bin/sh
# U-Media
# Jacky.Yang 19-Nov-2008, Monitor web server memory space
#

while true
do
	memSize=`ps |grep 'webs'|grep -v 'grep'|awk '{print $3}'`
	#echo $memSize
	if [ "$memSize" = "" ]
	then
		echo "Goahead has crashed, start Goahead again!"
		/root/mtlk/web/webs&
	elif [ $memSize -gt 4000 ]
	then
		echo "Re-Start goahead"
		kill `ps |grep 'webs'|grep -v 'grep'|awk '{print $1}'`
		/root/mtlk/web/webs&
	fi
	sleep 300
done