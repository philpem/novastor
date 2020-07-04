CFLAGS += -Ilzs-compression/c/src/liblzs

.PHONY: all

all: denova denovafile

denova:	denova.o lzs-compression/c/src/liblzs/lzs-compression.o lzs-compression/c/src/liblzs/lzs-decompression.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

denovafile:	denovafile.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

