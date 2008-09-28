tests = mpd_test file_keys_test

all: bar $(tests)

bar_obj = \
	bar.o \
	batt_part.o \
	load_part.o \
	main.o \
	mpd.o \
	mpd_part.o \
	time_part.o \
	util.o

CFLAGS = -D_GNU_SOURCE -g -Wall -Werror -MMD -MP $(CAIRO_CFLAGS)

mpd_test: mpd_test.o mpd.o

file_keys_test: file_keys_test.o util.o

bar: CAIRO_CFLAGS := $(shell pkg-config --cflags cairo-xlib)
bar: $(bar_obj)
	$(CC) -o $@ $^ $(shell pkg-config --libs cairo-xlib) -lXmu

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf bar $(tests) *.o *.d

.PHONY: all clean