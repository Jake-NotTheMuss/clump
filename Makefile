
CC=gcc

CFLAGS=-g -Wall -pedantic

src=main.c clump.c unclump.c hash.c path.c strt.c xmalloc.c file.c icmp.c opt.c
hdr=hash.h clump.h strt.h util.h opt.h

all: clump

clump: $(src) $(hdr)
	$(CC) $(CFLAGS) -o $@ $(src)

distclean: clean
	rm -f gdb_names.txt

clean:
	rm -f clump.exe

.PHONY: all clean distclean
