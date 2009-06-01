/*
 *  tests/maintests.c
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

#ifdef G_DISABLE_ASSERT
#  undef G_DISABLE_ASSERT
#endif 

#include <glib-object.h>
#include "../utimer.h"

#define TEST_DURATION_MAX_OFFSET_MSECONDS 100

/**
 * Runs the timer for the given duration
 * There is a timeout that prevents the function from running endlessly
 * Errors are thrown using asserts
 */
static void test_timer_duration_launcher (guint seconds, guint mseconds, guint max_mseconds_offset)
{
  GTimer *globaltimer = g_timer_new ();
  guint timeout = seconds*1000 + mseconds + max_mseconds_offset + 500;
  g_assert (!loop);
  g_test_timer_start ();
  g_test_queue_free (globaltimer);
  
  ut_timer *ttimer = timer_new_timer (seconds,
                                      mseconds,
                                      success_quitloop,
                                      error_quitloop,
                                      globaltimer);
  g_test_queue_free (ttimer);
  
  loop = g_main_loop_new (NULL, FALSE);
  
  g_idle_add ((GSourceFunc) timer_run_checkloop_thread, ttimer);
  
  g_debug ("%s: timeout is %u ms", __FUNCTION__, timeout);
  guint timeout_id = g_timeout_add (timeout, (GSourceFunc) error_quitloop, NULL);
  
  g_main_loop_run (loop);
  g_source_remove (timeout_id);
  g_main_loop_unref (loop);
  loop = NULL;
  
  g_assert (ut_config.current_exit_status_code == EXIT_SUCCESS);
  
  gdouble elapsed = g_test_timer_elapsed ();
  gdouble maxelapsed = (gdouble)seconds + (gdouble)(mseconds+max_mseconds_offset)/1000;
  
  gdouble minelapsed = 0;
  if (seconds != 0 || mseconds > max_mseconds_offset)
    minelapsed = (gdouble)seconds + (gdouble)mseconds/1000 - (gdouble)max_mseconds_offset/1000;
  
  g_debug ("elapsed: %f",    elapsed);
  g_debug ("maxelapsed: %f", maxelapsed);
  g_debug ("minelapsed: %f", minelapsed);
  
  g_assert_cmpfloat (minelapsed, <= , elapsed);
  g_assert_cmpfloat (elapsed, <= , maxelapsed);
}

/**
 * Timer duration tests
 * Runs a few duration tests using the Timer
 */
static void test_timer_duration1 (void)
{
  test_timer_duration_launcher (0, 0, TEST_DURATION_MAX_OFFSET_MSECONDS);
  test_timer_duration_launcher (0, 50, TEST_DURATION_MAX_OFFSET_MSECONDS);
  test_timer_duration_launcher (0, 200, TEST_DURATION_MAX_OFFSET_MSECONDS);
  
  if (!g_test_quick())
    test_timer_duration_launcher (1, 150, TEST_DURATION_MAX_OFFSET_MSECONDS);
}

/**
 * Suffix checks
 * Runs multiple checks against the apply_suffix() function
 */
static void test_suffix1 (void)
{
  gint i, count = 1000;
  
  if (g_test_quick())
    count = 100;
  
  for (i = 0; i < count; i++)
  {
    guint n = g_test_rand_int ();
    guint days = n,
          hours = n,
          minutes = n,
          seconds = n,
          seconds2 = n,
          seconds3 = n,
          dummy1   = n,
          overflow1= G_MAXUINT-145; /* substract a random value to see if it gives us G_MAXUINT anyway */
    
    /* they should all return TRUE */
    g_assert(apply_suffix (&days,     "d"));
    g_assert(apply_suffix (&hours,    "h"));
    g_assert(apply_suffix (&minutes,  "m"));
    g_assert(apply_suffix (&seconds,  "s"));
    g_assert(apply_suffix (&seconds2, ""));
    g_assert(apply_suffix (&seconds3, NULL));
    
    /* this should return FALSE as 'x' is unknown */
    g_assert(!apply_suffix (&dummy1, "x"));
    
    /* This should overflow */
    g_assert(apply_suffix (&overflow1, "d"));
    
    g_assert (days == n * 86400 || days == G_MAXUINT);
    g_assert (hours == n * 3600 || hours == G_MAXUINT);
    g_assert (minutes == n * 60 || minutes == G_MAXUINT);
    g_assert_cmpint (seconds,   ==, n);
    g_assert_cmpint (seconds2,  ==, n);
    g_assert_cmpint (seconds3,  ==, n);
    g_assert_cmpint (dummy1,    ==, n);
    g_assert_cmpint (overflow1, ==, G_MAXUINT);
  }
}

