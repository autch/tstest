
TARGET = tstest
CFLAGS += -O -g -Wall
LDFLAGS += -O -g -Wall

tstest: tstest.o crc.o mpegts.o
	$(CC) $(LDFLAGS) -o $@ $^

tstest.o: tstest.c mpegts.h mpegts_types.h
crc.o: crc.c crc.h
mpegts.o: mpegts.c mpegts.h mpegts_types.h

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o

