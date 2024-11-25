#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "version.h"
#include "../h/errs.h"
#include "../h/defs.h"
#include "h/form.h"
#include "h/verify.h"

public const char scats[256] =
{	   0,0,0,0,0,0,0,0,
	   0,E_SPACE,E_SPACE,E_SPACE,E_SPACE,E_SPACE,0,0,
	   0,0,0,0,0,0,0,0,
  	   0,0,0,0,0,0,0,0,
/* 0x20 */ E_TEL+E_SPACE+E_SNAME,0,0,0,0,0,0,E_SNAME,
	   0,0,0,0,0, E_TEL+E_SNAME, E_SNAME, E_FNAME+E_SNAME,
/* 0x30 */ 0x22, 0x22,0x22,0x22,0x22,0x22,0x22,0x22,
           0x22, 0x22,  0,  0,  0,  0,  0,  0,         0,
  0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,
  0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,
           0, 0, 0, 0, E_FNAME,        0,
  0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,
  0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,
           0, 0, 0, 0, 0
};

private Char * vfy_barcode(str)          /* result advanced => good */
	const Char * str;
{ fast const Char * s = str;
  fast Int ct = 0;
       Int lct = 0;

  while (*s != 0 && isdigit(*s))
  { ++s;
    ++ct;
  }
  if (*s == '@')
  { ++s;
    lct = 0x101;
  }
  while (*s != 0 && isalpha(*s))
  { ++s;
    ++lct;
  }
  if (toupper(s[-1]) == 'Z' && (lct == 0x102 || (lct & 0xff) == 5))
    lct = 4;
  if (lct == 0x101)
    lct = 4;

  return (Char *)(
            (lct == 0 && ct > 0 || (lct & 0xff) == 4)
       &&  ct < 50 && (*s <= ' ' || *s == '/' || *s == '%')
       		? s : str);
}

const char trcode[] = "@abcdefghijklmnopqrstuvwxyz.............";

public Id asc_to_bcsup(str, sup_ref)
        Char *   str;                 /* no leading spaces */
        Id      *sup_ref;
{ fast Char * s = str;
  Int fac;
  Int res = 0;
  Int sup = 0;
	Int sl = strlen(s);

	if (sl <= 14)
	  s += sl;
	else
		s += 12;

  if (isdigit(*(s-1)))
  { 
    for (fac = 1; fac < 100000 && --s >= str; fac *= 10)
      res += fac * (*s -'0');
  }
  else
  { if (toupper(*(s-1)) == 'Z')
    { res += 1;
      --s;
    }
    for (fac = 2; --s >= str && not isdigit(*s); fac *= 32)
      res += fac * (toupper(*s) -'@');

    res |= 0x40000000;
    ++s;
  }

  for (fac = 1; --s >= str; fac *= 10)
    sup += fac * (*s -'0');

  *sup_ref = sup;
  return res;
}

                                          /* bc_to_asc copied in cvtdb.c */

public Char * bc_to_asc(sup, key, bctext)
      Id      sup, key;
      Char   *bctext;
{ if (not (key & 0x40000000))
    if (sup == 0)
      sprintf(&bctext[0], "%5d", key);
    else
      sprintf(&bctext[0], "%d%05d", sup, key);
  else
  { sprintf(&bctext[0], sup == 0 ? "" : "%d", sup);
  { Char * bcp = &bctext[strlen(bctext)+4];      /* Short Format: c5c5c5c5z1 */
    bcp[1] = 0;
    bcp[0] = (key & 1) ? 'z' : 0;
    key = key >> 1;
    while (--bcp >= &bctext[0] && key > 0x400)
    { *bcp = trcode[key & 0x1f];
      key = key >> 5;
    }
  }}
  return &bctext[0];
}

#define BCPFX 2

public Char * get_bc(str, sup_ref, line_ref)   /* back => error, on => good */
	 Char *   str;
	 Id       *sup_ref;
	 Id       *line_ref;
{ fast Char * s = str;

  while (*s == ' ')
    ++s;

{ Bool gun = *s == BCPFX;
  if (gun)
    ++s;

  while (*s == '0')
    ++s;

  if (*s == 0)
    return str;

{ Char * res = vfy_barcode(s);
  if (res == s)
    return &str[-1];
  if (gun && res - s < 13)
    return &str[-1];
  if (gun || (res-s) > 12)              /* the checksum */
    res[-1] = 0;
{ Char sch = res[0];
  res[0] = 0;
  *line_ref = asc_to_bcsup(s, sup_ref);
  res[0] = sch;
  while (*res == ' ')
    ++res;
  return res;
}}}}

public Bool vfy_ointeger(str)
	const Char * str;
{ while (*str != 0 && isspace(*str))
    ++str;

{ fast const Char * s = str;
  if (*s == '-')
    ++s;

  while (*s != 0 && isdigit(*s))
    ++s;

  return not isalpha(*s);
}}



