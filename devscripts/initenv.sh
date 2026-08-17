#!/bin/bash

DEVSCRIPTS_PATH=/opt/devscripts

# add devscripts to path
export PATH="$PATH:$DEVSCRIPTS_PATH"

# select your board tree 
$DEVSCRIPTS_PATH/select-star.sh

# run the star script for setting up the environment
source /opt/star/kforwork-3.3
