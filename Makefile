CFLAGS += -Ilzs-compression/c/src/liblzs

.PHONY: all

all: denova denovafile

denova:	denova.o utils.o lzs-compression/c/src/liblzs/lzs-compression.o lzs-compression/c/src/liblzs/lzs-decompression.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

denovafile:	denovafile.o utils.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

