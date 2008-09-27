#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "mpd.h"

struct mpd {
  int socket;

  char *hostname;
  unsigned short port;

  char *version;

  mpd_error last_error;
  char *last_ack;
};

struct mpd_result {
  size_t count;
  size_t current;

  const char **keys;
  const char **values;

  char *buf;
};

static mpd_result _OK = { 0, 0, NULL, NULL, NULL };

mpd_result *MPD_OK;

static const char *_mpd_commands[] = {
  "play",
  "playid",
  "stop",
  "pause",
  "status",
  "kill",
  "close",
  "add",
  "addid",
  "delete",
  "deleteid",
  "playlist",
  "shuffle",
  "clear",
  "save",
  "load",
  "listplaylist",
  "listplaylistinfo",
  "lsinfo",
  "rm",
  "playlistinfo",
  "playlistid",
  "find",
  "search",
  "update",
  "next",
  "previous",
  "listall",
  "volume",
  "repeat",
  "random",
  "stats",
  "clearerror",
  "list",
  "move",
  "moveid",
  "swap",
  "swapid",
  "seek",
  "seekid",
  "listallinfo",
  "ping",
  "setvol",
  "password",
  "crossfade",
  "urlhandlers",
  "plchanges",
  "plchangesposid",
  "currentsong",
  "enableoutput",
  "disableoutput",
  "outputs",
  "commands",
  "notcommands",
  "playlistclear",
  "playlistadd",
  "playlistfind",
  "playlistsearch",
  "playlistmove",
  "playlistdelete",
  "tagtypes",
  "count",
  "rename"
};

const char *_mpd_errors[] = {
  "success",
  "unknown error",
  "not connected",
  "already connected",
  NULL, 			/* use MPD's ACK message */
  "not MPD on the specified port",
  "unable to connect",
};

static void __attribute__((__constructor__))
_mpd_init (void)
{
  MPD_OK = &_OK;
}

static inline mpd_error
_mpd_error (mpd *self, mpd_error code, const char *ack)
{
  self->last_error = code;
  if (ack)
    {
      if (self->last_ack)
	free (self->last_ack);
      self->last_ack = strdup (ack);
    }
  return code;
}

mpd *
mpd_create (void)
{
  mpd *self = malloc (sizeof (mpd));

  if (!self)
    return NULL;

  self->socket = -1;
  self->hostname = strdup ("localhost");
  self->port = 6600;

  self->version = NULL;
  self->last_error = MPD_SUCCESS;
  self->last_ack = NULL;

  if (self->hostname == NULL)
    {
      free (self);
      return NULL;
    }

  return self;
}

static void
_mpd_disconnect (mpd *self)
{
  if (self->version)
    {
      free (self->version);
      self->version = NULL;
    }
  if (self->socket != -1)
    {
      close (self->socket);
      self->socket = -1;
    }
}

void
mpd_destroy (mpd *self)
{ 
  if (!self)
    return;

  _mpd_disconnect (self);

  if (self->hostname)
    free (self->hostname);

  if (self->version)
    free (self->version);

  if (self->last_ack)
    free (self->last_ack);
}

const char *
mpd_get_last_error (mpd *self, mpd_error *id)
{
  assert (self != NULL);

  if (id)
    *id = self->last_error;

  if (self->last_error == MPD_ERROR_ACK
      || (self->last_error == MPD_ERROR && self->last_ack))
    {
      assert (self->last_ack != NULL);
      return self->last_ack;
    }

  assert (self->last_error >= MPD_SUCCESS);
  assert (self->last_error < __MPD_ERROR_LAST);

  return _mpd_errors[self->last_error];
}

const char *
mpd_get_version (mpd *self)
{
  assert (self != NULL);

  if (self->socket == -1)
    _mpd_error(self, MPD_ERROR_NOT_CONNECTED, NULL);

  return self->version;
}

mpd_error
mpd_set_hostname (mpd *self, const char *hostname)
{
  assert (self != NULL);
  assert (hostname != NULL);

  if (self->socket != -1)
    return _mpd_error (self, MPD_ERROR_ALREADY_CONNECTED, NULL);

  if (self->hostname)
    free (self->hostname);

  self->hostname = strdup (hostname);

  return MPD_SUCCESS;
}

mpd_error
mpd_set_port (mpd *self, unsigned short port)
{
  assert (self != NULL);

  if (self->socket != -1)
    return _mpd_error (self, MPD_ERROR_ALREADY_CONNECTED, NULL);

  self->port = port;

  return MPD_SUCCESS;
}

const char *
mpd_get_hostname (mpd *self)
{
  assert (self != NULL);

  return self->hostname;
}

