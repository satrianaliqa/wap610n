UPDATE_SRC=0
RECONFIGURE_PLATFORM=0
MAKE_CLEAN=0
MAKE_BUILD=0
MAKE_DEBUGFS=0
NFS_SERVER=0
NUM_OF_PARAMS=$#

NFS_DIR="tftp"
LOCAL_NFS_DIR="/tmp/nfs.$$"

STAR_DIR=.
NEW_PLATFORM=
CM_FILE=${STAR_DIR}/net_ver.cm
WLAN_ROOTFS_FILE=
MODENAME=
ROOTFS_DIR=rootfs-star
DEBUGFS_DIR=debugfs/apps
NFS_SERVER_DEFAULT=172.16.20.39

# Jacky.Yang 21-Jul-2008, define WEB Server path
ROOTDIR=`pwd`
WEBSERVER_PATH=`pwd`/apps/WebServer

####################################
# Functions
####################################
function display_usage()
{
 echo "$1" 
 echo "Usage: make_nv <command> [<options>]" 
 echo ""  
 echo "Supported commands:" 
 echo "       update [<net_ver.cm>]" 
 echo "              Update the sources from SVN repository according to net_ver.cm file" 
 echo "              It is assumed that current folder contains valid sources"  
 echo "       reconf [<platform_name>]" 
 echo "              Reselect current platform to work on. If the name of the platform" 
 echo "              is not specified, the user is requested to choose from the list"  
 echo "              It is assumed that current folder contains valid sources" 
 echo "       clean" 
 echo "              Make clean for all, including applications and rootfs image"  
 echo "       build [<wlan.tar.gz>]" 
 echo "              Build Net_Ver files; if the wlan.tar.gz is specified, build the bootp image as well" 
 echo "       debugfs <clean/build/install/all> [<NFS server>]" 
 echo "              Build, clean or install debug application files; in case of installation,"
 echo "              note the IP address of your NFS server or type 'default' to update a compilation linux server." 
 echo "              Commands of debugfs can be sticked together."
 echo "              For example: make_nv debugfs clean all install 10.0.0.1"
 echo "              install options:"
 echo "              install default - use default IP 172.16.20.39 for NFS server"
 echo "              install 10.1.2.3 - use IP 10.1.2.3 as NFS server"
 echo "              install local /tmp/path/to/local_directory - given path and install there debugfs"
 exit
}

function check_params()
{
 if [ "${1}" = "update" ]
 then
  if [ ${NUM_OF_PARAMS} -gt 2 ]
  then
   display_usage ""
  fi
  if [ $2 ]
  then
   CM_FILE=$2
  fi
  if [ ! -e ${CM_FILE} ]
  then
   display_usage "Configuration management file $CM_FILE not found"
  fi
  UPDATE_SRC=1

 elif [ "${1}" = "reconf" ]
 then
  if [ ${NUM_OF_PARAMS} -gt 2 ]
  then
   display_usage ""
  fi
  RECONFIGURE_PLATFORM=1
  NEW_PLATFORM=$2
  #Jacky.Yang 23-Dec-2008, We don't need to make clean.
  #MAKE_CLEAN=1

elif [ "${1}" = "clean" ]
 then
  #if [ ${NUM_OF_PARAMS} -ne 1 ]
  if [ ${NUM_OF_PARAMS} -ne 2 ]
  then
   display_usage ""
  fi
  MAKE_CLEAN=1
  WLAN_ROOTFS_FILE=$2

elif [ "${1}" = "build" ]
 then
  #if [ ${NUM_OF_PARAMS} -gt 2 ]
  if [ ${NUM_OF_PARAMS} -ne 3 ]
  then
   display_usage ""
  fi
  MAKE_BUILD=1
  WLAN_ROOTFS_FILE=$2
  MODENAME=$3
  ENV_CONF=${ROOTDIR}/config/.config_${MODENAME}
  if [ "${MODENAME}" = "WAP610N" ]; then
    echo "WAP610N mode"
  elif [ "${MODENAME}" = "WET610N" ]; then
    echo "WET610N mode"
  else
    display_usage ""
  fi
## Install bedug fs requeres an IP address of a NFS server
elif [ "${1}" = "debugfs" ]
 then
  if [ ${NUM_OF_PARAMS} -gt 2 ]
  then
   display_usage ""
  fi
  MAKE_DEBUGFS=1 
elif [ "${1}" = "install-debugfs" ]
 then
  if [ ${NUM_OF_PARAMS} -ne 2 ]
  then
   display_usage ""
  fi
  MAKE_DEBUGFS=2 
  NFS_SERVER=$2
else
  display_usage "Unknown command $1"
fi 

 return 0
}


