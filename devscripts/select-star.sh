#!/bin/bash

# This is a script to select the star directory to work with.
 
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}/boards" 2>/dev/null || cd "${ROOT_DIR}" || exit 1

echo -e "\nSelect which STAR source tree to use\n"
i=1
for dir in `ls | grep "^star-"`
do
	echo ${i}. $dir
	dirs[$i]=$dir
	i=`expr $i + 1`
done

if [[ $i == 1 ]] 
then 
	echo "Warning: No star source trees were found" 
	exit 1
fi

read -p "Enter selection:   " selected

if [ -z "${selected}" ] || [ -z "${dirs[$selected]}" ]
then
	echo Unknown directory. Quitting!
	exit 1
fi

echo -e "Working with ${dirs[$selected]}.\n"
if [ -h star ]
then 
	rm star
fi
ln -s ${dirs[$selected]} star

cd - > /dev/null

exit 0


