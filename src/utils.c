/*
 *  utils.c
 *
 *  Copyright 2008 Arnaud Soyez <weboide@codealpha.net>
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

strtodouble_error strtodouble (char *str, char *endptr, gdouble *result)
{
  gint errnum = 0;
  gdouble val;
  errno = 0;
  val = g_ascii_strtod(str, &endptr);
  g_debug("strtod returned %f", val);
  
  // copy errno into errnum just to make sure it doesn't change
  errnum = errno;
  
  result = &val;
  
  g_debug("result = %f", *result);
  
  // Check for overflow
  if (errnum == ERANGE && (val == DBL_MAX || val == DBL_MIN))
  {
    g_debug("Overflow!");
    return STD_OVERFLOW;
  }
  else if(errnum != 0 && val == 0) // Underflow
  {
    g_debug("Underflow!");
    return STD_UNDERFLOW;
  }

  // if str was empty (no digit found)
  if (endptr == str)
  {
    g_warning("No digits were found\n");
    return STD_EMPTY;
  }
  
  // If we have some chars left after the number found
  if (*endptr != '\0')
  {
    g_warning("Remaining characters after number: %s\n", endptr);
  }
  
  return STD_OK;
}

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

