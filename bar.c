#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <cairo-xlib.h>

#include "bar.h"

#define _MOTIF_WM_HINTS		"_MOTIF_WM_HINTS"
#define _NET_WM_STATE		"_NET_WM_STATE"
#define _NET_WM_STATE_STICKY	"_NET_WM_STATE_STICKY"

#define PROP_MWM_HINTS_ELEMENTS 3
#define MWM_HINTS_DECORATIONS	(1L << 1)
#define MWM_DECOR_ALL		(1L << 0)
#define MWM_DECOR_BORDER	(1L << 1)

typedef struct {
  unsigned long flags;
  unsigned long functions;
  unsigned long decorations;
} PropMwmHints;

struct element {
  struct part *type;
  void *data;
  struct element *next;
};

struct bar {
  Display *dpy;
  Window window;
  XSetWindowAttributes attr;

  cairo_surface_t *target, *hidden;
  cairo_pattern_t *pattern;
  cairo_t *cr, *cr_target;

  int width, height;
  double dwidth;

  time_t last_update;

  struct element *left;
  struct element *right;  
};

bar *
bar_create (Display *dpy)
{
  int screen;
  bar *self;
  XClassHint cls_hint;
  XSizeHints size_hints;
  PropMwmHints mwm_hints = {
    .flags = MWM_HINTS_DECORATIONS,
    .decorations = 0,
  };
  Atom motif_wm_hints = XInternAtom (dpy, _MOTIF_WM_HINTS, False);
  Atom net_wm_state = XInternAtom (dpy, _NET_WM_STATE, False);
  Atom net_wm_state_sticky = XInternAtom (dpy, _NET_WM_STATE_STICKY, False);
  Atom net_hints[1] = { net_wm_state_sticky };

  self = malloc (sizeof (bar));
  if (!self)
    return NULL;

  memset (self, 0, sizeof (bar));

  self->dpy = dpy;

  screen = DefaultScreen (dpy);
  self->attr = (XSetWindowAttributes)
    { .event_mask = ExposureMask };

  self->height = 16;
  self->width = DisplayWidth (dpy, screen);
  self->dwidth = (double) self->width;

  self->window = XCreateWindow (dpy, RootWindow (dpy, screen),
				1, DisplayHeight (dpy, screen) - self->height,
				self->width, self->height, 0,
				CopyFromParent, InputOutput,
				DefaultVisual (dpy, screen),
				CWEventMask, &self->attr);

  if (!self->window)
    goto window_error;

  cls_hint = (XClassHint)
    {
      .res_name = "bar",
      .res_class = "Bar"
    };

  size_hints = (XSizeHints)
    {
      .flags = PMinSize | PMaxSize | PPosition | PWinGravity,
      .x = 0, .y = 0,
      .min_width = self->width,
      .min_height = self->height,
      .max_width = self->width,
      .max_height = self->height,
      .win_gravity = SouthWestGravity
    };

  XSetWMProperties (dpy, self->window, NULL, NULL, NULL, 0,
		    &size_hints, NULL, &cls_hint);
  
  XChangeProperty (dpy, self->window, motif_wm_hints,
		   motif_wm_hints, 32, PropModeReplace,
		   (unsigned char *) &mwm_hints,
		   PROP_MWM_HINTS_ELEMENTS);

  XChangeProperty (dpy, self->window, net_wm_state,
		   XA_ATOM, 32, PropModeReplace,
		   (unsigned char *) &net_hints, 1);

  XMapWindow (dpy, self->window);

  self->target = cairo_xlib_surface_create (dpy, self->window,
					    DefaultVisual (dpy, screen),
					    self->width, self->height);
  if (!self->target)
    goto target_error;

  self->hidden = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
					     self->width, self->height);
  if (!self->hidden)
    goto hidden_error;

  self->cr_target = cairo_create (self->target);
  self->cr = cairo_create (self->hidden);

  cairo_set_source_surface (self->cr_target, self->hidden, 0.0, 0.0);
  cairo_select_font_face (self->cr, "DejaVu Sans",
			  CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (self->cr, 12.0);

  self->pattern = cairo_pattern_create_linear (0.0, 0.0, 0.0, self->height);
  cairo_pattern_add_color_stop_rgb (self->pattern, 0.0,
				    1.0, 1.0, 1.0);
  cairo_pattern_add_color_stop_rgb (self->pattern, 0.2,
				    0.9, 0.86, 0.84);
  cairo_pattern_add_color_stop_rgb (self->pattern, 1.0,
				    0.8, 0.78, 0.74);
  
  return self;

 hidden_error:
  cairo_surface_destroy (self->target);
 target_error:
  XDestroyWindow (dpy, self->window);
 window_error:
  free (self);
  return NULL;
}

