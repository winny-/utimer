/*
 *  log.c
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

#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "utimer.h"
#include "log.h"
#include "utils.h"

static void log_handler (const gchar *log_domain,
                        GLogLevelFlags log_level,
                        const gchar *message,
                        gpointer user_data)
{
  
  if ((log_level & G_LOG_LEVEL_MESSAGE))
  {
    if (!ut_config.quiet)
      g_print ("%s", message); /* There is no new line.
                               * (must be handled when calling g_message) */
    return;
  }
  
  if ((log_level & G_LOG_LEVEL_INFO))
  {
    if (!ut_config.quiet && ut_config.verbose)
      g_print ("%s\n", message);
    return;
  }
    
  if ((log_level & G_LOG_LEVEL_DEBUG))
  {
    if (ut_config.debug)
      g_print ("** DEBUG: %s\n", message);
    return;
  }
    
  if ((log_level & G_LOG_LEVEL_WARNING))
  {
    g_print (_("** WARNING: %s\n"), message);
    return;
  }
  
}


/* set up the verbose/debug log handler */
void setup_log_handler ()
{
  g_log_set_handler (NULL,
                     G_LOG_LEVEL_INFO
                      | G_LOG_LEVEL_MESSAGE
                      | G_LOG_LEVEL_DEBUG
                      | G_LOG_LEVEL_WARNING,
                     log_handler,
                     NULL);
}
