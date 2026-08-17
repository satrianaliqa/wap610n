#!/bin/sh

# This script creates a src release for Metalink reference designs
# ARGS:
# $1 - nv folder name
# $2 - full path to wlan.tar.gz file
#
# e.g. ./release-pack.sh  nv-1.2.12 /mnt/G/CombinedVersions/0-WLS_Versions/Integration/VB01/VB01.08/wlan.tar.gz

##set -x

if [ ! $2 ]
then
echo "USAGE: $0 <nv folder>  <path to wlan.tar.gz>"
echo -e "e.g.\n  ./release-pack.sh  nv-1.2.12 /mnt/G/CombinedVersions/0-WLS_Versions/Integration/VB01/VB01.08/wlan.tar.gz"
exit
fi


cd $1
cp $2 .
./make_nv.sh reconf star-6.7.2-mtlk-GPB237-VideoBridge-vela
cd boards/
rm -rf star-6.7.2-mtlk-U-Media-vela
cd ../..
echo "Creating ${1}-SRC.tgz"
tar --exclude=*.svn* -czf  ${1}-SRC.tgz $1
echo "--- Done ---"
