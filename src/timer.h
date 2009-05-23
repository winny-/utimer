/*
 *  timer.h
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

#ifndef TIMER_H
#define TIMER_H

#include "utils.h"

typedef struct {
  guint      tv_sec;
  guint      tv_usec;
  gboolean   negative;
} GTimeValDiff;

typedef struct
{
  GTimer        *start_timer;
  guint         seconds;
  guint         mseconds;
  void          (*success_callback)();
  void          (*error_callback)();
  guint         timer_print_source_id;
  gboolean      isCountdown;
} ut_timer;

gboolean              timer_print               (ut_timer *t);
gboolean              timer_check               (ut_timer *t);
int                   timer_run_checkloop_thread(ut_timer *t);
gboolean              parse_time_pattern        (gchar *pattern, ut_timer* timer);
void                  timer_add_seconds         (ut_timer* timer, guint seconds);
void                  timer_add_milliseconds    (ut_timer* timer, guint milliseconds);
GTimeVal              gtvaldiff_to_gtval        (GTimeValDiff g);
gchar*                timer_sec_msec_to_string  (guint sec, guint msec);
gchar*                timer_get_maximum_time    ();
gchar*                timer_ut_timer_to_string  (ut_timer g);
gchar*                timer_gtvaldiff_to_string (GTimeValDiff g);
void                  countdown_init            (ut_timer* t);
#endif /* TIMER_H */
