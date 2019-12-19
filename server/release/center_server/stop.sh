#!/bin/sh
source ../config.sh

if [ $# -gt 0 ]; then
	echo "param num:"$#
	for arg in $*
	do
		stop_svr_ $arg
	done
else
    for var in ${all_center_svrs[*]}
    do
        stop_svr_ ${var}
    done
fi






