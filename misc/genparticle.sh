#!/bin/sh

if [ $# -lt 3 ]; then
	echo usage: genparticle name pathprefix blendfunc
fi

i=1
echo "// generated by genparticle.sh"
for f in `ls *.png`; do
	echo $1$i
	echo "{"
	echo "	cull none"
	echo "	{"
	printf "	map %s/%04d.png\n" $2 $i
	echo "	blendfunc $3"
	echo "	}"
	echo "}"
	i=$((i+1))
done
