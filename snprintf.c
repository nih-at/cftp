/*
  $NiH: snprintf.c,v 1.3 2002/05/13 16:51:22 wiz Exp $

  This file provides replacements for the following library functions:
  	snprintf
	vsnprintf
	vasprintf
*/

/**************************************************************
 * Original:
 * Patrick Powell Tue Apr 11 09:48:21 PDT 1995
 * A bombproof version of doprnt (dopr) included.
 * Sigh.  This sort of thing is always nasty do deal with.  Note that
 * the version here does not include floating point...
 *
 * snprintf() is used instead of sprintf() as it does limit checks
 * for string length.  This covers a nasty loophole.
 *
 * The other functions are there to prevent NULL pointers from
 * causing nast effects.
 *
 * More Recently:
 *  Brandon Long <blong@fiction.net> 9/15/96 for mutt 0.43
 *  This was ugly.  It is still ugly.  I opted out of floating point
 *  numbers, but the formatter understands just about everything
 *  from the normal C string format, at least as far as I can tell from
 *  the Solaris 2.5 printf(3S) man page.
 *
 *  Brandon Long <blong@fiction.net> 10/22/97 for mutt 0.87.1
 *    Ok, added some minimal floating point support, which means this
 *    probably requires libm on most operating systems.  Don't yet
 *    support the exponent (e,E) and sigfig (g,G).  Also, fmtint()
 *    was pretty badly broken, it just wasn't being exercised in ways
 *    which showed it, so that's been fixed.  Also, formated the code
 *    to mutt conventions, and removed dead code left over from the
 *    original.  Also, there is now a builtin-test, just compile with:
 *           gcc -DTEST_SNPRINTF -o snprintf snprintf.c -lm
 *    and run snprintf for results.
 * 
 *  Thomas Roessler <roessler@guug.de> 01/27/98 for mutt 0.89i
 *    The PGP code was using unsigned hexadecimal formats. 
 *    Unfortunately, unsigned formats simply didn't work.
 *
 *  Michael Elkins <me@cs.hmc.edu> 03/05/98 for mutt 0.90.8
 *    The original code assumed that both snprintf() and vsnprintf() were
 *    missing.  Some systems only have snprintf() but not vsnprintf(), so
 *    the code is now broken down under HAVE_SNPRINTF and HAVE_VSNPRINTF.
 *
 *  Dieter Baron <dillo@giga.or.at> 04/16/02 for cg 0.4
 *    Add vasprintf front end, and the ability to resize the buffer on
 *    demand.  Make the test program compilable on systems that provide
 *    snprintf, and add string format tests.
 **************************************************************/

#include "config.h"

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF) || !defined(HAVE_VASPRINTF) || defined(TEST_SNPRINTF)

#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* varargs declarations: */

#if defined(HAVE_STDARG_H)
# include <stdarg.h>
# define HAVE_STDARGS    /* let's hope that works everywhere (mj) */
# define VA_LOCAL_DECL   va_list ap
# define VA_START(f)     va_start(ap, f)
# define VA_SHIFT(v,t)  ;   /* no-op for ANSI */
# define VA_END          va_end(ap)
#else
# include <varargs.h>
# undef HAVE_STDARGS
# define VA_LOCAL_DECL   va_list ap
# define VA_START(f)     va_start(ap)      /* f is ignored! */
# define VA_SHIFT(v,t) v = va_arg(ap,t)
# define VA_END        va_end(ap)
#endif

/*int snprintf (char *str, size_t count, const char *fmt, ...);*/
/*int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);*/
/*int vasprintf (char **strp, const char *fmt, va_list arg);*/

struct buf {
    char *buffer;	/* the actual buffer */
    size_t maxlen;	/* size of the buffer */
    size_t currlen;	/* length of used portion */
    int resize;		/* whether to resize buffer on overrun */
};

static int buf_grow(struct buf *buf);
static void dopr (struct buf *buf, const char *format, va_list args);
static void dopr_outch (struct buf *buf, char c);
static void fmtfp (struct buf *buf,
		   long double fvalue, int min, int max, int flags);
