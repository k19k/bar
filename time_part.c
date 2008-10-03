#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>

#include "parts.h"

#define LEN 63

struct info {
  char buf[LEN+1];
};

static struct info *
create (void)
{
  struct info *inf = malloc (sizeof (struct info));
  if (!inf)
    return NULL;

  inf->buf[0] = '\0';
  inf->buf[LEN] = '\0';

  return inf;
}

static void
destroy (struct info *inf)
{
  if (inf)
    free (inf);
}

static void
update (struct info *inf)
{
  struct timeval tv;
  char *p;

  assert (inf != NULL);

  gettimeofday (&tv, NULL);

  strncpy (inf->buf, ctime (&tv.tv_sec), LEN);
  p = strchr (inf->buf, '\n');
  if (p)
    *p = '\0';
}

static void
paint (struct info *inf, cairo_t *cr)
{
  assert (inf != NULL);
  assert (cr != NULL);

  cairo_show_text (cr, inf->buf);
}

static double
get_width (struct info *inf, cairo_t *cr)
{
  cairo_text_extents_t ext;

  assert (inf != NULL);
  assert (cr != NULL);

  cairo_text_extents (cr, inf->buf, &ext);
  return ext.width;
}

struct part time_part = {
  .name = "time",
  .create = (part_create) create,
  .destroy = (part_destroy) destroy,
  .update = (part_update) update,
  .paint = (part_paint) paint,
  .get_width = (part_get_width) get_width
};
