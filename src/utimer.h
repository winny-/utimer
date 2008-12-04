/*
 *  utimer.h
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

#ifndef UTIMER_H
#define UTIMER_H

#define SHORTDESCRIPTION "command-line \"timer\" which features a timer, a countdown and a stopwatch"
#define SUMMARY " µTimer (or \"utimer\", pronounced as \"micro-timer\") is a multifunction timer (command-line only).\n\n The main features are:\n\t- a timer (e.g. count from 0 to 4 minutes),\n\t- a countdown (e.g. count from 10 minutes to 0),\n\t- a stopwatch.\n\n Default action: Without any option, the timer (the -t option) is used. A time length is required as argument in this case.\n\n µTimer always exits after the timer or countdown are done unless specified otherwise. This can be useful for using countdowns in scripts, for example. The stopwatch, which is not concerned, needs to be stopped manually using 'ctrl+c' or 'q'."
/* - a timing function: measures how long a program took to run and exit (similar to the linux time function). */


#define DESCRIPTION "(DESCRIPTION ?)"



static    GMainLoop       *loop;
          gchar           *locale,
                          *isTimer=NULL;
                          
          gboolean        verbose;
static    char            **remaining_args;
struct    termios         savedttystate;

static GOptionEntry entries[] = {
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, N_("Verbose output"), NULL},
  {"timer", 't', 0, G_OPTION_ARG_STRING, &isTimer, N_("Timer mode: the program will exit after a certain TIMELENGTH."), N_("TIMELENGTH")},
  
  {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &remaining_args,
    NULL, NULL },
  {NULL}
};

void      quitloop                    ();
int       start_thread_exit_check     ();
int       check_exit_from_user        ();
void      quitloop                    ();
void      set_tty_canonical           (int state);


#endif /* UTIMER_H */