static void fmtint (struct buf *buf,
		    long value, int base, int min, int max, int flags);
static void fmtstr (struct buf *buf,
		    char *value, int flags, int min, int max);

/*
 * dopr(): poor man's version of doprintf
 */

/* format read states */
#define DP_S_DEFAULT 0
#define DP_S_FLAGS   1
#define DP_S_MIN     2
#define DP_S_DOT     3
#define DP_S_MAX     4
#define DP_S_MOD     5
#define DP_S_CONV    6
#define DP_S_DONE    7

/* format flags - Bits */
#define DP_F_MINUS 	(1 << 0)
#define DP_F_PLUS  	(1 << 1)
#define DP_F_SPACE 	(1 << 2)
#define DP_F_NUM   	(1 << 3)
#define DP_F_ZERO  	(1 << 4)
#define DP_F_UP    	(1 << 5)
#define DP_F_UNSIGNED 	(1 << 6)

/* Conversion Flags */
#define DP_C_SHORT   1
#define DP_C_LONG    2
#define DP_C_LDOUBLE 3

#define char_to_int(p) (p - '0')
#define MAXX(p,q) (((p) >= (q)) ? (p) : (q))

static void
dopr(struct buf *buf, const char *format, va_list args)
{
    char ch;
    long value;
  long double fvalue;
  char *strvalue;
  int min;
  int max;
  int state;
  int flags;
  int cflags;
    
  state = DP_S_DEFAULT;
  flags = cflags = min = 0;
  buf->currlen = 0;
  max = -1;
  ch = *format++;

  while (state != DP_S_DONE)
  {
    if ((ch == '\0') || (buf->currlen >= buf->maxlen && !buf->resize)) 
    state = DP_S_DONE;

    switch(state) 
    {
    case DP_S_DEFAULT:
      if (ch == '%') 
	state = DP_S_FLAGS;
      else 
	dopr_outch (buf, ch);
      ch = *format++;
      break;
    case DP_S_FLAGS:
      switch (ch) 
      {
      case '-':
	flags |= DP_F_MINUS;
        ch = *format++;
	break;
      case '+':
	flags |= DP_F_PLUS;
        ch = *format++;
	break;
      case ' ':
	flags |= DP_F_SPACE;
        ch = *format++;
	break;
      case '#':
	flags |= DP_F_NUM;
        ch = *format++;
	break;
      case '0':
	flags |= DP_F_ZERO;
        ch = *format++;
	break;
      default:
	state = DP_S_MIN;
	break;
      }
      break;
    case DP_S_MIN:
      if (isdigit((unsigned char)ch)) 
      {
	min = 10*min + char_to_int (ch);
	ch = *format++;
      } 
      else if (ch == '*') 
      {
	min = va_arg (args, int);
	ch = *format++;
	state = DP_S_DOT;
      } 
      else 
	state = DP_S_DOT;
      break;
    case DP_S_DOT:
      if (ch == '.') 
      {
	state = DP_S_MAX;
	ch = *format++;
      } 
      else 
	state = DP_S_MOD;
      break;
    case DP_S_MAX:
      if (isdigit((unsigned char)ch)) 
      {
	if (max < 0)
	  max = 0;
	max = 10*max + char_to_int (ch);
	ch = *format++;
      } 
      else if (ch == '*') 
      {
	max = va_arg (args, int);
	ch = *format++;
	state = DP_S_MOD;
      } 
      else 
	state = DP_S_MOD;
      break;
    case DP_S_MOD:
      /* Currently, we don't support Long Long, bummer */
      switch (ch) 
      {
      case 'h':
	cflags = DP_C_SHORT;
	ch = *format++;
	break;
      case 'l':
	cflags = DP_C_LONG;
	ch = *format++;
	break;
      case 'L':
	cflags = DP_C_LDOUBLE;
	ch = *format++;
	break;
      default:
	break;
      }
      state = DP_S_CONV;
      break;
    case DP_S_CONV:
      switch (ch) 
      {
      case 'd':
      case 'i':
	if (cflags == DP_C_SHORT) 
	  value = va_arg (args, short int);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, long int);
	else
	  value = va_arg (args, int);
	fmtint (buf, value, 10, min, max, flags);
	break;
      case 'o':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
	  value = va_arg (args, unsigned short int);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else
	  value = va_arg (args, unsigned int);
	fmtint (buf, value, 8, min, max, flags);
	break;
      case 'u':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
	  value = va_arg (args, unsigned short int);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else
	  value = va_arg (args, unsigned int);
	fmtint (buf, value, 10, min, max, flags);
	break;
      case 'X':
	flags |= DP_F_UP;
      case 'x':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
	  value = va_arg (args, unsigned short int);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else
	  value = va_arg (args, unsigned int);
	fmtint (buf, value, 16, min, max, flags);
	break;
      case 'f':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, long double);
	else
	  fvalue = va_arg (args, double);
	/* um, floating point? */
	fmtfp (buf, fvalue, min, max, flags);
	break;
      case 'E':
	flags |= DP_F_UP;
      case 'e':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, long double);
	else
	  fvalue = va_arg (args, double);
	break;
      case 'G':
	flags |= DP_F_UP;
      case 'g':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, long double);
	else
	  fvalue = va_arg (args, double);
	break;
      case 'c':
	dopr_outch (buf, va_arg (args, int));
	break;
      case 's':
	strvalue = va_arg (args, char *);
	fmtstr (buf, strvalue, flags, min, max);
	break;
      case 'p':
	strvalue = va_arg (args, void *);
	fmtint (buf, (long) strvalue, 16, min, max, flags);
	break;
      case 'n':
	if (cflags == DP_C_SHORT) 
	{
	  short int *num;
	  num = va_arg (args, short int *);
	  *num = buf->currlen;
        } 
	else if (cflags == DP_C_LONG) 
	{
	  long int *num;
	  num = va_arg (args, long int *);
	  *num = buf->currlen;
        } 
	else 
	{
	  int *num;
	  num = va_arg (args, int *);
	  *num = buf->currlen;
        }
	break;
      case '%':
	dopr_outch (buf, ch);
	break;
      case 'w':
	/* not supported yet, treat as next char */
	ch = *format++;
	break;
      default:
	/* Unknown, skip */
	break;
      }
      ch = *format++;
      state = DP_S_DEFAULT;
      flags = cflags = min = 0;
      max = -1;
      break;
    case DP_S_DONE:
      break;
    default:
      /* hmm? */
      break; /* some picky compilers need this */
    }
  }
  if (buf->currlen >= buf->maxlen) {
      if (buf->resize) {
	  if (buf_grow(buf) < 0)
	      return;
      }
      else
	  buf->buffer[buf->maxlen-1] = '\0';
  }
  else 
    buf->buffer[buf->currlen] = '\0';
}



