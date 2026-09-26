#!/bin/sh

# Check if $1 was specified. If not, use Drango as default hostname
if [ -z "$1" ]
then
	HOSTNAME=Dorango
else
	HOSTNAME=$1
fi

hostname $HOSTNAME

echo $HOSTNAME > /etc/hostname

export HOSTNAME=`/bin/hostname`
