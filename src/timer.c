/*
 *  timer.c
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

#include "timer.h"

gboolean timer_update_safe (ut_timer *t)
{
  GTimeVal current_time;
  GTimeValDiff delta;
  static gboolean check_overflow = TRUE;
  
  timer_update(t);
  
  return TRUE;
}

gboolean timer_update (ut_timer *t)
{
  GTimeVal current_time;
  GTimeValDiff delta;
  static gboolean check_overflow = TRUE;
  gchar* tmpchar;
  
  g_mutex_lock (update_timer_mutex);
  
  g_get_current_time(&current_time);
  delta = timer_get_diff(*(t->start_time), current_time, &check_overflow);
  tmpchar = timer_gtvaldiff_to_string(delta);
  g_print(_("\rElapsed Time: %s"), tmpchar);
  g_free(tmpchar);
  t->last_diff = &delta;
  g_mutex_unlock (update_timer_mutex);
  
  return TRUE;
}

gboolean timer_sleep (ut_timer *t)
{
  g_debug("sleeping for %lu.%03lu seconds", t->seconds, t->mseconds);
  g_print("\n");
  timer_update(t);
  
  if(t->seconds>0)
    sleep(t->seconds);
  if(t->mseconds>0)
    g_usleep(t->mseconds*1000);
  
  /* The Timer is done, so it should stop running. */
  g_source_remove(t->update_timer_safe_source_id);
  timer_update(t);
  t->success_callback();
  return TRUE;
}

static GTimeValDiff timer_get_diff (GTimeVal start, GTimeVal end, gboolean *must_be_positive)
{
  GTimeValDiff diff;
  GTimeVal *ptr_start, *ptr_end;
  
  diff.negative = !(end.tv_sec > start.tv_sec
                    || end.tv_sec == start.tv_sec
                       && end.tv_usec >= start.tv_usec);
  
  if(*must_be_positive && diff.negative)
  {
    g_warning(_("\n***************** IMPORTANT *****************"));
    g_warning(_("Possible overflow! (is it Year 2038 bug?). The start time is after the current time!"));
    g_warning(_("IMPORTANT: The timer will continue normally but the time displayed (elapsed time) may be invalid.\n"));
    *must_be_positive = FALSE;
  }
  
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

int timer_start_thread (ut_timer *t)
{
  GError  *error = NULL;
  
  g_debug("Starting Timer thread");
  if (!g_thread_create((GThreadFunc) timer_sleep, t, FALSE, &error))
  {
    g_error (_("Thread failed: %s"), error->message);
  }
  
  return FALSE; // to get removed from the main loop
}

gulong timer_get_maximum_value (TimeUnit unit)
{
  if(unit == TU_MILLISECOND)
    return G_MAXULONG/1000;
  
  if(unit == TU_SECOND)
    return G_MAXULONG;
  
  if(unit == TU_MINUTE)
    return G_MAXUSHORT;
  
  if(unit == TU_HOUR)
    return G_MAXUSHORT;
  
  if(unit == TU_DAY)
    return G_MAXUSHORT;
  
  if(unit == TU_YEAR)
    return G_MAXUSHORT;
  
  g_critical(_("%s doesn't support given unit (%d)."), __FUNCTION__, unit);
}

gchar* timer_get_maximum_pattern ()
{
  return g_strdup_printf("%lud%luh%lum%lus%lums",
                         timer_get_maximum_value(TU_DAY),
                         timer_get_maximum_value(TU_HOUR),
                         timer_get_maximum_value(TU_MINUTE),
                         timer_get_maximum_value(TU_SECOND),
                         timer_get_maximum_value(TU_MILLISECOND));
}

gchar* timer_get_maximum_time()
{
  ut_timer t;
  t.seconds = G_MAXULONG;
  t.mseconds = 999;
  
  return timer_ut_timer_to_string(t);
}

gboolean timer_parse_pattern (gchar *pattern, ut_timer* timer)
{
  int base = 10;
  gchar *endptr, *tmp;
  gulong val;
  
  if(!pattern)
    return FALSE;
  
  tmp = pattern;
  
  do
  {
    g_debug("Parsing: %s", tmp);
    
    errno = 0;    /* To distinguish success/failure after call */
    
    val = strtoul(tmp, &endptr, base);
    g_debug("strtoul() returned %lu", val);
    
    
    /* Check for various possible errors */
    
    if (errno == ERANGE && val == G_MAXULONG)
    {
      if(*endptr == '\0')
        g_warning(_("The last number is too big. It has been changed into: %lu"), val);
      else
        g_warning(_("The number before '%s' is too big. It has been changed into: %lu"), endptr, val);
    }
    
    if(endptr && g_str_has_prefix(endptr, "ms")) // if parsing the milliseconds
    {
      timer_add_milliseconds(timer, val);
      endptr = endptr + 2;
    }
    else if(endptr && timer_apply_suffix(&val, endptr)) // if parsing another unit
    {
      timer_add_seconds(timer, val);
      if(*endptr != '\0')
        endptr = endptr + 1;
    }
    else
    {
      g_error(_("Error when trying to parse: %s"), endptr);
    }
    
    tmp = endptr;
  }
  while(*endptr != '\0');

}

void timer_add_seconds(ut_timer* timer, gulong seconds)
{
  g_debug("Adding %lu seconds", seconds);
  timer->seconds = ul_add(timer->seconds, seconds);
  g_debug("timer.seconds = %lu", timer->seconds);
}

void timer_add_milliseconds(ut_timer* timer, gulong milliseconds)
{
  gulong bonus_seconds;
  gulong diff;
  
  bonus_seconds = milliseconds / 1000;
  if(bonus_seconds>0)
  {
    timer_add_seconds(timer, bonus_seconds);
    milliseconds -= bonus_seconds * 1000;
  }
  
  timer->mseconds = ul_add(timer->mseconds, milliseconds);
}


static gboolean timer_apply_suffix (gulong* value, gchar* suffix)
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
  
  g_debug("applying factor %d to %lu", factor, *value);
  (*value) = ul_mul(*value, factor);

  return TRUE;
}

gchar* timer_sec_msec_to_string(gulong sec, gulong msec)
{
  g_assert(msec < 1000);
  gulong all_secs = sec;
  gint days = sec / 86400;
  sec -= days * 86400;
  gint hours = sec / 3600;
  sec -= hours * 3600;
  gint minutes = sec / 60;
  sec -= minutes * 60;
  
  return g_strdup_printf(C_("NUMDAYS days HOURS:MINUTES:SECONDS:MILLISECONDS (SECONDS.MILLISECONDS seconds)",
                          "%i days %02i:%02i:%02lu.%03lu (%lu.%03lu seconds)"),
                         days,
                         hours,
                         minutes,
                         sec,
                         msec,
                         all_secs,
                         msec);
}

gchar* timer_gtvaldiff_to_string(GTimeValDiff g)
{
  return timer_sec_msec_to_string(g.tv_sec, g.tv_usec/1000);
}

gchar* timer_ut_timer_to_string(ut_timer g)
{
  return timer_sec_msec_to_string(g.seconds, g.mseconds);
}

void timer_init(ut_timer* t)
{
  t->seconds = 0;
  t->mseconds = 0;
}
