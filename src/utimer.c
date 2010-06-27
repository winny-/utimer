/*
 *  utimer.c
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

#include "utimer.h"

static gchar *timer_info, *countdown_info, *refresh_rate;
static gboolean *stopwatch = FALSE,
        *show_limits = FALSE,
        *show_version = FALSE;

static GOptionEntry entries[] = {

  {"countdown",
   'c',
   0,
   G_OPTION_ARG_STRING,
   &(countdown_info),
   N_("count from TIMELENGTH down to 0 then exit (e.g. -c 30d9h50s)"),
   N_("TIMELENGTH")},

  {"debug",
   'd',
   0,
   G_OPTION_ARG_NONE,
   &(ut_config.debug),
   N_("debug output"),
   NULL},

  {"limits",
   'L',
   0,
   G_OPTION_ARG_NONE,
   &(show_limits),
   N_("show the limits of µTimer"),
   NULL},

  {"quiet",
   'q',
   0,
   G_OPTION_ARG_NONE,
   &(ut_config.quiet),
   N_("quiet output"),
   NULL},

  {"quit-with-success",
   'Q',
   0,
   G_OPTION_ARG_NONE,
   &(ut_config.quit_with_success),
   N_("return a success exit status (0) when using 'q' to exit"),
   NULL},

  {"refresh-rate",
   'r',
   0,
   G_OPTION_ARG_STRING,
   &(refresh_rate),
   N_("display refresh rate. RATE can be 'm' for minute, 's' for\
 second, 'ms' for millisecond (default)"),
   N_("RATE")},

  {"stopwatch",
   's',
   0,
   G_OPTION_ARG_NONE,
   &(stopwatch),
   N_("start the stopwatch. Hit 'Q' to exit"),
   NULL},

  {"timer",
   't',
   0,
   G_OPTION_ARG_STRING,
   &(timer_info),
   N_("count from 0 to TIMELENGTH then exit (e.g. -t 31m27s300ms)"),
   N_("TIMELENGTH")},

  {"verbose",
   'v',
   0,
   G_OPTION_ARG_NONE,
   &(ut_config.verbose),
   N_("verbose output"),
   NULL},

  {"version",
   'V',
   0,
   G_OPTION_ARG_NONE,
   &(show_version),
   N_("display the current version of µTimer"),
   NULL},

  {NULL}
};

static void clean_up(void)
{
  free_config(&ut_config);

  if (countdown_info)
  {
    g_debug("Freeing countdown_info...");
    g_free(countdown_info);
    countdown_info = NULL;
  }

  if (timer_info)
  {
    g_debug("Freeing timer_info...");
    g_free(timer_info);
    timer_info = NULL;
  }

  if (refresh_rate)
  {
    g_debug("Freeing refresh_rate...");
    g_free(refresh_rate);
    refresh_rate = NULL;
  }
}

int main(int argc, char *argv[])
{ /* ================== MAIN STARTS ==================== */
  GError *error = NULL;
  GOptionContext *context;
  gchar *tmp = NULL;
  ut_timer *ttimer;
  gint print_refresh_rate;
  /* -------------- Initialization ------------- */

  tcgetattr(STDIN_FILENO, &savedttystate); /* Save current tty state  */

  g_thread_init(NULL);
  g_type_init();
  init_config(&ut_config);

  /* We cleanup at exit */
  g_atexit(clean_up);

  /* Define localization */
  ut_config.locale = setlocale(LC_ALL, "");
  if (!ut_config.locale)
  {
    g_error(_("Error during setting current locale. Falling back to default locale."));
    ut_config.locale = setlocale(LC_ALL, "C");
    if (!ut_config.locale)
    {
      g_critical(_("Couldn't set any locale. Exiting..."));
      exit(EXIT_FAILURE);
    }
  }

  /* i18n */
  g_set_prgname(PACKAGE);
  bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);


  /* Set the function to call in case of receiving signals:
   * we need to stop the main loop to exit correctly, but
   * with error code!
   */
  signal(SIGALRM, error_quitloop);
  signal(SIGHUP, error_quitloop);
  signal(SIGINT, error_quitloop);
  signal(SIGPIPE, error_quitloop);
  signal(SIGQUIT, error_quitloop);
  signal(SIGTERM, error_quitloop);

  /* Set up the log handler */
  setup_log_handler();

  /* ------------ Initialization is done ---------- */

  /* -------------- Options parsing ------------- */

  tmp = g_strconcat(_("[ARGUMENTS] - "), SHORTDESCRIPTION, NULL);
  context = g_option_context_new(tmp);
  g_free(tmp);

  g_option_context_set_summary(context, SUMMARY);

  tmp = g_strconcat(DESCRIPTION,
                    _("\nReport any bug to: https://bugs.launchpad.net/utimer\
 (bugs@utimer.codealpha.net)"),
                    NULL);
  g_option_context_set_description(context, tmp);
  g_free(tmp);

  g_option_context_add_main_entries(context, entries, NULL);

  if (!g_option_context_parse(context, &argc, &argv, &error))
  {
    g_printerr(_("Error while parsing options: %s.\n"), error->message);
    g_printerr(_("Run '%s --help' to see a full list of available command\
 line options.\n"), argv[0]);
    g_error_free(error);
    exit(EXIT_FAILURE);
  }

  g_option_context_free(context);


  /* ------------ Options parsing DONE ------------- */


  /* Verify that there is at least an option */
  if (!(timer_info
        || show_version
        || show_limits
        || countdown_info
        || stopwatch))
  {
    g_printerr(_("No option has been specified!\n"));
    g_printerr(_("Run '%s --help' to see a full list of available command\
 line options.\n"), argv[0]);
    exit(EXIT_FAILURE);
  }


  /* Check if there is any conflicting options */

  if (ut_config.debug)
    ut_config.quiet = FALSE;

  if (timer_info && (countdown_info || stopwatch)
      || countdown_info && stopwatch)
  {
    g_warning(_("Conflicting options!\nThe following options cannot\n\
 be used simultaneously:\n -t (timer mode), -c (countdown mode), -s\
 (stopwatch mode)."));
    exit(EXIT_FAILURE);
  }

  /* set up refresh rate if given */
  print_refresh_rate = 89;
  if (refresh_rate)
  {
    if (g_ascii_strcasecmp(refresh_rate, "m") == 0)
    {
      print_refresh_rate = 60 * 1000;
    }
    else if (g_ascii_strcasecmp(refresh_rate, "s") == 0)
    {
      print_refresh_rate = 1000;
    }
  }

  g_debug("G_MAXUINT = %u", G_MAXUINT);

  if (isatty(STDOUT_FILENO))
    g_debug("\033[34mYou are using a TTY.\033[m");
  else
    g_debug("You are not using a TTY.");

  if (ut_config.debug)
  {
    g_debug("Deactivating buffered output for stderr and stdout...");

    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
  }

  /* Showing limits */

  if (show_limits)
  {
    g_print(_("This is a list of the possible limits of µTimer for your\
 machine.\n\n"));

    tmp = timer_get_maximum_time();
    g_print(_("* Limits for the Timer/Countdown:\n"));
    g_print(_("\t- The maximum possible timer length is: %s.\n"), tmp);
    g_print(_("\t  If you enter a value that is exceeding it, it will be\
 replaced by the value above.\n"));

    g_free(tmp);
    tmp = NULL;

    exit(EXIT_SUCCESS);
  }

  /* Showing version */

  if (show_version)
  {
    g_print("%s\n\n%s\n\n%s\n", PACKAGE_STRING, COPYRIGHT, COPYRIGHT_TXT);
    exit(EXIT_SUCCESS);
  }

  /* ------------------ PROCESSING STARTS ------------------ */

  /* Prepare for starting the main loop */

  loop = g_main_loop_new(NULL, FALSE);

  //TODO: this is not clear how it stops the loop!
  g_idle_add((GSourceFunc) start_thread_exit_check, ttimer);

  /* -------------- TIMER & COUNTDOWN MODE -------------- */
  if (timer_info || countdown_info || stopwatch)
  {
    timer_precision prec = TIMER_PRECISION_MILLISECOND;


    /* set up refresh rate if given */
    if (refresh_rate)
    {
      if (g_ascii_strcasecmp(refresh_rate, "m") == 0)
      {
        prec = TIMER_PRECISION_MINUTE;
      }
      else if (g_ascii_strcasecmp(refresh_rate, "s") == 0)
      {
        prec = TIMER_PRECISION_SECOND;
      }
    }

    if (countdown_info)
    {
      g_debug("Countdown Mode");
      ttimer = timer_new_countdown(0, 0, success_quitloop, error_quitloop, ut_config.timer, prec);
      parse_time_pattern(countdown_info, &(ttimer->seconds), &(ttimer->mseconds));
    }
    else if (timer_info)
    {
      g_debug("Timer Mode");
      ttimer = timer_new_timer(0, 0, success_quitloop, error_quitloop, ut_config.timer, prec);
      parse_time_pattern(timer_info, &(ttimer->seconds), &(ttimer->mseconds));
    }
    else
    {
      g_debug("Stopwatch Mode");
      ttimer = timer_new_stopwatch(success_quitloop, error_quitloop, ut_config.timer, prec);
    }

    tmp = timer_get_maximum_time();
    g_debug("Maximum Timer length is %s.", tmp);
    g_free(tmp);
    tmp = NULL;

    if (!ut_config.quiet && ut_config.verbose && !stopwatch)
    {
      tmp = timer_ut_timer_to_string(ttimer);
      g_info(_("Timer will stop in approximately %s. Hit 'Q' to abort."), tmp);
      g_free(tmp);
      tmp = NULL;
    }

    // print timer once because the timeout loop waits before executing the func
    timer_print(ttimer);
    // create the timeout loop to print the timer every refresh_rate
    ttimer->timer_print_source_id = g_timeout_add(print_refresh_rate,
                                                  (GSourceFunc) timer_print,
                                                  ttimer);
    if (ttimer->mode != TIMER_MODE_STOPWATCH)
      g_idle_add((GSourceFunc) timer_run_checkloop_thread, ttimer);
  } /* -------------- END TIMER & COUNTDOWN MODE -------------- */
  else
  {
    /* No mode selected! We quit the loop ASAP. */
    g_idle_add((GSourceFunc) error_quitloop, NULL);
  }


  /* ------------- MAIN LOOP ---------------- */

  g_debug("Starting main loop...");
  g_main_loop_run(loop);
  g_main_loop_unref(loop);
  loop = NULL;
  g_debug("Exiting main loop...");

  /* Print the timer one more time to show the actual time (in case of slow
   * refresh rates. */
  timer_print(ttimer);

  /* ------------- END OF MAIN LOOP ---------------- */

  g_debug("Quitting with error code: %i", ut_config.current_exit_status_code);


  /* ================== CLEAN UP ==================== */
  g_message("\n"); // print a new line if not in quiet mode
  timer_destroy(ttimer);

  /* ================== MAIN DONE ==================== */
  return ut_config.current_exit_status_code;
}
