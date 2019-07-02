/*
 *  utils.h
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

#ifndef UTILS_H
  #define UTILS_H

typedef struct
{
  gchar *locale;
  gboolean verbose;
  gboolean quiet;
  gboolean debug;
  gboolean quit_with_success;
  gint current_exit_status_code;
  GTimer *timer;
  gushort terminal_cols;
} Config;

gulong ul_mul(gulong a, gulong b);
gulong ul_add(gulong a, gulong b);
guint ui_add(guint a, guint b);
guint ui_mul(guint a, guint b);
gboolean apply_suffix(guint *value, gchar *suffix);
void init_config(Config *conf);
void free_config(Config *conf);
gushort get_terminal_width();
gchar* get_progress_bar(gint8 perc, gushort width, gboolean go_right);



#endif /* UTILS_H */
