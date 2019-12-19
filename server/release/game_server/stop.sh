#!/bin/sh
source ../config.sh

if [ $# -gt 0 ]; then
	echo "param num:"$#
	for arg in $*
	do
		stop_svr_ $arg
	done
else
	echo "no param,stop all game server"
	#关闭全部游戏服
	for var in ${all_game_svrs[*]}
	do
		stop_svr_ ${var}
	done
fi