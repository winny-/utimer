/*
 *  utimer.c
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>

#include <glib.h>
#include <glib/gi18n-lib.h>


#include "utils.h"
#include "utimer.h"
#include "timer.h"
#include "log.h"

/**
 * Function main()
 */
int main (int argc, char *argv[])
{
  GError          *error = NULL;
  GOptionContext  *context;
  gchar           *tmp = NULL;
  gint            i;
  ut_timer        ttimer;
  gchar           *options_help;

  /* Initialization */
  exit_status_code = EXIT_SUCCESS;
  g_thread_init(NULL);
  g_type_init();
  
  g_set_prgname (PACKAGE);
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
  
  set_tty_canonical(1);
  
  // Get the current time
  GTimeVal start_time;
  g_get_current_time(&start_time);
   
  /* Set the function to call in case of receiving signals:
   * we need to stop the main loop to exit gracefully, but
   * with error code anyway!
   */
  signal(SIGALRM,  error_quitloop);
  signal(SIGHUP,   error_quitloop);
  signal(SIGINT,   error_quitloop);
  signal(SIGPIPE,  error_quitloop);
  signal(SIGQUIT,  error_quitloop);
  signal(SIGTERM,  error_quitloop);
   
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
  
  
  /* Parse the options */
  tmp = g_strconcat (_("[ARGUMENTS] - "), SHORTDESCRIPTION, NULL);
  context = g_option_context_new (tmp);
  g_free (tmp);
  
  g_option_context_set_summary (context, SUMMARY);
  
  tmp = g_strconcat (DESCRIPTION, 
                     _("\nReport any bug to: https://bugs.launchpad.net/utimer or bugs@utimer.codealpha.net"),
                     //~ PACKAGE_BUGREPORT, 
                     //~ ".",
                     NULL);
  g_option_context_set_description (context, tmp);
  g_free (tmp);
  
  g_option_context_add_main_entries (context, entries, NULL);
  
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_error (_("Option parsing failed: %s"), error->message);
  }
  
  g_option_context_free(context);
  
  /* Verify that there is at least an option */
  if (!( ut_config.isTimer
      || ut_config.show_version
      || ut_config.show_limits))
  {
    g_printerr (_("No option specified!\n\nUse \"-t TIMELENGTH\" if you wish to\
 use the the timer.\nTo see the usage help, type: utimer --help\n\n"));
    exit(EXIT_FAILURE);
  }

  
  /* ***************** PROCESSING STARTS ***************** */
  
  /* set up the verbose/debug log handler */
  g_log_set_handler (NULL,
                     G_LOG_LEVEL_INFO
                      | G_LOG_LEVEL_MESSAGE
                      | G_LOG_LEVEL_DEBUG
                      | G_LOG_LEVEL_WARNING,
                     log_handler,
                     NULL);
  
  
  g_debug("G_MAXLONG  = %li", G_MAXLONG);
  g_debug("G_MAXUINT  = %u", G_MAXUINT);
  g_debug("sizeof(gulong) = %i", sizeof(gulong));
  
  if(isatty (STDOUT_FILENO))
    g_debug("\033[34mTTY!!!\033[m");
  else
    g_debug("NOT A TTY!!!");
  
  if(ut_config.verbose)
  {
    g_debug("Deactivating buffered output for stderr and stdout...");
    
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
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
  g_strfreev(remaining_args);
  
  /* Showing limits */
  
  if(ut_config.show_limits)
  {
    g_print(_("This is a list of the possible limits of µTimer for your machine.\n\n"));
    
    tmp = timer_get_maximum_time();
    g_print(_("* Timer's Limits:\n"));
    g_print(_("\t- The maximum possible sleep is: %s.\n"), tmp);
    g_print(_("\t  If you enter a value that is exceeding it, it will be\
 replaced by the value above.\n"));
    
    /* Just in case! */
    if(sizeof(gulong)==4)
    {
      g_print(_("\t- This program is not Year-2038-bug safe (on a 32 bits machine)!\n"));
    }
    
    g_free(tmp);
    tmp = NULL;
    
    exit(EXIT_SUCCESS);
  }
  
  /* Showing version */
  
  if(ut_config.show_version)
  {
    g_print("%s\n\n%s\n\n%s\n", PACKAGE_STRING, COPYRIGHT, COPYRIGHT_TXT);
    exit(EXIT_SUCCESS);
  }
  
  
  /*==== Prepare for starting the main loop ====*/
  
  loop = g_main_loop_new (NULL, FALSE);
  
  g_idle_add((GSourceFunc) start_thread_exit_check, NULL);
  
  /* If timer mode selected */
  if(ut_config.isTimer)
  {
    timer_init(&ttimer);
    if(update_timer_mutex == NULL)
      update_timer_mutex = g_mutex_new ();
    
    g_debug("Timer Mode");
    
    tmp = timer_get_maximum_time();
    g_debug(_("Do not use time length that exceeds %s."), tmp);
    g_free(tmp);
    tmp = NULL;
    
    ttimer.success_callback = success_quitloop;
    ttimer.error_callback   = error_quitloop;
    ttimer.start_time = &start_time;
    
    timer_parse_pattern(ut_config.isTimer, &ttimer);
    
    tmp = timer_ut_timer_to_string(ttimer);
    g_info("Timer will exit after reaching: %s", tmp);
    g_free(tmp);
    tmp = NULL;
    
    ttimer.update_timer_safe_source_id = g_timeout_add(TIMER_REFRESH_RATE,
                                        (GSourceFunc) timer_update_safe,
                                        &ttimer);
    g_idle_add((GSourceFunc) timer_start_thread, &ttimer);
    
  }
  
  /* Starting the main loop */
  
  g_debug (_("Starting main loop..."));
  g_main_loop_run (loop);
  g_debug (_("Quitted main loop..."));
  
  
  set_tty_canonical(0);
  g_debug("Quitting with error code: %i", exit_status_code);
  return exit_status_code;
}

