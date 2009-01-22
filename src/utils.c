/*
 *  utils.c
 *
 *  Copyright 2008, 2009  Arnaud Soyez <weboide@codealpha.net>
 *
 *  This file is part of uTimer.
 *  (uTimer is a CLI program that features a timer, countdown, and a stopwatch)
 *
 *  uTimer is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  uTimer is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with uTimer.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "utils.h"

gulong ul_add(gulong a, gulong b)
{
  if(a > G_MAXULONG - b)
  {
    g_debug("Overflowing addition...");
    return G_MAXULONG;
  }
  else
    return a + b;
}

gulong ul_mul(gulong a, gulong b) // only positive multiplication
{
  g_assert(a >= 0 && b >= 0);
  
  if(a > G_MAXULONG / b)
  {
    g_debug("Overflowing multiplication...");
    return G_MAXULONG;
  }
  else
    return a * b;
}


guint ui_add(guint a, guint b)
{
  if(a > G_MAXUINT - b)
  {
    g_debug("Overflowing addition...");
    return G_MAXUINT;
  }
  else
    return a + b;
}

guint ui_mul(guint a, guint b) // only positive multiplication
{
  g_assert(a >= 0 && b >= 0);
  
  if(a > G_MAXUINT / b)
  {
    g_debug("Overflowing multiplication...");
    return G_MAXUINT;
  }
  else
    return a * b;
}
