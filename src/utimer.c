/*
 *  utimer.c
 *
 *  Copyright 2008 Arnaud Soyez <weboide@codealpha.net>
 *
 *  This file is part of utimer.
 *  (utimer is a CLI program that features a timer, countdown, and a stopwatch)
 *
 *  utimer is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  utimer is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with utimer.  If not, see <http://www.gnu.org/licenses/>.
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
#include "timer.h"


void set_tty_canonical(int state)
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

void quitloop ()
{
  g_print("\n");
  g_debug (_("Stopping Main Loop..."));
  g_main_loop_quit (loop);
  g_debug (_("Main Loop stopped."));
}

int check_exit_from_user ()
{
  gint c;
  do
  {
    c = fgetc(stdin);
    g_print("\b ");
  } while(c != 'q' || c == 27); // checks for 'q' key or Escape
  
  // If the user asks for exiting, we stop the loop.
  quitloop();
}

int main (int argc, char *argv[])
{
  GError  *error = NULL;
  GOptionContext *context;
  gchar   *tmp = NULL;
  gint    i;

  /* Some init */
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
  
  g_print("G_MAXULONG = %lu\n", G_MAXULONG);
  g_print("G_MAXLONG  = %ld\n", G_MAXLONG);
  g_print("G_MAXUINT  = %u\n", G_MAXUINT);
  
  // Set the function to call in case of receiving signals:
  // we need to stop the main loop to exit gracefully
  signal(SIGALRM,  quitloop);
  signal(SIGHUP,   quitloop);
  signal(SIGINT,   quitloop);
  signal(SIGPIPE,  quitloop);
  signal(SIGQUIT,  quitloop);
  signal(SIGTERM,  quitloop);
  
  if(isatty (STDOUT_FILENO))
    g_print("\033[34mTTY!!!\033[m\n\n");
  else
    g_warning("NOT A TTY!!!");
  
  locale = setlocale(LC_ALL, "");
  if (!locale)
  {
    g_error(_("Error during setting current locale. Falling back to default locale."));
    locale = setlocale(LC_ALL, "C");
    if (!locale)
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
  
  /******** PROCESSING STARTS *********/
  
  /* set up the verbose/debug log handler */
  //~ g_log_set_handler (NULL,
                     //~ G_LOG_LEVEL_INFO
                      //~ | G_LOG_LEVEL_MESSAGE
                      //~ | G_LOG_LEVEL_DEBUG
                      //~ | G_LOG_LEVEL_WARNING,
                     //~ log_handler,
                     //~ &untar_config);
  
  if(verbose)
  {
    g_debug("Deactivating buffered output for stderr and stdout...");
    
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
  }
  
  /* Check for arguments */
  if(remaining_args != NULL)
  {
    g_debug("Checking arguments...");
    i = 0;
    while(tmp = remaining_args[i])
    {
      g_message("argument: %s", tmp);
      i++;
    }
  }
  else
  {
    g_message(_("No argument found."));
  }
   
  /* Now that we treated every arg, we can free remaining_args. */
  g_strfreev(remaining_args);
  
  /*==== Prepare for starting the main loop ====*/
  
  loop = g_main_loop_new (NULL, FALSE);
  g_timeout_add_seconds(20, (GSourceFunc) quitloop, NULL);
  
  
  g_idle_add((GSourceFunc) start_thread_exit_check, NULL);
  
  if(isTimer)
  {
    ut_timer ttimer;
    
    g_debug("Timer Mode");
    g_message(_("Your computer supports sleeping for a maximum of ~%u years (%u days)."),
              get_maximum_sleep(TU_YEAR), get_maximum_sleep(TU_DAY));
    ttimer.callback   = quitloop;
    ttimer.start_time = &start_time;
    ttimer.seconds    = atol(isTimer); /** TODO: USE A PATTERN AND A REGEX **/
    ttimer.mseconds   = 0;
    
    g_debug("seconds= %ld (%s)", ttimer.seconds, isTimer);
    g_idle_add((GSourceFunc) start_thread_timer, &ttimer);
    g_timeout_add(TIMER_REFRESH_RATE,
                  (GSourceFunc) update_timer,
                  &ttimer);
  }
  
  g_debug (_("Starting main loop..."));
  g_main_loop_run (loop);
  g_debug (_("Quitted main loop..."));
  
  
  set_tty_canonical(0);
  
  /** TODO: Exit with error code when trap/signal (necessary for scripts) **/
  return EXIT_SUCCESS;
}

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

guint get_maximum_sleep(TimeUnit unit)
{
  
  if(unit == TU_SECOND)
    return G_MAXUINT;
  
  if(unit == TU_MINUTE)
    return G_MAXUINT/60;
  
  if(unit == TU_HOUR)
    return G_MAXUINT/3600;
  
  if(unit == TU_DAY)
    return G_MAXUINT/86400;
  
  if(unit == TU_MONTH)
    return G_MAXUINT/(30*86400);
    
  if(unit == TU_YEAR)
    return G_MAXUINT/(365*86400);
  
  return G_MAXUINT;
}
