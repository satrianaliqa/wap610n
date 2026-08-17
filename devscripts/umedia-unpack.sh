#!/bin/sh

# This script unpacks a src release for UMEDIA 
# ARGS:
# $1 - nv release name
#

##set -x

if [ ! $1 ]
then
echo "USAGE: $0 <nv release name (without suffix)>"
echo -e "e.g.\n  ./umedia-unpack.sh  nv-1.2.12"
exit
fi


echo "Unpacking tarball"
sudo tar xzf ${1}-UMEDIA.tgz
sudo chown -R `whoami` $1
cd $1
./make_nv.sh reconf star-6.7.2-mtlk-U-Media-vela
./make_nv.sh build wlan.tar.gz
