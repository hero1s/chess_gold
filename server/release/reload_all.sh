#!/bin/bash
source ./config.sh
root_dir=`pwd`

for var in ${all_dir[*]}
do
	dirname=${var}
	enter_exec_shell_ $root_dir $dirname "reload.sh";
done




