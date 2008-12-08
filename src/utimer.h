/*
 *  utimer.h
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

#ifndef UTIMER_H
#define UTIMER_H

#include "ut_config.h"

#define SHORTDESCRIPTION _("command-line \"timer\" which features a timer,\
 a countdown and a stopwatch")

#define SUMMARY _(" µTimer (or \"utimer\", pronounced as \"micro-timer\") is a\
 multifunction timer (command-line only).\n\n The main features are:\n\
\t- a timer (e.g. count from 0 to 4 minutes),\n\
\t- a countdown (e.g. count from 10 minutes to 0),\n\
\t- a stopwatch.\
\n\n µTimer always exits after\
 the timer or countdown are done unless specified otherwise. This can be useful\
 for using countdowns in scripts, for example. The stopwatch, which is not\
 concerned, needs to be stopped manually using 'ctrl+c' or 'q'.")

#define COPYRIGHT_TXT "License GPLv3+: GNU GPL version 3 or later\
 <http://gnu.org/licenses/gpl.html>\nThis is free software: you are free to\
 change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by\
 law."

#define COPYRIGHT "Copyright 2008 Arnaud Soyez <weboide@codealpha.net>"


#define DESCRIPTION ""

static    GMainLoop       *loop;

static    char            **remaining_args;
struct    termios         savedttystate;
static    int             exit_status_code;

static GOptionEntry entries[] = {
  {"verbose",
    'v',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.verbose),
    N_("Verbose output"),
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
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.debug),
    N_("Debug output (use only for debugging purposes or for bug reports)"),
    NULL
  },
  
  {"timer",
    't',
    0,
    G_OPTION_ARG_STRING,
    &(ut_config.isTimer),
    N_("Timer mode: the program will exit after a certain TIMELENGTH\
 (e.g. utimer -t 5d18h31m27s300ms)."),
    N_("TIMELENGTH")
  },
  
  {"limits",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.show_limits),
    N_("Show the limits of µTimer (Maximum time length, Year 2038 bug, etc...)"),
    NULL
  },
  
  {"quit-with-success",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &(ut_config.quit_with_success), 
    N_("When (and only when) the user hits the 'q' key to end the program,\
 the program will exit with a SUCCESS (0) exit status instead of a\
 non-zero/error (1) exit status (this might be useful for developers)."),
    NULL
  },
  
  {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &remaining_args,
    NULL, NULL },
  {NULL}
};

void      quitloop                    ();
int       start_thread_exit_check     ();
int       check_exit_from_user        ();
void      quitloop                    (int error_status);
void      error_quitloop              ();
void      success_quitloop            ();
void      set_tty_canonical           (int state);


#endif /* UTIMER_H */
