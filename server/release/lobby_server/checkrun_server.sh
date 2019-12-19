#!/bin/sh
source ../config.sh

if [ $# -gt 0 ]; then
    echo "param num:"$#
	for arg in $*
	do
		echo $arg
		svrid=$arg
		pid=`cat pid_${svrid}.txt`
        echo "check lobby svr:"$svrid" pid "${pid}
        processnum=`ps ax | awk '{ print $1 }' | grep ${pid}$ | grep server | grep -v grep | wc -l`
        if [ $processnum -lt 1 ];then
            ./lobbyServer --sid ${svrid} --logmysql "mysqlerror.txt" --cfg "../server_config/server_config.lua" &
        else
            echo "restart lobby server fail: "$svrid
        fi
	done
else
    for var in ${all_lobby_svrs[*]}
    do
        svrid=${var}
        pid=`cat pid_${svrid}.txt`
        echo "check lobby svr:"$svrid" pid "${pid}
        processnum=`ps ax | awk '{ print $1 }' | grep ${pid}$  | grep -v grep | wc -l`
        if [ $processnum -lt 1 ];then
            ./lobbyServer --sid ${svrid} --logmysql "mysqlerror.txt" --cfg "../server_config/server_config.lua" &
        fi
    done
fi








