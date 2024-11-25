#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include	"version.h"
#if XENIX
#include  <sys/ascii.h>
#endif
#include	"../../h/defs.h"
#include	"../page.h"
#include	"../recs.h"
#include	"../memdef.h"
#include	"../cache.h"
#include        "../b_tree.h"
#include        "../../h/prepargs.h"
#include        "cvt.h"
#define PRT_WDTH 512

static Int prt_col = 0;


#define DQUOTE 0x22
#define BSLASH 0x5c


extern Char dbg_file_name[];

static Int incident_ct = 0;
       Int ipline = 1;

extern Char * ctime();


public Cc ii_log(cc,f,a,b)
	Cc 	cc;
	Char *  f;
	Int     a, b;
{ fprintf(stderr,f,a,b);
  fprintf(stderr,"  Line %d\n", ipline);
  if (dbg_file_name[0] != 0)
  { long tm;
    FILE * dbg_file = fopen(dbg_file_name, "a");
    if (dbg_file != null)
    { setbuf(dbg_file, 0);
      time(&tm);
      fprintf(dbg_file, "%s line %d ", ctime(&tm), ipline);
      fprintf(dbg_file,f,a,b);
      fprintf(dbg_file,"\n");
      fclose(dbg_file);
    }
  }
  ++incident_ct;
  return cc;
}


public void alert(s)  
    const Char * s;
{ fprintf(stderr, "%s", s);
}

public void prt_int(d, size)
    Int  * d;
    Short  size;
{ prt_col += 7; /* say */
    
  if (prt_col >= PRT_WDTH)
  { fputc('\n', stdout);
    prt_col = 0;
  }
    
  printf("%ld ", size == sizeof(char)  ? *(char*)d  :
                 size == sizeof(short) ? *(short*)d : 
                 size == sizeof(long)   ? *d         : 0x80000000
        );
}



public void prt_n(n)
    Int  n;
{ prt_col += 7;
    
  if (prt_col >= PRT_WDTH)
  { fputc('\n', stdout);
    prt_col = 0;
  }
    
  printf("%ld ", n);
}



public void prt_str(s, lim)
    Char * s;
    Short  lim;   /* not yet used */
{ Int prt_col_ = prt_col;
  prt_col += strlen(s) + 2;
    
  if (prt_col_ != 0 and prt_col >= PRT_WDTH)
  { fputc('\n', stdout);
    prt_col = 0;
  }
  
{ Char ch;  
  fputc(DQUOTE, stdout);
  while ((ch = *s++) != 0)
  { if (ch == BSLASH or ch == '^' or ch == DQUOTE)
    { fputc(BSLASH,stdout);
      prt_col += 1;
    }
    fputc(ch,stdout);
  } 
  fputc(DQUOTE, stdout);
}}



public void prt_hex(s,size)
    Int * s;
    Short size;
{ Int prt_col_ = prt_col;
  prt_col += size * 2;
    
  if (prt_col_ != 0 and prt_col >= PRT_WDTH)
  { fputc('\n', stdout);
    prt_col = 0;
  }

  if (size <= 4)
  { if ((*s & 0xf0000000) == 0xf0000000)
      printf("-%lx", -*s);
    else
      printf("%lx", *s);
  }
  else  
  { while ((size -= 4) >= 0)
      printf("%08lx", *s++);
    size += 4;
  
    printf(size != 2 ? "" : "%02hx ", *((unsigned short*)s));
  }
}



public void prt_key(k, ix)
        Key    k;
        Short  ix;
{ switch (ddict[ix].kind_ix & (E_NU - 1))
  { when INTEGER:  prt_n(k->integer);
    when FXD_STR:  prt_str(k->string, ddict[ix].size_ix);
    when ENUM:     prt_str("E????",10);
    when LRA_KEY:  prt_str("L????",10);
    otherwise 
                   prt_str("ILL???", 10);
  }
}



public void prt_nl()

{ prt_col = 0;
  printf("\n");
}



public void prt_rptr()

{ prt_col = 2;
  printf("\n* ");
}



public void prt_endr()

{ prt_col = 2;
  printf("\n. ");
}

#if 0

public void prt_ix(upn,ix,pr)
        Sid   upn;
        Short ix;
        void  (*pr)();
{ Char * strm = ix_new_stream(upn, ix, null);
  Byte * p;

  while ((p = ix_next_rec(strm, null, null)) != null)
    pr(upn, p, stream_last_key(strm));
  
  ix_fini_stream(strm);
}

#endif


Bool cvt_sho_page = false;

