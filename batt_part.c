#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

#include "parts.h"
#include "util.h"

struct info {
  int max;
  char *buf;
};

static struct info *
create (void)
{
  struct info *inf = malloc (sizeof (struct info));
  if (!inf)
    return NULL;

  file_get_keys ("/proc/acpi/battery/BAT1/info",
		 "design capacity", "%d", 1, &inf->max,
		 NULL);

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
  char *buf = NULL;
  int value;
  enum { DISCHARGING, CHARGING, CHARGED, UNKNOWN } state = UNKNOWN;

  assert (inf != NULL);

  file_get_keys ("/proc/acpi/battery/BAT1/state",
		 "charging state", "%as", 1, &buf,
		 "remaining capacity", "%d", 1, &value,
		 NULL);
  if (buf != NULL)
    {
      if (strcmp (buf, "discharging") == 0)
	state = DISCHARGING;
      else if (strcmp (buf, "charging") == 0)
	state = CHARGING;
      else if (strcmp (buf, "charged") == 0)
	state = CHARGED;
      else
	state = UNKNOWN;

      free (buf);
    }

  if (inf->buf)
    {
      free (inf->buf);
      inf->buf = NULL;
    }

  value = (value * 100) / inf->max;

  switch (state)
    {
    case DISCHARGING: asprintf (&inf->buf, "< %d%% >", value); break;
    case CHARGING:    asprintf (&inf->buf, "[ %d%% ]", value); break;
    case CHARGED:     asprintf (&inf->buf, "| %d%% |", value); break;
    case UNKNOWN: break;
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

struct part batt_part = {
  .name = "battery",
  .create = (part_create) create,
  .destroy = (part_destroy) destroy,
  .update = (part_update) update,
  .paint = (part_paint) paint,
  .get_width = (part_get_width) get_width
};
