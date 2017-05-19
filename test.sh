#!/bin/bash

valid="source/valid"
invalid="source/invalid"
key=

for i in $( ls $valid); do
    echo "\$ ./rjicomp $valid/$i"
    head -n 15 "$valid/$i" | grep "^#"
    echo -e "\n"
    ./rjicomp "$valid/$i"
    echo -e "\n\n"
    read key
done
