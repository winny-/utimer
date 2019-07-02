/*
 *  utils.c
 *
 *  Copyright 2008-2010  Arnaud Soyez <weboide@codealpha.net>
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
  #include <config.h>
#endif

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <sys/ioctl.h>

#include "utils.h"

gulong ul_add(gulong a, gulong b)
{
  if (a > G_MAXULONG - b)
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

  if (a > G_MAXULONG / b)
  {
    g_debug("Overflowing multiplication...");
    return G_MAXULONG;
  }
  else
    return a * b;
}

guint ui_add(guint a, guint b)
{
  if (a > G_MAXUINT - b)
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

  if (a > G_MAXUINT / b)
  {
    g_debug("Overflowing multiplication...");
    return G_MAXUINT;
  }
  else
    return a * b;
}

gboolean apply_suffix(guint *value, gchar *suffix)
{
  gint factor = 1;

  g_return_val_if_fail(value, FALSE);

  if (suffix)
    switch (*suffix)
    {
      case 0:
      case 's':
        factor = 1;
        break;
      case 'm':
        factor = 60;
        break;
      case 'h':
        factor = 60 * 60;
        break;
      case 'd':
        factor = 60 * 60 * 24;
        break;
      default:
        return FALSE;
    }

  g_debug("%s: applying factor %d to %u", __FUNCTION__, factor, *value);
  (*value) = ui_mul(*value, factor);

  return TRUE;
}


void init_config(Config *conf)
{
  conf->locale = NULL;
  conf->verbose = FALSE;
  conf->quiet = FALSE;
  conf->debug = FALSE;
  conf->quit_with_success = FALSE;
  conf->current_exit_status_code = EXIT_SUCCESS;
  conf->timer = g_timer_new();
}

void free_config(Config *conf)
{
  if (conf->timer)
  {
    g_debug("Freeing config timer...");
    g_timer_destroy(conf->timer);
    conf->timer = NULL;
  }
}

/**
 * Returns the number of columns of the terminal or 0 if fails.
 */
gushort get_terminal_width()
{
  int fd;
  fd = fileno(stdout);

#ifdef TIOCGSIZE
  struct ttysize wsize;
#elif defined(TIOCGWINSZ)
  struct winsize wsize;
#else
  g_debug("%s: Fetching terminal size not supported",
          __FUNCTION__);
  return 0;
#endif

#ifdef TIOCGSIZE
  if (ioctl(fd, TIOCGSIZE, &wsize))
    return 0;
  return wsize.ts_cols;
#elif defined TIOCGWINSZ
  if (ioctl(fd, TIOCGWINSZ, &wsize))
    return 0;
  return wsize.ws_col;
#endif
}


/**
 * Return  a progress bar of a given size and percentage (in a gchar).
 * This returns a pointer to gchar containing a progress bar of
 * a given size and percentage.
 */
gchar* get_progress_bar(gint8 perc, gushort width, gboolean go_right)
{
  // [=====>    ]  if go_right
  // [    <=====]  if !go_right

  gushort real_width = 0;
  if (width >= 2)
    real_width = width - 2; // we exclude the outside brackets

  gushort bar_length = (guint) real_width * (guint) perc / 100;
  gushort empty_length = real_width - bar_length;

  gchar *ret = g_new(gchar, width+1);

  if(width == 0)
  {
    ret[0] = '\0';
    return ret;
  }

  ret[0] = '[';
  ret[width-1] = ']';
  ret[width] = '\0';

  gint start = 1;
  gint i = start;
  gchar* c = &ret[i];
  while(i < start+empty_length+bar_length)
  {
    if(go_right)
    {
      if(i<start+bar_length)
        *c = '=';
      else if(i==start+bar_length)
        *c = '>';
      else if(i<start+bar_length+empty_length)
        *c = ' ';
    }
    else
    {
      if(i<start+empty_length)
        *c = ' ';
      else if(i==start+empty_length)
        *c = '<';
      else if(i<start+empty_length+bar_length)
        *c = '=';
    }

    c = &ret[++i];
  }

  //g_debug("my progress bar: %s", ret);
  return ret;
}
