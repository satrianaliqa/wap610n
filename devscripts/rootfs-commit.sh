#!/bin/sh

cd ./rootfs-star

tar czf dev.tgz dev
mv dev ../dev-BAK
svn commit $*
mv ../dev-BAK dev
rm dev.tgz

cd -

