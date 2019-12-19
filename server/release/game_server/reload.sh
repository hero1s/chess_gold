#!/bin/sh
source ../config.sh

if [ $# -gt 0 ]; then
	echo "param num:"$#
	for arg in $*
	do
		echo $arg
		reload_svr_ $arg
	done
else
	echo "no param,reload all game server"
	for var in ${all_game_svrs[*]}
	do
		reload_svr_ ${var}
	done
fi