static void
fmtstr(struct buf *buf, char *value, int flags, int min, int max)
{
  int padlen;     /* amount to pad */
  int cnt;
  
  if (value == 0)
  {
    value = "(null)";
  }

  cnt = 0;
  padlen = min - strlen(value);
  if (padlen < 0) 
    padlen = 0;
  if (flags & DP_F_MINUS) 
    padlen = -padlen; /* Left Justify */

  while ((padlen > 0) && (max < 0 ||  cnt < max)) 
  {
    dopr_outch (buf, ' ');
    --padlen;
    ++cnt;
  }
  while (*value && (max < 0 || cnt < max)) 
  {
    dopr_outch (buf, *value++);
    ++cnt;
  }
  while ((padlen < 0) && (max < 0 || cnt < max)) 
  {
    dopr_outch (buf, ' ');
    ++padlen;
    ++cnt;
  }
}



/* Have to handle DP_F_NUM (ie 0x and 0 alternates) */

static void
fmtint(struct buf *buf, long value, int base, int min, int max, int flags)
{
  int signvalue = 0;
  unsigned long uvalue;
  char convert[20];
  int place = 0;
  int spadlen = 0; /* amount to space pad */
  int zpadlen = 0; /* amount to zero pad */
  int caps = 0;
  
  if (max < 0)
    max = 0;

  uvalue = value;

  if(!(flags & DP_F_UNSIGNED))
  {
    if( value < 0 ) {
      signvalue = '-';
      uvalue = -value;
    }
    else
      if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
	signvalue = '+';
    else
      if (flags & DP_F_SPACE)
	signvalue = ' ';
  }
  
  if (flags & DP_F_UP) caps = 1; /* Should characters be upper case? */

  do {
    convert[place++] =
      (caps? "0123456789ABCDEF":"0123456789abcdef")
      [uvalue % (unsigned)base  ];
    uvalue = (uvalue / (unsigned)base );
  } while(uvalue && (place < 20));
  if (place == 20) place--;
  convert[place] = 0;

  zpadlen = max - place;
  spadlen = min - MAXX(max, place) - (signvalue ? 1 : 0);
  if (zpadlen < 0) zpadlen = 0;
  if (spadlen < 0) spadlen = 0;
  if (flags & DP_F_ZERO)
  {
    zpadlen = MAXX(zpadlen, spadlen);
    spadlen = 0;
  }
  if (flags & DP_F_MINUS) 
    spadlen = -spadlen; /* Left Justifty */

#ifdef DEBUG_SNPRINTF
  dprint (1, (debugfile, "zpad: %d, spad: %d, min: %d, max: %d, place: %d\n",
      zpadlen, spadlen, min, max, place));
#endif

  /* Spaces */
  while (spadlen > 0) 
  {
    dopr_outch (buf, ' ');
    --spadlen;
  }

  /* Sign */
  if (signvalue) 
    dopr_outch (buf, signvalue);

  /* Zeros */
  if (zpadlen > 0) 
  {
    while (zpadlen > 0)
    {
      dopr_outch (buf, '0');
      --zpadlen;
    }
  }

  /* Digits */
  while (place > 0) 
    dopr_outch (buf, convert[--place]);
  
  /* Left Justified spaces */
  while (spadlen < 0) {
    dopr_outch (buf, ' ');
    ++spadlen;
  }
}



