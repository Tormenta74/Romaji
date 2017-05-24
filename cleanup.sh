#!/bin/sh

# delete repeated lines produced by printing %
sed 's/^\tscanf("$//' -i source/valid/*.q.c
sed 's/^".*$//' -i source/valid/*.q.c 
# delete empty lines
sed '/^\s*$/d' -i source/valid/*.q.c
