#!/bin/bash
set -e

# This is a script to select the star directory to work with.

if [ -z "${1}" ]
then
	cd boards || exit 1
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
		exit 1
	fi
	
	read -p "Enter selection:   " selected
	
	if [ -z "${selected}" ] || [ -z "${dirs[$selected]}" ]
	then
		echo Unknown directory. Quitting!
		exit 1
	fi
	SELECTED_PLATFORM=${dirs[$selected]}
	echo -e "Working with ${dirs[$selected]}.\n"
	cd - > /dev/null
else
	SELECTED_PLATFORM=${1%/}
	echo -e "Working with ${SELECTED_PLATFORM}.\n"
fi

[ -d "boards/${SELECTED_PLATFORM}" ] || { echo "ERROR: board directory boards/${SELECTED_PLATFORM} not found"; exit 1; }

# If there is RUNME script in the selected folder, run it
if [ -e boards/${SELECTED_PLATFORM}/RUNME ]
then
 pushd boards/${SELECTED_PLATFORM}
 ./RUNME
 popd
else
 # Update kernel configuration (skip forced make clean if .config is unchanged)
 pushd kernel/linux-2.6.16-star > /dev/null
 NEED_KCLEAN=1
 if [ -e .config ] && cmp -s .config "../../boards/${SELECTED_PLATFORM}/kernel/.config" 2>/dev/null; then
  NEED_KCLEAN=0
 fi
 rm -f .config
 ln -fs ../../boards/${SELECTED_PLATFORM}/kernel/.config
 export PATH=../../tools/arm-uclibc-3.4.6/bin/:$PATH
 if [ "$NEED_KCLEAN" -eq 1 ]; then
  make clean
 fi
 yes "" | make oldconfig
 cp -f ../../boards/${SELECTED_PLATFORM}/kernel/device_id.h arch/arm/boot/bootp/
 popd > /dev/null
 # Update applications configuration
 pushd apps > /dev/null
 rm -f .config
 if [ -e ../boards/${SELECTED_PLATFORM}/appsconfig/.config ]
 then
  ln -fs ../boards/${SELECTED_PLATFORM}/appsconfig/.config .config
 else
  echo "BOARD_NAME=legacy" > .config
 fi
 pushd busybox-1.8.1 > /dev/null
 rm -f .config
 ln -fs ../../boards/${SELECTED_PLATFORM}/busybox/.config .config
 popd > /dev/null
 popd > /dev/null
fi
exit 0

