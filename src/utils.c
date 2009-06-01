/*
 *  utils.c
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

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "utils.h"
#include "utimer.h"


gulong ul_add (gulong a, gulong b)
{
  if (a > G_MAXULONG - b)
  {
    g_debug ("Overflowing addition...");
    return G_MAXULONG;
  }
  else
    return a + b;
}

gulong ul_mul (gulong a, gulong b) // only positive multiplication
{
  g_assert (a >= 0 && b >= 0);
  
  if (a > G_MAXULONG / b)
  {
    g_debug ("Overflowing multiplication...");
    return G_MAXULONG;
  }
  else
    return a * b;
}


guint ui_add (guint a, guint b)
{
  if (a > G_MAXUINT - b)
  {
    g_debug ("Overflowing addition...");
    return G_MAXUINT;
  }
  else
    return a + b;
}

guint ui_mul (guint a, guint b) // only positive multiplication
{
  g_assert (a >= 0 && b >= 0);
  
  if (a > G_MAXUINT / b)
  {
    g_debug ("Overflowing multiplication...");
    return G_MAXUINT;
  }
  else
    return a * b;
}

gboolean apply_suffix (guint *value, gchar *suffix)
{
  gint factor = 1;
  
  g_return_val_if_fail (value, FALSE);
  
  if (suffix)
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
  
  g_debug ("%s: applying factor %d to %u", __FUNCTION__, factor, *value);
  (*value) = ui_mul (*value, factor);
  
  return TRUE;
}


/**
 * Starts the function check_exit_from_user in a thread.
 * Used to start the function check_exit_from_user that is used to check
 * if the user wants to end the program.
 */
gboolean start_thread_exit_check (ut_timer* timer)
{
  GError  *error = NULL;
  
  g_debug ("Starting thread exit check");
  if (!g_thread_create ((GThreadFunc) check_exit_from_user, NULL, FALSE, &error))
  {
    g_printerr (_("Thread creation failed: %s"), error->message);
    g_error_free (error);
    timer_stop_checkloop_thread (timer);
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
    g_debug ("Activating canonical mode.");
    tcgetattr (STDIN_FILENO, &ttystate);
    ttystate.c_lflag &= ~ICANON; // remove canonical mode
    ttystate.c_cc[VMIN] = 1; // minimum length to read before sending
    tcsetattr (STDIN_FILENO, TCSANOW, &ttystate); // apply the changes
  }
  else
  {
    g_debug ("Deactivating canonical mode.");
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
  g_assert (loop);
  g_message ("\n");
  g_debug ("%s: Stopping Main Loop (error code: %i)...", __FUNCTION__, error_status);
  
  // We set the exit status code
  ut_config.current_exit_status_code = error_status;
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
  quitloop (EXIT_FAILURE);
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
  g_atexit (reset_tty_canonical_mode); /* Deactivate canonical mode at exit */
  
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
          g_timer_continue (ut_config.timer);
          paused = FALSE;
        }
        else
        {
          g_timer_stop (ut_config.timer);
          paused = TRUE;
        }
      }
      
    }
  } while (c != 'q' && c != 'Q'); /* checks for 'q' key */
  
  /* If the user asks for exiting, we stop the loop. */
  quitloop ( (ut_config.quit_with_success ? EXIT_SUCCESS : EXIT_FAILURE) );
}

void init_config (Config *conf)
{
  conf->locale   = NULL;
  conf->verbose  = FALSE;
  conf->quiet    = FALSE;
  conf->debug    = FALSE;
  conf->quit_with_success        = FALSE;
  conf->current_exit_status_code = EXIT_SUCCESS;
  conf->timer    = g_timer_new ();
}

void free_config (Config *conf)
{
  if (conf->timer)
  {
    g_debug ("Freeing config timer...");
    g_timer_destroy (conf->timer);
    conf->timer = NULL;
  }
}
