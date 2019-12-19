#!/bin/sh
source ../config.sh

for var in ${all_lobby_svrs[*]}
do
	reload_svr_ ${var}
done
