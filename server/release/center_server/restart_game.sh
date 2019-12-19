#!/bin/sh
source ../config.sh

ulimit -c unlimited

if [ $# -gt 0 ]; then
	echo "param num:"$#
	for arg in $*
	do
		echo $arg
		svrid=$arg
		sh ./stop.sh $svrid
		sleep 2
		sh ./checkrun_server.sh $svrid
	done
else
	echo "no param,restart all center server"
	sh ./stop.sh
	sleep 2
    for var in ${all_center_svrs[*]}
    do
        svrid=${var}
        sh ./checkrun_server.sh $svrid
    done
fi


