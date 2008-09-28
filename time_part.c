#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>

#include "parts.h"

struct info {
  char *buf;
};

static struct info *
create (void)
{
  struct info *inf = malloc (sizeof (struct info));
  if (!inf)
    return NULL;

  inf->buf = NULL;

  return inf;
}

static void
destroy (struct info *inf)
{
  if (inf)
    {
      if (inf->buf)
	free (inf->buf);
      free (inf);
    }
}

static void
update (struct info *inf)
{
  struct timeval tv;

  assert (inf != NULL);

  if (inf->buf != NULL)
    {
      free (inf->buf);
      inf->buf = NULL;
    }

  gettimeofday (&tv, NULL);

  inf->buf = strdup (ctime (&tv.tv_sec));
  inf->buf[strlen (inf->buf) - 1] = '\0';
}

static void
paint (struct info *inf, cairo_t *cr)
{
  assert (inf != NULL);
  assert (cr != NULL);

  if (inf->buf == NULL)
    return;

  cairo_show_text (cr, inf->buf);
}

static double
get_width (struct info *inf, cairo_t *cr)
{
  cairo_text_extents_t ext;

  assert (inf != NULL);
  assert (cr != NULL);

  if (inf->buf == NULL)
    return 0.0;

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