unsigned short
mpd_get_port (mpd *self)
{
  assert (self != NULL);

  return self->port;
}

static mpd_error
_mpd_connect (mpd *self)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  char port[8];
  int s;

  if (self->socket != -1)
    return _mpd_error (self, MPD_ERROR_ALREADY_CONNECTED, NULL);

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  sprintf (port, "%hu", self->port);

  s = getaddrinfo (self->hostname, port, &hints, &result);
  if (s != 0)
    return _mpd_error (self, MPD_ERROR, gai_strerror (s));

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      self->socket = socket (rp->ai_family, rp->ai_socktype,
			     rp->ai_protocol);
      if (self->socket == -1)
	continue;

      if (connect (self->socket, rp->ai_addr, rp->ai_addrlen) != -1)
	break;

      close (self->socket);
      self->socket = -1;
    }

  freeaddrinfo (result);

  if (rp == NULL)
    return _mpd_error (self, MPD_ERROR_CONNECT, NULL);
  
  return MPD_SUCCESS;
}

#define OK_MPD "OK MPD"
#define OFFSET (sizeof (OK_MPD))

static void
_get_response (int fd, char **bufp, size_t *size)
{
  size_t len = 0;
  char buf[BUFSIZ];

  for ( ; ; )
    {
      ssize_t count;

      if (len + BUFSIZ > *size)
	{
	  *size *= 2;
	  char *new_buf = realloc (*bufp, *size);
	  if (!new_buf)
	    goto error;
	  *bufp = new_buf;
	}

      count = recv (fd, buf, BUFSIZ - 1, 0);
      if (count < 0)
	goto error;

      buf[count] = '\0';
      strcpy (*bufp + len, buf);
      len += count;
      
      if (count > 1)
	{
	  char *s = memrchr (buf, '\n', count - 1);
	  s = s ? s + 1 : buf;
	  if (strncmp(s, "OK", 2) == 0)
	    return;
	}
    }

 error:
  free (*bufp);
  *bufp = NULL;
}

static mpd_error
_mpd_recv (mpd *self, char **bufp)
{
  size_t size;

  assert (bufp != NULL);

  size = BUFSIZ;
  *bufp = malloc (size);
  if (*bufp == NULL)
    goto error;

  _get_response (self->socket, bufp, &size);

  if (*bufp == NULL)
    goto error;

  return MPD_SUCCESS;

 error:
    return _mpd_error (self, MPD_ERROR, strerror (errno));
}

static mpd_error
_mpd_verify (mpd *self)
{
  char *buf;
  char *p;

  /* TODO: if we're not connected to MPD, then we might end up waiting
     forever for a line that starts with "OK".  Not good, since
     _mpd_recv() will keep allocating memory for all received data
     until that line is received. Set a limit for time spent waiting
     and maximum data.

     Be somewhat forgiving on the data side, at least once we have a
     correct "OK MPD" response, since some commands (i.e. listall) may
     return a lot of data; for instance, expect to receive at least a
     megabyte in the case of a large collection.  My 12 gigabyte
     collection returns about 250k for listall, with 2888 entries. */

  if (_mpd_recv (self, &buf) != MPD_SUCCESS)
    return self->last_error;

  if (strncmp (buf, OK_MPD, OFFSET - 1) != 0)
    goto error_not_mpd;

  p = strchr (&buf[OFFSET], '\n');
  if (p == NULL)
    goto error_not_mpd;

  self->version = strndup (buf + OFFSET, p - (buf + OFFSET));

  if (self->version == NULL)
    {
      free (buf);
      return _mpd_error (self, MPD_ERROR, strerror (errno));
    }

  return MPD_SUCCESS;

 error_not_mpd:
  free (buf);
  return _mpd_error (self, MPD_ERROR_NOT_MPD, NULL);
  
}

#undef OK_MPD
#undef OFFSET

mpd_error
mpd_connect (mpd *self)
{
  assert (self != NULL);
  assert (self->hostname != NULL);

  if (_mpd_connect (self) != MPD_SUCCESS
      || _mpd_verify (self) != MPD_SUCCESS)
    {
      _mpd_disconnect (self);
      return self->last_error;
    }

  return MPD_SUCCESS;
}

mpd_error
mpd_disconnect (mpd *self)
{
  assert (self != NULL);

  if (self->socket == -1)
    return _mpd_error (self, MPD_ERROR_NOT_CONNECTED, NULL);

  _mpd_disconnect (self);
  
  return MPD_SUCCESS;
}

