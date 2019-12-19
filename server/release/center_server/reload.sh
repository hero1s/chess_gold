#!/bin/sh
source ../config.sh

for var in ${all_center_svrs[*]}
do
	reload_svr_ ${var}
done
