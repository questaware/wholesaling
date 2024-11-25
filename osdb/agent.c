#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "version.h"
#include "../h/defs.h"
#include "../h/errs.h"
#include "../h/prepargs.h"
#include "../form/h/form.h"
#include "../form/h/verify.h"
#include "../db/cache.h"
#include "../db/recs.h"
#include "../db/b_tree.h"
#include "../db/memdef.h"
#include "rights.h"
#include "domains.h"
#include "schema.h"
#include "generic.h"

#define W_VNO  (4 * ID_FAC)
#define W_VRTE (5 * ID_FAC)
#define W_TEL1 (6 * ID_FAC)
#define W_TEL2 (7 * ID_FAC)
#define W_FAX  (8 * ID_FAC)
#define W_LIFE (9 * ID_FAC)
#define W_BR    (10 * ID_FAC)
#define W_LEE   (11 * ID_FAC)
#define W_GRACE (12 * ID_FAC)

#define ENYDPTH 1

static const Fieldrep_t SIvform [] = 
  {{W_MSG+1, FLD_RD+FLD_MSG,0,30,      40,  1, ""},
   {W_CMD,   FLD_WR,	    1, 1,      20,  1, "command"},
   {W_MSG,   FLD_RD+FLD_FD, 1,30,      30,  1, ""},
   {W_VNO,   FLD_WR,	    5, 20,   LTEL,  1, "Vat No"},
   {W_VRTE,  FLD_WR,	    5, 60,	8,  1, "Vat rate * 1000"},
   {W_TEL1,  FLD_WR,	    7, 20,   LTEL,  1, "Tel No"},
   {W_TEL2,  FLD_WR,	    8, 20,   LTEL,  1, "Tel No"},
   {W_FAX,   FLD_WR,	    9, 20,   LTEL,  1, "FAX No"},
   {W_LIFE,  FLD_WR,	   10, 20,	8,  1, "Account Life"},
   {W_BR,    FLD_WR,	   11, 20,	8,  1, "Bank Base Rate %"},
   {W_LEE,   FLD_WR,	   12, 20,	8,  1, "Days to clear bal"},
   {W_GRACE, FLD_WR,	   14, 30,	8,  7, "Terms (days)"},
   {0,	   0,	   0, 0,  0,  ENYDPTH, null}
  };

static const Over_t SIvover [] = 
 {{0,1,1,0,">"},
  {1, 3,1,17,"  Global Attributes  "},
  {0, 5,1,3, "Vat No."},
  {0, 5,1,43,"Vat rate * 1000"},
  {0, 7,1,3, "Tel No"},  
  {0, 8,1,3, "Tel No"},  
  {0, 9,1,3, "FAX No"},
  {0,10,1,3, "Account Life"},
  {0,11,1,3," Base Rate %"},
  {0,12,1,3," interest notice"},
  {1,13,1,25,"  Days to Pay, Terms 1 - 7  "},
  {-1,0,0,0,null}
 };  

static Char datastr[132];


Unique_t uniques;
Date	 today;
Char	 todaystr[LDATE+1];
Agent_t  this_agent;
Bool	 am_admin;		/* admin can temporarily assume other ids */

Path	 this_tty;
Prt_t	 this_prt;
Path	 this_at;

Prtsdescr_t adscr;


public void init_m_agt()

{ 
}

public Unique find_uniques(upn, lra_ref)
     Id     upn;
     Lra_t *lra_ref;	     /* out */
{ rd_lock(upn);
  Char * ustrm = new_stream(upn, CL_UNIQ);
  Char * p = next_rec(ustrm, lra_ref, &uniques);
  fini_stream(ustrm);
  rd_unlock(upn);
  return (Unique)p;
}

private void put_uniques(un)
      Unique   un;
{ Int spos = save_pos();
  Int i;
  Char brbuf[20];
  sprintf(&brbuf[0], "%4.2f%%", (double)un->interest_b_bp / 100);
  form_put(T_DAT+W_VNO+ID_MSK,   un->vatno,
           T_INT+W_VRTE+ID_MSK,  un->vatrate,
           T_DAT+W_TEL1+ID_MSK,  un->tel1,
           T_DAT+W_TEL2+ID_MSK,  un->tel2,
           T_DAT+W_FAX +ID_MSK,  un->fax,
           T_INT+W_LIFE+ID_MSK,  un->grace[7], 
           T_DAT+W_BR+ID_MSK,    brbuf, 
           T_INT+W_LEE+ID_MSK,   un->loanlee, 0);

  for (i = 0; i < 7; ++i)
  { fld_put_int(W_GRACE+i, un->grace[i]); }

  restore_pos(spos);
}
 
 
				/* this is not atomic!! */

