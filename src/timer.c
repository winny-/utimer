/*
 *  timer.c
 *
 *  Copyright 2008 Arnaud Soyez <weboide@codealpha.net>
 *
 *  This file is part of utimer.
 *  (utimer is a CLI program that features a timer, countdown, and a stopwatch)
 *
 *  utimer is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  utimer is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with utimer.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

//~ #include <stdio.h>
//~ #include <unistd.h>
//~ #include <stdlib.h>
//~ #include <termios.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "timer.h"

int update_timer(ut_timer *t)
{
  GTimeVal current_time;
  GTimeValDiff delta;

  g_get_current_time(&current_time);
  
  delta = get_diff(*(t->start_time), current_time);
  /** TODO: BETTER FORMATTING **/
  g_print("\rElasped Time: %ld.%06ld", delta.tv_sec, delta.tv_usec);
  
  return TRUE;
}

int sleep_timer(ut_timer *t)
{
  g_debug("sleeping for %ld.%06d seconds", t->seconds, t->mseconds*1000);
  g_print("\n");
  update_timer(t);
  if(t->seconds>0)
    sleep(t->seconds);
  if(t->mseconds>0)
    g_usleep(t->mseconds*1000);
  update_timer(t);
  t->callback();
  return TRUE;
}

GTimeValDiff get_diff(GTimeVal start, GTimeVal end)
{
  GTimeValDiff diff;
  GTimeVal *ptr_start, *ptr_end;
  
  diff.negative = !(end.tv_sec > start.tv_sec
                    || end.tv_sec == start.tv_sec
                       && end.tv_usec >= start.tv_usec);
  if(diff.negative)
  {
    ptr_start = &end;
    ptr_end = &start;
  }
  else
  {
    ptr_start = &start;
    ptr_end = &end;
  }
  
  diff.tv_sec = ptr_end->tv_sec - ptr_start->tv_sec;
  diff.tv_usec = ptr_end->tv_usec - ptr_start->tv_usec;
  
  if(diff.tv_usec < 0)
  {
    diff.tv_usec += G_USEC_PER_SEC;
    diff.tv_sec--;
  }
  
  return diff;
}

int start_thread_timer(ut_timer *t)
{
  GError  *error = NULL;
  
  g_debug("Starting Timer thread");
  if (!g_thread_create((GThreadFunc) sleep_timer, t, FALSE, &error))
  {
    g_error (_("Thread failed: %s"), error->message);
  }
  
  return FALSE; // to get removed from the main loop
}
