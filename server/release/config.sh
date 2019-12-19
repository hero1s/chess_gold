#!/usr/bin/env bash

#全部游戏服
all_game_svrs=(11 21 )

#全部大厅服
all_lobby_svrs=(1 2)

#中心服
all_center_svrs=(8888 )

all_dir=("./center_server" "./lobby_server" "./game_server")

#进入目录执行脚本
function enter_exec_shell_(){
    root_dir=$1
    dirname=$2
    exesh=$3
	cd $dirname
	sh ./$exesh;
	cd $root_dir;
}
#进入目录组执行脚本
function enter_group_exec_shell_(){
    root_dir=$1
    dirnames=$2
    exesh=$3
    for var in ${dirnames[*]}
    do
        cd $var
        sh ./$exesh
        cd ../
    done
	cd $root_dir;
}
#reload脚本
function reload_svr_() {
  svrid=$1
	echo "reload svr:"$svrid
	cat pid_${svrid}.txt | xargs kill -12
}
#stop服务器
function stop_svr_() {
  svrid=$1
	echo "stop svr:"$svrid
	cat pid_${svrid}.txt | xargs kill -10
}

#start服务器



