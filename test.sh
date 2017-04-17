#!/bin/bash

valid="source/valid"
invalid="source/invalid"

for i in $( ls $valid); do
    echo "\$ ./rji $valid/$i"
    head -n 15 "$valid/$i" | grep "^#"
    echo -e "\n"
    ./rji "$valid/$i"
    echo -e "\n\n"
done
