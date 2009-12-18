/*
 *  utimer.h
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

#ifndef UTIMER_H
#define UTIMER_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "utils.h"
#include "timer.h"
#include "log.h"

#define SHORTDESCRIPTION _("command-line \"timer\" which features a timer,\
 a countdown and a stopwatch")

#define SUMMARY _(" µTimer (or \"utimer\", pronounced as \"micro-timer\") is a\
 multifunction timer (command-line only).\n\n The main features are:\n\
\t- a timer (e.g. count from 0 to 4 minutes),\n\
\t- a countdown (e.g. count from 10 minutes down to 0),\n\
\t- a stopwatch.\
\n\n µTimer always exits after\
 the timer or countdown are done counting. This can be useful\
 for using countdowns in scripts, for example. The stopwatch, which is not\
 concerned, needs to be stopped manually using 'q'.")

#define COPYRIGHT_TXT "License GPLv3+: GNU GPL version 3 or later\
 <http://gnu.org/licenses/gpl.html>\nThis is free software: you are free to\
 change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by\
 law."

#define COPYRIGHT "Copyright 2008, 2009   Arnaud Soyez <weboide@codealpha.net>"


#define DESCRIPTION ""

#define TIMER_PRINT_RATE_MSEC  79
#define TIMER_CHECK_RATE_MSEC  500

GMainLoop         *loop;
gboolean          paused;
struct termios    savedttystate;
Config            ut_config;

#endif /* UTIMER_H */
