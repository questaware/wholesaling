#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include  <stdarg.h>
#include  "version.h"
#include  "../../env/tcap.h"
#include  "../h/defs.h"
#include  "../h/errs.h"
#include  "h/form.h"
#include  "h/verify.h"


#define OVERFLOWCH '*'

#define WORKMID  60
#define WORKEND 183
static Char workstring[WORKEND+2];

#define intstr (&workstring[WORKMID])


Char * fmt_rstr(Char*,Short,Char*) forward;


public void init_fmtform(void)

{ fast Char * t;

  for (t = &workstring[WORKMID]; t >= &workstring[0]; --t)
    *t = ' ';
}



public void chop3(strarr, str, ch)
        char *   * strarr;
        char *     str;
        char       ch;
{ fast char * s = &str[-1];
  fast Char * * t =strarr;

  strarr[0] = str;

  while (*++s != 0)  /* and not break */
    if (*s == ch)
    { *s = 0;
      *++t = &s[1];
      if (t == &strarr[2])
        break;
    }
  while (++t <= &strarr[2])
    *t = s;
}        

public Char * fmt_dec(t_, len, v_)
        Char * t_;
        Short  len;
        Int    v_;
{ fast Char * t = t_;
       Char * stt = &t[1-len];
  fast Int scale = 10;
       Int pscale = 1;
  fast Int v = v_;
  if (v < 0)
  { if (v == MININT)
      return fmt_rstr(t_, len, "NaN");
    else     
    { v = -v;    
      t -= 1;
    }
  }

  if (v >= 1000000000)
  { pscale = 1000000000;
    t -= 9;
  }
  else 
    for ( ; scale <= v; scale *= 10)
    { pscale = scale;
      t -= 1;
    }

  scale = pscale;
  
  if (len <= 0)
  { stt = t_;
    t_ = &t_[t_-t];
    t = stt;
  }

  if (t < stt)
    for (t = stt; t <= t_; )
      *t++ = OVERFLOWCH;
  else 
  { while (stt < t)
      *stt++ = ' ';
      
    if (v != v_)
      *t++ = '-';
  
    while (t <= t_)
    { Char ch = '0';
  
      while ((v -= scale) >= 0)
        ch += 1;
    
      v = (v + scale) * 10;
      *t++ = ch;
    }
  }
  *t = 0;
  return t;
}

public Char * fmt_cash(t_, len, v_)
        Char * t_;
        Short  len;
        Int    v_;
{ fast Char * t = fmt_dec(len > 0 ? t_-1 : t_+1, len-1, v_);
  t[0]  = t[-1];
  if      (t[-2] == '-')
    memcpy(&t[-3], "-.0", 3);
  else
  { t[-1] = t[-2] == ' ' ? '0' : t[-2];
    t[-2] = '.';
  }
  *++t = 0;
  return t;
}
					/* format left justified */
					/* right end of str at t_ */
public Char * fmt_lstr(t_, len, str)
        Char * t_;
        Short  len;
        Char * str;
{ if (str == null)
    str = "";
{ fast Char * t;
  fast Char * s = str;
  
  if (len <= 0)
    len = strlen(s);

  for (t = &t_[1-len]; t <= t_ and *s != 0; ++t)
    *t = *s++;
    
  for (; t <= t_; ++t)
    *t = ' ';
  
  *t = 0;
  return t;
}}
					/* format right justified */
					/* len == 0 => left of str at t_ */
					/* else     => right of str at t_ */
public Char * fmt_rstr(t_, len, str)
        Char * t_;
        Short  len;
        Char * str;
{ if (str == null)
    str = "";
{ fast Char * stt_ = &t_[1-len];
   /*  Char * stt = &stt_[len - strlen(str)]; */
       Int spce =         len - strlen(str);
 
  if      (len <= 0)
  { stt_ = t_;
    t_ += strlen(str) - 1;
  }
  else if (/*stt < stt_*/spce < 0)
    str = &str[/*stt_ - stt*/-spce];
  else 
    while (--spce >= 0/*stt_ < stt*/)
      *stt_++ = ' ';

  while (stt_ <= t_)
    *stt_++ = *str++;
  *stt_ = 0;
  return stt_;
}}

