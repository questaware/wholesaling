#include <stdio.h>
#include <string.h>
#include "version.h"
#include "../h/defs.h"
#include "../h/errs.h"
#include "../form/h/form.h"
#include "../db/cache.h"
#include "../db/recs.h"
#include "domains.h"
#include "schema.h"
#include "rights.h"
#include "generic.h"
#include "trail.h"

#define private 

#define W_BU   (3 * ID_FAC)
#define W_INU  (4 * ID_FAC)
#define W_CAT  (5 * ID_FAC)
#define W_NAME (6 * ID_FAC)
#define W_FILE (7 * ID_FAC)
#define W_PLEN (8 * ID_FAC)
#define W_TCAP (9 * ID_FAC)

#define SL 5

static const Fieldrep_t SIform [] = 
  {{W_MSG+1,FLD_RD+FLD_MSG,0,20,50,  1, ""},
   {W_CMD, FLD_WR,        1, 1, 20,  1, comd},
   {W_MSG, FLD_RD+FLD_FD, 1,30, 40,  1, ""},
   {W_BU,  FLD_WR,        3,18,LPATH,1, "backup device"},

   {W_INU, FLD_WR+FLD_BOL,7, 0, 14, SL, "owner"},
   {W_CAT, FLD_WR,	  7,15,  2, SL, "category"},
   {W_NAME,FLD_WR,        7,20, 15, SL, "printer"},
   {W_FILE,FLD_WR,        7,38, 20, SL, "file"},
   {W_PLEN,FLD_WR,        7,60,  4, SL, "page length"},
   {W_TCAP,FLD_WR+FLD_EOL,7,66,LTEL,SL, "vdu type"},
   {0,     0,      0, 0,  0,  1, null}
  };

static const Over_t SIover [] = 
 {{0,1,1,0,">"},
  {0,3,1,0,"Backup Device:"},
  {0,4,1,17,"also: PUSH filename   temporarily use filename"},
  {0,5,1,17,"      POP             recover from filename"},
  {1,6,1,0, " Owner    Category Printer             File          Page Length "},
  {1,SL+7,1,0, "-------- Printers "},
  {1,SL+7,1,18, hline(47)},
  {1,SL+9,1,2, "in Owner field CR to insert"},
  {1,SL+10,1,2,"               D to delete"},
  {-1,0,0,0,null}
 };

private void put_prt(ix)
      Int  ix;
{ Prt  prt = &adscr.cs[adscr.curr_base + ix];
  Int spos = save_pos();
  Char str[40];
  goto_fld(W_INU + ix);   
  if (adscr.curr_base + ix > adscr.curr_last)
    put_eeol();
  else  
  { strcpy(&str[0], "/dev/tty");
    *(Int*)&str[8] = prt->in_use;
    str[prt->in_use ? 12 : 0] = 0;
    if (strcmp(str, this_tty) == 0)
      strcpy(&str[0], "YOURS");
  				   put_data(str);
    str[0] = prt->type;
    str[1] = 0;
    form_put(T_DAT+W_CAT + ix,   str,
             T_DAT+W_NAME + ix,  prt->path,
             T_DAT+W_FILE + ix,  prt->file,
             T_OINT+W_PLEN + ix, prt->pagelen,
             T_DAT+W_TCAP + ix,  prt->tcap, 0);
  }
  restore_pos(spos);
}



private void scroll_prt(stt, dir)
	Int     stt, dir;
{ Prt pa = adscr.cs;
  Int limix = adscr.arr_sz - 1;
  Prt tgt = &pa[ dir < 0 ? stt   : limix];
  Prt src = &pa[ dir < 0 ? stt+1 : limix-1];
  Prt lim = &pa[ dir < 0 ? limix : stt];
  
  if (dir < 0)
    while (src <= lim)
      *tgt++ = *src++;
  else
    while (tgt >= lim)
      *tgt-- = *src--;
{ Lra lraa = adscr.clra;
  Lra tgt_ = &lraa[ dir < 0 ? stt   : limix];
  Lra src_ = &lraa[ dir < 0 ? stt+1 : limix-1];
  Lra lim_ = &lraa[ dir < 0 ? limix : stt];
  
  if (dir < 0)
    while (src_ <= lim_)
      *tgt_++ = *src_++;
  else
    while (tgt_ >= lim_)
      *tgt_-- = *src_--;
}}

