/*
 *  timer.h
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

#ifndef TIMER_H
#define TIMER_H

#define TIMER_REFRESH_RATE  100

#include "utils.h"

GMutex *update_timer_mutex;


typedef struct {
  gulong     tv_sec;
  gulong     tv_usec;
  gboolean   negative;
} GTimeValDiff;

typedef struct
{
  GTimer        *start_timer;
  gulong        seconds;
  gulong        mseconds;
  void          (*success_callback)();
  void          (*error_callback)();
  guint         update_timer_safe_source_id;
} ut_timer;

gboolean      timer_update              (ut_timer *t);
gboolean      timer_sleep               (ut_timer *t);
int           timer_start_thread        (ut_timer *t);
gboolean      parse_time_pattern       (gchar *pattern, ut_timer* timer);
void          timer_add_seconds         (ut_timer* timer, gulong seconds);
void          timer_add_milliseconds    (ut_timer* timer, gulong milliseconds);
GTimeVal      gtvaldiff_to_gtval        (GTimeValDiff g);
gchar*        timer_sec_msec_to_string  (gulong sec, gulong msec);
gchar*        timer_get_maximum_time    ();
gchar*        timer_ut_timer_to_string  (ut_timer g);
gchar*        timer_gtvaldiff_to_string (GTimeValDiff g);

static 
gboolean      timer_apply_suffix        (gulong* value, gchar* suffix);

static
GTimeValDiff  timer_get_diff            (GTimer *start);
#endif /* TIMER_H */
