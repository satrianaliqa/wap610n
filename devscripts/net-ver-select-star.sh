#!/bin/bash

# This is a script to select the star directory to work with.

if [ ! ${1} ]
then
	cd boards
	echo -e "\nSelect STAR platform\n"
	i=1
	for dir in `ls`
	do
		echo ${i}. $dir
		dirs[${i}]=$dir
		i=`expr ${i} + 1`
	done
	
	if [[ $i == 1 ]] 
	then 
		echo "Warning: No star source trees were found" 
		exit
	fi
	
	read -p "Enter selection:   " selected
	
	if expr `echo ${dirs[$selected]} | wc -c` = 1 > /dev/null
	then
		echo Unknown directory. Quitting!
		exit
	fi
	SELECTED_PLATFORM=${dirs[$selected]}
	echo -e "Working with ${dirs[$selected]}.\n"
	cd -  > /dev/null
else
	SELECTED_PLATFORM=${1}
	echo -e "Working with ${SELECTED_PLATFORM}.\n"
fi

# If there is RUNME script in the selected folder, run it
if [ -e boards/${SELECTED_PLATFORM}/RUNME ]
then
 pushd boards/${SELECTED_PLATFORM}
 ./RUNME
 popd
else
 # Update kernel configuration
 pushd kernel/linux-2.6.16-star > /dev/null
 rm -f .config
 ln -fs ../../boards/${SELECTED_PLATFORM}/kernel/.config
 export PATH=../../tools/arm-uclibc-3.4.6/bin/:$PATH
 make clean
 make oldconfig
 cp -f ../../boards/${SELECTED_PLATFORM}/kernel/device_id.h arch/arm/boot/bootp/
 popd  > /dev/null
 # Update applications configuration
 pushd apps > /dev/null
 rm -f .config
 if [ -e ../boards/${SELECTED_PLATFORM}/appsconfig/.config ]
 then
  ln -fs ../boards/${SELECTED_PLATFORM}/appsconfig/.config .config
 else
  echo "BOARD_NAME=legacy" > .config
 fi
 pushd busybox-1.8.1
 rm -f .config
 ln -fs ../../boards/${SELECTED_PLATFORM}/busybox/.config .config
 popd  > /dev/null
 popd  > /dev/null
fi
exit

