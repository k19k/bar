tests = mpd_test

all: $(tests)

CFLAGS = -D_GNU_SOURCE -g -Wall -MMD -MP

mpd_test: mpd_test.o mpd.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(tests) *.o *.d

.PHONY: all clean