#!/bin/bash

cat data/texture_list.txt | cut -f1 -d' ' /dev/stdin > temp_texpaths.txt
while read p; do
	./bin/superswizzle/superswizzle $p /mnt/ramdisk/test/$(basename $p).png || true
done <temp_texpaths.txt