static long double
abs_val(long double value)
{
  long double result = value;

  if (value < 0)
    result = -value;

  return result;
}



static long double
pow10(int exp)
{
  long double result = 1;

  while (exp)
  {
    result *= 10;
    exp--;
  }
  
  return result;
}



static long
round(long double value)
{
  long intpart;

  intpart = value;
  value = value - intpart;
  if (value >= 0.5)
    intpart++;

  return intpart;
}



static void
fmtfp(struct buf *buf, long double fvalue, int min, int max, int flags)
{
  int signvalue = 0;
  long double ufvalue;
  char iconvert[20];
  char fconvert[20];
  int iplace = 0;
  int fplace = 0;
  int padlen = 0; /* amount to pad */
  int zpadlen = 0; 
  int caps = 0;
  long intpart;
  long fracpart;
  
  /* 
   * AIX manpage says the default is 0, but Solaris says the default
   * is 6, and sprintf on AIX defaults to 6
   */
  if (max < 0)
    max = 6;

  ufvalue = abs_val (fvalue);

  if (fvalue < 0)
    signvalue = '-';
  else
    if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
      signvalue = '+';
    else
      if (flags & DP_F_SPACE)
	signvalue = ' ';

#if 0
  if (flags & DP_F_UP) caps = 1; /* Should characters be upper case? */
#endif

  intpart = ufvalue;

  /* 
   * Sorry, we only support 9 digits past the decimal because of our 
   * conversion method
   */
  if (max > 9)
    max = 9;

  /* We "cheat" by converting the fractional part to integer by
   * multiplying by a factor of 10
   */
  fracpart = round ((pow10 (max)) * (ufvalue - intpart));

  if (fracpart >= pow10 (max))
  {
    intpart++;
    fracpart -= pow10 (max);
  }

#ifdef DEBUG_SNPRINTF
  dprint (1, (debugfile, "fmtfp: %f =? %d.%d\n", fvalue, intpart, fracpart));
#endif

  /* Convert integer part */
  do {
    iconvert[iplace++] =
      (caps? "0123456789ABCDEF":"0123456789abcdef")[intpart % 10];
    intpart = (intpart / 10);
  } while(intpart && (iplace < 20));
  if (iplace == 20) iplace--;
  iconvert[iplace] = 0;

  /* Convert fractional part */
  do {
    fconvert[fplace++] =
      (caps? "0123456789ABCDEF":"0123456789abcdef")[fracpart % 10];
    fracpart = (fracpart / 10);
  } while(fracpart && (fplace < 20));
  if (fplace == 20) fplace--;
  fconvert[fplace] = 0;

  /* -1 for decimal point, another -1 if we are printing a sign */
  padlen = min - iplace - max - 1 - ((signvalue) ? 1 : 0); 
  zpadlen = max - fplace;
  if (zpadlen < 0)
    zpadlen = 0;
  if (padlen < 0) 
    padlen = 0;
  if (flags & DP_F_MINUS) 
    padlen = -padlen; /* Left Justifty */

  if ((flags & DP_F_ZERO) && (padlen > 0)) 
  {
    if (signvalue) 
    {
      dopr_outch (buf, signvalue);
      --padlen;
      signvalue = 0;
    }
    while (padlen > 0)
    {
      dopr_outch (buf, '0');
      --padlen;
    }
  }
  while (padlen > 0)
  {
    dopr_outch (buf, ' ');
    --padlen;
  }
  if (signvalue) 
    dopr_outch (buf, signvalue);

  while (iplace > 0) 
    dopr_outch (buf, iconvert[--iplace]);

  /*
   * Decimal point.  This should probably use locale to find the correct
   * char to print out.
   */
  dopr_outch (buf, '.');

  while (fplace > 0) 
    dopr_outch (buf, fconvert[--fplace]);

  while (zpadlen > 0)
  {
    dopr_outch (buf, '0');
    --zpadlen;
  }

  while (padlen < 0) 
  {
    dopr_outch (buf, ' ');
    ++padlen;
  }
}