public char * fmt_str(Char * tgt,
                      const Char * fmt,
                      va_list args)
{ // va_list args;
  // va_start(args, fmt);

  fast Char * t = tgt;
  fast const Char * s = fmt;
       Char * lnstt = tgt;
  
  while (*s != 0)
  { if (     *s != '%')
    { if (*s == '\n')
        lnstt = &t[1];
      *t++ = *s++;
    }
    else if (*++s == '%')
      *t++ = *s++;
    else
    { Int len = 0;
      *t++ = ' ';                 /* the % */
      while (isdigit(*s))
      { *t++ = ' ';               /* each digit */
        len = len * 10 + *s++ - '0';
      }
      if (len == 0)		    /* for %10a its the a for %a its the % */
        t -= 1;
      *t = ' ';                   /* the letter */
      if      (*s == 'd' || *s == 'f')
      { char * t_ = t;
        t = fmt_dec(t, len, va_arg(args,int));
        if (*s == 'f')
          t = t_;
      }
      else if (*s == 'v')
        t = fmt_cash(t, len, va_arg(args,int));
      else if (*s == 'l')
        t = fmt_lstr(t, len, va_arg(args,char *));
      else if (*s == 'r')
        t = fmt_rstr(t, len, va_arg(args,char *));
      else 
        if (*s == '!')
        { while (t - lnstt < len)
            *t++ = ' ';
          t = lnstt + len;
        }
      s += 1;
    }      
  }
  *t = 0;
  return tgt;
}


public int fmt_outer(Char * tgt,
                     const Char * fmt,
                     ...)
{ va_list args;
  va_start(args, fmt);
  char * res = fmt_str(tgt,fmt,args);
  printf("%s\n", res);
  return 0;
}

void test_fmt(void)

{ char tgt[513];
  int cc = fmt_outer(tgt,"          %10r      %4d\n", "abcdef", 40);
}



public Char *  fmt_data(tgt, fmt, x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12)
        Char * tgt;
        Char * fmt;
        Int  x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12;
{      Int * param = &x1;
  fast Char * t = tgt;
  fast Char * s = fmt;
       Char * lnstt = tgt;
  
  while (*s != 0)
  { /*
    if      (*s == '/')
      if (*++s == 'n')
      { *t++ = '\n';
        ++s;
      }
      else 
      { *t++ = '/';
        *t++ = *s++;
      }
    else   */
         if (*s != '%')
    { if (*s == '\n')
        lnstt = &t[1];
      *t++ = *s++;
    }
    else if (*++s == '%')
      *t++ = *s++;
    else
    { Int len = 0;
      *t++ = ' ';                 /* the % */
      while (isdigit(*s))
      { *t++ = ' ';               /* each digit */
        len = len * 10 + *s++ - '0';
      }
      if (len == 0)		    /* for %10a its the a for %a its the % */
        t -= 1;
      *t = ' ';                   /* the letter */
      if      (*s == 'd')
        t = fmt_dec(t, len, *param);
      else if (*s == 'v')
        t = fmt_cash(t, len, *param);
      else if (*s == 'l')
        t = fmt_lstr(t, len, (Char*)*param);
      else if (*s == 'r')
        t = fmt_rstr(t, len, (Char*)*param);
      else if (*s == 'f')
        ;
      else 
      { param -= 1;
        if (*s == '!')
        { while (t - lnstt < len)
            *t++ = ' ';
          t = lnstt + len;
        }
      }
      s += 1;
      param += 1;
    }      
  }
  *t = 0;
  return t;
}

