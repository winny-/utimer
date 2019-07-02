/*
 *  tests/maintests.c
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

#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>

#include "../timer.h"

#ifdef G_DISABLE_ASSERT
  #undef G_DISABLE_ASSERT
#endif


#define TEST_DURATION_MAX_OFFSET_MSECONDS 100

GMainLoop *loop;
Config ut_config;

static void quitloop(int error_status)
{
  g_assert(loop);
  g_debug("%s: Stopping Main Loop (error code: %i)...", __FUNCTION__, error_status);

  // We set the exit status code
  ut_config.current_exit_status_code = error_status;
  g_main_loop_quit(loop);
}

static void error_quitloop()
{
  quitloop(EXIT_FAILURE);
}

static void success_quitloop()
{
  quitloop(EXIT_SUCCESS);
}

/**
 * Runs the timer for the given duration
 * There is a timeout that prevents the function from running endlessly
 * Errors are thrown using asserts
 */
static void timer_duration_launcher(guint seconds, guint mseconds, guint max_mseconds_offset)
{
  GTimer *globaltimer = g_timer_new();
  guint timeout = seconds * 1000 + mseconds + max_mseconds_offset + 500;
  g_assert(!loop);
  g_test_timer_start();
  g_test_queue_free(globaltimer);

  ut_timer *ttimer = timer_new_timer(seconds,
                                     mseconds,
                                     success_quitloop,
                                     error_quitloop,
                                     globaltimer,
                                     TIMER_PRECISION_DEFAULT,
                                     NULL);
  g_test_queue_free(ttimer);

  loop = g_main_loop_new(NULL, FALSE);

  g_debug("Starting Timer thread");

  g_assert(g_thread_create((GThreadFunc) timer_check_loop, ttimer, FALSE, NULL));

  g_debug("%s: timeout is %u ms", __FUNCTION__, timeout);
  guint timeout_id = g_timeout_add(timeout, (GSourceFunc) error_quitloop, NULL);

  g_main_loop_run(loop);
  g_source_remove(timeout_id);
  g_main_loop_unref(loop);
  loop = NULL;

  g_assert(ut_config.current_exit_status_code == EXIT_SUCCESS);

  gdouble elapsed = g_test_timer_elapsed();
  gdouble maxelapsed = (gdouble) seconds + (gdouble) (mseconds + max_mseconds_offset) / 1000;

  gdouble minelapsed = 0;
  if (seconds != 0 || mseconds > max_mseconds_offset)
    minelapsed = (gdouble) seconds + (gdouble) mseconds / 1000 - (gdouble) max_mseconds_offset / 1000;

  g_debug("elapsed: %f", elapsed);
  g_debug("maxelapsed: %f", maxelapsed);
  g_debug("minelapsed: %f", minelapsed);

  g_assert_cmpfloat(minelapsed, <=, elapsed);
  g_assert_cmpfloat(elapsed, <=, maxelapsed);
}

/**
 * Timer duration tests
 * Runs a few duration tests using the Timer
 */
static void test_timer_duration1(void)
{
  g_debug("START: %s", __FUNCTION__);
  timer_duration_launcher(0, 0, TEST_DURATION_MAX_OFFSET_MSECONDS);
  timer_duration_launcher(0, 10, TEST_DURATION_MAX_OFFSET_MSECONDS);
  timer_duration_launcher(0, 50, TEST_DURATION_MAX_OFFSET_MSECONDS);
  timer_duration_launcher(0, 100, TEST_DURATION_MAX_OFFSET_MSECONDS);
  timer_duration_launcher(0, 200, TEST_DURATION_MAX_OFFSET_MSECONDS);

  if (!g_test_quick())
  {
    timer_duration_launcher(1, 150, TEST_DURATION_MAX_OFFSET_MSECONDS);
    timer_duration_launcher(2, 99, TEST_DURATION_MAX_OFFSET_MSECONDS);
  }

  g_debug("END: %s", __FUNCTION__);
}

/**
 * Suffix checks
 * Runs multiple checks against the apply_suffix() function
 */
