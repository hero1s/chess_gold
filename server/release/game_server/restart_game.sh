#!/bin/sh
source ../config.sh

if [ $# -gt 0 ]; then
	echo "param num:"$#
	for arg in $*
	do
		svrid=$arg
		sh ./stop.sh $svrid
		sleep 2
		sh ./checkrun_server.sh $svrid
	done	
else
	echo "no param,restart all game server"
	#关闭全部游戏服
    sh ./stop.sh
	  sleep 2
	#重启全部游戏服
    for var in ${all_game_svrs[*]}
    do
        svrid=${var}
        sh ./checkrun_server.sh $svrid
    done
fi	



