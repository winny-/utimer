/*
 *  timer.c
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

#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "utimer.h"
#include "timer.h"

static GTimeValDiff timer_get_diff(ut_timer *timer)
{
  GTimeValDiff diff;
  gulong tmpul;

  g_assert(timer && timer->gtimer);

  diff.tv_sec = g_timer_elapsed(timer->gtimer, &tmpul);
  diff.tv_usec = (guint) tmpul;

  g_debug("timer_get_diff: %u.%06u", diff.tv_sec, diff.tv_usec);
  return diff;
}

static GTimeValDiff countdown_get_diff(ut_timer *t)
{
  GTimeValDiff diff;
  gulong tmpul;

  /* diff = elapsed time */
  diff.tv_sec = g_timer_elapsed(t->gtimer, &tmpul);
  diff.tv_usec = tmpul;

  /* ------- We need: diff = given time - elapsed time ------- */

  diff.tv_sec = t->seconds - diff.tv_sec;

  /* If there are more than one second, and usec < tv_usec */
  if (diff.tv_sec > 0 && t->mseconds * 1000 < diff.tv_usec)
  {
    diff.tv_sec--;
    diff.tv_usec = 1000000 - (diff.tv_usec - t->mseconds * 1000);
  }
  else if (t->mseconds * 1000 >= diff.tv_usec) /* usec >= tv_usec, normal substraction */
  {
    diff.tv_usec = t->mseconds * 1000 - diff.tv_usec;
  }
  else /* if 0 seconds and usec < tv_usec, we are already late! */
  {
    diff.tv_usec = 0;
  }

  g_debug("countdown_get_diff: %u.%06u", diff.tv_sec, diff.tv_usec);
  return diff;
}

gboolean timer_print(ut_timer *t)
{
  GTimeValDiff delta;
  gchar* tmpchar;

  if (t->mode == TIMER_MODE_COUNTDOWN)
    delta = countdown_get_diff(t);
  else
    delta = timer_get_diff(t);

  tmpchar = timer_gtvaldiff_to_string(delta, t->precision);

  if (t->mode == TIMER_MODE_COUNTDOWN)
    g_message(_("\rTime Remaining: %s "), tmpchar); /* trailing space needed! */
  else
    g_message(_("\rElapsed Time: %s "), tmpchar); /* trailing space needed! */

  g_free(tmpchar);

  return TRUE;
}

/** Sleeps for the given ut_timer time.
 * This function sleeps for the whole time of the given ut_timer or until
 * t->checkloop_thread_stop_with_error is set to false.
 * @param t a pointer to a ut_timer
 */
gboolean timer_check_loop(ut_timer *t)
{
  GTimeValDiff elapsed;
  guint wanted_usec = t->mseconds * 1000;
  guint wanted_sec = t->seconds;

  g_assert(TIMER_CHECK_RATE_MSEC < 1000);

  while (!t->checkloop_thread_stop_with_error)
  {
    elapsed = timer_get_diff(t);

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
      if (remaining_sec == 0 && TIMER_CHECK_RATE_MSEC * 1000 > remaining_usec)
      {
        g_debug("sleeping for remaining: %u us (< %u us)", remaining_usec, TIMER_CHECK_RATE_MSEC * 1000);
        g_usleep(remaining_usec); /* we sleep for the remaining part */
      }
      else
      {
        g_debug("sleeping for normal rate: %u us", TIMER_CHECK_RATE_MSEC * 1000);
        g_usleep(TIMER_CHECK_RATE_MSEC * 1000); /* otherwise we sleep another 'rate' */
      }
    }
    else
      break;
  }

  /* Time's up! request to stop updating, and returns */
  if (t->timer_print_source_id)
    g_source_remove(t->timer_print_source_id);

  if (t->checkloop_thread_stop_with_error) /* if we quitted the loop with an error */
  {
    g_debug("%s: thread stopped with error", __FUNCTION__);
    if (t->error_callback)
      t->error_callback();
    return FALSE;
  }

  g_debug("%s: thread stopped with success", __FUNCTION__);
  if (t->success_callback)
    t->success_callback();
  return TRUE;
}