static void test_suffix1(void)
{
  g_debug("START: %s", __FUNCTION__);
  gint i, count = 3000;

  if (g_test_quick())
    count = 100;

  g_debug("%s: Suffix test count: %i", __FUNCTION__, count);

  for (i = 0; i < count; i++)
  {
    guint n = g_test_rand_int();
    guint days = n,
            hours = n,
            minutes = n,
            seconds = n,
            seconds2 = n,
            seconds3 = n,
            dummy1 = n,
            overflow1 = G_MAXUINT - 195; /* forcing an overflow, see below */

    /* they should all return TRUE */
    g_assert(apply_suffix(&days, "d"));
    g_assert(apply_suffix(&hours, "h"));
    g_assert(apply_suffix(&minutes, "m"));
    g_assert(apply_suffix(&seconds, "s"));
    g_assert(apply_suffix(&seconds2, ""));
    g_assert(apply_suffix(&seconds3, NULL));

    /* this should return FALSE as 'x' is unknown */
    g_assert(!apply_suffix(&dummy1, "x"));

    /* This overflows so overflow1 should be MAXUINT */
    g_assert(apply_suffix(&overflow1, "d"));
    g_assert_cmpint(overflow1, ==, G_MAXUINT);

    g_assert(days == n * 86400 || days == G_MAXUINT);
    g_assert(hours == n * 3600 || hours == G_MAXUINT);
    g_assert(minutes == n * 60 || minutes == G_MAXUINT);
    g_assert_cmpint(seconds, ==, n);
    g_assert_cmpint(seconds2, ==, n);
    g_assert_cmpint(seconds3, ==, n);
    g_assert_cmpint(dummy1, ==, n);
  }

  g_debug("END: %s", __FUNCTION__);
}

/**
 * Tests the creation of a Timer ut_timer
 */
static void test_creation_timer()
{
  g_debug("START: %s", __FUNCTION__);
  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);
  guint sec = g_test_rand_int();
  guint msec = g_test_rand_int();

  ut_timer *ttimer = timer_new_timer(sec, msec, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_MILLISECOND, NULL);
  g_assert(ttimer);
  g_test_queue_free(ttimer);

  g_assert_cmpuint(ttimer->seconds, ==, ui_add(sec, msec / 1000));
  g_assert_cmpuint(ttimer->mseconds, ==, msec % 1000);
  g_assert(ttimer->gtimer == gtimer);
  g_assert(ttimer->success_callback == success_quitloop);
  g_assert(ttimer->error_callback == error_quitloop);
  g_assert_cmpuint(ttimer->mode, ==, TIMER_MODE_TIMER);
  g_assert(ttimer->checkloop_thread_stop_with_error == FALSE);
  g_assert(ttimer->precision == TIMER_PRECISION_MILLISECOND);
  g_debug("END: %s", __FUNCTION__);
}

/**
 * Tests the creation of a Stopwatch ut_timer
 */
static void test_creation_stopwatch()
{
  g_debug("START: %s", __FUNCTION__);
  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);

  ut_timer *ttimer = timer_new_stopwatch(success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_SECOND, NULL);
  g_assert(ttimer);
  g_test_queue_free(ttimer);

  g_assert_cmpuint(ttimer->seconds, ==, 0);
  g_assert_cmpuint(ttimer->mseconds, ==, 0);
  g_assert(ttimer->gtimer == gtimer);
  g_assert(ttimer->success_callback == success_quitloop);
  g_assert(ttimer->error_callback == error_quitloop);
  g_assert_cmpuint(ttimer->mode, ==, TIMER_MODE_STOPWATCH);
  g_assert(ttimer->checkloop_thread_stop_with_error == FALSE);
  g_assert(ttimer->precision == TIMER_PRECISION_SECOND);
  g_debug("END: %s", __FUNCTION__);
}

/**
 * Tests the creation of a Countdown ut_timer
 */