private void direct_integer (fld, n)
	Field   fld;
	Int     n;
{ Char * s = &workstring[WORKMID];
    
  fmt_dec(s, 0, n);
  direct_data(fld, &s[strlen(s) - fld->class->size]);
}


public void put_integer(n)
	Int     n;
{ Field f = &this_tbl->c[this_tbl->curr_fld];
  direct_integer(f, n);
}



public void put_ointeger (n)
	Int     n;
{ if (n == 0)
  { Field f = &this_tbl->c[this_tbl->curr_fld];
    direct_data(f, "");
  }
  else 
    put_integer(n);
}


public void put_cash (v)
	Cash    v;
{ Field fld = (Field)&this_tbl->c[this_tbl->curr_fld];
  fmt_cash(&workstring[WORKEND], fld->class->size, v);
  direct_data(fld, &workstring[WORKEND+1-fld->class->size]);
}


public void put_ocash (n)
	Cash    n;
{ if (n == 0)
    direct_data((Field)&this_tbl->c[this_tbl->curr_fld], "");
  else 
    put_cash(n);
}


public Char * to_client_ref(tgt, id)
	Char * tgt;
	Int    id;
{  Vint width = (id >> 20) & 0xf;
   Char ch = id >> 24;
   if (not in_range(ch, '0', 'z'))
     ch = ' ';
   tgt[0] = 0;
   if (id != 0)
     sprintf(&tgt[0], "%c%0*d", ch, width, id & 0xfffff);
   return tgt;
}

public Cc to_integer (ds, int_ref)
	Char *  ds;
	Int    *int_ref;
{ fast Char * s = ds;
                                  /* code from verify.c to disconnect modules */
  while (*s != 0 and isspace(*s))
    ++s;
    
  if (*s == '-')
    ++s;

{ Char * ss = s;
  Int v = *s - '0';

  if (in_range(v, 0, 9))
    for (s += 1; *s != 0 and isdigit(*s); ++s)
      v = v * 10 + *s - '0';
  
  if (ss == s or isalpha(*s))
    return NOT_FOUND;
  else 
  { *int_ref = v;
    return SUCCESS;
  }
}}

						/* max field size 60 */
public Cc get_integer (int_ref)
	Int    *int_ref;
{ get_data(&intstr[0]);
  return to_integer(intstr, int_ref);
}

						/* max field size 60 */
public Cc get_money (int_ref)
	Cash   *int_ref;
{ return get_integer(int_ref);
}


public Cc get_ointeger (int_ref)
	Int    *int_ref;
{ get_data(&intstr[0]);
  (void)to_integer(intstr, int_ref);
  return SUCCESS;
}


public Cc get_any_integer (int_ref, fld_ref)
	Int    *int_ref;
	Field  *fld_ref;
{ get_any_data(&intstr[0], fld_ref);
  return to_integer(intstr, int_ref);
}


public Cc get_ocref(int_ref, data)
        Int    * int_ref;
        Char *   data;
{ Char * p = skipspaces(data);
  Vint kind = (p[0] & 0xff);
  if (in_range(kind, '0', '9') or kind == 0)
  { kind = 0;
    --p;
  }
  ++p;
  if (not vfy_ointeger(p))
    return -1;
{ Vint width = 0;
  while (in_range(p[width], '0', '9'))
    ++width;
    
  *int_ref = ((Int)kind  << 24) + 
             ((Int)width << 20) + (atol(p) &  0xfffff);
  return OK;
}}



public Cash strtocash(Char * s_)

{ Cash pounds = 0;
  Cash pence = 0;
  Int sign = 1;
  Char * s = skipspaces(s_);
  if (s[0] == '-')
  { ++s;
    sign = -1;
  }
  
  for (; in_range(*s, '0', '9'); ++s)
    pounds = pounds * 10 + *s - '0';
  
  if (*s == '.')
    for (++s; in_range(*s, '0', '9'); ++s)
      pence = pence * 10 + *s - '0';
  
  return sign * (pounds * 100 + pence);
}
