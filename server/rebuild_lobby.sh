#!/usr/bin/env bash

root_dir=`pwd`

rm ./release/lobby_server/lobbyServer -f

all_dir=("./servers/lobby_server/build"

	     )

while getopts rR opt
do
	case $opt in
		r)
			for var in ${all_dir[*]}
			do
				dirname=${var}
				cd $dirname
				sh ./rebuild.sh -r;
				cd $root_dir;
			done
			
            exit 0
            ;;
		*)
			;;
	esac
done
for var in ${all_dir[*]}
do
	dirname=${var}
	cd $dirname
	sh ./rebuild.sh;
	cd $root_dir;
done

