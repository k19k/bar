#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "parts.h"

struct info {
  char *buf;
  size_t n;
};

static struct info *
create (void)
{
  struct info *inf = malloc (sizeof (struct info));
  if (!inf)
    return NULL;

  inf->buf = NULL;
  inf->n = 0;

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
  FILE *f;

  assert (inf != NULL);

  f = fopen ("/proc/loadavg", "r");
  if (f)
    {
      getline (&inf->buf, &inf->n, f);
      inf->buf[strlen (inf->buf) - 1] = '\0';
      fclose (f);
    }
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

struct part load_part = {
  .name = "load",
  .create = (part_create) create,
  .destroy = (part_destroy) destroy,
  .update = (part_update) update,
  .paint = (part_paint) paint,
  .get_width = (part_get_width) get_width
};
