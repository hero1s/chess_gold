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
	echo "no param,restart all lobby server"
	sh ./stop.sh
	sleep 2
	#重启全部大厅服
    for var in ${all_lobby_svrs[*]}
    do
        svrid=${var}
        sh ./checkrun_server.sh $svrid
    done
fi