static void
_mpd_result_split (mpd_result *r)
{
  char *p, *q;

  p = r->buf;
  assert (p != NULL);

  for (r->current = 0; r->current < r->count; p = q + 1, r->current++)
    {
      char *sep;

      r->keys[r->current] = p;

      q = strchr (p, '\n');
      if (!q)
	q = memchr (p, '\0', (size_t) -1);
      else
	*q = '\0';

      sep = (char *) memchr (p, ':', q - p);
      if (sep)
	{
	  *sep = '\0';
	  r->values[r->current] = sep + 2;
	}
      else
	{
	  r->values[r->current] = NULL;
	}
    }

    r->current = 0;
}

static mpd_error
_mpd_result_init (mpd_result *r)
{
  char *p = r->buf;

  if (strncmp (p, "ACK", 3) == 0)
    return MPD_ERROR_ACK;

  if (strncmp (p, "OK", 2) == 0)
    return MPD_SUCCESS;

  for (r->count = 0; (p = strchr (p, '\n')); p++, r->count++)
    ;

  /* remove the terminating OK line */
  r->count--;
  r->current = 0;

  if (r->count == 0)
    return MPD_SUCCESS;

  r->keys = malloc (sizeof (char *) * r->count);
  r->values = malloc (sizeof (char *) * r->count);

  if (!r->keys || !r->values)
    return MPD_ERROR;

  _mpd_result_split (r);

  return MPD_SUCCESS;
}

static mpd_result *
_mpd_get_result (mpd *self)
{
  mpd_error e;
  mpd_result *r = malloc (sizeof (mpd_result));
  if (!r)
    goto libc_error;

  memset (r, 0, sizeof (mpd_result));

  if (_mpd_recv (self, &r->buf) != MPD_SUCCESS)
    goto libc_error;

  if ( (e = _mpd_result_init (r)) != MPD_SUCCESS)
    {
      if (e == MPD_ERROR_ACK)
	{
	  size_t len = strlen (r->buf) - 4;
	  char *tmp = alloca (len + 1);
	  strncpy (tmp, r->buf + 4, len);
	  tmp[len] = '\0';
	  _mpd_error (self, e, tmp);
	}
      else if (e == MPD_ERROR)
	_mpd_error (self, e, strerror (errno));
      else
	_mpd_error (self, e, NULL);

      mpd_result_free (r);
      return NULL;
    }

  /* return the special MPD_OK result for simple OK results */
  if (r->count == 0)
    {
      mpd_result_free (r);
      return MPD_OK;
    }

  return r;

 libc_error:
  if (r)
    free (r);
  _mpd_error (self, MPD_ERROR, strerror (errno));
  return NULL;
}

static mpd_error
_mpd_send (mpd *self, const char *p)
{
  size_t rem = strlen (p);

  while (rem > 0)
    {
      ssize_t count = send (self->socket, p, rem, 0);
      if (count < 0)
	return _mpd_error (self, MPD_ERROR, strerror (errno));
      p += count;
      rem -= count;
    }

  return MPD_SUCCESS;
}

mpd_result *
mpd_send_command (mpd *self, mpd_command command, ...)
{
  va_list ap;
  const char *arg;

  assert (self != NULL);
  assert (command >= 0);
  assert (command < __MPD_COMMAND_LAST);

  if (self->socket == -1)
    {
      _mpd_error (self, MPD_ERROR_NOT_CONNECTED, NULL);
      return NULL;
    }

  if (_mpd_send (self, _mpd_commands[command]) != MPD_SUCCESS)
    return NULL;

  va_start (ap, command);
  while ( (arg = va_arg (ap, const char *)) != NULL)
    {
      if (_mpd_send (self, arg) != MPD_SUCCESS)
	{
	  va_end (ap);
	  return NULL;
	}
    }
  va_end (ap);

  if (_mpd_send (self, "\n") != MPD_SUCCESS)
    return NULL;

  return _mpd_get_result (self);
}

const char *
mpd_result_get (mpd_result *r, const char *key)
{
  size_t n;

  assert (r != NULL);

  if (key == NULL)
    {
      if (r->current == 0 || r->count == 0)
	return NULL;

      return r->values[r->current - 1];
    }

  for (n = 0; n < r->count; n++)
    {
      if (strcmp (r->keys[n], key) == 0)
	return r->values[n];
    }

  return NULL;
}

const char *
mpd_result_next_key (mpd_result *r)
{
  assert (r != NULL);

  if (r->count == 0 || r->current >= r->count)
    return NULL;

  return r->keys[r->current++];
}

void
mpd_result_reset (mpd_result *r)
{
  assert (r != NULL);

  r->current = 0;
}

void
mpd_result_free (mpd_result *r)
{
  if (r == NULL || r == MPD_OK)
    return;

  if (r->keys)
    free (r->keys);
  if (r->values)
    free (r->values);
  if (r->buf)
    free (r->buf);
  free (r);
}
