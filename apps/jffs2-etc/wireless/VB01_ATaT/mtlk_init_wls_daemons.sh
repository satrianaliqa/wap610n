#!/bin/sh


# Start the BCL Server
# TODO: We may want to remove the automatic BCL execution on customer dongles
# For debugging redirect to: /tmp/bcl.log, instead of /dev/null
echo "Starting BCL Server..."
if [ -e /bin/nice ]
then nice -n 18 $BCL_PATH/BclSockServer > /dev/null &
else
$BCL_PATH/BclSockServer > /dev/null &
fi


if expr $WLAN_NUM_BANDS = 2
then
	# Start the second BCL server
	if [ -e /bin/nice ]
	then nice -n 18 $BCL_PATH/bcl2 > /dev/null &
	else
	$BCL_PATH/bcl2 > /dev/null &
	fi
	
fi


# Start Web Server
# For debugging redirect to: /tmp/web.log, instead of /dev/null
echo "Starting Web Server..."
# use low priority
if [ -e /bin/nice ]
then 
	nice -n 18 $WEB_PATH/webs -c 0 > /dev/null &
else
	$WEB_PATH/webs -c 0 > /dev/null &
fi

if expr $WLAN_NUM_BANDS = 2
then
	# Start the second web server on port 81
if [ -e /bin/nice ]
then 
	nice -n 18 $WEB_PATH/webs -c 1 > /dev/null &
else
	$WEB_PATH/webs -c 1 > /dev/null &
fi
     
fi