/**
 * Tests the creation of a Timer ut_timer
 */
static void test_creation_timer ()
{
  GTimer *gtimer = g_timer_new ();
  g_test_queue_free (gtimer);
  guint sec = g_test_rand_int ();
  guint msec = g_test_rand_int ();
  
  ut_timer *ttimer = timer_new_timer (sec, msec, success_quitloop, error_quitloop, gtimer);
  
  g_assert (ttimer);
  
  g_assert_cmpint (ttimer->seconds, ==, sec);
  g_assert_cmpint (ttimer->mseconds, ==, msec);
  g_assert (ttimer->gtimer == gtimer);
  g_assert (ttimer->success_callback == success_quitloop);
  g_assert (ttimer->error_callback == error_quitloop);
  g_assert_cmpint (ttimer->mode, ==, TIMER_MODE_TIMER);
  g_assert (ttimer->checkloop_thread_stop_with_error == FALSE);
}

/**
 * Tests the creation of a Stopwatch ut_timer
 */
static void test_creation_stopwatch ()
{
  GTimer *gtimer = g_timer_new ();
  g_test_queue_free (gtimer);
  
  ut_timer *ttimer = stopwatch_new_timer (success_quitloop, error_quitloop, gtimer);
  
  g_assert (ttimer);
  
  g_assert_cmpint (ttimer->seconds, ==, 0);
  g_assert_cmpint (ttimer->mseconds, ==, 0);
  g_assert (ttimer->gtimer == gtimer);
  g_assert (ttimer->success_callback == success_quitloop);
  g_assert (ttimer->error_callback == error_quitloop);
  g_assert_cmpint (ttimer->mode, ==, TIMER_MODE_STOPWATCH);
  g_assert (ttimer->checkloop_thread_stop_with_error == FALSE);
}

/**
 * Tests the creation of a Countdown ut_timer
 */
static void test_creation_countdown ()
{
  GTimer *gtimer = g_timer_new ();
  g_test_queue_free (gtimer);
  guint sec = g_test_rand_int ();
  guint msec = g_test_rand_int ();
  
  ut_timer *ttimer = countdown_new_timer (sec, msec, success_quitloop, error_quitloop, gtimer);
  
  g_assert (ttimer);
  
  g_assert_cmpint (ttimer->seconds, ==, sec);
  g_assert_cmpint (ttimer->mseconds, ==, msec);
  g_assert (ttimer->gtimer == gtimer);
  g_assert (ttimer->success_callback == success_quitloop);
  g_assert (ttimer->error_callback == error_quitloop);
  g_assert_cmpint (ttimer->mode, ==, TIMER_MODE_COUNTDOWN);
  g_assert (ttimer->checkloop_thread_stop_with_error == FALSE);
}

/**
 * Main tests' Main()
 * Starts the main testing units
 */
gint main (gint argc, gchar *argv[])
{
  // initialize test program
  g_test_init (&argc, &argv, NULL);
  
  g_thread_init (NULL);
  g_type_init ();
  
  // set verbosity
  ut_config.verbose = g_test_verbose();
  ut_config.debug   = g_test_verbose();
  ut_config.quiet   = g_test_quiet() || !g_test_verbose();
  
  setup_log_handler ();
  
  // hook up the test functions
  g_test_add_func ("/Utils/Suffix/Test1", test_suffix1);
  g_test_add_func ("/Timer/Duration/Test1", test_timer_duration1);
  g_test_add_func ("/General/TimerCreation/Timer", test_creation_timer);
  g_test_add_func ("/General/TimerCreation/Stopwatch", test_creation_stopwatch);
  g_test_add_func ("/General/TimerCreation/Countdown", test_creation_countdown);
  
  // run tests from the suite
  return g_test_run();
}
