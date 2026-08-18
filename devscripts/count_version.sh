#!/bin/sh
ROOTDIR=${ROOTDIR:-$(pwd)}
#if [ -z "$ROOTDIR" ]
#then
#	echo "ROOTDIR is not set" >&2
#	exit 1
#fi

MODENAME=$1
VER_PATH=${ROOTDIR}/config
ENV_CONF=${ROOTDIR}/config/.config_$MODENAME
UI_CONF=${ROOTDIR}/config/autoconf.h
WEB_FW_VERSION=${ROOTDIR}/_wlan.tar.gz/web/fw_version.txt

. ${ENV_CONF}

VER_FILE=${VER_PATH}/${ProjectName}_VERSION

#if [ `expr match ${ProjectName} '\"'` -ge 1 ]; then
#	LEN=`expr length ${ProjectName}`
#	LEN=`expr ${LEN} - 2`
#	ProjectName=`expr substr ${ProjectName} 2 ${LEN}`
#	VER_FILE=${VER_PATH}/${ProjectName}_VERSION
#else
#	VER_FILE=${VER_PATH}/${ProjectName}_VERSION
#fi

if [ ! -e ${VER_FILE} ]; then
	echo "The file not exist, create ${VER_FILE}"
	echo > ${VER_FILE}
fi
#. ${ENV_CONF}
echo FIRMWARE_MAJOR=${FIRMWARE_MAJOR} FIRMWARE_MINOR=${FIRMWARE_MINOR} FIRMWARE_PATCH=${FIRMWARE_PATCH}
##############################################################################
# Include ${VER_FILE} file first, and check value.
. ${VER_FILE}
CHECK_FLAG=0
if [ ${MAJOR} -ne ${FIRMWARE_MAJOR} ]; then
	CHECK_FLAG=1
fi
if [ ${MINOR} -ne ${FIRMWARE_MINOR} ]; then
	CHECK_FLAG=1
fi
if [ ${PATCH} -ne ${FIRMWARE_PATCH} ]; then
	CHECK_FLAG=1
fi

if [ ${CHECK_FLAG} -eq 1 ]; then
	BUILD=0
#else
#	BUILD=`expr ${BUILD} + 1`
fi

############################################################################

#VER_STR="${FIRMWARE_MAJOR}.${FIRMWARE_MINOR}.${FIRMWARE_PATCH}.${BUILD}"
FIRMWARE_VERSION="${FIRMWARE_MAJOR}.${FIRMWARE_MINOR}.${FIRMWARE_PATCH}"
VER_STR="${FIRMWARE_MAJOR}.${FIRMWARE_MINOR}.${FIRMWARE_PATCH} build ${BUILD}"
VER_FILENAME="${ProjectName}_v${FIRMWARE_MAJOR}.${FIRMWARE_MINOR}.${FIRMWARE_PATCH}_build_${BUILD}"
echo "Current version: ${VER_STR}"

# Write back new version
echo "MAJOR=${FIRMWARE_MAJOR}" > ${VER_FILE}
echo "MINOR=${FIRMWARE_MINOR}" >> ${VER_FILE}
echo "PATCH=${FIRMWARE_PATCH}" >> ${VER_FILE}
echo "BUILD=${BUILD}" >> ${VER_FILE}
#echo "CUSTOMER_VER=${CUSTOMER_VER}" >> ${VER_FILE}
echo "FIRMWARE_VERSION=\"${FIRMWARE_VERSION}\"" >> ${VER_FILE}
#echo "ProjectName=\"${ProjectName}\"" > ${WEB_FW_VERSION}
#echo "FIRMWARE_VERSION=\"${FIRMWARE_VERSION}\"" >> ${WEB_FW_VERSION}

# Jacky.Yang 11-Jan-2008, Write to environment config.
sed -i '/FIRMWARE_BUILD/d' ${ENV_CONF}
sed -i '/FIRMWARE_VERSION/d' ${ENV_CONF}
sed -i '/FIRMWARE_FILENAME/d' ${ENV_CONF}
sed -i '/ProjectFirmwareVersionDate/d' ${ENV_CONF}
echo "FIRMWARE_BUILD=${BUILD}" >> ${ENV_CONF}
echo "FIRMWARE_VERSION=\"${FIRMWARE_VERSION}\"" >> ${ENV_CONF}
echo "FIRMWARE_FILENAME=\"${VER_FILENAME}\"" >> ${ENV_CONF}
#echo "ProjectFirmwareVersionDate=\""${VER_STR}", "`date +%e-%b-%Y`\" >> ${ENV_CONF}
#echo "ProjectFirmwareVersionDate=\""${VER_STR}", "`date +%e-%b-%Y`\" >> ${WEB_FW_VERSION}
echo "ProjectFirmwareVersionDate=\""${VER_STR}", "`date +'%b %e, %Y'`\" >> ${ENV_CONF}
#echo "ProjectFirmwareVersionDate=\""${VER_STR}", "`date +'%b %e, %Y'`\" >> ${WEB_FW_VERSION}

# Jacky.Yang 11-Jan-2008, Write to environment /config/autoconf.h for get ProjectFirmwareVersionDate in UI
#sed -i '/ProjectFirmwareVersionDate/d' ${UI_CONF}
#echo "#define ProjectFirmwareVersionDate \""${VER_STR}", "`date +%e-%b-%Y`\" >> ${UI_CONF}

#cp -p -v $ROOTDIR/output/bootpImage $ROOTDIR/output/${VER_FILENAME}
