# Create a gzipped rootfs file system, containing all the files in the rootfs dir.
# $id$
# TODO: Add error handling where needed

ROOTFS_DIR=${ROOTFS_DIR:-/opt/rootfs-star}
IMAGE_DIR=${IMAGE_DIR:-/opt/star/images}

#Jacky.Yang 6-May-2009
ROOTPATH=`cd ; pwd`
mkdir -p "${ROOTPATH}/METALINK/tmp/mnt/rd"
TMPFOLDER="${ROOTPATH}/METALINK/tmp/mnt/"

if [ ! -e "$ROOTFS_DIR" ]
then
	echo "No existing filesystem at $ROOTFS_DIR!"
	exit 1
fi

echo "Mounting rootfs"
# Run the mount script to mount the rootfs
mount-rd.sh -c

# Remove all the files from the old rootfs
# (unneeded now, because mount-rd is run with -c=clean option)
#sudo rm -rf /mnt/rd/*

echo "Copying rootfs files"
# Copy all the current files but skip svn directories
# (use tar exclude feature for this)
cd "$ROOTFS_DIR" || exit 1
echo -n "SVN Revision $(svnversion . 2>/dev/null || echo 'unknown')  " > etc/version
echo `date` >> etc/version
#sudo tar --exclude=*.svn* -czf /mnt/rd.tgz .
sudo tar --exclude=*.svn* -czf "${TMPFOLDER}/rd.tgz" .
cd - > /dev/null
#cd /mnt/rd
cd "${TMPFOLDER}/rd" || exit 1
#sudo tar xzf /mnt/rd.tgz
sudo tar xzf "${TMPFOLDER}/rd.tgz"
#sudo rm /mnt/rd.tgz
sudo rm "${TMPFOLDER}/rd.tgz"
cd - > /dev/null

echo "Unmounting rootfs"
# Unmount the rootfs and gzip it (and don't make a backup for a rootfs that is managed in svn)
umount-rd.sh -n 1