/**
 * Starts the function check_exit_from_user in a thread.
 * Used to start the function check_exit_from_user that is used to check
 * if the user wants to end the program.
 */
int start_thread_exit_check ()
{
  GError  *error = NULL;
  
  g_debug("Starting thread exit check");
  if (!g_thread_create((GThreadFunc) check_exit_from_user, NULL, FALSE, &error))
  {
    g_error (_("Thread failed: %s"), error->message);
  }
  
  return FALSE; // to get removed from the main loop
}

/**
 * Activate/Deactivate the canonical mode from a TTY.
 */
void set_tty_canonical (int state)
{
  struct termios ttystate;

  tcgetattr(STDIN_FILENO, &savedttystate);
  tcgetattr(STDIN_FILENO, &ttystate);

  if (state==1)
  {  
    ttystate.c_lflag &= ~ICANON; // remove canonical mode
    ttystate.c_cc[VMIN] = 1; // minimum length to read before sending
  }
  else if (state==0)
  {
    tcsetattr(STDIN_FILENO, TCSANOW, &savedttystate); // put canonical mode back
  }
  
  tcsetattr(STDIN_FILENO, TCSANOW, &ttystate); // apply the changes
}

/**
 * Quits the main loop to end the program.
 * Quits the main loop to end the program with error_status
 * (EXIT_SUCCESS or EXIT_FAILURE).
 */
void quitloop (int error_status)
{
  g_print("\n");
  g_debug (_("Stopping Main Loop (error code: %i)..."), error_status);
  
  // We set the exit status code
  exit_status_code = error_status;
  g_main_loop_quit (loop);
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
  quitloop(EXIT_SUCCESS);
}

/**
 * Check to see if the user wants to quit.
 * This function waits till the user hits the 'q' key to quit the program,
 * then it will call the quitloop function. This is called by a thread
 * to avoid blocking the program.
 */
int check_exit_from_user ()
{
  gint c;
  do
  {
    c = fgetc(stdin);
    g_print("\b ");
  } while(c != 'q' || c == 27); // checks for 'q' key or Escape
  
  // If the user asks for exiting, we stop the loop.
  quitloop( (ut_config.quit_with_success ? EXIT_SUCCESS : EXIT_FAILURE) );
}
