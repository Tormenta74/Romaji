#!/bin/bash

valid="source/valid"
invalid="source/invalid"
key=
verbose=0
compiled=0

if [ $1 == "-v" ]; then
    verbose=1
fi

for i in $(ls $valid); do
    echo "\$ ./rjicomp $valid/$i"
    if [ $verbose == 1 ]; then
        head -n 15 "$valid/$i" | grep "^#"
        echo -e "\n"
        ./rjicomp "$valid/$i"
        rc=$?; if [[ $rc == 0 ]]; then let "compiled++"; fi
        read key
    else
        ./rjicomp "$valid/$i" &> /dev/null
        rc=$?
        if [[ $rc == 0 ]]; then
            let "compiled++"
            echo -e "\033[0;32mgood\033[0m"
        else
            echo -e "\033[0;31mbad\033[0m"
        fi
    fi
done

echo -e "\$ ./cleanup.sh"
./cleanup.sh

echo -e "\033[0;32m$compiled\033[0m out of 12 compiled"
