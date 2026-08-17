#!/bin/sh

# This script unpacks a src release for Metalink reference boards
# ARGS:
# $1 - nv release name
#

##set -x

if [ ! $1 ]
then
echo "USAGE: $0 <nv release name (without suffix)>"
echo -e "e.g.\n  $0  nv-1.2.12"
exit
fi

echo "--- Unpacking  ${1}-SRC.tgz ---"
sudo tar xzf ${1}-SRC.tgz
sudo chown -R `whoami` $1
cd $1
echo "--- Configuring default platform ---"
./make_nv.sh reconf vb4mb
echo "--- Building default platform ---"
./make_nv.sh build wlan.tar.gz
echo "--- DONE ---"