gboolean timer_stop_checkloop_thread(ut_timer *t)
{
  if (!t)
    return FALSE;
  g_debug("%s: request to stop checkloop thread", __FUNCTION__);
  g_debug("%s: checkloop_thread_stop_with_error = %d", __FUNCTION__, t->checkloop_thread_stop_with_error);
  t->checkloop_thread_stop_with_error = TRUE;
  g_debug("%s: checkloop_thread_stop_with_error = %d", __FUNCTION__, t->checkloop_thread_stop_with_error);
  return TRUE;
}

gboolean timer_run_checkloop_thread(ut_timer *t)
{
  GError *error = NULL;

  g_debug("Starting Timer thread");

  if (!g_thread_create((GThreadFunc) timer_check_loop, t, FALSE, &error))
  {
    g_printerr(_("Thread creation failed: %s"), error->message);
    g_error_free(error);
    error_quitloop();
  }

  return FALSE; // return FALSE to get removed from the main loop
}

gchar* timer_get_maximum_time()
{
  ut_timer *t = g_new(ut_timer, 1);
  gchar* ret;
  t->seconds = G_MAXUINT;
  t->mseconds = 999;
  t->precision = TIMER_PRECISION_MILLISECOND;

  ret = timer_ut_timer_to_string(t);
  g_free(t);
  return ret;
}

gboolean parse_time_pattern(gchar *pattern, guint *seconds, guint *mseconds)
{
  gchar *endptr, *tmp;
  guint val;

  if (!pattern || !seconds || !mseconds)
    return FALSE;

  *seconds = 0;
  *mseconds = 0;

  tmp = pattern;

  do
  {
    g_debug("Parsing: %s", tmp);

    errno = 0; /* To distinguish success/failure after call */

    val = (guint) strtoul(tmp, &endptr, 10);
    g_debug("strtoul() returned %u", val);

    /* Check for various possible errors */

    if (errno == ERANGE && val == G_MAXUINT)
    {
      if (*endptr == '\0')
        g_warning(_("The last number is too big. It has been changed into: %u"), val);
      else
        g_warning(_("The number before '%s' is too big. It has been changed into: %u"), endptr, val);
    }

    // if parsing the milliseconds
    if (endptr && g_str_has_prefix(endptr, "ms"))
    {
      *mseconds = ui_add(*mseconds, val);
      endptr = endptr + 2; // we go after the 'ms' part
    }
    else if (endptr && apply_suffix(&val, endptr)) // if parsing another unit
    {
      *seconds = ui_add(*seconds, val);
      if (*endptr != '\0')
        endptr = endptr + 1;
    }
    else
    {
      g_error(_("Error when trying to parse: %s"), endptr);
      return FALSE;
    }

    tmp = endptr;
  } while (*endptr != '\0');

  return TRUE;
}

void timer_add_seconds(ut_timer* timer, guint seconds)
{
  g_debug("Adding %u seconds", seconds);
  timer->seconds = ui_add(timer->seconds, seconds);
  g_debug("timer.seconds = %u", timer->seconds);
}

void timer_add_milliseconds(ut_timer* timer, guint milliseconds)
{
  guint extra_seconds;

  extra_seconds = milliseconds / 1000;
  if (extra_seconds > 0)
  {
    timer_add_seconds(timer, extra_seconds);
    milliseconds -= extra_seconds * 1000;
  }

  timer->mseconds = ui_add(timer->mseconds, milliseconds);
}

/**
 * Return human readable string for the given time.
 */
