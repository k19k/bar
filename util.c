#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

struct _vkinf {
  const char *key;
  const char *fmt;
  size_t nargs;
  va_list argp;

  struct _vkinf *next;
};

static void
_vget_kinf (va_list ap, struct _vkinf *head)
{
  struct _vkinf *ki = head;
  while (ki != NULL)
    {
      size_t n;
      ki->next = NULL;
      ki->key = va_arg (ap, const char *);
      if (ki->key != NULL)
	{
	  ki->fmt = va_arg (ap, const char *);
	  ki->nargs = va_arg (ap, size_t);
	  va_copy (ki->argp, ap);
	  for (n = 0; n < ki->nargs; n++)
	    va_arg (ap, void *);
	  ki->next = malloc (sizeof (struct _vkinf));
	}
      ki = ki->next;
    }
}

static void
_free_kinf (struct _vkinf *head)
{
  struct _vkinf *ki = head->next;
  while (ki)
    {
      struct _vkinf *next = ki->next;
      if (ki->key)
	va_end (ki->argp);
      free (ki);
      ki = next;
    }
}

static void
_kinf_parse_line (struct _vkinf *ki, char *line, char sep)
{
  char *key = line;
  char *value = strchr (line, sep);
  char *p = value;
  if (!value)
    return;

  /* removing leading whitespace from the value */
  *(value++) = '\0';
  while (isspace (*value))
    value++;

  /* remove trailing whitespace from the key */
  while (p > key && isspace (*(--p)))
    *p = '\0';

  /* remove leading whitespace from the key */
  while (isspace (*key))
    key++;

  for ( ; ki && ki->key; ki = ki->next)
    {
      va_list ap;

      if (strcmp (ki->key, key) != 0)
	continue;

      va_copy (ap, ki->argp);
      vsscanf (value, ki->fmt, ap);
      va_end (ap);
    }
}

static void
_file_get_keys (FILE *f, struct _vkinf *ki, char sep)
{
  char *line = NULL;
  size_t n = 0;

  while (!feof (f))
    {
      ssize_t len;
      if ( (len = getline (&line, &n, f)) < 0)
	break;
      while (len > 0 && isspace (line[--len]))
	line[len] = '\0';
      _kinf_parse_line (ki, line, sep);
    }
  if (line != NULL)
    free (line);
}

void
file_get_keys (const char *filename, char sep, ...)
//	       const char *key, const char *scanfmt, size_t nargs, ...)
{
  va_list ap;
  FILE *f;
  struct _vkinf ki;

  assert (filename != NULL);

  va_start (ap, sep);
  _vget_kinf (ap, &ki);
  va_end (ap);

  f = fopen (filename, "r");
  if (f)
    {
      _file_get_keys (f, &ki, sep);
      fclose (f);
    }

  _free_kinf (&ki);
}