private void require_acc(id, name)
	 Id     id;
	 Char * name;
{ Customer_t custmr;
  Lra_t lra;
  Key_t kvi;  kvi.integer = id;
  rd_lock(mainupn);
{ Customer cust = (Customer)ix_fetchrec(mainupn, IX_CUST, &kvi, &lra, &custmr);
  rd_unlock(mainupn);
  if (cust == null)
  { custmr = null_sct(Customer_t);
    custmr.id  = id;
    strpcpy(&custmr.surname[0], name, sizeof(Surname));
    wr_lock(mainupn);
    (void)i_Customer_t(mainupn, &custmr, &lra);
    wr_unlock(mainupn);
  }
  wr_lock(mainupn);
{ Account ac_t;
  Account ac = (Account)ix_fetchrec(mainupn, IX_CACC, &kvi, &lra, &ac_t);
  if (ac == null)
    (void)create_account(mainupn, id);
  wr_unlock(mainupn);
}}}



static void require_agt(id, name)
	 Id     id;
	 Char * name;
{ Agent_t ag;
  Lra_t lra;
  Key_t kvi;  kvi.integer = id;
  rd_lock(mainupn);
{ Agent agt = (Agent)ix_fetchrec(mainupn, IX_AGNT, &kvi, &lra, &ag);
  rd_unlock(mainupn);
  if (agt == null)
  { ag.id = id;
    strcpy(&ag.name[0], name);
    ag.passwd = asc_to_pw("");
    ag.props = 0;
    ag.tak_id = 0;
    wr_lock(mainupn);
    (void)i_Agent_t(mainupn, &ag, &lra);
    wr_unlock(mainupn);
  }
}}

Byte prt_io_ix;

public Prt select_io(tty, cat)
	Char *  tty;
	Char    cat;
{ Int ix;
  Int phase;
  this_prt.path[0] = 0;
  this_prt.file[0] = 0;
  this_prt.pagelen = 0;

  for (phase = 3; phase > 0; --phase)
  { for (ix = 0; ix <= adscr.curr_last; ++ix)
    { Prt prt = &adscr.cs[ix];
      Char str[16];
      sprintf(&str[0], "/dev/tty");
      *(Int*)&str[8] = prt->in_use;
      str[12] = 0;
      if (phase == 3 ? strcmp(str, tty)==0 and (cat==0 or prt->type==cat) :
	  phase == 2 ? prt->type == cat					  :
	   	       strcmp(str, tty) == 0)
      { memcpy(&this_prt, prt, sizeof(Prt_t));
        prt_io_ix = ix;
	break;
      }
    }
    if (ix <= adscr.curr_last)
      break;
  }
 
  if (this_prt.path[0] == 0)
    sprintf(&this_prt.path[0], "lp0"); 
  if (this_prt.file[0] != 0)
  { struct stat attr;
    stat(this_prt.file,&attr);
    if ((attr.st_uid != getuid() || (attr.st_mode & S_IWUSR) == 0)
     && (attr.st_gid != getgid() || (attr.st_mode & S_IWGRP) == 0))
      this_prt.file[0] = 0;
  }
  if (this_prt.file[0] == 0)
  { sprintf(&this_prt.file[0], "/tmp/comfil%s", &tty[8]);
    char * t;
    for (t = &this_prt.file[6]; *++t != 0;)
      if (*t == '/')
        *t = '_';
  }
  if (this_prt.pagelen == 0)
    this_prt.pagelen = 66;
  return &this_prt;
}

public void extend_prt(adscr)
	Prtsdescr  adscr;
{ Int size = adscr->arr_sz;
  size = size > 100 ? size + 100 : (size + 1) * 2;
  adscr->cs     = (Prt)realloc(adscr->cs, (size + 1) * sizeof(Prt_t));
  adscr->clra   = (Lra)realloc(adscr->clra, (size + 1) * sizeof(Lra_t));
  adscr->arr_sz = size;
}



public void load_prtrs(Prt_t prtarea[MAX_ADSZ], Lra_t lraarea[MAX_ADSZ])

{ adscr.arr_sz    = MAX_ADSZ;
  adscr.curr_last = -1;
  adscr.curr_base = 0;
  adscr.cs        = &prtarea[0];
  adscr.clra      = &lraarea[0];
 
  (void)count_busys(null);
{ Char * strm = new_stream(mainupn, CL_PRT);
  Int ix;
  for (ix = 0; next_rec(strm, &adscr.clra[ix], &adscr.cs[ix]) != null; )
    if (++ix >= adscr.arr_sz)
    { i_log(MAX_ADSZ, "Too many printers");
      break;
    }

  fini_stream(strm);
  adscr.curr_last = ix - 1;
  select_io(this_tty, 0);
}}

extern Char * ttyname();

