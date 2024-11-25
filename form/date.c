#include  <stdio.h>
#include  <ctype.h>
#include  "version.h"
#include  "../h/defs.h"
#include  "../h/errs.h"
#include  "h/form.h"

/* leap_year(y) == y mod 400 == 0 or 
                   y mod 100 != 0 and y mod 4 == 0
*/
extern long time();
#define UNIX_TO_BASE (366+365+365)
#define BASE_YEAR 1973
#define F1461  1461
#define F1095  1095
#define F365   365
#define F10227 ((F365*4 + 1)*7)

			/* first entry not used */
private const Short M0 [] = {0,0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
private const Short M1 [] = {0,0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

private const Char W [] = { 52,52,52,53,52,52,52,
		            52,53,52,52,52,52,52,
		            53,52,52,52,52,53,52,
		            52,52,52,52,53,52,52};

private const Short offset [] = {24, 13, 2, 21, 10, 28, 18, 7, 26, 15, 
                                  4, 23, 12, 1, 20, 9, 27, 17, 6};
typedef struct 
{ Short y, m, d;
} Dmy_t, *Dmy;


typedef struct 
{ Char   oldstate;
  Char   ch;
  Char   newstate;
} Fse;

const Fse fsa [] = 
              {{0, 'J', 1}, {0, 'F', 2}, {0, 'M', 3}, {0, 'A', 4}, {0, 'S', 5},
              {0, 'O', 6}, {0, 'N', 7}, {0, 'D', 8},
	      {1, 'A', 9}, {1, 'U',10},
	      {9, 'N',-1}, {10,'N',-6}, {10,'L',-7}, 
	      {2, 'E',11}, {11,'B',-2}, 
	      {3, 'A',12}, {12,'R',-3}, {12,'Y',-5},
	      {4, 'P',13}, {4, 'U',14}, {13,'R',-4}, {14, 'G', -8},
	      {5, 'E',15}, {15,'P',-9},
	      {6, 'C',16}, {16,'T',-10},
	      {7, 'O',17}, {17,'V',-11},
	      {8, 'E',18}, {18,'C',-12}, {-1,'0',0}};

static const Char MONTH[] [4] = 
 { "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

public Date day_no(y, m, d)
	Short y, m, d;
{ if ((unsigned)m > 12)
    return 0;
  if ((unsigned)d > 31) 
    return 0;

  return ((y - BASE_YEAR) >> 2) * F1461 + d + 
                    ((y & 3) == 0 ? F1095 + M1[m] : ((y-1) & 3) * F365 + M0[m]);
}


const Char weekdays[][6] = { "Sun", "Mon", "Tue", "Wed", "Thurs", "Fri", "Sat" };


public const Char * date_to_weekday(date)
	Date    date;
{ return &weekdays[date % 7][0];
}	



private void to_date(dn, dmy_ref)
	Short    dn;
	Dmy_t *  dmy_ref;
{ if ((dn % F1461) == 0)
  { dmy_ref->y = BASE_YEAR + (dn / F1461) * 4 - 1;
    dmy_ref->m = 12;
    dmy_ref->d = 31;
  }
  else 
  { Int dd = dn % F1461; 
    dmy_ref->y = BASE_YEAR + (dn / F1461) * 4 + (dd - 1) / F365;
    
  { Int dd_ = ((dd - 1) % F365) + 1;
    Int m = 12;
    
    if ((dmy_ref->y & 3) == 0)
    { while (dd_ <= M1[m]) 
        --m;
      dmy_ref->d = dd_ - M1[m];
    }
    else 
    { while (dd_ <= M0[m]) 
        --m;
      dmy_ref->d = dd_ - M0[m];
    }
    dmy_ref->m = m;
  }}
}  
    
#if 0

public Short to_week(dn) 
        Date dn;
{ Short dd = dn % F10227;
  Short yr;
  Short wk;
 
  if (dd == 0)
  { yr = 0;
    wk = 52;
  }
  else 
  { yr = 1;
    wk = (dd - 1)/7 + 1;
    while (wk > W[yr])
    { wk -= W[yr];
      yr += 1;
    }
  }
/*yr += BASE_YEAR + dn/F10227)*28 - 1; */
  return wk;
}


  
public Date easter(y)
	Short y;
{ Short g = 1 + y % 19;
  return ((offset[g] +  day_no(y,3,21))/7 + 1)*7;
}

/* shrove tuesday = easter - 40
   ascension day  = easter + 39
   whit monday    = easter + 50
*/

#endif


public Short this_month(d)
        Short d;
{ Dmy_t  dmy;  to_date(d, &dmy);
  return dmy.y * 12 + dmy.m;
}

public Short next_month(d)
        Short d;
{ Dmy_t  dmy;  to_date(d, &dmy);
  dmy.d = 0;
  dmy.m += 1;
  if (dmy.m == 13)
  { dmy.y += 1;
    dmy.m = 1;
  }
  return day_no(dmy.y, dmy.m, dmy.d);
}

public Date asc_to_date(str)
	Char * str;
{ fast Char * s = &str[0];
       Short day = 0;
       Short mon;

  while (isspace(*s))
    ++s;
    
  if (*s == 0)
    return time(0) / (60*60*24) - UNIX_TO_BASE + 1;

  for (; isdigit(*s); ++s)
    day = day * 10 + *s - '0';
    
  if (not in_range(day, 1, 31))
    return -1;

  while (isspace(*s))
    ++s;
    
  if (*s == '/')
  { mon = atol(&s[1]);
    for (++s; *s != 0 and *s != '/'; ++s)
      ;
    if (*s == '/')
      ++s;
  }
  else
  { const Fse * p = &fsa[0]-1;
    Short cstate;
    for (cstate = 0; cstate >= 0 ; cstate = p->newstate)
    {  
      while ((++p)->oldstate != cstate)
				if (p->oldstate == -1)
				  return -2;
      while (p->oldstate == cstate and p->ch != toupper(*s))
				++p;
      if (p->oldstate != cstate)
				return -3;
      
      ++s;
    }
    mon = -cstate;
  }

{ Int yr = atol(s);
  if (in_range(yr, 70, 99))
    yr += 1900;
  else if (yr < 1000)
    yr += 2000;
  if (not in_range(yr, BASE_YEAR, 2054))
    return -4;
   
{ Date dn = day_no((Short)yr, mon, day);
  Dmy_t dmy; to_date(dn, &dmy);
  
/*  printf("YR %d, MTH %d, DAY %d\n", dmy.y, dmy.m, dmy.d);*/
  if (dmy.y != yr or dmy.m != mon or dmy.d != day)
    return -5;
  return dn;
}}}

static const Char * dfmt[] = { "%2d %s %02d", "%2d%02ld%4d" , "%02d%02ld%4d", 
			       "%2d/%02ld/%02d", };

public Char * date_to_asc(str, dn, fmtno)
	Char * str;
	Date dn;
	Int  fmtno;
{ *str = 0;
  if (dn >= 0)
  { union { const char * str_; Int int_; } arg2;
    Dmy_t dm;  to_date(dn, &dm);
    while (dm.y >= 100 and fmtno < 1)
      dm.y -= 100;
      
    arg2.str_ = MONTH[dm.m-1];
    if (fmtno >= 1)
      arg2.int_ = dm.m;
    if (fmtno >= 3)
    { dm.y -= 2000;
      while (dm.y < 0)
        dm.y += 100;
    }

    sprintf(str, dfmt[fmtno], dm.d, arg2, (Short)dm.y);
  }
  return str;
}



public Int date_to_day(dn)
	Date dn;
{ return dn % 7;
}



#if STANDALONE

#include <time.h>

main(argc, argv)
   char * argv[];
{ Char buf1[30], buf2[30];
  Char * str = argv[1];
  if (str != null)
  { printf("Str is %s\n", str);
  { Date dn = asc_to_date(str);
    printf("Day is %d\n", dn);
    
  { struct tm tm;
    Int day, year, month;
    Char smon[20];
    sscanf(str, "%d %s %d", &day, &smon, &year);
    smon[0] = tolower(smon[0]);
    month = 
             strcmp("jan", smon) == 0 ? 1 :
             strcmp("feb", smon) == 0 ? 2 :
             strcmp("mar", smon) == 0 ? 3 :
             strcmp("apr", smon) == 0 ? 4 :
             strcmp("may", smon) == 0 ? 5 :
             strcmp("jun", smon) == 0 ? 6 :
             strcmp("jul", smon) == 0 ? 7 :
             strcmp("aug", smon) == 0 ? 8 :
             strcmp("sep", smon) == 0 ? 9 :
             strcmp("oct", smon) == 0 ? 10 :
             strcmp("nov", smon) == 0 ? 11 :
             strcmp("dec", smon) == 0 ? 12 : -1;
    if (year < 50) year += 100;
    memset(&tm, 0, sizeof(tm));
    tm.tm_mday = day;
    tm.tm_mon = month - 1;
    tm.tm_year = year;
    printf("U %d %d %d\n", tm.tm_mday, tm.tm_mon, tm.tm_year);
  { time_t secs = mktime(&tm);
    Int ud = secs / (60L*60L*24);
    printf("Uday is %d %d\n", ud, ud - 1095);

    printf("Reformatted: %s,\t%s\n", date_to_asc(&buf1[0], dn, 0),
                                     date_to_asc(&buf2[0], dn, 1)
          );
  }}}}
}

#endif
