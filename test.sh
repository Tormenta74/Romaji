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
    head -n 15 "$valid/$i" | grep "^#"
    echo -e "\n"
    if [ $verbose == 1 ]; then
        ./rjicomp "$valid/$i"
        rc=$?; if [[ $rc == 0 ]]; then let "compiled++"; fi
        read key
    else
        ./rjicomp "$valid/$i" &> /dev/null
        rc=$?; if [[ $rc == 0 ]]; then let "compiled++"; fi
    fi
done

echo -e "\033[0;33m$compiled\033[0m out of 9 compiled"
