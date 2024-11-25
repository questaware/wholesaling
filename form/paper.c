#include  <stdio.h>
#include  <string.h>
#include  <stdarg.h>
#include  "version.h"
#include  "../h/defs.h"
#include  "h/form.h"
#include  "../h/errs.h"


extern Id this_pid;
extern FILE * new_file();


typedef struct Prt_s    /* from domains.h */
{ Char  type;
  short pagelen;
  Int   in_use;
  Char  path[20];       /* say */
} Prt_t, *Prt;


FILE * op_file = null;
static Char * g_tfile;
static Char   this_pdev[30];

#define NOT_STARTED (-10000000)

#define LINELEN  85
#define PAGELEN  66

static Short pagelen = PAGELEN;
static Short headlen = 0;
static Short taillen = 0;
static Int freelines = -1;

static Char head[15 * LINELEN+1];
static Char tail[15 * LINELEN+1];
static Char line[15 * LINELEN+1];

static Char  col_head[15 * LINELEN+1];
static Short col_headlen = 0;
static Char scolch;                   /* saving of col_head[0] */
static Char col_format[15 * LINELEN+1];


static const char lpcmd[] = "/usr/bin/lpr -h -P%s %s &";

private Short ct_nl(str)
         Char * str;
{ fast Char * s = &str[-1];
       Short ct = 0;
  while (*++s != 0)
    if (*s == '\n')
      ct += 1;
  return ct;
}



public void prt_set_title(str)
       Char * str;
{ head[0] = 0;
  if (str != null)
    strpcpy(&head[0], str, 15 * LINELEN);
  headlen = ct_nl(head);
}


public void prt_set_footer(str)
       Char * str;
{ tail[0] = 0;
  if (str != null)
    strpcpy(&tail[0], str, 15 * LINELEN);
{ Short ntaillen = ct_nl(tail);
  freelines -= ntaillen - taillen;
  taillen = ntaillen;
}}

public void prt_close_printer()

{ if (op_file != null)
    fclose(op_file);
  op_file = null;
}



extern Char ds1[133];

public Char * prt_open_printer(void * dev_)

{ Prt    dev = (Prt)dev_;
  Short len = strlen(dev->path);
  static Char tempname[30];
  Char * user = getenv("PWD");

  prt_close_printer();
  this_pdev[0] = 0;
  g_tfile = &tempname[0];

  if      (dev->path[0] != '/' || *strmatch("/home/peter",user) == 0)
  { strpcpy(&this_pdev[0], &dev->path[0], 30);
    sprintf(&tempname[0], "/tmp/pRt%d", this_pid);
    op_file = fopen(g_tfile, "w");
  }
  else if (dev->path[len - 1] != '/')
  { g_tfile = dev->path;
    op_file = fopen(g_tfile, "w"); 
  }
  else 
  { strcpy(&tempname[0], dev->path);
    tempname[len-1] = 0;
    op_file = new_file(tempname);
  }
  pagelen = dev->pagelen;
  if (op_file != null)
    return null;
  sprintf(&ds1[0], "Cannot open list file %s", g_tfile);
  return ds1;
}



public void prt_init_report()

{ head[0] = 0;
  headlen = 0;
  tail[0] = 0;
  taillen = 0;
  col_head[0] = 0;
  col_headlen = 0;
  freelines = NOT_STARTED;
}



public Char * prt_one_report(dev, header, footer)
        void * dev;
        Char * header, * footer;
{ prt_init_report();
  prt_set_title(header);
  prt_set_footer(footer);
  return prt_open_printer(dev);  
}

public void prt_set_cols(headstr, fmtstr)
     Char * headstr;
     Char * fmtstr;
{ strpcpy(&col_head[0], headstr, 15 * LINELEN);
  col_headlen = ct_nl(col_head);
  scolch = col_head[0];
  strpcpy(&col_format[0], fmtstr, 15 * LINELEN);
}


/*public void prt_on_cols() { col_head[0] = scolch; }*/

public void prt_off_cols() { scolch = col_head[0];  col_head[0] = 0; }



public void prt_head()

{ fprintf(op_file, head);
  freelines = pagelen - headlen - taillen;  
  if (col_head[0] != 0)
  { fprintf(op_file, col_head);
    freelines -= col_headlen;
  }
}



private void prt_tail()

{ if (taillen > 0)
    fprintf(op_file, tail);
  freelines -= taillen;
}



public int prt_newpage()

{ if (freelines < pagelen)
    for (; freelines > 0; freelines -= 1)
      fprintf(op_file, "\n");
  prt_tail();  
}


public void prt_need(n)
        Short  n;
{ if (freelines >= 0 and freelines < n)
    prt_newpage();
}        


public void prt_raw(str)
        Char * str;
{ freelines -= ct_nl(str);
  fprintf(op_file, str);
}

public void prt_text(str)
         Char *  str;
{ Short ct = ct_nl(str);
  prt_need(ct);
  if (freelines < ct)
  { if (freelines >= 0)
      prt_tail();
    prt_head();
  }
  freelines -= ct;
  fputs(str, op_file);
}



public void prt_fmt(const Char * fmt, ...)
//                  Int x1, int x2, int x3, int x4, int x5, int x6, 
//                  int x7, int x8, int x9, int x10, int x11, int x12)
{ va_list args;
  va_start(args, fmt);
  char * res = fmt_str(&line[0],fmt,args);
  prt_text(res);
}



public void prt_row(Int x0, ...)

{ va_list args;
  va_start(args, x0);
  char * res = fmt_str(&line[0],col_format,args);
  prt_text(res);
}



public void prt_fini(const char * email )

{ if (op_file != null) 
  { prt_newpage();
    prt_close_printer();
    if (g_tfile != null and this_pdev[0] != 0)
    { 
#if PRT_TO_FILE
      sprintf(&head[0], "cat %s > /tmp/prt%d", g_tfile, time(null) & 0xffff);
#else
      sprintf(&head[0], lpcmd, this_pdev, g_tfile);
#endif
/*    i_log(42, "PRF %s", head); */
      do_osys(head);
      g_tfile = null;
    }
  }
}


public void prt_fini_idle(const char * email)

{ prt_fini(email);
  idleform();
}


public Char *  prt_lpt_file(dev_, file)
    Char *  dev_;
    Char *  file;
{ Prt dev = (Prt)dev_;
  if (dev == null or dev->path[0] == 0)
    return "You have no listing device";

#if PRT_TO_FILE
  sprintf(&head[0], "cat %s > /tmp/prt%d", file, time(null) & 0xffff);
#else
  sprintf(&head[0], lpcmd, dev->path, file);
#endif
/*i_log(42, "PRLF %s", head);*/
  do_osys(head);
  return null;
}



public Short prt_freelines()

{ return freelines;
}
