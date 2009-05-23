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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>

#include <glib.h>
#include <glib/gi18n-lib.h>



#include "utimer.h"

static    GMainLoop       *loop;

//~ static    char            **remaining_args;
struct    termios         savedttystate;
static    int             exit_status_code;
static    gboolean        paused;
GTimer          *start_timer;

static GOptionEntry entries[] = {
  
  {"timer",
    't',
    0,
    G_OPTION_ARG_STRING,
    &(ut_config.isTimer),
    N_("count from 0 to TIMELENGTH and then exit\
 (e.g. utimer -t 31m27s300ms)."),
    N_("TIMELENGTH")
  },
  
  {"countdown",
    'c',
    0,
    G_OPTION_ARG_STRING,
    &(ut_config.isCountdown),
    N_("count from TIMELENGTH down to 0 and exit (e.g.\
 utimer -c 30d9h50s)."),
    N_("TIMELENGTH")
  },
  
  {"verbose",
    'v',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.verbose),
    N_("Verbose output."),
    NULL
  },
  
  {"quiet",
    'q',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.quiet),
    N_("Quiet output."),
    NULL
  },
  
  {"quit-with-success",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.quit_with_success), 
    N_("When hitting 'q' to end the program, exit with a SUCCESS exit\
 status (0)."),
    NULL
  },

  {"limits",
    'L',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.show_limits),
    N_("Show the limits of µTimer (Maximum time length, Accuracy...)"),
    NULL
  },
  
  {"version",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.show_version),
    N_("Display the current version of µTimer."),
    NULL
  },
  
  {"debug",
    'd',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.debug),
    N_("Debug output."),
    NULL
  },

  //~ {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &remaining_args,
    //~ NULL, NULL },
  {NULL}
};


int main (int argc, char *argv[])
{        /* ================== MAIN STARTS ==================== */
  GError          *error = NULL;
  GOptionContext  *context;
  gchar           *tmp = NULL;
  gint            i;
  gchar           *options_help;
               /* -------------- Initialization ------------- */
  
  tcgetattr(STDIN_FILENO, &savedttystate); /* Save current tty state  */
  
  exit_status_code = EXIT_SUCCESS;
  g_thread_init (NULL);
  g_type_init ();
  
  /* We cleanup at ext */
  atexit (clean_up);
  
  // Initiate the timer
  start_timer =  g_timer_new ();
  
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
  if (!( ut_config.isTimer
      || ut_config.show_version
      || ut_config.show_limits
      || ut_config.isCountdown))
  {
    g_printerr (_("No option has been specified!\n"));
    g_printerr (_("Run '%s --help' to see a full list of available command\
 line options.\n"), argv[0]);
    exit(EXIT_FAILURE);
  }
  
  
  /* Check if there is any conflicting options */
  
  if (ut_config.debug)
    ut_config.quiet = FALSE;
  
  if (ut_config.isTimer && ut_config.isCountdown)
  {
    g_warning (_("The following options cannot be used simultaneously:\
 -t (timer mode), -c (countdown mode), -s (stopwatch mode). The Timer mode will\
  be used by default."));
    g_free (ut_config.isCountdown);
    ut_config.isCountdown = NULL;
  }
  
  /* set up the verbose/debug log handler */
  g_log_set_handler (NULL,
                     G_LOG_LEVEL_INFO
                      | G_LOG_LEVEL_MESSAGE
                      | G_LOG_LEVEL_DEBUG
                      | G_LOG_LEVEL_WARNING,
                     log_handler,
                     NULL);
  
  
  g_debug ("G_MAXUINT = %u", G_MAXUINT);
  
  if (isatty (STDOUT_FILENO))
    g_debug ("\033[34mYou are using a TTY.\033[m");
  else
    g_debug ("You are not using a TTY.");
  
  if (ut_config.verbose || ut_config.debug)
  {
    g_debug ("Deactivating buffered output for stderr and stdout...");
    
    setbuf (stdout, NULL);
    setbuf (stderr, NULL);
  }
  
  /* Check for arguments */
  /* NO NEED FOR NOW */
  //~ if(remaining_args != NULL)
  //~ {
    //~ g_debug("Checking arguments...");
    //~ i = 0;
    //~ while(tmp = remaining_args[i])
    //~ {
      //~ g_message("argument: %s", tmp);
      //~ i++;
    //~ }
  //~ }
  //~ else
  //~ {
    //~ g_message(_("No argument found."));
  //~ }
  
  /* Now that we treated every arg, we can free remaining_args. */
  //~ g_strfreev(remaining_args);
  
  /* Showing limits */
  
  if (ut_config.show_limits)
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
  
  if(ut_config.show_version)
  {
    g_print("%s\n\n%s\n\n%s\n", PACKAGE_STRING, COPYRIGHT, COPYRIGHT_TXT);
    exit(EXIT_SUCCESS);
  }
  
        /* ------------------ PROCESSING STARTS ------------------ */
  
  /* Prepare for starting the main loop */
  
  loop = g_main_loop_new (NULL, FALSE);
  
  g_idle_add((GSourceFunc) start_thread_exit_check, NULL);
  
  /* -------------- TIMER&COUNTDOWN MODE -------------- */
  if(ut_config.isTimer || ut_config.isCountdown)
  {
    
    if(ut_config.isCountdown)
    {
      g_debug ("Countdown Mode");
      ttimer = countdown_new_timer ();
    }
    else
    {
      g_debug ("Timer Mode");
      ttimer = timer_new_timer ();
    }
    
    tmp = timer_get_maximum_time ();
    g_debug ("Maximum Timer length is %s.", tmp);
    g_free (tmp);
    tmp = NULL;
    
    /* Bind some callback functions */
    ttimer->success_callback = success_quitloop;
    ttimer->error_callback   = error_quitloop;
    ttimer->start_timer      = start_timer; /* bind the GTimer too */
    
    /* Parse the user-given time length */
    if (ut_config.isCountdown)
      parse_time_pattern (ut_config.isCountdown, ttimer);
    else
      parse_time_pattern (ut_config.isTimer, ttimer);
    
    tmp = timer_ut_timer_to_string (ttimer);
    g_info (_("Timer will exit after reaching: %s"), tmp);
    g_free (tmp);
    tmp = NULL;
    
    ttimer->timer_print_source_id = g_timeout_add (TIMER_PRINT_RATE_MSEC,
                                        (GSourceFunc) timer_print,
                                        ttimer);
    g_idle_add ((GSourceFunc) timer_run_checkloop_thread, ttimer);
  } /* -------------- END TIMER&COUNTDOWN MODE -------------- */
  else
  { /* No mode selected! We quit the loop ASAP. */
    g_idle_add  ((GSourceFunc) error_quitloop, NULL);
  }
  
  
  /* ------------- Starting the main loop ---------------- */
  
  g_debug ("Starting main loop...");
  g_main_loop_run (loop);
  g_debug ("Exiting main loop...");
  
  /* ------------- Main loop Exited ---------------- */
  
  g_debug ("Quitting with error code: %i", exit_status_code);
  
        /* ================== MAIN DONE ==================== */
  return exit_status_code;
}