static void test_creation_countdown()
{
  g_debug("START: %s", __FUNCTION__);
  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);
  guint sec = g_test_rand_int();
  guint msec = g_test_rand_int();

  ut_timer *ttimer = timer_new_countdown(sec, msec, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_MINUTE, NULL);
  g_assert(ttimer);
  g_test_queue_free(ttimer);

  g_assert_cmpuint(ttimer->seconds, ==, ui_add(sec, msec / 1000));
  g_assert_cmpuint(ttimer->mseconds, ==, msec % 1000);
  g_assert(ttimer->gtimer == gtimer);
  g_assert(ttimer->success_callback == success_quitloop);
  g_assert(ttimer->error_callback == error_quitloop);
  g_assert_cmpuint(ttimer->mode, ==, TIMER_MODE_COUNTDOWN);
  g_assert(ttimer->checkloop_thread_stop_with_error == FALSE);
  g_assert(ttimer->precision == TIMER_PRECISION_MINUTE);
  g_debug("END: %s", __FUNCTION__);
}

/**
 * Basic tests for timer_sec_msec_to_string
 */
static void test_timer_sec_msec_to_string()
{
  g_debug("START: %s", __FUNCTION__);
  gchar* tmp;

  tmp = timer_sec_msec_to_string(0, 0, TIMER_PRECISION_MILLISECOND);
  g_assert(tmp);
  g_test_queue_free(tmp);

  tmp = timer_sec_msec_to_string(1000000, 0, TIMER_PRECISION_HOUR);
  g_assert(tmp);
  g_test_queue_free(tmp);

  tmp = timer_sec_msec_to_string(1000000, 999, TIMER_PRECISION_MINUTE);
  g_assert(tmp);
  g_test_queue_free(tmp);

  tmp = timer_sec_msec_to_string(1, 1, TIMER_PRECISION_SECOND);
  g_assert(tmp);
  g_test_queue_free(tmp);
  g_debug("END: %s", __FUNCTION__);
}

/**
 * Basic tests for timer_add_seconds
 */
static void test_timer_add_seconds()
{
  g_debug("START: %s", __FUNCTION__);

  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);
  guint init_sec = g_test_rand_int();
  guint init_msec = g_test_rand_int();

  ut_timer *ttimer = timer_new_timer(init_sec, init_msec, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_DEFAULT, NULL);
  g_assert(ttimer);
  g_test_queue_free(ttimer);

  g_debug("ttimer->seconds = %u", ttimer->seconds);

  guint add_sec = g_test_rand_int();
  timer_add_seconds(ttimer, add_sec);

  guint result = ui_add(ui_add(init_sec, init_msec / 1000), add_sec);

  g_debug("ttimer->seconds = %u", ttimer->seconds);
  g_debug("result = %u", result);
  g_assert_cmpuint(ttimer->seconds, ==, result);
  g_debug("END: %s", __FUNCTION__);
}

/**
 * Basic tests for timer_add_milliseconds
 */
static void test_timer_add_milliseconds()
{
  g_debug("START: %s", __FUNCTION__);

  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);
  guint init_sec = g_test_rand_int();
  guint init_msec = g_test_rand_int();

  ut_timer *ttimer = timer_new_timer(init_sec, init_msec, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_DEFAULT, NULL);
  g_assert(ttimer);
  g_test_queue_free(ttimer);

  g_debug("ttimer->mseconds = %u", ttimer->mseconds);

  guint add_msec = g_test_rand_int();
  timer_add_milliseconds(ttimer, add_msec);

  guint result = ui_add(init_msec % 1000, add_msec % 1000);

  g_debug("ttimer->mseconds = %u", ttimer->mseconds);
  g_debug("result = %u", result);
  g_assert_cmpuint(ttimer->mseconds, ==, result);
  g_debug("END: %s", __FUNCTION__);
}

/**
 * Basic tests for timer_get_progress_percent
 */
static void test_timer_get_progress_percent_1()
{
  g_debug("START: %s", __FUNCTION__);

  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);

  ut_timer *ttimer = timer_new_timer(0, 0, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_DEFAULT, NULL);

  gint perc = timer_get_progress_percent(ttimer);
  g_debug("%s: Returned perc: %d", __FUNCTION__, perc);
  g_assert_cmpint(perc, ==, 100);
  g_debug("END: %s", __FUNCTION__);
}

