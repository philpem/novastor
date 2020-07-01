CFLAGS += -Ilzs-compression/c/src/liblzs

denova:	denova.o lzs-compression/c/src/liblzs/lzs-compression.o lzs-compression/c/src/liblzs/lzs-decompression.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

