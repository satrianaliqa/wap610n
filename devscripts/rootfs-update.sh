#!/bin/sh

cd /opt/rootfs-star

mv dev dev-BAK
svn update $*
sudo tar xzf dev.tgz
rm -f dev.tgz
ls -al dev-BAK > dev-BAK.txt
ls -al dev > dev.txt
diff dev-BAK.txt dev.txt
rm dev-BAK.txt dev.txt

read -p "Delete old dev folder?   "  yesno
if [ $yesno = "y" ]
then
	rm -rf dev-BAK
else
	echo "Make your changes and then delete dev-BAK!"
fi

cd -
