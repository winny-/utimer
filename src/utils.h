/*
 *  utils.h
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

#ifndef UTILS_H
#define UTILS_H

typedef enum
{
  TU_DAY    = 0,
  TU_MONTH  = 1,
  TU_YEAR   = 2,
  TU_HOUR   = 3,
  TU_MINUTE = 4,
  TU_SECOND = 5,
  TU_MILLISECOND = 6
} TimeUnit;

typedef enum
{
  STD_OK        = 0,
  STD_OVERFLOW  = 1,
  STD_UNDERFLOW = 2,
  STD_EMPTY     = 3,
  STD_UNKNOWN   = 4
} strtodouble_error;

strtodouble_error strtodouble (char *str, char *endptr, gdouble *result);

gulong        ul_mul                    (gulong a, gulong b);
gulong        ul_add                    (gulong a, gulong b);

#endif /* UTILS_H */