static void
dopr_outch (struct buf *buf, char c)
{
    if (buf->currlen >= buf->maxlen) {
	if (buf->resize) {
	    if (buf_grow(buf) < 0)
		return;
	}
	else
	    return;
    }
    buf->buffer[buf->currlen++] = c;
}



static int
buf_grow(struct buf *buf)
{
    if (buf->buffer == NULL) {
	buf->buffer = malloc(64);
	buf->maxlen = 64;
    }
    else {
	buf->maxlen *= 2;
	buf->buffer = realloc(buf->buffer, buf->maxlen);
    }
    if (buf->buffer == NULL) {
	buf->resize = 0;
	buf->maxlen = -1;
	return -1;
    }

    return 0;
}

#endif /* !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF) */



#ifndef HAVE_VASPRINTF
int
vasprintf (char **strp, const char *fmt, va_list args)
{
  struct buf buf;
  
  buf.buffer = NULL;
  buf.maxlen = 0;
  buf.resize = 1;
  dopr(&buf, fmt, args);
  *strp = buf.buffer;
  if (buf.buffer == NULL)
      return -1;
  
  return(strlen(buf.buffer));
}
#endif



#if !defined(HAVE_VSNPRINTF) || defined(TEST_SNPRINTF)

#if defined(TEST_SNPRINTF)
#define vsnprintf my_vsnprintf
#endif

int
vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
  struct buf buf;
  
  str[0] = 0;
  buf.buffer = str;
  buf.maxlen = count;
  buf.resize = 0;
  dopr(&buf, fmt, args);
  return(strlen(str));
}
#endif /* !HAVE_VSNPRINTF || TEST_SNPRINTF */



#if !defined(HAVE_SNPRINTF) || defined(TEST_SNPRINTF)

#if defined(TEST_SNPRINTF)
#define snprintf my_snprintf
#define vsnprintf my_vsnprintf
#endif

