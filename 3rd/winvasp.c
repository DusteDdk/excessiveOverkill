#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "winvasp.h"

static int
int_vasprintf (result, format, args)
     char **result;
     const char *format;
     va_list *args;
{
  const char *p = format;
  /* Add one to make sure that it is never zero, which might cause malloc
     to return NULL.  */
  int total_width = strlen (format) + 1;
  va_list ap;

  memcpy ( &ap, args, sizeof (va_list));

  while (*p != '\0')
    {
      if (*p++ == '%')
	{
	  while (strchr ("-+ #0", *p))
	    ++p;
	  if (*p == '*')
	    {
	      ++p;
	      total_width += abs (va_arg (ap, int));
	    }
	  else
	    total_width += strtoul (p, (char **) &p, 10);
	  if (*p == '.')
	    {
	      ++p;
	      if (*p == '*')
		{
		  ++p;
		  total_width += abs (va_arg (ap, int));
		}
	      else
	      total_width += strtoul (p, (char **) &p, 10);
	    }
	  while (strchr ("hlL", *p))
	    ++p;
	  /* Should be big enough for any format specifier except %s and floats.  */
	  total_width += 30;
	  switch (*p)
	    {
	    case 'd':
	    case 'i':
	    case 'o':
	    case 'u':
	    case 'x':
	    case 'X':
	    case 'c':
	      (void) va_arg (ap, int);
	      break;
	    case 'f':
	    case 'e':
	    case 'E':
	    case 'g':
	    case 'G':
	      (void) va_arg (ap, double);
	      /* Since an ieee double can have an exponent of 307, we'll
		 make the buffer wide enough to cover the gross case. */
	      total_width += 307;
	      break;
	    case 's':
	      total_width += strlen (va_arg (ap, char *));
	      break;
	    case 'p':
	    case 'n':
	      (void) va_arg (ap, char *);
	      break;
	    }
	  p++;
	}
    }
#ifdef TEST
  global_total_width = total_width;
#endif
  *result = malloc (total_width);
  if (*result != NULL)
    return vsprintf (*result, format, *args);
  else
    return 0;
}

int
vasprintf (result, format, args)
     char **result;
     const char *format;
#if defined (_BSD_VA_LIST_) && defined (__FreeBSD__)
     _BSD_VA_LIST_ args;
#else
     va_list args;
#endif
{
  return int_vasprintf (result, format, &args);
}

