#!/bin/sh
#
#
# mtlk_read_ENV config_file var_name env_var_name
#
# Read a variable from the ENV and place it into the configuration file as  var_name
CONFIG_FILE=$1
VAR_NAME=$2
ENV_VAR_NAME=$3

grep -v $VAR_NAME $CONFIG_FILE >$CONFIG_FILE.tmp
mv $CONFIG_FILE.tmp $CONFIG_FILE
echo "$VAR_NAME = `get_env_param $ENV_VAR_NAME`" >> $CONFIG_FILE