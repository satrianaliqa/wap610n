#!/bin/sh

HOSTNAME=$1

hostname $1

echo $1 > /etc/hostname

export HOSTNAME=$1