function svn_update ()
{
# Workaround for the svn update problem - checkout current folder (non-recursive)
# and apply externals
	
	# Delete all local svn-versioned files, because we perform a checkout of the 
	# current folder with every update
	LOCAL_VERSIONED_FILES=`svn list`
	for f in $LOCAL_VERSIONED_FILES
	do
		# mv if modified, otherwise delete
		SVNSTAT=`svn status $f | wc -l`
		if [ $SVNSTAT -ne 0 ]
		then
			mv $f ${f}.ORIG
			echo -e "\nLOCALLY MODIFIED FILE $f WAS MOVED TO ${f}.ORIG"
		else
			rm $f
		fi
	done
	rm -rf .svn

	svn checkout http://narnia/svn/wl/wl/linux/trunk/STAR/Networking-versions/working-space -N . > /dev/null

	echo "Applying $CM_FILE ..."
	svn propset svn:externals . -F $CM_FILE  > /dev/null
	if [ 0 != $? ]
	then
		echo "Failed to set svn:externals"
		exit
	fi

	echo "Updating sources ..."

	# For SVN 1.5+, you must set update depth before updating externals, or else
	# external folders will be ignored.
	# For old svn (-1.4), this will produce an error, and then we will run
	# svn update without the new parameter
	svn update --set-depth infinity $* 2> /dev/null
	if [ 0 != $? ]
	then
		svn update  $*
	fi
	if [ 0 != $? ]
	then
		echo "Failed to svn update"
		exit
	fi
	cd ${ROOTFS_DIR}
	sudo tar xzf dev.tgz
	rm -f dev.tgz
	cd -  > /dev/null

	return 0
}


####################################
# Script body
####################################
LOGFILE=`pwd`/log.log
echo "" > $LOGFILE

if [[ ${NUM_OF_PARAMS} -eq 0 || ${NUM_OF_PARAMS} -gt 6 ]]
then
 display_usage ""
else
 check_params $*
fi


#####
# Bring the sources from SVN
#####
if [ ${UPDATE_SRC} -eq 1 ]
then
 svn_update 2>&1 | tee -a $LOGFILE
fi

