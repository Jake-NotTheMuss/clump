#!/bin/sh

srcdir=`dirname "$0"`
cd "$srcdir" || exit

LC_ALL=C
export LC_ALL

status=0

dotest() {
	name=$1
	rm -rf $name
	./clump -u $name.clump || return
	cd $name
	rm -f $name.clump
	find scripts -type f | sort | xargs ../clump -ao$name.clump
	cmp -b ../$name.clump $name.clump || status=1
	cd ..
}

dotest scriptgdb
dotest scriptsrc

test $status = 0 && echo Tests passed
