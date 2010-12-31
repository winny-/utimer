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
static gboolean stopwatch = FALSE,
        show_limits = FALSE,
        show_version = FALSE,
        show_bar = FALSE,
        show_perc = FALSE,
        show_text = TRUE;

static GOptionEntry entries[] = {

  {"bar",
   0,
   0,
   G_OPTION_ARG_NONE,
   &show_bar,
   N_("show the progress bar (default) (only for timer and countdown)"),
   NULL},

  {"no-bar",
   0,
   G_OPTION_FLAG_REVERSE,
   G_OPTION_ARG_NONE,
   &show_bar,
   N_("hide the progress bar (see --bar)"),
   NULL},

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

  {"perc",
   0,
   0,
   G_OPTION_ARG_NONE,
   &show_perc,
   N_("show a percentage of the time left (default) (only for timer and countdown)"),
   NULL},

  {"no-perc",
   0,
   G_OPTION_FLAG_REVERSE,
   G_OPTION_ARG_NONE,
   &show_perc,
   N_("hide the percentage of the time left (see --perc)"),
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

  {"time",
   0,
   0,
   G_OPTION_ARG_NONE,
   &show_text,
   N_("show time left as text (default)"),
   NULL},

  {"no-time",
   0,
   G_OPTION_FLAG_REVERSE,
   G_OPTION_ARG_NONE,
   &show_text,
   N_("hide the text showing the time left (see --time)"),
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

/**
 * Activate/Deactivate the canonical mode from a TTY.
 */
void set_tty_canonical(int state)
{
  struct termios ttystate;

  if (state == 1)
  {
    g_debug("Activating canonical mode.");
    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag &= ~ICANON; // remove canonical mode
    ttystate.c_cc[VMIN] = 1; // minimum length to read before sending
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate); // apply the changes
  }
  else
  {
    g_debug("Deactivating canonical mode.");
    tcsetattr(STDIN_FILENO, TCSANOW, &savedttystate); // put canonical mode back
  }
}

/**
 * Deactivate the canoncial mode (used with atexit())
 */
void reset_tty_canonical_mode()
{
  set_tty_canonical(0);
}

/**
 * Quits the main loop to end the program.
 * Quits the main loop to end the program with error_status
 * (EXIT_SUCCESS or EXIT_FAILURE).
 */
void quitloop(int error_status)
{
  g_assert(loop);
  g_debug("%s: Stopping Main Loop (error code: %i)...", __FUNCTION__, error_status);

  // We set the exit status code
  ut_config.current_exit_status_code = error_status;
  g_main_loop_quit(loop);
}

/**
 * Quits the main loop with error to end the program.
 * Quits the main loop to end the program ungracefully (EXIT_FAILURE).
 * This function just calls quitloop(EXIT_FAILURE).
 * This is needed for signal traps.
 */
void error_quitloop()
{
  quitloop(EXIT_FAILURE);
}

/**
 * Quits the main loop with success code to end the program.
 * Quits the main loop to end the program gracefully (EXIT_SUCCESS).
 * This function just calls quitloop(EXIT_SUCCESS).
 * This is needed for callbacks.
 */
void success_quitloop()
{
  quitloop(EXIT_SUCCESS);
}

/**
 * Updates terminal_cols and starts waiting for signal again.
 */
void terminal_size_changed(gint s)
{
  ut_config.terminal_cols = get_terminal_width();
  g_message("\r ");
  g_debug("%s: Received SIGWINCH cols=%d",
          __FUNCTION__, ut_config.terminal_cols);
}

/**
 * Check to see if the user wants to quit.
 * This function waits till the user hits the 'q' key to quit the program,
 * then it will call the quitloop function. This is called by a thread
 * to avoid blocking the program.
 */
int check_exit_from_user()
{
  set_tty_canonical(1); /* Apply canonical mode to TTY*/
  g_atexit(reset_tty_canonical_mode); /* Deactivate canonical mode at exit */

  gint c;
  do
  {
    c = fgetc(stdin);
    g_print("\b \b"); // backspace, write a space to clear, backspace again
    switch (c)
    {
      case ' ':
      {
        if (paused)
        {
          g_timer_continue(ut_config.timer);
          paused = FALSE;
        }
        else
        {
          g_timer_stop(ut_config.timer);
          paused = TRUE;
        }
      }

    }
  } while (c != 'q' && c != 'Q'); /* checks for 'q' key */

  /* If the user asks for exiting, we stop the loop. */
  quitloop((ut_config.quit_with_success ? EXIT_SUCCESS : EXIT_FAILURE));
}

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

  /* Set up the log handler */
  setup_log_handler();

  /* Handles any change of size for the terminal */
  (void) signal(SIGWINCH, terminal_size_changed);
  /* force the signal to get the current terminal size */
  terminal_size_changed(0);

  /* Set the function to call in case of receiving signals:
   * we need to stop the main loop to exit correctly, but
   * with error code!
   */
  (void) signal(SIGALRM, error_quitloop);
  (void) signal(SIGHUP, error_quitloop);
  (void) signal(SIGINT, error_quitloop);
  (void) signal(SIGPIPE, error_quitloop);
  (void) signal(SIGQUIT, error_quitloop);
  (void) signal(SIGTERM, error_quitloop);



  /* ------------ Initialization is done ---------- */

  /* -------------- Options parsing ------------- */

  tmp = g_strconcat(_("[ARGUMENTS] - "), SHORTDESCRIPTION, NULL);
  context = g_option_context_new(tmp);
  g_free(tmp);

  g_option_context_set_summary(context, SUMMARY);

  tmp = g_strconcat(DESCRIPTION,
                    _("\nReport any bug to: https://bugs.launchpad.net/utimer/+filebug"),
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
    g_printerr(_("No main option (-t, -c or -s) has been specified!\n"));
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

  /* -------------- TIMER & COUNTDOWN MODE -------------- */
  if (timer_info || countdown_info || stopwatch)
  {
    g_debug("Setting up default precision");
    timer_precision precision = TIMER_PRECISION_MILLISECOND;
    g_debug("Setting up default display");
    g_debug("perc %i", show_perc);
    g_debug("text %i", show_text);
    g_debug("bar %i", show_bar);
    timer_display options_timer_display = { .perc = show_perc,
                                            .text = show_text,
                                            .bar = show_bar };

    /* set up refresh rate if given */
    if (refresh_rate)
    {
      if (g_ascii_strcasecmp(refresh_rate, "m") == 0)
      {
        precision = TIMER_PRECISION_MINUTE;
      }
      else if (g_ascii_strcasecmp(refresh_rate, "s") == 0)
      {
        precision = TIMER_PRECISION_SECOND;
      }
    }

    guint seconds = 0;
    guint mseconds = 0;

    if (countdown_info)
    {
      g_debug("Countdown Mode");
      parse_time_pattern(countdown_info, &(seconds), &(mseconds));
      ttimer = timer_new_countdown(seconds, mseconds, success_quitloop, error_quitloop, ut_config.timer, precision, &options_timer_display);
    }
    else if (timer_info)
    {
      g_debug("Timer Mode");
      parse_time_pattern(timer_info, &(seconds), &(mseconds));
      ttimer = timer_new_timer(seconds, mseconds, success_quitloop, error_quitloop, ut_config.timer, precision, &options_timer_display);
    }
    else
    {
      g_debug("Stopwatch Mode");
      ttimer = timer_new_stopwatch(success_quitloop, error_quitloop, ut_config.timer, precision, &options_timer_display);
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
    {
      g_debug("Starting Timer thread");

      if (!g_thread_create((GThreadFunc) timer_check_loop, ttimer, FALSE, &error))
      {
        g_printerr(_("Thread creation failed: %s"), error->message);
        g_error_free(error);
        error_quitloop();
        exit(EXIT_FAILURE);
      }
    }
  } /* -------------- END TIMER & COUNTDOWN MODE -------------- */
  else
  {
    /* No mode selected! We quit the loop ASAP. */
    g_idle_add((GSourceFunc) error_quitloop, NULL);
  }

  g_debug("Creating thread exit check");
  if (!g_thread_create((GThreadFunc) check_exit_from_user, NULL, FALSE, &error))
  {
    // thread creation failed!
    g_printerr(_("Thread creation failed: %s"), error->message);
    g_error_free(error);
    exit(EXIT_FAILURE);
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