static void
_element_destroy (struct element *e)
{
  e->type->destroy (e->data);
  free (e);
}

static int
_element_search_and_destroy (struct element **lst, struct element *e)
{
  struct element *iter;

  assert (e != NULL);
  assert (lst != NULL);

  iter = *lst;

  if (e == iter)
    {
      *lst = e->next;
      _element_destroy (e);
      return 1;
    }
  iter = iter->next;
  while (iter)
    {
      if (e == iter)
	{
	  iter->next = e->next;
	  _element_destroy (e);
	  return 1;
	}
    }
  return 0;
}

static void
_element_destroy_all (struct element *e)
{
  while (e)
    {
      struct element *tmp = e->next;
      _element_destroy (e);
      e = tmp;
    }
}

void
bar_destroy (bar *self)
{
  if (!self)
    return;

  _element_destroy_all (self->left);
  _element_destroy_all (self->right);

  if (self->cr) cairo_destroy (self->cr);
  if (self->cr_target) cairo_destroy (self->cr_target);
  if (self->pattern) cairo_pattern_destroy (self->pattern);
  if (self->hidden) cairo_surface_destroy (self->hidden);
  if (self->target) cairo_surface_destroy (self->target);
  if (self->dpy)
    {
      if (self->window != None)
	XDestroyWindow (self->dpy, self->window);
      XCloseDisplay (self->dpy);
    }

  free (self);
}

int
bar_add_part (bar *self, struct part *type, int flags)
{
  struct element *e;

  assert (self != NULL);
  assert (type != NULL);

  e = malloc (sizeof (struct element));
  if (!e)
    return 0;

  e->type = type;
  e->data = type->create ();
  if (!e->data)
    fprintf (stderr, "WARNING: creating part of type %s failed\n", type->name);
  e->next = NULL;

  if (flags & BAR_APPEND)
    {
      struct element *lst = (flags & BAR_LEFT) ? self->left : self->right;
      if (lst)
	{
	  while (lst->next)
	    lst = lst->next;
	  lst->next = e;
	}
      else if (flags & BAR_LEFT)
	self->left = e;
      else
	self->right = e;
    }
  else if (flags & BAR_LEFT)
    {
      e->next = self->left;
      self->left = e;
    }
  else
    {
      e->next= self->right;
      self->right = e;
    }

  return (int) e;
}

void
bar_remove_part (bar *self, int id)
{
  struct element *e = (struct element *) id;

  assert (self != NULL);

  if (id == 0)
    return;

  if (!_element_search_and_destroy (&self->left, e))
    _element_search_and_destroy (&self->right, e);
}

static void
_element_paint_all (struct element *e, cairo_t *cr,
		    double x, int direction, double padding)
{
  double dir = (double) direction;
  while (e)
    {
      double w = e->type->get_width (e->data, cr);
      double newx = x + (w * dir);
      cairo_move_to (cr, (dir < 0) ? newx : x, 12.0);
      e->type->paint (e->data, cr);
      x = newx + padding * dir;
      e = e->next;
    }
}

static void
_bar_paint (bar *self)
{
  double padding = 5.0;

  assert (self != NULL);

  cairo_set_source (self->cr, self->pattern);
  cairo_paint (self->cr);

  cairo_set_source_rgb (self->cr, 0.0, 0.0, 0.0);
  _element_paint_all (self->left, self->cr,
		      padding, 1, padding);
  _element_paint_all (self->right, self->cr,
		      self->dwidth - padding, -1, padding);

  cairo_paint (self->cr_target);
}

static void
_element_update_all (struct element *e)
{
  while (e)
    {
      e->type->update (e->data);
      e = e->next;
    }
}

int
bar_update (bar *self)
{
  int fd;
  struct timeval tv;
  fd_set rfds;
  XEvent ev;

  assert (self != NULL);

  fd = ConnectionNumber (self->dpy);

  gettimeofday (&tv, NULL);

  if (tv.tv_sec > self->last_update)
    {
      self->last_update = tv.tv_sec;

      _element_update_all (self->left);
      _element_update_all (self->right);

      _bar_paint (self);

      XFlush (self->dpy);
    }

  tv.tv_sec = 1;
  tv.tv_usec = 0;
  FD_ZERO (&rfds);
  FD_SET (fd, &rfds);

  if (select (fd + 1, &rfds, NULL, NULL, &tv) != 1)
    return 1;

  while (XCheckWindowEvent (self->dpy, self->window,
			    self->attr.event_mask, &ev))
    {
      switch (ev.type)
	{
	case Expose:
	  _bar_paint (self);
	  break;
	case DestroyNotify:
	  return 0;
	}
    }

  return 1;
}

