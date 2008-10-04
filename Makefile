tests = mpd_test

all: bar $(tests)

bar_obj = \
	bar.o \
	batt_part.o \
	load_part.o \
	main.o \
	mpd.o \
	mpd_part.o \
	time_part.o

CFLAGS = -D_GNU_SOURCE -g -Wall -Werror -MMD -MP \
	$(CAIRO_CFLAGS) $(LIBHAL_CFLAGS)

mpd_test: mpd_test.o mpd.o

bar: CAIRO_CFLAGS := $(shell pkg-config --cflags cairo-xlib)
bar: LIBHAL_CFLAGS := $(shell pkg-config --cflags hal)
bar: $(bar_obj)
	$(CC) -o $@ $^ \
		$(shell pkg-config --libs cairo-xlib) \
		$(shell pkg-config --libs hal) \
		-lXmu

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf bar $(tests) *.o *.d

.PHONY: all clean