Char * do_ins_prt(upn, ix, prt, ds)
	 Id     upn;
	 Int    ix;
	 Prt    prt;
	 Char * ds;
{ put_eeol();
{ Short ix_ = ix - adscr.curr_base;
  while (ix_ < SL - 1)
    put_prt(++ix_);
  
  ix_ = ix - adscr.curr_base;
  salert("Port?");
  get_data(ds);
  if (strlen(ds) <= 8 or strmatch("/dev/tty", ds) == ds)
    return "/dev/ttyX...";
  prt->in_use = *(Int*)&ds[8];

  goto_fld(W_CAT + ix_);
  while (true)
  { salert("Category A-Z");
    get_data(ds);
    prt->type = toupper(*ds);
    if (prt->type == 0 or in_range(prt->type, 'A', 'Z'))
      break;
  }

  goto_fld(W_NAME + ix_);
  get_data(&prt->path[0]);
  
  goto_fld(W_FILE + ix_);  get_data(&prt->file[0]);
  goto_fld(W_PLEN + ix_);

  while (true)
  { Int i;
    Cc cc = get_integer(&i);
    prt->pagelen = i;
    if (cc == OK)
      break;
    salert(nreq);
  }
  goto_fld(W_TCAP + ix_);  get_data(&prt->tcap[0]);
  
  wr_lock(upn);
{ Cc cc = new_init_rec(upn, CL_PRT, prt, &adscr.clra[ix]);
  wr_unlock(upn);
  return cc == OK ? null : iner;
}}}

private Char * do_edit_prtrs(upn, fld, ds, ix_ref)
	 Id       upn;
	 Field    fld;
 	 Char *   ds;
 	 Int    * ix_ref;
{ Int fix = fld->id & ID_MSK;
  Int fldtyp = fld->id & ~ID_MSK;

  if (disallow(R_PRT))
    return preq('P');
  
{ Int ix_ = adscr.curr_base + fix;
  Int ix = ix_ <= adscr.curr_last ? ix_ : adscr.curr_last + 1;
  if (ix + 1 >= adscr.arr_sz)
    extend_prt(&adscr);
  
{ Prt prt = &adscr.cs[ix];
  Lra_t lra = adscr.clra[ix];

  ds = skipspaces(ds);
  if (ix_ > adscr.curr_last and (fldtyp != W_INU or *ds != 0))
    return ngth;

  if      (fldtyp == W_INU)
  { if      (*ds == 0)
    { scroll_prt(ix, 1);
    { Char * error = do_ins_prt(upn, ix, prt, ds);
      if (error == null)
      { *ix_ref = ix-1;
	adscr.curr_last += 1;
      }
      else
	scroll_prt(ix, -1);
      return error;
    }}
    else if (toupper(*ds) == 'D' and ds[1] == 0)
    { Cc cc = del_rec(upn, CL_PRT, lra);
      if (cc == OK)
      { *ix_ref = ix-1;
	scroll_prt(ix, -1);
	adscr.curr_last -= 1;
	return null;
      }
      return ntfd;
    }
    else
    { if (strlen(ds) <= 8 or strmatch("/dev/tty", ds) == ds)
	return "/dev/ttyX...";
      prt->in_use = *(Int*)&ds[8];
    }
  }
  else if (fldtyp == W_CAT)
  { if (ds[0] == ' ')
      ds[0] = 0;
    if (not in_range(toupper(ds[0]), 'A', 'Z') and not ds[0] == 0)
      return "A-Z or empty";
    prt->type = ds[0];
  }
  else if (fldtyp == W_NAME)
    strpcpy(&prt->path[0], ds, sizeof(Path));
  else if (fldtyp == W_FILE)
    strpcpy(&prt->file[0], ds, sizeof(Path));
  else if (fldtyp == W_PLEN)
    prt->pagelen = atoi(ds);
  else if (fldtyp == W_TCAP)
    strpcpy(&prt->tcap[0], ds, sizeof(Tel));
  
  wr_lock(mainupn);
  write_rec(mainupn, CL_PRT, lra, prt);
  wr_unlock(mainupn);
  put_prt(ix, prt);
  return null;
}}}