static void test_timer_get_progress_percent_2()
{
  g_debug("START: %s", __FUNCTION__);

  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);

  ut_timer *ttimer = timer_new_timer(5, 0, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_DEFAULT, NULL);
  sleep(1);

  gint perc = timer_get_progress_percent(ttimer);
  g_debug("%s: Returned perc: %d", __FUNCTION__, perc);
  g_assert_cmpint(perc, <=, 25);
  g_assert_cmpint(perc, >=, 20);
  g_debug("END: %s", __FUNCTION__);
}

static void test_timer_get_progress_percent_3()
{
  g_debug("START: %s", __FUNCTION__);

  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);

  ut_timer *ttimer = timer_new_timer(100000, 0, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_DEFAULT, NULL);

  gint perc = timer_get_progress_percent(ttimer);
  g_debug("%s: Returned perc: %d", __FUNCTION__, perc);
  g_assert_cmpint(perc, <=, 1);
  g_debug("END: %s", __FUNCTION__);
}

static void test_timer_get_progress_percent_4()
{
  g_debug("START: %s", __FUNCTION__);

  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);

  ut_timer *ttimer = timer_new_timer(0, 999, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_DEFAULT, NULL);
  usleep(450000);

  gint perc = timer_get_progress_percent(ttimer);
  g_debug("%s: Returned perc: %d", __FUNCTION__, perc);
  g_assert_cmpint(perc, <=, 60);
  g_assert_cmpint(perc, >=, 45);
  g_debug("END: %s", __FUNCTION__);
}

static void test_timer_get_progress_percent_5()
{
  g_debug("START: %s", __FUNCTION__);

  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);

  ut_timer *ttimer = timer_new_timer(1, 0, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_DEFAULT, NULL);
  usleep(500000);

  gint perc = timer_get_progress_percent(ttimer);
  g_debug("%s: Returned perc: %d", __FUNCTION__, perc);
  g_assert_cmpint(perc, <=, 55);
  g_assert_cmpint(perc, >=, 50);
  g_debug("END: %s", __FUNCTION__);
}

static void test_timer_get_progress_percent_6()
{
  g_debug("START: %s", __FUNCTION__);

  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);

  ut_timer *ttimer = timer_new_timer(1000, 0, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_DEFAULT, NULL);
  sleep(10);

  gint perc = timer_get_progress_percent(ttimer);
  g_debug("%s: Returned perc: %d", __FUNCTION__, perc);
  g_assert_cmpint(perc, ==, 1);
  g_debug("END: %s", __FUNCTION__);
}

static void test_get_progress_bar()
{
  g_debug("START: %s", __FUNCTION__);

  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);

  ut_timer *ttimer = timer_new_timer(0, 999, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_DEFAULT, NULL);
  usleep(450000);

  gint8 perc = timer_get_progress_percent(ttimer);
  gchar* bar = get_progress_bar(perc, 20, TRUE);
  g_test_queue_free(bar);
  g_debug("%s: Returned bar: %s", __FUNCTION__, bar);

  g_assert_cmpuint(strlen(bar), ==, 20);

  g_debug("END: %s", __FUNCTION__);
}

static void test_get_progress_bar2()
{
  g_debug("START: %s", __FUNCTION__);

  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);

  ut_timer *ttimer = timer_new_timer(0, 0, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_DEFAULT, NULL);
  usleep(450000);

  gint8 perc = timer_get_progress_percent(ttimer);
  gchar* bar = get_progress_bar(perc, 30, TRUE);
  g_test_queue_free(bar);
  g_debug("%s: Returned bar: %s", __FUNCTION__, bar);

  g_assert_cmpuint(strlen(bar), ==, 30);

  g_debug("END: %s", __FUNCTION__);
}

static void test_get_progress_bar3()
{
  g_debug("START: %s", __FUNCTION__);

  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);

  ut_timer *ttimer = timer_new_timer(0, 0, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_DEFAULT, NULL);

  gint8 perc = timer_get_progress_percent(ttimer);
  gchar* bar = get_progress_bar(perc, 30, TRUE);
  g_test_queue_free(bar);
  g_debug("%s: Returned bar: %s", __FUNCTION__, bar);

  g_assert_cmpuint(strlen(bar), ==, 30);

  g_debug("END: %s", __FUNCTION__);
}

