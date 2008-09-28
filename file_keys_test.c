#include <stdio.h>
#include <stdlib.h>
#include "util.h"

int
main (int argc, char *argv[])
{
  char *buf = NULL;
  int cap;

  file_get_keys ("/proc/acpi/battery/BAT1/state", ':',
		 "charging state", "%as", 1, &buf,
		 "remaining capacity", "%d", 1, &cap,
		 NULL);

  if (buf)
    {
      printf ("state is %s\n", buf);
      free (buf);
    }
  printf ("remaining: %d\n", cap);

  return 0;
}
