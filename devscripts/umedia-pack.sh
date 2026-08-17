#!/bin/sh

# This script creates a src release for UMEDIA 
# ARGS:
# $1 - nv folder name
# $2 - full path to wlan.tar.gz file
#
# e.g. ./umedia-release.sh  nv-1.2.12 /mnt/G/CombinedVersions/0-WLS_Versions/Integration/VB01/VB01.08/wlan.tar.gz

##set -x

#######################################################################
# List of folders/files that must be deleted when releasing to UMedia,
# according to Linksys GPL requirements:
DELETE_FILES="rootfs-star/sbin/iptables rootfs-star/etc/init.d rootfs-star/etc/network rootfs-star/lib/iptables rootfs-star/usr/bin/ldd rootfs-star/usr/bin/pmap rootfs-star/usr/sbin/ftl_check rootfs-star/usr/sbin/ftl_format"
#######################################################################



if [ ! $2 ]
then
	echo "USAGE: $0 <nv folder>  <path to wlan.tar.gz>"
	echo -e "e.g.\n  ./umedia-pack.sh  nv-1.2.12 /mnt/G/CombinedVersions/0-WLS_Versions/Integration/VB01/VB01.08/wlan.tar.gz"
	exit
fi

cd $1
cp $2 .
./make_nv.sh reconf star-6.7.2-mtlk-U-Media-vela

# Remove uneeded boards
cd boards/
rm -rf star-6.7.2-dorado star-6.7.2-mtlk-GPB236-LowCostRouter star-6.7.2-mtlk-GPB239-9102-Master star-6.7.2-mtlk-GPB239-9109-Slave star-6.7.2-mtlk-GPB244-LowCostRouter star-6.7.2-mtlk-GPB237-VideoBridge-vela star-6.7.2-mtlk-GPB261-VideoBridge star-6.7.2-mtlk-GPB262 star-6.7.2-mtlk-U-Media-vela-mPCI_HW vb4mb-2.3.10 vb4mb-profiling
cd ..

# Remove proprietary src code (e.g. upnpd based on Intel Device Builder)
cd apps
if [ -d mtlk_vb_upnpd ]
then
	# First make sure upnpd was compiled and installed at least once
	cd mtlk_vb_upnpd
	E_TOPDIR=`pwd`/../ make -f Makefile.MTLK CLEAN COMP INSTALL STRIP	
	cd -
	
	rm -rf mtlk_vb_upnpd
fi
cd ..

# Remove unneeded code when releasing to UMedia, per Linksys GPL requirements
for f in $DELETE_FILES
do
	if [ -e $f ] 
	then
		rm -rf $f
	fi
done

cd ..

# Remove any trailing slashes if a path name was given
NV_NAME=`echo $1 | sed "s/\///g"`

echo "Creating ${NV_NAME}-UMEDIA.tgz"
tar --exclude=*.svn* -czf  ${NV_NAME}-UMEDIA.tgz $1
echo "--- Done ---"