/* VARARGS3 */
#ifdef HAVE_STDARGS
int snprintf (char *str,size_t count,const char *fmt,...)
#else
int snprintf (va_alist) va_dcl
#endif
{
#ifndef HAVE_STDARGS
  char *str;
  size_t count;
  char *fmt;
#endif
  VA_LOCAL_DECL;
    
  VA_START (fmt);
  VA_SHIFT (str, char *);
  VA_SHIFT (count, size_t );
  VA_SHIFT (fmt, char *);
  (void) vsnprintf(str, count, fmt, ap);
  VA_END;
  return(strlen(str));
}

#if defined(TEST_SNPRINTF)
#undef snprintf
#undef vsnprintf
#endif

#endif /* !HAVE_SNPRINTF || TEST_SNPRINTF */



#ifdef TEST_SNPRINTF
#ifndef LONG_STRING
#define LONG_STRING 1024
#endif

int
main (void)
{
  char buf1[LONG_STRING];
  char buf2[LONG_STRING];
  char *fp_fmt[] = {
    "%-1.5f",
    "%1.5f",
    "%123.9f",
    "%10.5f",
    "% 10.5f",
    "%+22.9f",
    "%+4.9f",
    "%01.3f",
    "%4f",
    "%3.1f",
    "%3.2f",
    NULL
  };
  double fp_nums[] = { -1.5, 134.21, 91340.2, 341.1234, 0203.9, 0.96, 0.996, 
    0.9996, 1.996, 4.136, 0};
  char *int_fmt[] = {
    "%-1.5d",
    "%1.5d",
    "%123.9d",
    "%5.5d",
    "%10.5d",
    "% 10.5d",
    "%+22.33d",
    "%01.3d",
    "%4d",
    NULL
  };
  long int_nums[] = { -1, 134, 91340, 341, 0203, 0};
  char *str_fmt[] = {
      "%s",
      "%5s",
      "%.5s",
      "%5.5s",
      "%5.10s",
      "%-5.10s",
      "%10s",
      "%-10s",
      "%10.10s",
      "%-10.10s",
      "%10.5s",
      "%-10.5s",
      "foo: %s :bar",
      NULL
  };
  char *str = "0123456";
  
  int x, y;
  int fail = 0;
  int num = 0;

  printf ("Testing snprintf format codes against system sprintf...\n");

  for (x = 0; fp_fmt[x] != NULL ; x++)
    for (y = 0; fp_nums[y] != 0 ; y++)
    {
      my_snprintf (buf1, sizeof (buf1), fp_fmt[x], fp_nums[y]);
      sprintf (buf2, fp_fmt[x], fp_nums[y]);
      if (strcmp (buf1, buf2))
      {
	printf("snprintf doesn't match Format: %s\n\tsnprintf = %s\n\tsprintf  = %s\n", 
	    fp_fmt[x], buf1, buf2);
	fail++;
      }
      num++;
    }

  for (x = 0; int_fmt[x] != NULL ; x++)
    for (y = 0; int_nums[y] != 0 ; y++)
    {
      my_snprintf (buf1, sizeof (buf1), int_fmt[x], int_nums[y]);
      sprintf (buf2, int_fmt[x], int_nums[y]);
      if (strcmp (buf1, buf2))
      {
	printf("snprintf doesn't match Format: %s\n\tsnprintf = %s\n\tsprintf  = %s\n", 
	    int_fmt[x], buf1, buf2);
	fail++;
      }
      num++;
    }

  for (x = 0; int_fmt[x] != NULL ; x++)
  {
    my_snprintf (buf1, sizeof (buf1), str_fmt[x], str);
    sprintf (buf2, str_fmt[x], str);
    if (strcmp (buf1, buf2))
    {
      printf("snprintf doesn't match Format: %s\n\tsnprintf = %s\n\tsprintf  = %s\n", 
	     str_fmt[x], buf1, buf2);
      fail++;
    }
    num++;
  }
  
  printf ("%d tests failed out of %d.\n", fail, num);

  exit(0);
}
#endif /* SNPRINTF_TEST */

