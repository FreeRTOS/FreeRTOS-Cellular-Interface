#!/bin/bash
PWD=$(/usr/bin/pwd)
# dir=$(find . -mindepth 1 -maxdepth 1 -type d)
# $echo dir
echo $PWD
for D in *; do
    if [ -d "${D}" ]; then
        echo "process folder ${D}"   # your processing here
        cd ${D}
        make clean
        cd ..
    fi
done
cd $PWD
