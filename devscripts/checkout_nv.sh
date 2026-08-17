NUM_OF_PARAMS=$#

STAR_DIR=./star
CM_FILE=
ROOTFS_DIR=rootfs-star

####################################
# Functions
####################################
function display_usage()
{
 echo "$1"
 echo "Usage: checkout_nv <net_ver.cm> [<out_dir>]"
 echo "              Bring the sources from SVN repository into <out_dir>"
 echo "              If not specified, <out_dir> is assumed to be ./star" 
 echo "              net_ver.cm file defines the version of the modules" 
 exit
}


####################################
# Script body
####################################

#####
# Check usage
#####
if [[ ${NUM_OF_PARAMS} -eq 0 || ${NUM_OF_PARAMS} -gt 2 ]]
then
	display_usage ""
fi

CM_FILE=${1}
if [ ! -e ${CM_FILE} ]
then
	display_usage "Configuration management file $CM_FILE not found"
fi

if [ $2 ]
then
	STAR_DIR=${2}
fi

if [ -e $STAR_DIR ]
then
	rm -rf $STAR_DIR
fi

#####
# Prepare the working folder
#####
mkdir $STAR_DIR
cp $CM_FILE $STAR_DIR/net_ver.cm  > /dev/null
pushd $STAR_DIR > /dev/null
mkdir output
mkdir images
ln -s devscripts/make_nv.sh .
echo "Copying $CM_FILE to $STAR_DIR/net_ver.cm and applying it ..."
CM_FILE=net_ver.cm

# Workaround for the svn update problem
rm -rf .svn
svn checkout http://narnia/svn/wl/wl/linux/trunk/STAR/Networking-versions/working-space -N . > /dev/null

#####
# Bring the sources from SVN
#####
svn propset svn:externals . -F $CM_FILE  > /dev/null
if [ 0 != $? ]
then
	echo "Failed to set svn:externals"
	exit
fi
if [ -e ${ROOTFS_DIR}/dev ]
then
	rm -rf dev-BAK
	echo "Saving old previous version of dev in dev-BAK ..."
	mv ${ROOTFS_DIR}/dev ${ROOTFS_DIR}/dev-BAK
fi
echo "Updating sources ..."

# For SVN 1.5+, you must set update depth before updating externals, or else
# external folders will be ignored.
# For old svn (-1.4), this will produce an error, and then we will run
# svn update without the new parameter
svn update --set-depth infinity 2> /dev/null
if [ 0 != $? ]
then
	svn update
fi
if [ 0 != $? ]
then
	echo "Failed to perform svn update"
	exit
fi
pushd ${ROOTFS_DIR} > /dev/null
sudo tar xzf dev.tgz
#rm -f dev.tgz
popd  > /dev/null

# Configure applications
pushd apps > /dev/null
PWD=`pwd`
E_CROSS=${PWD}/../tools/arm-uclibc-3.4.6 E_KERNEL=${PWD}../kernel/linux-2.6.16-star make -f Makefile.MTLK link config
popd > /dev/null

popd  > /dev/null

exit

