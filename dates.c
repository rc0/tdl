/*
   $Header: /cvs/src/tdl/dates.c,v 1.5.2.1 2004/01/07 00:09:05 richard Exp $
  
   tdl - A console program for managing to-do lists
   Copyright (C) 2001-2004  Richard P. Curnow

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
   */

/* This file deals with parsing dates and relative time expressions. */

#include "tdl.h"
#include <string.h>
#include <ctype.h>

static time_t tm_to_time_t (struct tm *stm)/*{{{*/
{
  /* Convert from (struct tm) to time_t.  This has to be an inverse of
   * localtime().  It's not obvious that mktime() does this right, especially
   * with daylight saving time, on all systems (as I found out writing the RTC
   * support for chrony!).  This function will apply any compensation that is
   * needed. */ 

  struct tm temp1, temp2;
  long diff;
  time_t t1, t2;
  temp1 = *stm;
  temp1.tm_isdst = 0;
  t1 = mktime(&temp1);
  temp2 = *localtime(&t1);
  temp2.tm_isdst = 0;
  t2 = mktime(&temp2);
  diff = t2 - t1;
  return t1 - diff;
}
/*}}}*/
static int parse_time(char *d, int *hour, int *min, int *sec)/*{{{*/
{
  int n;
  *hour = *min = *sec = 0;
  switch (strlen(d)) {
    case 2:
      n = sscanf(d, "%2d", hour);
      if (n != 1) {
        fprintf(stderr, "Can't parse hour from %s\n", d);
        return -1;
      }
      break;
    case 4:
      n = sscanf(d, "%2d%2d", hour, min);
      if (n != 2) {
        fprintf(stderr, "Can't parse hour and minute from %s\n", d);
        return -1;
      }
      break;
    case 6:
      n = sscanf(d, "%2d%2d%2d", hour, min, sec);
      if (n != 3) {
        fprintf(stderr, "Can't parse hour, minute and second from %s\n", d);
        return -1;
      }
      break;
    default:
      fprintf(stderr, "Cannot parse time\n");
      return -1;
      break;
  }

  return 0;
}
/*}}}*/
time_t parse_date(char *d, time_t ref, int default_positive, int *error)/*{{{*/
{
  int len;
  char *hyphenpos;
  char *d0;
  time_t result;
  
  *error = 0; /* default : success */
  
  d0 = (*d == '-') ? d+1 : d;
  hyphenpos = strchr(d0, '-');

  if (hyphenpos) *hyphenpos = 0;
  
  len = strlen(d);

  if (!strcmp(d, ".")) {
    result = ref;
  
  } else if (isalpha(d[0]) ||
      (((d[0] == '+')|| (d[0] == '-')) && isalpha(d[1]))) {

    /* Look for dayname, +dayname, or -dayname */
    int dow = -1;
    int posneg, diff;
    char *dd = d;
    struct tm stm;

    if      (*dd == '+') posneg = 1, dd++;
    else if (*dd == '-') posneg = 0, dd++;
    else                 posneg = default_positive;

    /* This really needs to be internationalized properly.  Somebody who
     * understands how to make locales work properly needs to fix that. */
    
    if      (!strncasecmp(dd, "sun", 3)) dow = 0;
    else if (!strncasecmp(dd, "mon", 3)) dow = 1;
    else if (!strncasecmp(dd, "tue", 3)) dow = 2;
    else if (!strncasecmp(dd, "wed", 3)) dow = 3;
    else if (!strncasecmp(dd, "thu", 3)) dow = 4;
    else if (!strncasecmp(dd, "fri", 3)) dow = 5;
    else if (!strncasecmp(dd, "sat", 3)) dow = 6;

    if (dow < 0) {
      fprintf(stderr, "Cannot understand day of week\n");
    }

    stm = *localtime(&ref);
    diff = dow - stm.tm_wday;
    
    if (diff == 0) {
      /* If the day specified is the same day of the week as today, step a
       * whole week in the required direction. */
      if (posneg) result = ref + (7*86400);
      else        result = ref - (7*86400);
      
    } else if (diff > 0) {
      if (posneg) result = ref + (diff * 86400);
      else        result = ref - ((7-diff) * 86400);
    } else { /* diff < 0 */
      if (posneg) result = ref + ((7+diff) * 86400);
      else        result = ref + (diff * 86400);
    }

    stm = *localtime(&result);
    
    stm.tm_hour = 12;
    stm.tm_min = 0;
    stm.tm_sec = 0;
    result = tm_to_time_t(&stm);
    
  } else if ((len > 1) && isalpha(d[len-1])) {
    /* Relative time */
    long interval;
    int nc;
    int posneg;

    if      (*d == '+') posneg = 1, d++;
    else if (*d == '-') posneg = 0, d++;
    else                posneg = default_positive;

    if (sscanf(d, "%ld%n", &interval, &nc) != 1) {
      fprintf(stderr, "Cannot recognize interval '%s'\n", d);
      *error = -1;
      return (time_t) 0; /* arbitrary */
    }
    
    d += nc;
    switch (d[0]) {
      case 'h': interval *= 3600; break;
      case 'd': interval *= 86400; break;
      case 'w': interval *= 7 * 86400; break;
      case 'm': interval *= 30 * 86400; break;
      case 'y': interval *= 365 * 86400; break;
      case 's': break; /* use seconds */
      default:
        fprintf(stderr, "Can't understand interval multiplier '%s'\n", d);
        *error = -1;
        return (time_t) 0; /* arbitrary */
    }
    if (!posneg) interval = -interval;
  
    result = ref + interval;

  } else {
    int year, month, day, n;
    struct tm stm;

    stm = *localtime(&ref);

    /* Try to parse absolute date.
     * Formats allowed : dd, mmdd, yymmdd, yyyymmdd.
     * Time is assumed to be noon on the specified day */
    switch (len) {
      case 0:
        day = stm.tm_mday;
        month = stm.tm_mon + 1;
        year = stm.tm_year;
        break;
      case 2:
        n = sscanf(d, "%2d", &day);
        if (n != 1) {
          fprintf(stderr, "Can't parse day from %s\n", d);
          *error = -1;
          return (time_t) 0; /* arbitrary */
        }
        month = stm.tm_mon + 1;
        year = stm.tm_year;
        break;
      case 4:
        n = sscanf(d, "%2d%2d", &month, &day);
        if (n != 2) {
          fprintf(stderr, "Can't parse month and day from %s\n", d);
          *error = -1;
          return (time_t) 0; /* arbitrary */
        }
        year = stm.tm_year;
        break;
      case 6:
        n = sscanf(d, "%2d%2d%2d", &year, &month, &day);
        if (n != 3) {
          fprintf(stderr, "Can't parse year, month and day from %s\n", d);
          *error = -1;
          return (time_t) 0; /* arbitrary */
        }
        if (year < 70) year += 100;
        break;
      case 8:
        n = sscanf(d, "%4d%2d%2d", &year, &month, &day);
        if (n != 3) {
          fprintf(stderr, "Can't parse year, month and day from %s\n", d);
          *error = -1;
          return (time_t) 0; /* arbitrary */
        }
        year -= 1900;
        break;
      default:
       fprintf(stderr, "Can't parse date from %s\n", d);
        *error = -2;
        return (time_t) 0; /* arbitrary */
       break;
    }
    stm.tm_year = year;
    stm.tm_mon = month - 1;
    stm.tm_mday = day;
    stm.tm_hour = 12;
    stm.tm_min = 0;
    stm.tm_sec = 0;
    result = tm_to_time_t(&stm);
  }

  if (hyphenpos) {
    int hour, min, sec;
    struct tm stm;
    
    parse_time(hyphenpos+1, &hour, &min, &sec);
    stm = *localtime(&result);
    stm.tm_hour = hour;
    stm.tm_min = min;
    stm.tm_sec = sec;
    result = tm_to_time_t(&stm);
  }

  return result;
  
}/*}}}*/

/*{{{  Test code*/
#ifdef TEST

#include <locale.h>

int main (int argc, char **argv)
{
  time_t ref, newtime;
  char buffer[64];

  setlocale(LC_ALL, "");
  
  ref = time(NULL);

  if (argc < 2) {
    fprintf(stderr, "Require an argument\n");
    return 1;
  }

  newtime = parse_date(argv[1], ref, (argc > 2));

  strftime(buffer, sizeof(buffer), "%a %A %d %b %B %Y %H:%M:%S", localtime(&newtime));
  printf("%s\n", buffer);

  return 0;

}
#endif

/*}}}*/

