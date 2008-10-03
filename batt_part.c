#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>

#include "parts.h"
#include "util.h"

/* TODO: reading /proc/acpi/battery/BAT1/<file> triggers some other
   processes.  Find a way of reading or being notified of the battery
   state that avoids this.

   Update: it doesn't look especially feasible.  The best we can do
   for now is to wait for notification from acpid to see if we
   actually need to update the battery state.

   Ahrrm... keep looking into this.  Sometimes acpid sends out battery
   messages much more often than I thought. */

struct info {
  int max;
  char *buf;

  int fd;
};

static void
_info_update (struct info *inf)
{
  char *buf = NULL;
  int value;
  enum { DISCHARGING, CHARGING, CHARGED, UNKNOWN } state = UNKNOWN;

  file_get_keys ("/proc/acpi/battery/BAT1/state", ':',
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

static struct info *
create (void)
{
  struct sockaddr_un sun;
  struct info *inf = malloc (sizeof (struct info));
  if (!inf)
    return NULL;

  file_get_keys ("/proc/acpi/battery/BAT1/info", ':',
		 "design capacity", "%d", 1, &inf->max,
		 NULL);

  inf->buf = NULL;

  inf->fd = socket (PF_UNIX, SOCK_STREAM, 0);
  if (inf->fd == -1)
    goto error;

  sun.sun_family = AF_UNIX;
  strncpy (sun.sun_path, "/var/run/acpid.socket", sizeof (sun.sun_path));
  if (connect (inf->fd, (struct sockaddr *) &sun, sizeof (sun)) == -1)
    goto connect_error;

  _info_update (inf);

  return inf;

 connect_error:
  close (inf->fd);
 error:
  free (inf);
  return NULL;
}

static void
destroy (struct info *inf)
{
  if (inf)
    {
      if (inf->buf)
	free (inf->buf);
      close (inf->fd);
      free (inf);
    }
}

static int
_ready (int fd)
{
  fd_set fds;
  struct timeval tv = { 0, 0 };

  FD_ZERO (&fds);
  FD_SET (fd, &fds);
  return (select (fd + 1, &fds, NULL, NULL, &tv) == 1);
}

static void
update (struct info *inf)
{
  char buf[BUFSIZ+1];
  char *p;
  int offset = 0;
  ssize_t len;
 
  assert (inf != NULL);
  assert (inf->fd != -1);

  buf[0] = '\0';

  for ( ; ; )
    {
      if (!_ready (inf->fd))
	break;

      len = recv (inf->fd, buf + offset, BUFSIZ - offset, 0);
      if (len == -1)
	return;

      buf[len] = '\0';
      p = buf - 1;
      fprintf (stderr, "[%.*s]\n", len-1, buf);
      while (p)
	{
	  p++;
	  if (strncmp (p, "battery", 7) == 0
	      || strncmp (p, "ac_adapter", 10) == 0)
	    break;
	  p = strchr (p, '\n');
	}
      if (p)
	{
	  _info_update (inf);
	  while (_ready (inf->fd))
	    recv (inf->fd, buf, BUFSIZ, 0);
	  break;
	}

      p = memrchr (buf, '\n', len);
      if (p)
	{
	  offset = len - (++p - buf);
	  memmove (buf, p, offset);
	}
      else
	offset = 0;
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