static void test_get_progress_bar4()
{
  g_debug("START: %s", __FUNCTION__);

  GTimer *gtimer = g_timer_new();
  g_test_queue_free(gtimer);

  ut_timer *ttimer = timer_new_timer(0, 500, success_quitloop, error_quitloop, gtimer, TIMER_PRECISION_DEFAULT, NULL);

  gint8 perc = timer_get_progress_percent(ttimer);
  gchar* bar = get_progress_bar(perc, 30, TRUE);
  g_test_queue_free(bar);
  g_debug("%s: Returned bar: %s", __FUNCTION__, bar);

  g_assert_cmpuint(strlen(bar), ==, 30);

  g_debug("END: %s", __FUNCTION__);
}

static void test_get_progress_bar_width()
{
  g_debug("START: %s", __FUNCTION__);

  gchar *bar;
  gushort i;

  for(i = 0; i <= 100; i++)
  {
    bar = get_progress_bar(0, i, TRUE);
    g_test_queue_free(bar);
    g_assert_cmpuint(strlen(bar), ==, i);
  }

  bar = get_progress_bar(0, G_MAXUSHORT, TRUE);
  g_test_queue_free(bar);
  g_assert_cmpuint(strlen(bar), ==, G_MAXUSHORT);

  g_debug("END: %s", __FUNCTION__);
}

/**
 * Main tests' Main()
 * Starts the main testing units
 */
gint main(gint argc, gchar *argv[])
{
  // initialize test program
  g_test_init(&argc, &argv, NULL);

  g_thread_init(NULL);
  g_type_init();

  // set verbosity
  ut_config.verbose = g_test_verbose();
  ut_config.debug = g_test_verbose();
  ut_config.quiet = g_test_quiet() || !g_test_verbose();

  setup_log_handler();

  // hook up the test functions
  g_test_add_func("/Utils/Suffix/Test1", test_suffix1);

  g_test_add_func("/General/TimerCreation/Timer", test_creation_timer);
  g_test_add_func("/General/TimerCreation/Stopwatch", test_creation_stopwatch);
  g_test_add_func("/General/TimerCreation/Countdown", test_creation_countdown);
  g_test_add_func("/General/TimerDuration/Test1", test_timer_duration1);

  g_test_add_func("/General/Functions/timer_sec_msec_to_string", test_timer_sec_msec_to_string);
  g_test_add_func("/General/Functions/timer_add_seconds", test_timer_add_seconds);
  g_test_add_func("/General/Functions/timer_timer_add_milliseconds", test_timer_add_milliseconds);
  g_test_add_func("/General/Functions/timer_timer_get_progress_percent1", test_timer_get_progress_percent_1);
  g_test_add_func("/General/Functions/timer_timer_get_progress_percent2", test_timer_get_progress_percent_2);
  g_test_add_func("/General/Functions/timer_timer_get_progress_percent3", test_timer_get_progress_percent_3);
  g_test_add_func("/General/Functions/timer_timer_get_progress_percent4", test_timer_get_progress_percent_4);
  g_test_add_func("/General/Functions/timer_timer_get_progress_percent5", test_timer_get_progress_percent_5);

  if (g_test_slow())
  {
    g_test_add_func("/General/Functions/timer_timer_get_progress_percent6", test_timer_get_progress_percent_6);
  }

  g_test_add_func("/General/Functions/timer_get_progress_bar1", test_get_progress_bar);
  g_test_add_func("/General/Functions/timer_get_progress_bar2", test_get_progress_bar2);
  g_test_add_func("/General/Functions/timer_get_progress_bar3", test_get_progress_bar3);
  g_test_add_func("/General/Functions/timer_get_progress_bar4", test_get_progress_bar4);
  g_test_add_func("/General/Functions/timer_get_progress_bar_width", test_get_progress_bar_width);

  // run tests from the suite
  return g_test_run();
}