#define push_bu biglistbuff

private Cc do_prtrs(upn,  ds)
	 Id	  upn;
	 Char *   ds;
{ static const char wmsg[] = "Wait 20 seconds before changing disk";

  push_bu[0] = 0;

  clr_alert();
  goto_fld(W_INU);
{ Int ix = -1;
  Cc contcc = SUCCESS;
  Char * error = null;

  fld_put_data(W_BU, uniques.at_path);
  goto_fld(W_CMD);

  while (contcc == SUCCESS)
  { while (ix < SL-1)
      put_prt(++ix);
    
    if (error != null)
      alert(error);
    error = null;
     
  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);
    vfy_lock(upn);
    
    if	    (fld->id == W_CMD)
    { if      (*strmatch(".", ds) == 0)
	contcc = HALTED;
      error = dtex;
    }
    else if (cc == HALTED)
    { if (fld->props & FLD_FST)
      { if (adscr.curr_base > 0)
	{ wclrgrp();
	  adscr.curr_base -= 1;
	  ix = - 1;
	} 
      }
      else 
	if (adscr.curr_base + SL - 2 < adscr.curr_last)
	{ wscroll(1);
	  adscr.curr_base += 1;
	  ix -= 1;
	}
    }
    else if (cc == PAGE_UP or cc == PAGE_DOWN)
    { Short base = adscr.curr_base + (cc == PAGE_DOWN ? SL - 1 : 1 - SL);
      if (base + SL - 1 > adscr.curr_last)
	base = adscr.curr_last - SL + 2;
      if (base < 0)
	base = 0;
      if (base != adscr.curr_base)
      { wclrgrp();
	adscr.curr_base = base;
	ix = - 1;
      } 
    }
    else if (cc == SUCCESS)
    { if      (upn_ro)
	error = rdoy;
      else if (in_range(fld->id, W_INU, W_TCAP+SL))
	error = do_edit_prtrs(upn, fld, ds, &ix);
      else if (fld->id == W_BU)
      { if (this_agent.id != 1)
	  error = ymba;
	else 
	{ Char const * ds_ = *strmatch("PUSH ", ds) == 0 ? &ds[5]	 :
		             *strmatch("POP", ds) == 0   ? &push_bu[0] : ds;
	  Char * ds__ = skipspaces(ds_);
	  
	  error = null;
	  
	  if	  (ds_ == &push_bu[0])
	  { if (push_bu[0] == 0)
	      error = "Nothing pushed";
	    else
	      at_pop(&this_at[0], push_bu);
	  }
	  else if (*ds__ == 0)
	    error = "file | PUSH file | POP";
	  else if (ds_ == &ds[5])	/* PUSH */
	  { strpcpy(&push_bu[0], this_at, sizeof(push_bu));
	    at_push(ds__);
	  }
	  else
	    at_chg(ds_);
	  if (error == null)
	  { wr_lock(upn);
	  { Lra_t  lra;
	    Unique un = find_uniques(upn, &lra);
	    strpcpy(&un->at_path[0], ds__, sizeof(Path));
	    strpcpy(&this_at[0],     ds__, sizeof(Path));
	    (void)write_rec(upn, CL_UNIQ, lra, &uniques);
	    wr_unlock(upn);
	    salert(wmsg);
	    *ds__ = 0;		/* to clear push_bu */
	    error = null;
	  }}
	}
      }
    }
  }}
  end_interpret();
  return contcc;
}}

public const Char * do_printers()

{ Tabled tbl1 = mk_form(SIform,0);
  if (tbl1 == null)
    return iner;

  wopen(tbl1,SIover);

{ Prt_t prtarea[MAX_ADSZ];
  Lra_t lraarea[MAX_ADSZ];
  load_prtrs(prtarea, lraarea);
  (void)find_uniques(mainupn, &trash);
  strpcpy(&this_at[0], uniques.at_path, LPATH + 1);

  while (true)
  { Cc ccc = do_prtrs(mainupn, ds);
    if (ccc == HALTED)
      break;
  }
  
  freem(tbl1);
  return null;
}}
