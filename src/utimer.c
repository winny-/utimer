/*
 *  utimer.c
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

#include "utimer.h"

//~ static    char            **remaining_args;
static ut_timer        *ttimer;
static gchar           *timer_info, *countdown_info;
static gboolean        *stopwatch = FALSE,
                       *show_limits = FALSE,
                       *show_version = FALSE;

static GOptionEntry entries[] = {
  
  {"timer",
    't',
    0,
    G_OPTION_ARG_STRING,
    &(timer_info),
    N_("count from 0 to TIMELENGTH and then exit\
 (e.g. utimer -t 31m27s300ms)"),
    N_("TIMELENGTH")
  },
  
  {"countdown",
    'c',
    0,
    G_OPTION_ARG_STRING,
    &(countdown_info),
    N_("count from TIMELENGTH down to 0 and exit (e.g.\
 utimer -c 30d9h50s)"),
    N_("TIMELENGTH")
  },
  
  {"stopwatch",
    's',
    0,
    G_OPTION_ARG_NONE,
    &(stopwatch),
    N_("start the stopwatch. Use 'Q' key to quit"),
    NULL
  },
  
  {"verbose",
    'v',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.verbose),
    N_("Verbose output"),
    NULL
  },
  
  {"quiet",
    'q',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.quiet),
    N_("Quiet output"),
    NULL
  },
  
  {"quit-with-success",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.quit_with_success), 
    N_("When hitting 'q' to end the program, exit with a SUCCESS exit\
 status (0)"),
    NULL
  },

  {"limits",
    'L',
    0,
    G_OPTION_ARG_NONE,
    &(show_limits),
    N_("Show the limits of µTimer (Maximum time length, Accuracy...)"),
    NULL
  },
  
  {"version",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &(show_version),
    N_("Display the current version of µTimer"),
    NULL
  },
  
  {"debug",
    'd',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.debug),
    N_("Debug output"),
    NULL
  },

  //~ {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &remaining_args,
    //~ NULL, NULL },
  {NULL}
};



static void clean_up (void)
{
  free_config (&ut_config);
  
  if (ttimer)
  {
    g_debug ("Freeing ttimer...");
    g_free (ttimer);
    ttimer = NULL;
  }
  
  if (countdown_info)
  {
    g_debug ("Freeing isCountdown...");
    g_free (countdown_info);
    countdown_info = NULL;
  }
  
  if (timer_info)
  {
    g_debug ("Freeing isTimer...");
    g_free (timer_info);
    timer_info = NULL;
  }
}

int main (int argc, char *argv[])
{        /* ================== MAIN STARTS ==================== */
  GError          *error = NULL;
  GOptionContext  *context;
  gchar           *tmp = NULL;
  gint            i;
  gchar           *options_help;
               /* -------------- Initialization ------------- */
  
  tcgetattr(STDIN_FILENO, &savedttystate); /* Save current tty state  */
  
  g_thread_init (NULL);
  g_type_init ();
  init_config (&ut_config);
  
  /* We cleanup at exit */
  g_atexit (clean_up);
  
  /* Define localization */
  ut_config.locale = setlocale (LC_ALL, "");
  if (!ut_config.locale)
  {
    g_error (_("Error during setting current locale. Falling back to default locale."));
    ut_config.locale = setlocale (LC_ALL, "C");
    if (!ut_config.locale)
    {
      g_critical (_("Couldn't set any locale. Exiting..."));
      exit (EXIT_FAILURE);
    }
  }
  
  /* i18n */
  g_set_prgname (PACKAGE);
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
  
  
  /* Set the function to call in case of receiving signals:
   * we need to stop the main loop to exit correctly, but
   * with error code!
   */
  signal (SIGALRM,  error_quitloop);
  signal (SIGHUP,   error_quitloop);
  signal (SIGINT,   error_quitloop);
  signal (SIGPIPE,  error_quitloop);
  signal (SIGQUIT,  error_quitloop);
  signal (SIGTERM,  error_quitloop);
  
  /* Set up the log handler */
  setup_log_handler ();
  
            /* ------------ Initialization is done ---------- */
  
            /* -------------- Options parsing ------------- */
  
  tmp = g_strconcat (_("[ARGUMENTS] - "), SHORTDESCRIPTION, NULL);
  context = g_option_context_new (tmp);
  g_free (tmp);
  
  g_option_context_set_summary (context, SUMMARY);
  
  tmp = g_strconcat (DESCRIPTION, 
                     _("\nReport any bug to: https://bugs.launchpad.net/utimer\
 (bugs@utimer.codealpha.net)"),
                     NULL);
  g_option_context_set_description (context, tmp);
  g_free (tmp);
  
  g_option_context_add_main_entries (context, entries, NULL);
  
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_printerr (_("Error while parsing options: %s.\n"),error->message);
    g_printerr (_("Run '%s --help' to see a full list of available command\
 line options.\n"), argv[0]);
    g_error_free (error);
    exit (EXIT_FAILURE);
  }
  
  g_option_context_free (context);

  
            /* ------------ Options parsing DONE ------------- */
  
  
  /* Verify that there is at least an option */
  if (!( timer_info
      || show_version
      || show_limits
      || countdown_info
      || stopwatch))
  {
    g_printerr (_("No option has been specified!\n"));
    g_printerr (_("Run '%s --help' to see a full list of available command\
 line options.\n"), argv[0]);
    exit (EXIT_FAILURE);
  }
  
  
  /* Check if there is any conflicting options */
  
  if (ut_config.debug)
    ut_config.quiet = FALSE;
  
  if (timer_info && (countdown_info || stopwatch)
    || countdown_info && stopwatch)
  {
    g_warning (_("Conflicting options!\nThe following options cannot\n\
 be used simultaneously:\n -t (timer mode), -c (countdown mode), -s\
 (stopwatch mode)."));
    exit (EXIT_FAILURE);
  }
  
  
  g_debug ("G_MAXUINT = %u", G_MAXUINT);
  
  if (isatty (STDOUT_FILENO))
    g_debug ("\033[34mYou are using a TTY.\033[m");
  else
    g_debug ("You are not using a TTY.");
  
  if (ut_config.debug)
  {
    g_debug ("Deactivating buffered output for stderr and stdout...");
    
    setbuf (stdout, NULL);
    setbuf (stderr, NULL);
  }
  
  /* Check for arguments */
  /* NO NEED FOR NOW */
  //~ if (remaining_args != NULL)
  //~ {
    //~ g_debug ("Checking arguments...");
    //~ i = 0;
    //~ while (tmp = remaining_args[i])
    //~ {
      //~ g_message ("argument: %s", tmp);
      //~ i++;
    //~ }
  //~ }
  //~ else
  //~ {
    //~ g_message (_("No argument found."));
  //~ }
  
  /* Now that we treated every arg, we can free remaining_args. */
  //~ g_strfreev(remaining_args);
  
  /* Showing limits */
  
  if (show_limits)
  {
    g_print (_("This is a list of the possible limits of µTimer for your\
 machine.\n\n"));
    
    tmp = timer_get_maximum_time ();
    g_print (_("* Limits for the Timer/Countdown:\n"));
    g_print (_("\t- The maximum possible timer length is: %s.\n"), tmp);
    g_print (_("\t  If you enter a value that is exceeding it, it will be\
 replaced by the value above.\n"));
    
    g_free (tmp);
    tmp = NULL;
    
    exit (EXIT_SUCCESS);
  }
  
  /* Showing version */
  
  if (show_version)
  {
    g_print ("%s\n\n%s\n\n%s\n", PACKAGE_STRING, COPYRIGHT, COPYRIGHT_TXT);
    exit (EXIT_SUCCESS);
  }
  
        /* ------------------ PROCESSING STARTS ------------------ */
  
  /* Prepare for starting the main loop */
  
  loop = g_main_loop_new (NULL, FALSE);
  
  g_idle_add ((GSourceFunc) start_thread_exit_check, ttimer);
  
  /* -------------- TIMER&COUNTDOWN MODE -------------- */
  if (timer_info || countdown_info || stopwatch)
  {
    
    if (countdown_info)
    {
      g_debug ("Countdown Mode");
      ttimer = countdown_new_timer (0, 0, success_quitloop, error_quitloop, ut_config.timer);
      parse_time_pattern (countdown_info, &(ttimer->seconds), &(ttimer->mseconds));
    }
    else if (timer_info)
    {
      g_debug ("Timer Mode");
      ttimer = timer_new_timer (0, 0, success_quitloop, error_quitloop, ut_config.timer);
      parse_time_pattern (timer_info, &(ttimer->seconds), &(ttimer->mseconds));
    }
    else
    {
      g_debug ("Stopwatch Mode");
      ttimer = stopwatch_new_timer (success_quitloop, error_quitloop, ut_config.timer);
    }
    
    tmp = timer_get_maximum_time ();
    g_debug ("Maximum Timer length is %s.", tmp);
    g_free (tmp);
    tmp = NULL;
    
    if (!ut_config.quiet && ut_config.verbose && !stopwatch)
    {
      tmp = timer_ut_timer_to_string (ttimer);
      g_info (_("Timer will exit after reaching: %s"), tmp);
      g_free (tmp);
      tmp = NULL;
    }
    
    ttimer->timer_print_source_id = g_timeout_add (TIMER_PRINT_RATE_MSEC,
                                                   (GSourceFunc) timer_print,
                                                   ttimer);
    if (ttimer->mode != TIMER_MODE_STOPWATCH)
      g_idle_add ((GSourceFunc) timer_run_checkloop_thread, ttimer);
  } /* -------------- END TIMER&COUNTDOWN MODE -------------- */
  else
  { /* No mode selected! We quit the loop ASAP. */
    g_idle_add ((GSourceFunc) error_quitloop, NULL);
  }
  
  
  /* ------------- Starting the main loop ---------------- */
  
  g_debug ("Starting main loop...");
  g_main_loop_run (loop);
  g_main_loop_unref (loop);
  loop = NULL;
  g_debug ("Exiting main loop...");
  
  /* ------------- Main loop Exited ---------------- */
  
  g_debug ("Quitting with error code: %i", ut_config.current_exit_status_code);
  
        /* ================== MAIN DONE ==================== */
  return ut_config.current_exit_status_code;
}