/**
 * Starts the function check_exit_from_user in a thread.
 * Used to start the function check_exit_from_user that is used to check
 * if the user wants to end the program.
 */
gboolean start_thread_exit_check ()
{
  GError  *error = NULL;
  
  g_debug ("Starting thread exit check");
  if (!g_thread_create ((GThreadFunc) check_exit_from_user, NULL, FALSE, &error))
  {
    g_printerr (_("Thread creation failed: %s"), error->message);
    g_error_free (error);
    timer_stop_checkloop_thread (ttimer);
  }
  
  return FALSE; // to get removed from the main loop
}

/**
 * Activate/Deactivate the canonical mode from a TTY.
 */
void set_tty_canonical (int state)
{
  struct termios ttystate;
  
  if (state==1)
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
    tcsetattr (STDIN_FILENO, TCSANOW, &savedttystate); // put canonical mode back
  }
}

/**
 * Deactivate the canoncial mode (used with atexit())
 */
void reset_tty_canonical_mode ()
{
  set_tty_canonical (0);
}

/**
 * Quits the main loop to end the program.
 * Quits the main loop to end the program with error_status
 * (EXIT_SUCCESS or EXIT_FAILURE).
 */
void quitloop (int error_status)
{
  g_message ("\n");
  g_debug ("Stopping Main Loop (error code: %i)...", error_status);
  
  // We set the exit status code
  exit_status_code = error_status;
  g_main_loop_quit (loop);
  g_main_loop_unref (loop);
  loop = NULL;
}

/**
 * Quits the main loop with error to end the program.
 * Quits the main loop to end the program ungracefully (EXIT_FAILURE).
 * This function just calls quitloop(EXIT_FAILURE).
 * This is needed for signal traps.
 */
void error_quitloop ()
{
  quitloop(EXIT_FAILURE);
}

/**
 * Quits the main loop with success code to end the program.
 * Quits the main loop to end the program gracefully (EXIT_SUCCESS).
 * This function just calls quitloop(EXIT_SUCCESS).
 * This is needed for callbacks.
 */
void success_quitloop ()
{
  quitloop (EXIT_SUCCESS);
}

/**
 * Check to see if the user wants to quit.
 * This function waits till the user hits the 'q' key to quit the program,
 * then it will call the quitloop function. This is called by a thread
 * to avoid blocking the program.
 */
int check_exit_from_user ()
{
  set_tty_canonical (1);             /* Apply canonical mode to TTY*/
  atexit (reset_tty_canonical_mode); /* Deactivate canonical mode at exit */
  
  gint c;
  do
  {
    c = fgetc (stdin);
    g_print ("\b ");
    switch (c)
    {
      case ' ':
      {
        if (paused)
        {
          g_timer_continue (start_timer);
          paused = FALSE;
        }
        else
        {
          g_timer_stop (start_timer);
          paused = TRUE;
        }
      }
      
    }
  } while (c != 'q'); /* checks for 'q' key */
  
  /* If the user asks for exiting, we stop the loop. */
  quitloop ( (ut_config.quit_with_success ? EXIT_SUCCESS : EXIT_FAILURE) );
}

void clean_up (void)
{
  if (ttimer)
  {
    g_free (ttimer);
    ttimer = NULL;
  }
  
  if (start_timer)
  {
    g_timer_destroy (start_timer);
    start_timer = NULL;
  }
  
  if (ut_config.locale)
  {
    g_free (ut_config.locale);
    ut_config.locale = NULL;
  }
}
