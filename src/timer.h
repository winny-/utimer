/*
 *  timer.h
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

#ifndef TIMER_H
#define TIMER_H

#define TIMER_REFRESH_RATE  100

typedef struct
{
  GTimeVal *start_time;
  glong seconds;
  gint mseconds;
  void (*callback)();
} ut_timer;

typedef struct {
  glong tv_sec;
  glong tv_usec;
  gboolean negative;
} GTimeValDiff;


int           update_timer              (ut_timer *t);
int           sleep_timer               (ut_timer *t);
GTimeValDiff  get_diff                  (GTimeVal start, GTimeVal end);
int           start_thread_timer        (ut_timer *t);



#endif /* TIMER_H */
