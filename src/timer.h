/*
 *  timer.h
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

#ifndef TIMER_H
  #define TIMER_H

  #include "utils.h"
  #define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
  #define TIMER_PERC_IGNORE_MSEC_THRESHOLD 1000 // lower bound of when to ignore milliseconds (in seconds)

typedef struct
{
  guint tv_sec;
  guint tv_usec;
  gboolean negative;
} GTimeValDiff;

typedef enum
{
  TIMER_MODE_NONE,
  TIMER_MODE_COUNTDOWN,
  TIMER_MODE_STOPWATCH,
  TIMER_MODE_TIMER
} timer_mode;

typedef enum
{
  TIMER_PRECISION_DEFAULT,
  TIMER_PRECISION_MILLISECOND,
  TIMER_PRECISION_SECOND,
  TIMER_PRECISION_MINUTE,
  TIMER_PRECISION_HOUR
} timer_precision;

typedef struct
{
  gboolean perc: 1, text: 1, bar: 1;
} timer_display;



typedef struct
{
  GTimer *gtimer;
  guint seconds;
  guint mseconds;
  GVoidFunc success_callback;
  GVoidFunc error_callback;
  guint timer_print_source_id;
  timer_mode mode;
  gboolean checkloop_thread_stop_with_error;
  timer_precision precision;
  timer_display display;
} ut_timer;

gboolean timer_print(ut_timer *t);
gboolean timer_check_loop(ut_timer *t);
gboolean timer_run_checkloop_thread(ut_timer *t);
gboolean parse_time_pattern(gchar *pattern, guint *seconds, guint *mseconds);
void timer_add_seconds(ut_timer* timer, guint seconds);
void timer_add_milliseconds(ut_timer* timer, guint milliseconds);
GTimeVal gtvaldiff_to_gtval(GTimeValDiff g);
gchar* timer_sec_msec_to_string(guint sec, guint msec, timer_precision precision /* = TIMER_PRECISION_MILLISECOND */);
gchar* timer_get_maximum_time();
gchar* timer_ut_timer_to_string(ut_timer *g);
gchar* timer_gtvaldiff_to_string(GTimeValDiff g, timer_precision precision /* = TIMER_PRECISION_MILLISECOND */);
ut_timer* timer_new_timer(guint seconds,
                          guint mseconds,
                          GVoidFunc success_callback,
                          GVoidFunc error_callback,
                          GTimer* timer,
                          timer_precision precision,
                          const timer_display* display);
ut_timer* timer_new_countdown(guint seconds,
                              guint mseconds,
                              GVoidFunc success_callback,
                              GVoidFunc error_callback,
                              GTimer* timer,
                              timer_precision precision,
                              const timer_display* display);
ut_timer* timer_new_stopwatch(GVoidFunc success_callback,
                              GVoidFunc error_callback,
                              GTimer* timer,
                              timer_precision precision,
                              const timer_display* display);
gint8 timer_get_progress_percent(const ut_timer *t);
void inline timer_set_precision (ut_timer *t, timer_precision precision);
void inline timer_set_display (ut_timer *t, timer_display display);
#endif /* TIMER_H */
