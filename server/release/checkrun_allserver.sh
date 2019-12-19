#!/bin/bash
source ./config.sh
root_dir=`pwd`

while true
do
        for var in ${all_dir[*]}
        do
                dirname=${var}
                enter_exec_shell_ $root_dir $dirname "checkrun_server.sh";
        done
sleep 30
done
