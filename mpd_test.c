#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mpd.h"

int
main(int argc, char *argv[])
{
  mpd *mpdc = mpd_create ();
  mpd_result *r;

  if (!mpdc)
    goto absolute_failure;

  if (mpd_connect (mpdc) != MPD_SUCCESS)
    goto error;

  printf ("connected to MPD version %s\n", mpd_get_version (mpdc));

  r = mpd_send_command (mpdc, MPD_COMMAND_CURRENTSONG, NULL);
  if (!r)
    goto error;

  if (r == MPD_OK)
    {
      printf ("no song playing\n");
    }
  else
    {
      const char *s = mpd_result_next_key (r);

      printf("All keys and values:\n");

      while (s != NULL)
	{
	  printf ("  %10s: ", s);
	  s = mpd_result_get (r, NULL);
	  printf ("%s\n", s);
	  s = mpd_result_next_key (r);
	}
    }
  
  mpd_result_free (r);
  mpd_disconnect (mpdc);
  mpd_destroy (mpdc);

  return EXIT_SUCCESS;

 error:
  fprintf (stderr, "%s: %s\n", argv[0], mpd_get_last_error (mpdc, NULL));
  mpd_destroy (mpdc);
  return EXIT_FAILURE;

 absolute_failure:
  perror (argv[0]);
  return EXIT_FAILURE;
}
