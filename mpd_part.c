#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "parts.h"
#include "mpd.h"

#define MAX_LEN 255

struct info {
  mpd *client;

  int filehash;
  char buf[MAX_LEN + 1];
  enum { PLAY, STOP } state;
};

static struct info *
create (void)
{
  struct info *inf = malloc (sizeof (struct info));
  if (!inf)
    return NULL;

  inf->client = mpd_create ();
  if (!inf->client)
    goto error;

  inf->filehash = 0;
  inf->buf[0] = '\0';
  inf->state = STOP;

  return inf;

 error:
  free (inf);
  return NULL;
}

static void
destroy (struct info *inf)
{
  if (!inf)
    return;

  if (inf->client)
    mpd_destroy (inf->client);
  free (inf);
}

static void
format_result (struct info *inf, mpd_result *r)
{
  const char *s;
  int len = 0;

  inf->state = PLAY;

  s = mpd_result_get (r, "Composer");
  if (s)
    len = snprintf (inf->buf, MAX_LEN, "%s", s);
  else
    {
      s = mpd_result_get (r, "Artist");
      if (s)
	len = snprintf (inf->buf, MAX_LEN, "%s", s);
    }

  s = mpd_result_get (r, "Title");
  if (s)
    len += snprintf (inf->buf + len, MAX_LEN - len,
		     (len > 0) ? " - %s" : "%s", s);

  if (len == 0)
    {
      s = mpd_result_get (r, "file");
      if (s)
	snprintf (inf->buf, MAX_LEN, "%s", s);
      else
	snprintf (inf->buf, MAX_LEN, "[unknown]");
    }
}

static void
update (struct info *inf)
{
  mpd_result *r;

  assert (inf != NULL);

  if (mpd_connect (inf->client) != MPD_SUCCESS)
    goto stop;

  r = mpd_send_command (inf->client, MPD_COMMAND_CURRENTSONG, NULL);
  if (!r || r == MPD_OK)
    goto disconnect;

  format_result (inf, r);

  mpd_result_free (r);

 disconnect:
  mpd_disconnect (inf->client);
 stop:
  inf->state = STOP;
}

static const char *
_get_text (struct info *inf)
{
  return inf->state == PLAY ? inf->buf : "[not playing]";
}

static void
paint (struct info *inf, cairo_t *cr)
{
  assert (inf != NULL);
  assert (cr != NULL);

  cairo_show_text (cr, _get_text (inf));
}

static double
get_width (struct info *inf, cairo_t *cr)
{
  cairo_text_extents_t ext;

  assert (inf != NULL);
  assert (cr != NULL);

  cairo_text_extents (cr, _get_text (inf), &ext);
  return ext.width;
}

struct part mpd_part = {
  .name = "mpd",
  .create = (part_create) create,
  .destroy = (part_destroy) destroy,
  .update = (part_update) update,
  .paint = (part_paint) paint,
  .get_width = (part_get_width) get_width
};
