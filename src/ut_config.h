/*
 *  ut_config.h
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

#ifndef UT_CONFIG_H
#define UT_CONFIG_H

#include <glib.h>


typedef struct
{
  gchar       *locale;
  gchar       *isTimer;
  gchar       *isCountdown;
  gboolean    verbose;
  gboolean    quiet;
  gboolean    show_version;
  gboolean    debug;
  gboolean    show_limits;
  gboolean    quit_with_success;
} utconfig;

utconfig ut_config;

#endif /* UT_CONFIG_H */
