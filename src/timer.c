/*
 *  timer.c
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

#include "utimer.h"
#include "timer.h"

static GTimeValDiff timer_get_diff (GTimer *start)
{
  GTimeValDiff diff;
  gulong tmpul;
  
  diff.tv_sec = g_timer_elapsed (start, &tmpul);
  diff.tv_usec = (guint) tmpul;
  
  g_debug ("timer_get_diff: %u.%06u", diff.tv_sec, diff.tv_usec);
  return diff;
}

static gboolean timer_apply_suffix (guint* value, gchar* suffix)
{
  int factor;

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
  
  g_debug ("applying factor %d to %u", factor, *value);
  (*value) = ul_mul (*value, factor);

  return TRUE;
}

static GTimeValDiff countdown_get_diff (ut_timer *t)
{
  GTimeValDiff diff;
  gulong tmpul;
  
  /* diff = elapsed time */
  diff.tv_sec = g_timer_elapsed (t->start_timer, &tmpul);
  diff.tv_usec = tmpul;
  
  /* ------- We need: diff = given time - elapsed time ------- */
  
  diff.tv_sec = t->seconds - diff.tv_sec;
  
  /* If there are more than one second, and usec < tv_usec */
  if (diff.tv_sec > 0 && t->mseconds*1000 < diff.tv_usec)
  {
    diff.tv_sec--;
    diff.tv_usec = 1000000-(diff.tv_usec - t->mseconds*1000);
  }
  else if (t->mseconds*1000 >= diff.tv_usec) /* usec >= tv_usec, normal substraction */
  {
    diff.tv_usec = t->mseconds*1000 - diff.tv_usec;
  }
  else /* if 0 seconds and usec < tv_usec, we are already late! */
  {
    diff.tv_usec = 0;
  }
  
  g_debug ("countdown_get_diff: %u.%06u", diff.tv_sec, diff.tv_usec);
  return diff;
}

gboolean timer_print (ut_timer *t)
{
  GTimeValDiff delta;
  gchar* tmpchar;
  
  if (t->mode == TIMER_MODE_COUNTDOWN)
    delta = countdown_get_diff(t);
  else
    delta = timer_get_diff(t->start_timer);
  
  tmpchar = timer_gtvaldiff_to_string(delta);
  
  if (t->mode == TIMER_MODE_COUNTDOWN)
    g_message (_("\rTime Remaining: %s "), tmpchar); /* trailing space needed! */
  else
    g_message (_("\rElapsed Time: %s "), tmpchar); /* trailing space needed! */
  
  g_free (tmpchar);
  
  return TRUE;
}

gboolean timer_check_loop (ut_timer *t)
{
  GTimeValDiff elapsed;
  guint wanted_usec = t->mseconds*1000;
  guint wanted_sec  = t->seconds;
  
  g_assert (TIMER_CHECK_RATE_MSEC < 1000);
  
  while (!t->checkloop_thread_stop_with_error)
  {
    elapsed = timer_get_diff (t->start_timer);
    
    if (elapsed.tv_sec < wanted_sec || elapsed.tv_sec == wanted_sec && elapsed.tv_usec < wanted_usec)
    {
      
      guint remaining_sec = wanted_sec - elapsed.tv_sec;
      guint remaining_usec = 0;
      
      /* -If- more than 1 second remaining, and elapsed.usec > wanted_usec */
      if (remaining_sec >= 1 && elapsed.tv_usec > wanted_usec)
      {
        /* (wanted_usec - elapsed.tv_usec)  would be negative,
         * so we need to do this (this is like a decomposed substraction):
         */
        remaining_sec--;
        remaining_usec = 1000000 - (elapsed.tv_usec - wanted_usec);
      }
      else if (wanted_usec >= elapsed.tv_usec) 
      {
        /* -If- wanted_usec >= elapsed.usec, normal substraction
         * (we explicetly don't care about remaining_sec)
         */
        
        remaining_usec = wanted_usec - elapsed.tv_usec;
      }
      else /* if 0 seconds and usec < tv_usec, we are already late! */
      {
        remaining_usec = 0;
      }
      
      /* if less than a second is remaining and rate is too big */
      if (remaining_sec == 0 && TIMER_CHECK_RATE_MSEC*1000 > remaining_usec)
      {
        g_debug ("sleeping for remaining: %u us (< %u us)", remaining_usec, TIMER_CHECK_RATE_MSEC*1000);
        g_usleep(remaining_usec); /* we sleep for the remaining part */
      }
      else
      {
        g_debug ("sleeping for normal rate: %u us", TIMER_CHECK_RATE_MSEC*1000);
        g_usleep(TIMER_CHECK_RATE_MSEC*1000); /* otherwise we sleep another 'rate' */
      }
    }
    else
      break;
  }
  
  /* Time's up! request to stop updating, and returns */
  g_source_remove (t->timer_print_source_id);
  timer_print(t);
  
  if (t->checkloop_thread_stop_with_error) /* if we quitted the loop with an error */
  {
    g_debug ("%s: thread stopped with error", __FUNCTION__);
    t->error_callback();
    return FALSE;
  }
  
  g_debug ("%s: thread stopped with success", __FUNCTION__);
  t->success_callback();
  return TRUE;
}