gchar* timer_sec_msec_to_string(guint sec, guint msec, timer_precision precision /* = TIMER_PRECISION_MILLISECOND */)
{
  g_assert(msec < 1000);
  guint all_secs = sec;
  guint all_min = sec / 60;
  guint days = sec / 86400;
  sec -= days * 86400;
  guint hours = sec / 3600;
  sec -= hours * 3600;
  guint minutes = sec / 60;
  sec -= minutes * 60;

  if (precision == TIMER_PRECISION_HOUR)
    return g_strdup_printf(C_("DAYCOUNT days HOURS hours",
                              "%u days %02u hours"),
                           days,
                           hours
                           );
  else if (precision == TIMER_PRECISION_MINUTE)
    return g_strdup_printf(C_("DAYCOUNT days HOURS:MINUTES (MINUTES minutes)",
                              "%u days %02u:%02u (%02u minutes)"),
                           days,
                           hours,
                           minutes,
                           all_min
                           );
  else if (precision == TIMER_PRECISION_SECOND)
    return g_strdup_printf(C_("DAYCOUNT days HOURS:MINUTES:SECONDS (SECONDS seconds)",
                              "%u days %02u:%02u:%02u (%u seconds)"),
                           days,
                           hours,
                           minutes,
                           sec,
                           all_secs
                           );
  else
    return g_strdup_printf(C_("DAYCOUNT days HOURS:MINUTES:SECONDS:MILLISECONDS (SECONDS.MILLISECONDS seconds)",
                              "%u days %02u:%02u:%02u.%03u (%u.%03u seconds)"),
                           days,
                           hours,
                           minutes,
                           sec,
                           msec,
                           all_secs,
                           msec);

}

gchar* timer_gtvaldiff_to_string(GTimeValDiff g, timer_precision precision /* = TIMER_PRECISION_MILLISECOND */)
{
  return timer_sec_msec_to_string(g.tv_sec, g.tv_usec / 1000, precision);
}

gchar* timer_ut_timer_to_string(ut_timer *g)
{
  if (!g)
    return NULL;
  return timer_sec_msec_to_string(g->seconds, g->mseconds, g->precision);
}

ut_timer* timer_new(guint seconds,
                    guint mseconds,
                    timer_mode mode,
                    GVoidFunc success_callback,
                    GVoidFunc error_callback,
                    GTimer* timer,
                    timer_precision precision)
{
  if (!timer)
  {
    g_debug("%s: timer is NULL. Returning NULL.", __FUNCTION__);
    return NULL;
  }

  ut_timer* t;
  t = g_new(ut_timer, 1);
  t->seconds = seconds;

  // if we have mseconds > 1000, we have seconds to add to t->seconds
  if (mseconds >= 1000)
  {
    t->seconds = ui_add(t->seconds, mseconds / 1000);
    t->mseconds = mseconds % 1000;
  }
  else
    t->mseconds = mseconds;

  // our mseconds must be lower than 1000
  g_assert_cmpuint(t->mseconds, <, 1000);

  t->mode = mode;
  t->checkloop_thread_stop_with_error = FALSE;
  t->success_callback = success_callback;
  t->error_callback = error_callback;
  t->gtimer = timer;
  t->precision = precision;
  return t;
}

ut_timer* timer_new_timer(guint seconds,
                          guint mseconds,
                          GVoidFunc success_callback,
                          GVoidFunc error_callback,
                          GTimer* timer,
                          timer_precision precision)
{
  return timer_new(seconds, mseconds, TIMER_MODE_TIMER, success_callback, error_callback, timer, precision);
}

ut_timer* timer_new_countdown(guint seconds,
                              guint mseconds,
                              GVoidFunc success_callback,
                              GVoidFunc error_callback,
                              GTimer* timer,
                              timer_precision precision)
{
  return timer_new(seconds, mseconds, TIMER_MODE_COUNTDOWN, success_callback, error_callback, timer, precision);
}

ut_timer* timer_new_stopwatch(GVoidFunc success_callback,
                              GVoidFunc error_callback,
                              GTimer* timer,
                              timer_precision precision)
{
  return timer_new(0, 0, TIMER_MODE_STOPWATCH, success_callback, error_callback, timer, precision);
}

/** Destroy the ut_timer and assigns NULL to t
 * This destroys the given ut_timer and assigns NULL to the pointer t.
 * @param t a pointer to a ut_timer
 */
gboolean timer_destroy(ut_timer* t)
{
  if (!t)
    return TRUE;

  g_free(t);
  t = NULL;

  return TRUE;
}