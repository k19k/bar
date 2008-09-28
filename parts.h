#ifndef PARTS_H
#define PARTS_H

#include <cairo.h>

typedef void *(* part_create) (void);
typedef void  (* part_destroy) (void *);

typedef void   (* part_update) (void *);
typedef void   (* part_paint) (void *, cairo_t *);
typedef double (* part_get_width) (void *, cairo_t *);

struct part {
  const char *const name;

  part_create    create;
  part_destroy   destroy;
  part_update    update;
  part_paint     paint;
  part_get_width get_width;
};

extern struct part batt_part;
extern struct part load_part;
extern struct part mpd_part;
extern struct part time_part;
/* TODO: extern struct part wifi_part; */

#endif	/* PARTS_H */