#####
# Select the platform if requested
#####
if [ $RECONFIGURE_PLATFORM -eq 1 ]
then
 devscripts/net-ver-select-star.sh $NEW_PLATFORM  2>&1  | tee -a $LOGFILE
 # Jacky.Yang 23-Dec-2008, Start - We need make clean and re-config it
 # Make clean and configure for applications
 pushd apps/scripts > /dev/null
 PWD=`pwd`
 E_CROSS=${PWD}/../../tools/arm-uclibc-3.4.6 E_KERNEL=${PWD}/../../kernel/linux-2.6.16-star make -f Makefile.MTLK link clean config  | tee -a $LOGFILE
 popd > /dev/null

 
 # Make clean in kernel tree
 cd kernel/linux-2.6.16-star
 export PATH=../../tools/arm-uclibc-3.4.6/bin/:$PATH
 make clean | tee -a $LOGFILE
 cd - > /dev/null
 rm -rf images/*
 rm -rf output/* 
 # Jacky.Yang 23-Dec-2008, End - We need make clean and re-config it
 
fi

#####
# Perform cleaning if requested
#####
if [ $MAKE_CLEAN -eq 1 ]
then
 # Make clean and configure for applications
 rm -f ${PWD}/apps/l
 pushd apps/scripts > /dev/null
 PWD=`pwd`
 # Jacky.Yang 23-Dec-2008, We don't need re-config it, becasue this is make clean procedure.
 #E_CROSS=${PWD}/../../tools/arm-uclibc-3.4.6 E_KERNEL=${PWD}/../../kernel/linux-2.6.16-star make -f Makefile.MTLK link clean config 2>&1 | tee -a $LOGFILE
 E_CROSS=${PWD}/../../tools/arm-uclibc-3.4.6 E_KERNEL=${PWD}/../../kernel/linux-2.6.16-star make -f Makefile.MTLK link clean 2>&1 | tee -a $LOGFILE
 popd > /dev/null

 # Make clean in kernel tree
 cd kernel/linux-2.6.16-star
 export PATH=`pwd`/../../tools/arm-uclibc-3.4.6/bin/:$PATH
 make clean 2>&1  | tee -a $LOGFILE
 cd - > /dev/null
 rm -rf images/*
 rm -rf output/*
 rm -f ${PWD}/apps/l/*
 rm -f ${PWD}/kernel/linux-2.6.16-star/.config
 rm -f ${PWD}/kernel/linux-2.6.16-star/.config.old
 rm -f ${PWD}/kernel/linux-2.6.16-star/.version
fi

#####
# If it is debugfs build - build it and exit
#####

# make_modules - build kernel modules.
# used to build modules (e.g. for debugfs or VLAN support)
function make_modules
{
	pushd kernel/linux-2.6.16-star
	echo Making dynamic modules  | tee -a $LOGFILE
	export PATH=`pwd`/../../tools/arm-uclibc-3.4.6/bin/:$PATH
	make modules 2>&1 | tee -a $LOGFILE
	popd > /dev/null
}

# Just to minimize the code
function make_debugfs
{
	pushd ${DEBUGFS_DIR}  > /dev/null
	make -f Makefile.MTLK $1
	popd  > /dev/null
}

function set_nfs_server
{
	if [ "$1" != "" ]
	then
		NFS_SERVER=$3
	else
		NFS_SERVER=${NFS_SERVER_DEFAULT}
	fi
	echo Set NFS server to $1
}

# Compilation if debugfs
# Commands of debugfs may be sticked together.
# For example: make_nv debugfs clean all install 10.0.0.1

if [ 1 = ${MAKE_DEBUGFS} ]
then
	while [ $2 ]
	do
		if [ "clean" = ${2} ]
		then
			echo Cleaning up debugfs
			make_debugfs clean
			shift 1

		elif [ "install" = ${2} ]
		then
			if [ ${3} = "local" ]
			then # Installation to local directory
				if [ ! -d $4 ]
				then
					install 777 $4
				fi
				pushd ${DEBUGFS_DIR}
				E_DEBUGFS=${4} make -f Makefile.MTLK install
				popd  > /dev/null
				shift 3		

			else # NFS installation
	
				set_nfs_server $3
				echo Installation of debugfs on ${NFS_SERVER}
				sudo devscripts/mount_nfs.sh ${NFS_SERVER} ${NFS_DIR} ${LOCAL_NFS_DIR}
				if [ 0 != $? ]
				then
					echo Error on NFS mounting. Stop.
					sudo rmdir ${LOCAL_NFS_DIR}
					exit
				fi
				# Now run installation of the debugfs with custom parameters of the 
				pushd ${DEBUGFS_DIR}
				E_DEBUGFS=${LOCAL_NFS_DIR} make -f Makefile.MTLK install
				popd  > /dev/null
				# Uninstall the NFS dir
				sudo devscripts/umount_nfs.sh ${LOCAL_NFS_DIR}
				if [ $3 ]
				then
					shift 2
				else
					shift 1
				fi
			fi
			

		elif [ "all" = ${2} ]
		then
			pushd ${DEBUGFS_DIR}
			E_DEBUGFS=${LOCAL_NFS_DIR} make -f Makefile.MTLK clean all 
			popd > /dev/null
			make_modules
			shift 1
			
		else # If debugfs ran without arguments - debugfs all, without clean or installation
			echo Compilation of debugfs
			make_debugfs all		
			make_modules
			shift 1
		fi
	done
	exit
fi




#####
# Build the image
#####
if [ $MAKE_BUILD -eq 1 ]
then
 # Make for kernel and applications

 # First compile zImage in order to be able to compile gpio driver
 pushd kernel/linux-2.6.16-star
 export PATH=`pwd`/../../tools/arm-uclibc-3.4.6/bin/:$PATH
 make zImage -j3 2>&1 | tee -a $LOGFILE
 if [ 0 != $? ]
 then
   echo "Failed to build kernel zImage" | tee -a $LOGFILE
   exit
 fi
 popd > /dev/null

 # Compile kernel modules
 make_modules

 # Now compile the apps
 pushd apps/scripts > /dev/null
 PWD=`pwd`
 E_CROSS=${PWD}/../../tools/arm-uclibc-3.4.6 E_KERNEL=${PWD}/../../kernel/linux-2.6.16-star make -f Makefile.MTLK comp install strip customize 2>&1 | tee -a $LOGFILE
 if [ 0 != $? ]
 then
   echo "Failed to build applications" | tee -a $LOGFILE
   exit
 fi
 popd > /dev/null
# pushd rootfs > /dev/null
# sudo tar xzf dev.tgz
# rm -f dev.tgz
# popd > /dev/null


#Jacky.Yang 21-Jul-2008, Begin automatically to build Web Server
	cd ${ROOTDIR}
    if [ -e _${WLAN_ROOTFS_FILE} ]; then
        echo "Folder _${WLAN_ROOTFS_FILE} had exist."
#    else
	#echo "Folder _${WLAN_ROOTFS_FILE} don't exist, create it."
	#mkdir _${WLAN_ROOTFS_FILE}
	#tar xzvf ${WLAN_ROOTFS_FILE} -C _${WLAN_ROOTFS_FILE}
    fi
	# Keep intact Linksys GUI assets from wlan.tar.gz
	echo "--> Keeping full Linksys GUI web assets for ${MODENAME}..."
	
	#cd ${WEBSERVER_PATH}
	#./release.sh
	cd ${ROOTDIR}
	#Jacky.Yang 21-Jul-2008, create build number and re-name firmware file name.
	sed -i '/ProjectName/d' ${ENV_CONF}
	echo "ProjectName=${MODENAME}" >> ${ENV_CONF}
	devscripts/count_version.sh ${MODENAME}
	#cd ${ROOTDIR}/_${WLAN_ROOTFS_FILE}
	#tar cvf - * | gzip > ../${WLAN_ROOTFS_FILE}
	#tar cvf - * --exclude=*.svn | gzip > ../${WLAN_ROOTFS_FILE}
	cd ${ROOTDIR}
#Jacky.Yang 21-Jul-2008, End automatically to build Web Server

 # Create rootfs image
 mkdir -p ${ROOTDIR}/output ${ROOTDIR}/images
 devscripts/net-ver-mkrootfs.sh $WLAN_ROOTFS_FILE 2>&1 | tee -a $LOGFILE

 # Make net_ver images/bootp image
 cd kernel/linux-2.6.16-star > /dev/null
# export PATH=../../tools/arm-uclibc-3.4.6/bin/:$PATH
 ./mkbootp.sh ../.. 2>&1 | tee -a $LOGFILE
 if [ 0 != $? ]
 then
   echo "Failed to build kernel image" | tee -a $LOGFILE
   exit
 fi
cd - > /dev/null

 cp kernel/linux-2.6.16-star/arch/arm/boot/bootp/kernel.o output/
 cp kernel/linux-2.6.16-star/arch/arm/boot/bootp/init.o output/
 cp kernel/linux-2.6.16-star/arch/arm/boot/bootp/initrd.S output/
 cp kernel/linux-2.6.16-star/arch/arm/boot/bootp/bootp.lds output/
 cp images/ramdisk_2.6.16.img.lzma output/
## cp images/ramdisk_2.6.16.img.gz output/
 cp ${CM_FILE} output/

 cd output/
 tar czf na.tar.gz kernel.o init.o initrd.S bootp.lds ramdisk_2.6.16.img.lzma
## tar czf na.tar.gz kernel.o init.o initrd.S bootp.lds ramdisk_2.6.16.img.gz
 cd - > /dev/null


 #Jacky.Yang 21-Jul-2008, re-name firmware file name.
 cd ${ROOTDIR}
 devscripts/fw_rename.sh ${MODENAME}
 cd - > /dev/null

fi


# Create a summary of compilation errors:
NUM_ERRS=`grep -v ignored $LOGFILE | grep -c Err`
if [ $NUM_ERRS -gt 0 ]
then
	echo -e "\n-----------------------------------------------------"  | tee -a $LOGFILE
	echo SUMMARY OF COMPILATION ERRORS:  | tee -a $LOGFILE 
	grep -v ignored $LOGFILE | grep Err -B 4 -A 1  | tee -a $LOGFILE
	echo -e "\nEND OF COMPILATION ERRORS."  | tee -a $LOGFILE 
	echo -e "-----------------------------------------------------"  | tee -a $LOGFILE
fi
# Jacky.Yang 19-Mar-2009, delete log file for svn commit.
rm -f $LOGFILE

exit