public Bool vfy_integer(str)
	const Char * str;
{ while (*str != 0 && isspace(*str))
    ++str;

  if (*str == '-')
    ++str;

{ fast const Char * s = str;

  while (*s != 0 && isdigit(*s))
    ++s;

  return s != str && not isalpha(*s);
}}

public Bool vfy_nninteger(str)
	const Char * str;
{ while (*str != 0 && isspace(*str))
    ++str;

{ fast const Char * s = str;

  while (*s != 0 && isdigit(*s))
    ++s;

  return s != str && not isalpha(*s);
}}


public Bool vfy_cash(str)
	const Char * str;
{ fast const Char * s = str;

  while (*s != 0 && isspace(*s))
    ++s;

  if (*s == '-')
    ++s;

{ const Char * ss = s;
  while (*s != 0 && isdigit(*s))
    ++s;

  return ss != s && *s == 0;
}}


public Bool vfy_real(str)
        const Char * str;
{ fast const Char * s = &str[-1];
  Int dotct = 0;

  while (*++s != 0)
    if      (*s == '.')
      if (s[1] == '.')
        return false;
      else
        ++dotct;
    else if (not in_range(*s, '0', '9'))
      return false;
  return dotct <= 1;
}


public Bool vfy_zrate(str)
	const Char * str;
{ return str[0] == 0 or
          (toupper(str[0]) == 'Z' || str[0] == ' ') && str[1] == 0;
}

public Bool vfy_postcode(str)
    const Char * str;
{ return true;
}



public Char * get_b_sname(str, tgt)   /* < str => error, == str => not found */
	 Char * str, *tgt;
{ fast Char * t = tgt;
  fast Char * s = str;
  *t = 0;

  while (*s == ' ')
    ++s;

  if (*s == 0)
    return str;
  if (vfy_str(s,scats,E_SNAME) <= s)
    return &str[-1];

  for (; *s != 0 && *s != '/'; ++s)   /* brown/jones for prt_custmr */
    *t++ = *s;
  while (*--t == ' ')
    ;
  *++t = 0;
  return s;
}

private Char * get_sqty(str, qty_ref)      /* advanced => good */
         Char *   str;
	 Int     *qty_ref;
{ fast Char * s = str;
       Bool neg = false;

  while (*s == ' ')
    ++s;

  if (*s == '-')
  { ++s;
    neg = true;
  }

  if (isdigit(*s))
  { Int qty = *s++ - '0';
    while (isdigit(*s))
      qty = qty * 10 + *s++ - '0';

    if (*s <= '0')
    { *qty_ref = not neg ? qty : -qty;
      return s;
    }
  }
  return str;
}



public Char * get_qty(str, qty_ref)      /* advanced => good */
         Char *   str;
	 Int     *qty_ref;
{ Int q = *qty_ref;
  Char * res = get_sqty(str, qty_ref);
  if (res == null && *qty_ref < 0)
  { *qty_ref = q;
    return str; 		/* Minus not allowed */
  }
  return res;
}


public Char * get_cash(str, cash_ref)      /* advanced => good */
         Char *   str;
	 Int     *cash_ref;
{ Char * s = get_sqty(str, cash_ref);
  return s;
}

public Char * rgt_to_asc(v_, str)
         Int    v_;
         Char * str;
{ fast Char * t = str;
       Char ch = 'A';
  fast unsigned long v = v_;
  
  if (v & 1)
  { sprintf(t, "(%d)", v >> 16);
    t += strlen(t);
    v &= 0xfffe;
  }

  for (; (v = v >> 1) != 0; )		/* portability */
  { if (v & 1)
      *t++ = ch;
    ++ch;
  }

  *t = 0;
  return str;
}


public Int asc_to_rgt(str, allow_int)
         const Char * str;
         Bool   allow_int;
{      Int res = 0;
       Int fixed = 0;
       Bool isfixed = false;
  fast const Char * s;

  for (s = str; *s != 0; ++s)
  { Char ch = toupper(*s);
    if      (in_range(ch, 'A', 'Z'))     /* ASCII */
      res |= (1 << (ch - '@'));
    else if (in_range(ch, '0', '9') && allow_int)
    { isfixed = true;
      fixed = fixed * 10 + ch - '0';
    }
    else if (ch == '(' || ch == ')')
      ;
    else
      return -1;
  }
  if (isfixed)
    res += (fixed << 16) + 1;
  return res;
}

public Char * vfy_str(s_, tbl, cat)
	Char *  s_;
	const Char tbl[];
	Quot    cat;
{ fast Char * s = s_;

  while (tbl[*s] & cat)
    ++s;

  return  s;
}



public Int asc_to_pw(s_)
  Char * s_;
{ fast Char * s = s_;
  Int r1 = 7;
  Int r2 = *s == 0 ? 0 : 5;

  for (; *s != 0; ++s)
  { r2 = r2 * *s + r1 * *s;
    r1 *= *s;
  }

  return r2;
}