public void prt_cl(upn, cl, pr)
        Sid    upn;
        Sid    cl;
        void   (*pr)();
{ if (upn < 0)
  { Id ix;
    upn = -upn;
    for (ix = MAX_IX; --ix > 0; )
      if (ddict[ix].kind_ix == INTEGER and  ddict[ix].cl_ix == cl)
        break;
    if (ix > 0)
    { Char * strm = ix_new_stream(upn, ix, null);
      Byte * p;
      while ((p = ix_next_rec(strm, null, null)) != null)
      	pr(upn, p, stream_last_key(strm));
      
      ix_fini_stream(strm);
      return;
    }}
{ Char * strm = init_brec_strm(upn, cl);
  Page_nr  page;
  Page_nr  prev = 0;
  Offs	offs;
  Char * p;
  Cc cc;

  while ((cc = next_brec(strm, &page, &offs, &p)) >= OK)
  { if (cvt_sho_page and prev != page)
      printf("P%d ", page);
    pr(upn, p, null);
    prev = page;
  }
  
  fini_brec_strm((void*)strm);
}}

public Int rd_char()

{ Int ch = fgetc(stdin);
  if (ch <= A_CR)
    ++ipline;
  return ch;
}




public Int rd_past()

{ fast Int ch;

  while (true)
  { ch = fgetc(stdin);
    if (ch <= A_CR)
      ++ipline;
    if (ch < 0 or ch == 0 or not isspace(ch) and ch != A_CR)
      return ch;
  }
}



public Int rd_int(size,d_ref)
    Short  size;
    Int  * d_ref;
{ fast Int i = 0x80000000;
  fast Int ch = rd_past();
       Short sgn = 1;
       
  if (ch == '-')
  { sgn = -1;
    ch = rd_past();
  }

  while (in_range(ch, '0', '9'))
  { i = i * 10 + ch - '0';
    ch = fgetc(stdin);
    if (ch <= A_CR)
      ++ipline;
  }
  
  i = i * sgn;

  if (ch <= 0)
  { ii_log(0, "Premature Eof", 0L, 0L);
    return 0;
  }

  if      (size == sizeof(char))
    *(char*)d_ref = i;
  else if (size == sizeof(short))
    *(short*)d_ref = i;
  else if (size == sizeof(long))
    *d_ref = i;
  return i;
}

public void rd_str(lim, s)
    Short  lim;
    Char * s;
{ Int ch = 0;  
  *s = 0;
  while ((ch = fgetc(stdin)) != DQUOTE)
  { if      (ch <= A_CR)
      ++ipline;
    if (ch <= 0)
    { ii_log(0, "Premature Eof", 0L, 0L);
      return;
    }
  }
      
  while (--lim >= 0)
  { ch = fgetc(stdin);
    if (ch <= A_CR)
      ++ipline;
    if (ch == DQUOTE)
      break;
    if      (ch == BSLASH)
      ch = fgetc(stdin);
    if (ch <= 0)
    { ii_log(0, "Premature Eof", 0L, 0L);
      break;
    }
    *s++ = ch;
  } 
  *s = 0;
  if (lim < 0 and ch != DQUOTE)
    ii_log(1, "String too long", 0L, 0L);
}


public void rd_name(lim, s)
    Short  lim;
    Char * s;
{ Int ch = 0;
  *s = 0;
      
  while (--lim >= 0)
  { ch = fgetc(stdin);
    if (ch <= A_CR)
      ++ipline;
    if (ch != '_' and not isalpha(ch))
      break;
    if (ch <= 0)
    { ii_log(0, "Premature Eof", 0L, 0L);
      break;
    }
    else 
      *s++ = ch;
  } 
  *s = 0;
  if (lim < 0 and ch != DQUOTE)
    ii_log(1, "String too long", 0L, 0L);
}


public void rd_hex(size,t)
    Short size;
    Int   * t;
{      Bool neg = false;
  fast Int  ch = rd_past();
  fast Short w = 0;

  fast Int i = 0x80000000;
  
  size *= 2;
  
  while (--size >= 0)
  { if (ch <= 0)
      break;
    else if (in_range(ch, '0', '9'))
      ch -= '0';
    else if (in_range(ch, 'A', 'F'))
      ch -= 'A' - 10;
    else if (in_range(ch, 'a', 'f'))
      ch -= 'a' - 10;
    else if (ch == '-')
    { neg = true;
      ch = 0;
      size += 1;
    }
    else
      break;
    i = i * 16 + ch;
    if ((++w & 0x7) == 0)
      *t++ = not neg ? i : -i;
    ch = fgetc(stdin);
    if (ch <= A_CR)
      ++ipline;
  }
  if ((++w & 0x7) != 0)
    *t++ = not neg ? i : -i;
  
  if (ch <= 0)
  { ii_log(0, "Premature Eof", 0L, 0L);
    return;
  }
}



public void prt_cpage(upn,page)
	Sid       upn;
	Page_nr   page;
{ 
  Short i,j;
  Byte * p = page_ref(upn,page);
 
  fprintf(stdout, "Upn %d, Page %d (%x)\n", upn, page, p[0]);

  for (i = 0; i < BLOCK_SIZE; i += 16)
  { fprintf(stdout, "%4x: ", i);
    for (j =0; j < 16; j += 4)
      fprintf(stdout, " %2x%2x%2x%2x", 
	       p[i+j]     & 255, p[i+j + 1] & 255,
	       p[i+j + 2] & 255, p[i+j + 3] & 255);

    fprintf(stdout,"\n");
  }
}


