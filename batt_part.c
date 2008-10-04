#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <libhal.h>

#include "parts.h"

#define BAT1_UDI \
  "/org/freedesktop/Hal/devices/computer_power_supply_battery_BAT1"

#define BAT_PROP_CHRG "battery.rechargeable.is_charging"
#define BAT_PROP_DISCHRG "battery.rechargeable.is_discharging"
#define BAT_PROP_PERCENT "battery.charge_level.percentage"

struct info {
  char *buf;
  DBusConnection *dbus;
  LibHalContext *hal;
};

static struct info *
create (void)
{
  DBusError dbe;
  struct info *inf = malloc (sizeof (struct info));
  if (!inf)
    return NULL;

  memset (inf, 0, sizeof (struct info));

  inf->hal = libhal_ctx_new ();
  if (!inf->hal)
    goto error;

  dbus_error_init (&dbe);
  inf->dbus = dbus_bus_get (DBUS_BUS_SYSTEM, &dbe);
  if (dbus_error_is_set (&dbe))
    goto dbus_error;

  if (!libhal_ctx_set_dbus_connection (inf->hal, inf->dbus))
    goto misc_error;

  if (!libhal_ctx_init (inf->hal, &dbe))
    goto dbus_error;

  if (libhal_device_exists (inf->hal, BAT1_UDI, &dbe))
    {
      /* just making sure... */
    }
  else if (dbus_error_is_set (&dbe))
    goto dbus_error;
  else
    {
      fprintf (stderr, "battery not found");
      goto misc_error;
    }

  return inf;

 dbus_error:
  fprintf (stderr, "dbus: %s\n", dbe.message);
  dbus_error_free (&dbe);
 misc_error:
  if (inf->dbus)
    dbus_connection_unref (inf->dbus);
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
      libhal_ctx_free (inf->hal);
      dbus_connection_unref (inf->dbus);
      free (inf);
    }
}

static void
update (struct info *inf)
{
  int pct;
  int charging;

  pct = libhal_device_get_property_int (inf->hal, BAT1_UDI,
					BAT_PROP_PERCENT, NULL);
  charging = libhal_device_get_property_bool (inf->hal, BAT1_UDI,
					      BAT_PROP_CHRG, NULL);
  if (!charging)
    {
      charging = -libhal_device_get_property_bool (inf->hal, BAT1_UDI,
						   BAT_PROP_DISCHRG, NULL);
    }

  if (inf->buf)
    {
      free (inf->buf);
      inf->buf = NULL;
    }

  switch (charging)
    {
    case -1: asprintf (&inf->buf, "< %d%% >", pct); break;
    case  0: asprintf (&inf->buf, "| %d%% |", pct); break;
    case  1: asprintf (&inf->buf, "[ %d%% ]", pct); break;
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
