#ifndef BAR_H
#define BAR_H

#include "parts.h"

typedef struct bar bar;

#define BAR_INSERT 0
#define BAR_APPEND 1
#define BAR_LEFT   2
#define BAR_RIGHT  0

bar *
bar_create (void);

void
bar_destroy (bar *);

int
bar_add_part (bar *, struct part *, int flags)
  __attribute__ ((__nonnull__ (1, 2)));

void
bar_remove_part (bar *, int id)
  __attribute__ ((__nonnull__ (1)));

int
bar_update (bar *)
  __attribute__ ((__nonnull__ (1)));

#endif	/* BAR_H */