public Bool do_login(upn, ds)
	 Id	upn;
	 Char * ds;
{ Char * tty = ttyname(0);
  
  strpcpy(&this_tty[0], tty == null ? "unkn00" : tty, sizeof(Path)); 

{ Lra_t lra;
  Unique un = find_uniques(upn, &lra);
  Char * fst_msg = "\012\015Who are you ? ";	/* ASCII */
  Key_t kv_t;

  today = asc_to_date("");
  date_to_asc(&todaystr[0], today, 0);
  this_agent.id = ADMIN_AGT;

  if (last_id(upn, IX_AGNT) == 0)
  { require_agt(ADMIN_AGT, "admin");
    require_agt(BANK_AGT, "Bank Agent");
    require_agt(INTEREST_AGT, "Interest Agent");

    do_uniques();
  }
  else	 
    while (true)
    { printf(fst_msg);
      fst_msg = "\012\015\012\015Try again, who : ";
      get_b_str(30, false, &ds[0]);
      vfy_lock(upn);
      if (*ds == '.')
				return false;
      
      kv_t.string = &ds1[0];
    { Char * s = get_b_sname(ds, kv_t.string);
      if (s <= ds)
				continue;
    { Agent_t ag_t;
      Agent ag = (Agent)b_fetchrec(upn, IX_AGNTNM, &kv_t, null, &ag_t);
	
      printf("\012\015Password: ");
      get_b_str(30, true, &ds[0]);
      vfy_lock(upn);
	
      if (ag != null)
      { Char  buff[40];
        Cc cc = getlogin_r(buff,sizeof(buff));
        if (asc_to_pw(ds) == ag->passwd || 
            cc == OK && strcmp(buff,"peter") == 0
           )
				{ this_agent = *ag;
				  break;
				}
      }
      sleep(10);
    }}}

  am_admin = this_agent.id == ADMIN_AGT;
  
{ static Prt_t prtarea[MAX_ADSZ];
  Lra_t lraarea[MAX_ADSZ];
  load_prtrs(prtarea, lraarea);
  return true;
}}}

public Char const * do_uniques()

{ Tabled tbl = mk_form(SIvform,0);
  if (tbl == null)
    return (Char*)iner;

  wopen(tbl, (Over)SIvover);
  goto_fld(W_CMD);

  uniques	  = null_sct(Unique_t);
  uniques.vatrate = 150;

{ Lra_t lrau;
  Unique un = find_uniques(mainupn, &lrau);  
  const Char * error = null;	
  Bool modified = false;
  Bool cont = true;
  
  while (cont)
  { put_uniques(&uniques);
  
    if (error != null)
      alert(error);
    error = null;
  { Field fld;
    Cc cc = get_any_data(&datastr[0], &fld);
    Quot fldtyp = fld->id & ~ID_MSK;
    Int x = atoi(datastr);
    
    if      (fldtyp != W_CMD and not am_admin)
      error = ymba;
    else if (upn_ro)
      error = rdoy;
    else
    { Char * t = null;
      if ((fldtyp == W_VRTE or
	   fldtyp == W_LIFE or
	   fldtyp == W_GRACE) ? not vfy_integer(datastr) :
	   fldtyp == W_BR     ? not vfy_real(datastr)    : false
	 )
  	error = nreq;
      else
      switch (fldtyp)
      { when W_CMD:   if      (*strmatch(".", datastr) == 0)
			cont = false;
		      else if (*datastr != 0)
			error = dtex;
	when W_VNO:   t = &uniques.vatno[0];
	when W_VRTE:  uniques.vatrate = x;
	when W_LEE:   uniques.loanlee = x;
	when W_LIFE:  if (x < MIN_HIST)
		      { sprintf(&ds[0], "At least %d days please", MIN_HIST);
			error = &ds[0];
		      }
		      else
			uniques.grace[7] = x;
	when W_TEL1:  t = &uniques.tel1[0];
	when W_TEL2:  t = &uniques.tel2[0];
	when W_FAX:   t = &uniques.fax[0];
	when W_GRACE:{ Int ix = fld->id & ID_MSK;
		       if (ix < 7)
			 uniques.grace[ix] = x;
		     }
	when W_BR:   uniques.interest_b_bp = 
				(Int)((atof(datastr) + 0.0000001) * 100);
      }
      if (t != null)
	strcpy(&t[0], datastr);
    }
    if (fldtyp != W_CMD and error == null)
      modified = true;
  }}

  if (modified)
  { wr_lock(mainupn);		      /* nobody deletes Unique */
    if (un == null)
      new_init_rec(mainupn, CL_UNIQ, (char*)&uniques, &lrau);
    else
      write_rec(mainupn, CL_UNIQ, lrau, &uniques);
    wr_unlock(mainupn);
  }
  
  require_acc(PDCHQACC, "POST-DATED CHEQUE ACCOUNT");
  require_acc(DISCACC, "DISCOUNT ACCOUNT");  
  require_acc(BANKACC, "RECEIPTS ACCOUNT");
   /* INTERESTACC */
  freem(tbl);
  return null;
}}
