#!/bin/bash

# This is a script to select the star directory to work with.
 
cd /opt

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
	exit
fi

read -p "Enter selection:   " selected

if expr `echo ${dirs[$selected]} | wc -c` = 1 > /dev/null
then
	echo Unknown directory. Quitting!
	exit
fi

echo -e "Working with ${dirs[$selected]}.\n"
if [ -h star ]
then 
	rm star
fi
ln -s ${dirs[$selected]} star

cd -

exit


