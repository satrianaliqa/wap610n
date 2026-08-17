#!/bin/sh

cd /opt/
svn checkout http://narnia/svn/wl/wl/linux/trunk/STAR/rootfs-star
cd rootfs-star
sudo tar xzf dev.tgz
rm -f dev.tgz
