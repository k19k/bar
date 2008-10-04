#include <stdlib.h>
#include <signal.h>

#include "bar.h"

static int signalled = 0;

static void
sighandler (int signum)
{
  if (signum != SIGHUP)
    signalled = 1;
}

static void
setup_signals (void)
{
  struct sigaction sa;
  sigemptyset (&sa.sa_mask);
  sa.sa_handler = sighandler;
  sa.sa_flags = 0;
  sigaction (SIGINT, &sa, NULL);
  sigaction (SIGTERM, &sa, NULL);
  sigaction (SIGQUIT, &sa, NULL);
  sigaction (SIGHUP, &sa, NULL);
}

int
main (int argc, char *argv[])
{
  bar *app;
  Display *dpy;

  setup_signals ();

  dpy = XOpenDisplay (NULL);
  if (!dpy)
    goto error;

  app = bar_create (dpy);
  if (!app)
    goto bar_error;

  bar_add_part (app, &mpd_part, BAR_LEFT);
  bar_add_part (app, &load_part, BAR_RIGHT);
  bar_add_part (app, &batt_part, BAR_RIGHT);
  bar_add_part (app, &time_part, BAR_RIGHT);

  while (bar_update (app) && !signalled)
    ;

  bar_destroy (app);

  return EXIT_SUCCESS;

 bar_error:
  XCloseDisplay (dpy);
 error:
  return EXIT_FAILURE;
}