gboolean timer_stop_checkloop_thread (ut_timer *t)
{
  if (!t)
    return FALSE;
  g_debug ("%s: request to stop checkloop thread", __FUNCTION__);
  g_debug ("%s: checkloop_thread_stop_with_error = %d", __FUNCTION__, t->checkloop_thread_stop_with_error);
  t->checkloop_thread_stop_with_error = TRUE;
  g_debug ("%s: checkloop_thread_stop_with_error = %d", __FUNCTION__, t->checkloop_thread_stop_with_error);
  return TRUE;
}

gboolean timer_run_checkloop_thread (ut_timer *t)
{
  GError  *error = NULL;
  
  g_debug ("Starting Timer thread");
  
  if (!g_thread_create((GThreadFunc) timer_check_loop, t, FALSE, &error))
  {
    g_printerr (_("Thread failed: %s"), error->message);
    g_error_free (error);
    error_quitloop ();
  }
  
  return FALSE; // return FALSE to get removed from the main loop
}

gchar* timer_get_maximum_time ()
{
  ut_timer *t = g_new (ut_timer, 1);
  gchar* ret;
  t->seconds = G_MAXUINT;
  t->mseconds = 999;
  
  ret = timer_ut_timer_to_string(t);
  g_free (t);
  return ret;
}

gboolean parse_time_pattern (gchar *pattern, ut_timer* timer)
{
  gchar *endptr, *tmp;
  guint val;
  
  if (!pattern)
    return FALSE;
  
  tmp = pattern;
  
  do
  {
    g_debug ("Parsing: %s", tmp);
    
    errno = 0;    /* To distinguish success/failure after call */
    
    val = (guint) strtoul (tmp, &endptr, 10);
    g_debug ("strtoul() returned %u", val);
    
    
    /* Check for various possible errors */
    
    if (errno == ERANGE && val == G_MAXUINT)
    {
      if (*endptr == '\0')
        g_warning (_("The last number is too big. It has been changed into: %u"), val);
      else
        g_warning (_("The number before '%s' is too big. It has been changed into: %u"), endptr, val);
    }
    
    if (endptr && g_str_has_prefix (endptr, "ms")) // if parsing the milliseconds
    {
      timer_add_milliseconds (timer, val);
      endptr = endptr + 2;
    }
    else if (endptr && timer_apply_suffix (&val, endptr)) // if parsing another unit
    {
      timer_add_seconds (timer, val);
      if (*endptr != '\0')
        endptr = endptr + 1;
    }
    else
    {
      g_error (_("Error when trying to parse: %s"), endptr);
    }
    
    tmp = endptr;
  }
  while (*endptr != '\0');

}

void timer_add_seconds (ut_timer* timer, guint seconds)
{
  g_debug ("Adding %u seconds", seconds);
  timer->seconds = ui_add(timer->seconds, seconds);
  g_debug ("timer.seconds = %u", timer->seconds);
}

void timer_add_milliseconds (ut_timer* timer, guint milliseconds)
{
  guint bonus_seconds;
  guint diff;
  
  bonus_seconds = milliseconds / 1000;
  if (bonus_seconds>0)
  {
    timer_add_seconds (timer, bonus_seconds);
    milliseconds -= bonus_seconds * 1000;
  }
  
  timer->mseconds = ui_add (timer->mseconds, milliseconds);
}

/**
 * Return human readable string for the given time.
 */
gchar* timer_sec_msec_to_string(guint sec, guint msec)
{
  g_assert (msec < 1000);
  guint all_secs = sec;
  guint days = sec / 86400;
  sec -= days * 86400;
  guint hours = sec / 3600;
  sec -= hours * 3600;
  guint minutes = sec / 60;
  sec -= minutes * 60;
  
  return g_strdup_printf(C_("NUMDAYS days HOURS:MINUTES:SECONDS:MILLISECONDS (SECONDS.MILLISECONDS seconds)",
                          "%u days %02u:%02u:%02u.%03u (%u.%03u seconds)"),
                         days,
                         hours,
                         minutes,
                         sec,
                         msec,
                         all_secs,
                         msec);
}

gchar* timer_gtvaldiff_to_string (GTimeValDiff g)
{
  return timer_sec_msec_to_string (g.tv_sec, g.tv_usec/1000);
}

gchar* timer_ut_timer_to_string (ut_timer *g)
{
  if (!g)
    return NULL;
  return timer_sec_msec_to_string (g->seconds, g->mseconds);
}

ut_timer* timer_new_timer ()
{
  ut_timer* t;
  t = g_new (ut_timer, 1);
  t->seconds = 0;
  t->mseconds = 0;
  t->mode = TIMER_MODE_TIMER;
  t->checkloop_thread_stop_with_error = FALSE;
  return t;
}

ut_timer* countdown_new_timer ()
{
  ut_timer* t;
  t = g_new (ut_timer, 1);
  t->seconds = 0;
  t->mseconds = 0;
  t->mode = TIMER_MODE_COUNTDOWN;
  t->checkloop_thread_stop_with_error = FALSE;
  return t;
}


ut_timer* stopwatch_new_timer ()
{
  ut_timer* t;
  t = g_new (ut_timer, 1);
  t->seconds = 0;
  t->mseconds = 0;
  t->mode = TIMER_MODE_STOPWATCH;
  t->checkloop_thread_stop_with_error = FALSE;
  return t;
}
