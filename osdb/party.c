#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "version.h"
#include "../h/defs.h"
#include "../h/errs.h"
#include "../h/prepargs.h"
#include "../form/h/form.h"
#include "../form/h/verify.h"
#include "../db/cache.h"
#include "../db/b_tree.h"
#include "../db/recs.h"
#include "../db/memdef.h"
#include "domains.h"
#include "schema.h"
#include "rights.h"
#include "generic.h"

#define private 

#define W_ID   (4 * ID_FAC)
#define W_NAME (5 * ID_FAC)
#define W_RGTS (26 * ID_FAC)
#define W_PW   (27 * ID_FAC)

#define SLA 10
#define ENYDPTHA 1

#define W_BACC  (6 * ID_FAC)
#define W_TERMS (7 * ID_FAC)
#define W_CLIM  (8 * ID_FAC)
#define W_OWES  (9 * ID_FAC)
#define W_FNAME (10* ID_FAC)
#define W_INTEREST (11 * ID_FAC)
#define W_LEEWAY   (12 * ID_FAC)
#define W_PC    (13 * ID_FAC)
#define W_TEL   (14 * ID_FAC)
#define W_TT    (15 * ID_FAC)
#define W_ADDR  (16 * ID_FAC)
#define W_CUST  (17 * ID_FAC)
#define W_NID		(18 * ID_FAC)
#define W_NRGTS	(19 * ID_FAC)
#define W_NADDR (20 * ID_FAC)

#define SL ((SCRN_LINES-10)/3)
#define ENYDPTH 4
#define SL2 ((SCRN_LINES-10)/2)
#define ENYDPTH2 3

#define FIELD_TT  13
#define FIELD_FAC 13

static const Over_t SAover [] =
 {{0,1,1,0,">"},
  {1,4,1,0,
 "  Id  Name                     Rights                   Password         "},
  {1,SLA+6,1,0, hline(73)},
  {-1,0,0,0, null}
 };


static const Fieldrep_t SAform [] =
  {{W_MSG+1,FLD_RD+FLD_MSG,0,20,40,		1, ""},
   {W_CMD, FLD_WR,	  1, 1, 20,		1, "command"},
   {W_MSG, FLD_RD+FLD_FD, 1,30, 40,		1, ""},
   {W_ID,  FLD_WR+FLD_BOL,5, 0,  5,	       -SLA,"Number"},
   {W_NAME,FLD_WR,	  5, 6,LSNAME-10,      -SLA,"Name"},
   {W_RGTS,FLD_WR,	  5,-2+LSNAME,27,      -SLA,"A-Z"},
   {W_PW,  FLD_WR+FLD_NOECHO,
			  5,26+LSNAME,LSNAME-7,-SLA,"Password"},
   {0,	   0,	   0, 0,  0,  ENYDPTHA, null}
  };


static const Over_t SI1over [] =
  {{0,1,1,0,">"},
   {1,3,1,0,
 " Id     Name                             Their-Ref  Terms   Credit-limit  Owes"},
   {1,4,1,0,
 "             Title                        Interest Leeway Postcode Telephone"},
   {-1,0,0,0,"Customers"}
  };

static const Fieldrep_t SIform [] =
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20, 60,  1, ""},
   {W_CMD,   FLD_WR,        1, 1, 20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,30, 30,  1, ""},

   {W_ID,    FLD_WR+FLD_BOL,5, 0,  5,   -SL, "Customer Id"},
   {W_NAME,  FLD_WR,	    5, 6,LSNAME,-SL, "Name"},
   {W_BACC,  FLD_WR,	    5,40,  6,   -SL, "Their Ref"},
   {W_TERMS, FLD_WR,	    5,47, 15,   -SL, "Terms: A-W*(#)Z{#|@#}"},
   {W_CLIM,  FLD_WR,	    5,65,  6,   -SL, "Credit limit"},
   {W_OWES,  FLD_WR,	    5,72,  8,   -SL, "Owes"},
   {W_FNAME, FLD_WR,	    6,23,LFNAMES,-SL,"Title"},
   {W_INTEREST, FLD_WR,   6,40,  4,   -SL,"Interest %"},
   {W_LEEWAY,FLD_WR,      6,45,  4,   -SL,"Leeway days"},
   {W_PC,    FLD_WR,	    6,60,LPCODE,-SL, "Postcode"},
   {W_TEL,   FLD_WR,	    6,68,LTEL,  -SL, teln},
   {W_ADDR,  FLD_WR,	    7, 1,LPADDR,-SL, psta},
   {W_TT,    FLD_WR,	    8,25,FIELD_TT+FIELD_FAC+3, -SL, "TT ids"},
   {W_BAN,   FLD_RD,     -5,0,  80, 1,""},
   {0,       0,             0, 0,  0,   ENYDPTH, null}
  };

static const Over_t SI2over [] =
  {{0,1,1,0,">"},
   {1,3,1,0,
    " Id     Name                             Postcode    Telephone-Number "},
// {0,-5, 1, 0, hline(71)},
   {-1,0,0,0,""}
  };


static const Fieldrep_t SI2form [] =
 {{W_MSG+1, FLD_RD+FLD_MSG,0,20, 60,  1, ""},
  {W_CMD,   FLD_WR,	    1, 1, 20,  1, comd},
  {W_MSG,   FLD_RD+FLD_FD, 1,30, 30,  1, ""},

  {W_ID,    FLD_WR+FLD_BOL,4, 0,  8,   -SL2, "Manuf. Code"},
  {W_NAME,  FLD_WR,	   4, 9,LSNAME,-SL2, "Name"},
  {W_PC,    FLD_WR,	   4,40,LPCODE,-SL2, "Postcode"},
  {W_TEL,   FLD_WR,	   4,58,LTEL,  -SL2, teln},
  {W_ADDR,  FLD_WR,	   5, 2,LPADDR,-SL2, psta},
  {W_TT,    FLD_WR,	   6,25,FIELD_TT+FIELD_FAC+3, -SL, "TT ids"},
  {W_BAN,   FLD_RD,       -4,0,  80, 1,""},
  {0,0,0,0,0,ENYDPTH2,null}
 };

static const char agent []    = "  AGENTS   ";
static const char customer [] = "  CUSTOMERS";
static const char supplier [] = "  SUPPLIERS";

#define SL3 ((SCRN_LINES-10)/3)

static Over_t SIsover [] = /* not constant */
  {{0,1,1,0,">"},
   {1,3,1,0, customer},
   {1,3,1,11, spaces(31)},
   {1,SL3+5,1,0, ssvce},
   {1,SL3+5,1,23, hline(19)},
   {-1,0,0,0,null}
  };

#define LSSNM (LSNAME-10)

static const Fieldrep_t SMform [] =
  {{W_MSG+1,FLD_RD+FLD_MSG,0,20,40,		1, ""},
   {W_CMD,  FLD_WR,	  1, 1, 20,		1, "command"},
   {W_MSG,  FLD_RD+FLD_FD, 1,30, 40,		   1, ""},
   {W_NID,  FLD_WR,		     3, 0,  5,       1, "New Id"},
   {W_NRGTS,FLD_WR,				 3, 8,  6,			 1, "New Uses"},
   {W_NADDR,FLD_WR,				 3,16, 35,		   1, "New Email"},
   {W_ID,   FLD_WR+FLD_BOL,6, 0,  6,	    -1,"Number"},
   {W_NAME, FLD_WR,	       6,8, LSSNM,    -1,"Name"},
   {W_RGTS, FLD_WR,	       6,8+LSSNM+2,5, -1,"Uses"},
   {W_ADDR, FLD_WR,        6,8+LSSNM+8,40,-1,"Email Addr"},
   {0,	    0,	   0, 0,  0,  1, null}
  };

#define SL4 (SCRN_LINES-10)

static Over_t SMover [] =
  {{0,1,1,0,">"},
   {1,2,1,0," Id     Uses    Email-Address"},
   {1,5,1,0," Id     Name                  Uses  Email-Address"},
   {1,SL4+5,1,0, "-- Email Addresses -----------------------------------------------"},
   {-1,0,0,0,null}
  };

static const Char const * optext[] =
{ " A           -- Add",
  " U  name     -- Update",
  " UA name     -- Update",
  " B name      -- Browse",
  " BA name     -- Browse",
  " L name name -- List",
  " D           -- Debts",
  " M name      -- Email",
  null
};

extern Tabled seltbl;
static Tabled gentbl;

static Int enysz;
static Int pty_sl;

const int EM_DATE_OFFS = 8;
const int EM_USES_OFFS = 21;
const int EM_ADDR_OFFS = 28;

private void init_m_cust()

{
}



double s_interest_rate(Customer cu)

{ Lra_t lra;
  Unique un = find_uniques(mainupn, &lra);
  if (un == null)
  { i_log(0, "Lost Uniq");
    return 0.0;
  }
  return ((double)((un)->interest_b_bp+((cu)->interest_01/2)*10)/10000);
}

private void put_agnt(ag)
	Agent	 ag;
{ Char rgtstr[28];
  Int spos = save_pos();
  Field fld = goto_fld(ID_MSK);

  rgt_to_asc(ag->props, &rgtstr[0]);
  if (not (fld->props & FLD_BOL))
    left_fld(FLD_BOL, &fld);
  
  form_put(T_INT+W_ID+ID_MSK,   ag->id,
           T_DAT+W_NAME+ID_MSK, ag->name,
           T_DAT+W_RGTS+ID_MSK, rgtstr,
           T_OINT+W_PW+ID_MSK,  ag->passwd, 0);
  restore_pos(spos);
}


private char * extract_tt_ids(char tt_id[FIELD_TT+FIELD_FAC+10],
                              const char * src,
                              int inc_space)
{ tt_id[0] = 0;
  if (src != NULL)
  { const char * s, *ss;
    char * t;
    strpcpy(tt_id, src,FIELD_TT+FIELD_FAC+10);
    for (s = src - 1; *++s != 0 && *s != ','; )
      ;
    if (*s == 0)
      return NULL;
    if (s - src > FIELD_TT)
      return NULL;
      
    for (ss = s; *--ss == ' ' && ss >= src;)
      ;
    t = &tt_id[ss - src];
    *++t = ',';
    if (inc_space)
      *++t = ' ';
    
    while (*++s == ' ')
      ;
    for (ss = s - 1; *++ss != 0 && *ss != ' ';)
      *++t = *ss;
    
    if (ss - s  > FIELD_FAC + 1)
    { tt_id[0] = 0;
      return NULL;
    }
    *++t = 0;
  }
  
  return tt_id;
}



private void put_cmr(ci, tt_reg, bal)
	Customer ci;
  Tt_reg   tt_reg;
	Cash     bal;
{ Int spos = save_pos();
  Field fld = goto_fld(ID_MSK);
  Char buf[19], tt_id[FIELD_TT+FIELD_FAC+2];
  (void)extract_tt_ids(tt_id,tt_reg == NULL ? NULL : &tt_reg->tt_id[0],true);
  
  if (not (fld->props & FLD_BOL))
    left_fld(FLD_BOL, &fld);

  put_integer( ci->id);
  
  form_put(T_DAT+ W_NAME+ID_MSK, ci->surname,
           T_DAT+W_BACC+ID_MSK,  to_client_ref(&buf[0], ci->bank_acc),
           T_DAT+W_TERMS+ID_MSK, this_sale_terms,
           T_INT+W_CLIM+ID_MSK,  ci->creditlim /(ci->terms & M_BILL ? 1 : 100), 0);

  if (not disallow(R_VACCT))
    form_put(T_CSH+W_OWES+ID_MSK, bal, 0);
  
  buf[0] = 0;
  if (ci->interest_01 != 0)
    sprintf(&buf[0], "%4.2f%s", s_interest_rate(ci)*100, 
    				ci->props & P_HIDEINT ? "t" : "");
  form_put(T_DAT+ W_FNAME+ID_MSK, ci->forenames,
           T_DAT+ W_INTEREST+ID_MSK, buf,
           T_INT+ W_LEEWAY+ID_MSK,ci->loanlee,
           T_DAT+ W_PC+ID_MSK,    ci->postcode,
           T_DAT+ W_TEL+ID_MSK,   ci->tel,
           T_DAT+ W_ADDR+ID_MSK,  ci->postaddr,
           T_DAT+ W_TT+ID_MSK,    tt_id,0);
  restore_pos( spos);
}


private void put_sup(ci, tt_reg)
	Supplier ci;
  Tt_reg   tt_reg;
{ Int spos = save_pos();
  Field fld = goto_fld(ID_MSK);
  if (not (fld->props & FLD_BOL))
    left_fld(FLD_BOL, &fld);
  
{ Char buf[19], tt_id[FIELD_TT+FIELD_FAC+2];
  (void)extract_tt_ids(tt_id,tt_reg == NULL ? NULL : &tt_reg->tt_id[0],true);
    
  if (not old_bill_alt(ci->terms) and (ci->terms & P_DLTD))
			     put_data("******");
  else
			     put_integer(ci->barcode);
  form_put(T_DAT+W_NAME+ID_MSK, &ci->surname[0],
           T_DAT+W_PC+ID_MSK,   &ci->postcode[0],
        /* T_INT+W_TERMS+ID_MSK, ci->terms, */
           T_DAT+W_TEL+ID_MSK,  &ci->tel[0],
           T_DAT+W_TT+ID_MSK,    tt_id,
           T_DAT+W_ADDR+ID_MSK, &ci->postaddr[0], 0);
  restore_pos(spos);
}}



private void put_email(Id id, 
											 const char * name, const char * uses, const char * addr)
{ Int spos = save_pos();
  Field fld = goto_fld(ID_MSK);
  if (not (fld->props & FLD_BOL))
    left_fld(FLD_BOL, &fld);
  
  form_put(T_INT+W_ID+ID_MSK,   id,
           T_DAT+W_NAME+ID_MSK, name,
           T_DAT+W_RGTS+ID_MSK, uses,
           T_DAT+W_ADDR+ID_MSK, addr, 0);
  restore_pos(spos);
}



private void scroll_tbl(keytbl, sl, n)
	Int    keytbl[];
	Int    sl, n;
{ Int * s = &keytbl[n];
  Int i;
  memcpy(&keytbl[0], s, (Int)&keytbl[sl+1] - (Int)s);
  for (i = n - 1; i >= 0; --i)
    keytbl[sl - i - 1] = 0;
}



private Char * get_acc_no(acc_no_ref)
	 Cash *    acc_no_ref;			/* actually stored in Cash !! */
{ if (get_integer(acc_no_ref) != OK)
    return "Account Number Required";
  else
  { Key_t key;
    key.integer = *acc_no_ref;
  { Lra_t lra;
    Cc cc = ix_search(mainupn, IX_CUST, &key, &lra);
    return cc == OK ? null : (char*)ntfd;
  }}
}

private const Char * do_add(whix, ds)
	Quot   whix;
	Char * ds;
{ if (upn_ro)
    return (char*)rdoy;
  if (whix == IX_AGNT and not am_admin)
    return ymba;
  if (whix == IX_CUST and disallow(R_CUST))
    return preq('C');
  if (whix == IX_SUP and disallow(R_GIN))
    return preq('N');

  wopen(gentbl, whix == IX_AGNT ? SAover  :
            		whix == IX_CUST ? SI1over : SI2over);
  while (true)
  { Lra_t lra;
    Key_t  kvi, kv_t;
    Field fld = goto_fld(W_NAME+ID_MSK);
    Cc cc = get_data(&ds[0]);
    if (cc == HALTED)
    { goto_fld(W_CMD);
      (void)get_data(&ds[0]);
    }
    if (ds[0] == '.')
      break;
  { Cc ccc = cc != OK ? cc : get_sname(&ds[0], &kv_t);
    if (ccc != OK)
    { alert(*ds != 0 ? "invalid name" : dtex);
      continue;
    }

    if      (whix == IX_AGNT)
    { Agent_t ag;
      strcpy(ag.name, kv_t.string);
    { Agent agt = (Agent)ix_fetchrec(mainupn, IX_AGNTNM, &kv_t, null, null);
      if (agt != null)
      { alert(arth);
      	put_agnt(agt);
      	continue;
      }

      goto_fld(W_RGTS+ID_MSK);
      cc = get_data(&ds[0]);
      ag.props = asc_to_rgt(ds,false);	

      while (true)
      { fld = goto_fld(W_PW+ID_MSK);
      	(void)get_data(ds);
      	put_data("");

      { Int pw = asc_to_pw(ds);
      	salert("New Password Again");
      	(void)get_data(ds);
      	if (pw == asc_to_pw(ds))
      	{ ag.passwd = pw;
      	  put_data("");
      	  clr_alert();
      	  break;
      	}
      	alert("Different!!");
      }}

      wr_lock(mainupn);
      ag.id = last_id(mainupn, IX_AGNT) + 1;
      ag.tak_id = 0;
      put_agnt(&ag);
      cc = i_Agent_t(mainupn, &ag, &lra);
      wr_unlock(mainupn);
      if (cc != SUCCESS)
      	alert(iner);
    }}
    else

    if (whix == IX_CUST)
    { Tt_reg_t tt_reg;
      Customer_t ci;
      strpcpy(&ci.surname[0], kv_t.string, sizeof(Surname));

      while (true)
      { goto_fld(W_BACC+ID_MSK);
      	ci.bank_acc = 0;
      	get_data(ds);
      	if (get_ocref( &ci.bank_acc, ds) == OK)
      	  break;
      	alert("need Letter number");
      }

      goto_fld(W_TERMS+ID_MSK);
      get_data(ds);
      ci.terms = asc_to_terms(ds);
      terms_to_asc(ci.terms, &this_sale_terms[0]);
      if (ci.terms & M_BILL)
        salert("Account No to Bill");

      while (true)
      { Char * err;
      	goto_fld(W_CLIM+ID_MSK);
      	if ((ci.terms & M_BILL) == 0)
      	{ if (get_integer( &ci.creditlim) == OK)
      	  { ci.creditlim *= 100;
      	    break;
      	  }
      	  err = "cash amount required";
      	}
       	else
      	{ rd_lock(mainupn);
      	  err = get_acc_no(&ci.creditlim);
          rd_unlock(mainupn);
      	  if (err == null)
    	      break;
        }
      	alert(err);
      }

      goto_fld(W_FNAME+ID_MSK);
      get_data( &ci.forenames[0]);

      while (true)
      { goto_fld(W_PC+ID_MSK);
        get_data(ds);
        if (vfy_postcode(ds))
          break;
        alert(invf);
      }
      strpcpy(&ci.postcode[0], ds, sizeof(Postcode));

      while (true)
      { goto_fld(W_TEL+ID_MSK);
        get_data(ds);
        if (*vfy_str(ds, scats, E_TEL))
          break;  
        alert("digits only");
      }
  		strpcpy(&ci.tel[0], ds, sizeof(Tel));
  		goto_fld(W_ADDR+ID_MSK);
  		get_data( &ci.postaddr[0]);

      while (true)
      { goto_fld(W_TT+ID_MSK);
        get_data(ds);
        tt_reg.tt_id[0];
        if (ds[0] == 0)
          break;
      { char * s = extract_tt_ids(&tt_reg.tt_id[0],ds,false);
        if (s != NULL)
          break;  
        alert("TT_ID comma FAC_ID");
      }}

  		wr_lock(mainupn);
  	{ Lra_t 	 lra;
  		ci.id = last_id(mainupn, IX_CUST) + 1;
  		if (ci.id > 32000)
  			i_log(0, "Too many C's");
  		cc = i_Customer_t(mainupn, &ci, &lra);
  		if (cc != SUCCESS)
  			i_log(cc, "Corrupt Customer Area");
      if (tt_reg.tt_id[0] != 0)
      { tt_reg.cid = ci.id;
        cc = i_Tt_reg_t(mainupn,&tt_reg,&lra);
    		if (cc != SUCCESS)
    			i_log(cc, "Corrupt TT Reg Area");
      }

  		wr_unlock(mainupn);

      (void)goto_fld(W_NAME+ID_MSK);
  		left_fld(FLD_BOL, &fld);
  		put_cmr(&ci, &tt_reg, 0);
  	}}
  	else

  	{ Supplier_t ci; ci = null_sct(Supplier_t);
  		strpcpy(&ci.surname[0], kv_t.string, sizeof(Surname));

	  	ccc = ix_search(mainupn, IX_SUPDCN, &kv_t, &lra);
  		if (ccc == OK)
  		{ alert(arth);
  			continue;
  		}

  		goto_fld(W_ID+ID_MSK);

  		while ((cc = get_data(&ds[0])) != OK)
  			;

  		ci.barcode = atoi(ds);
  		if (not vfy_nninteger(ds))
  		{ alert(nreq);
  			continue;
  		}

  		kv_t.integer = ci.barcode;
  		cc = ix_search(mainupn, IX_MANBC, &kv_t, &lra);
  		if (cc == OK)
  		{ alert(arth);
  			continue;
  		}

  		while (true)
  		{ goto_fld(W_PC+ID_MSK);
  			get_data(ds);
  			if (vfy_postcode(ds))
  				break;
  			alert(invf);
  		}
  		strpcpy(&ci.postcode[0], ds, sizeof(Postcode));

  		while (true)
  		{ goto_fld(W_TEL+ID_MSK);
  			get_data( ds);
  			if (*vfy_str(ds, scats, E_TEL))
  				break;
  			alert("digits only");
	  	}
  		strpcpy(&ci.tel[0], ds, sizeof(Tel));
  		goto_fld(W_ADDR+ID_MSK);
  		get_data( &ci.postaddr[0]);

  		wr_lock(mainupn);
  		ci.id = first_neg_id(mainupn, IX_SUP) - 1;
  		if (ci.id > -100)
    		ci.id = -100;
  		cc = i_Supplier_t(mainupn, &ci, &lra);
  		if (cc < OK)
       	i_log(cc, "Corr S Area");
  		wr_unlock(mainupn);

  		left_fld(FLD_BOL, &fld);
  		put_sup(&ci,null);
  	}
  	if (fld->props & FLD_LST)
  		wscroll(enysz);
  	else
  		down_fld(FLD_BOL, &fld);
	}}
	return null;
}

private Char * do_del_agnt(lra, ag)
	 Lra_t	 lra;
	 Agent	 ag;
{ Agent_t ag_t; ag_t = *ag;
	if (not am_admin)
		return ymba;
	if (ag_t.id == 1)
		return "admin is permanent";

{ Cc cc = d_Agent_t(mainupn, &ag_t, lra);
	if (cc < OK)
		i_log(cc, "do_del_agnt:.");
	return cc == OK ? null : sodi;
}}



private Char * do_del_trim_cmrsup(whix, act, py, lra)
	 Quot 	whix; 		/* write locked */
	 Quot 	act;
	 Party	py;
	 Lra_t	lra;
{ Id	bill_acc = whix == IX_CUST ? py->customer.id
				 : py->supplier.id;
	Id	bill_acc_ = bill_acc;
	Set acc_terms;
	Cash bal = get_cacc(&bill_acc, &acc_terms, whix==IX_CUST ? IX_CUST : IX_SUP);

	Char * res = null;
	if (bal != 0 and bill_acc == bill_acc_)
		res = ahnz;
	else
	{ Int * p = whix == IX_CUST ? &py->customer.terms : &py->supplier.terms;

		if (act == P_DLTD or bal != 0)
		{ Char kb_ch = salerth("Press CR to Close the Account");
			if (kb_ch != A_CR)
      	return ndon;
			if			(*p== -1)
      	*p = P_DLTD;
			else if (*p & P_DLTD)
      	res = "Already Deleted";
		  else
      	*p |= P_DLTD;
			if (res == null)
				write_rec(mainupn, whix == IX_CUST ? CL_CUST : CL_SUP, lra, py);
		}
		else
		{ Cc cc = whix == IX_CUST ? d_Customer_t(mainupn, py, lra)
						: d_Supplier_t(mainupn, py, lra);
			if (cc < OK)
			{ i_log(cc, "ddtcmrsup");
        	res = iner;
			}
			else
			{ if (*p == -1)
      		*p = 0;
      	*p |= P_DLTD;
      	if (bill_acc == bill_acc_)
      		(void)del_acc(mainupn, bill_acc);
			}
		}
	}
	return res;
}

private Char * do_renum_py(whix, opy, d)
			Quot	 whix;		/* IX_AGNT, IX_CUST, IX_SUP */
			Party  opy;
			Char * d;
{ Key_t  kv_t;	kv_t.integer = atol(d);
	if (whix == IX_SUP)
		whix = IX_MANBC;
{ Lra_t lra;
	Cc cc = ix_search(mainupn, whix, &kv_t, &lra);
	if (cc == OK)
		return arth;

{ Key_t  okv;  
	okv.integer = whix == IX_MANBC ? opy->supplier.barcode
																 : opy->agent.id;
{ Cc cc = ix_delete(mainupn, whix, &okv, &lra);
	if (cc == OK)
	{ if (whix == IX_MANBC)
			opy->supplier.barcode = kv_t.integer;
		else
			opy->agent.id = kv_t.integer;
		(void)ix_insert(mainupn, whix, &kv_t, &lra);
		(void)write_rec(mainupn, whix == IX_AGNT ? CL_AGNT :
								 whix == IX_CUST ? CL_CUST : CL_SUP, lra, opy);
		if (whix == IX_CUST)
		{ Account_t ac_t;
			Account ac = (Account)ix_fetchrec(mainupn, IX_CACC, &kv_t, &lra, &ac_t);
			if (ac != null)
			{ (void)ix_delete(mainupn, whix, &okv, &lra);
      	(void)ix_insert(mainupn, whix, &kv_t, &lra);
      	ac->id = kv_t.integer;
      	write_rec(mainupn, IX_CACC, lra, ac);
			}
		}
	}
	return null;
}}}}

private Char * do_edit(whix, fld, keytbl, data)
	 Quot 	whix; 	/* IX_AGNT, IX_CUST, IX_SUP */
	 Field		fld;
	 Int			keytbl[];
	 Char * 	data;
{ const Char * error = null;
	Quot ftyp = s_fid(fld);
	Int row = 	s_frow(fld);
	Key_t  kv_t;	kv_t.integer = keytbl[row];
	if (kv_t.integer == 0 or row >= pty_sl)
		return (Char*)ngth;
	if (upn_ro)
		return (Char*)rdoy;

	wr_lock(mainupn);
{ Tt_reg_t tt_reg_t;
  Tt_reg   tt_reg = NULL;
  Lra_t lra;
	Party_t py_t;
	Party py = (Party)ix_fetchrec(mainupn, whix, &kv_t, &lra, &py_t);
	if			(py == null)
		error = sodi;
	else if (ftyp == W_ID)
	{ if			(toupper(*data) == 'D' and data[1] == 0)
		{ error = not am_admin		? ymba					:
      				whix == IX_AGNT ? do_del_agnt(lra, (Agent)py) :
			                        	do_del_trim_cmrsup(whix, P_DLTD, py, lra);
		}
		else if (in_range(*data, '0', '9'))
			error = not am_admin ? ymba : do_renum_py(whix, py, data);
		else
		{ error = "D deletes";
			put_integer(kv_t.integer);
		}
		if (whix == IX_AGNT)
			py = null;
	}
	else if (ftyp == W_RGTS)
		if (not am_admin)
			error = ymba;
		else
			py->agent.props = asc_to_rgt(data, false);	
	else if (ftyp == W_PW)
	{ Int pw = asc_to_pw(data);
		error = pw == py->agent.passwd or am_admin ? null : "Old password please";
		if (error == null)
		{ if (not am_admin)
			{ salert("New Password");
      	(void)get_data(data);
      	pw = asc_to_pw(data);
			}
			salert("New Password Again");
			(void)get_data(data);
			clr_alert();
			if (pw != asc_to_pw(data))
      	error = "Different!";
			else
      	py->agent.passwd = pw;
		}
	}
	else if (ftyp == W_NAME)
	{ if (this_agent.id != 1)
			error = ymba;
		else
		{ Cc cc = get_sname(data, &kv_t);
			if (cc != OK)
      	error = illk;
			else
			{ Quot ix = whix == IX_AGNT ? IX_AGNTNM :
			          	whix == IX_CUST ? IX_CUSTSN : IX_SUPDCN;
      	Char * onm = whix == IX_AGNT ? &py->agent.name[0] 	 :
            				 whix == IX_CUST ? &py->customer.surname[0] :
						                        	 &py->supplier.surname[0];
      	Surname name;
      	Key_t kvo; kvo.string = &name[0];
      	strpcpy(&name[0], onm, sizeof(Surname));
      	cc = ix_delete(mainupn, ix, &kvo, &lra);
			{ Cc cc_ = ix_insert(mainupn, ix, &kv_t, &lra);
      	if (cc != OK or cc_ != OK)
      		i_log(cc != OK ? cc : cc_, "DB ACS cons");
      	else
      		strpcpy(&onm[0], kv_t.string, sizeof(Surname));
			}}
		}
	}

	else if (ftyp == W_PC)
		strcpy(whix == IX_CUST ? &py->customer.postcode[0]
                  				 : &py->supplier.postcode[0], data);
	else if (ftyp == W_BACC)
	{ if (get_ocref(&py->customer.bank_acc, data) != OK)
			error = "need Letter number";
	}
	else if (ftyp == W_OWES)
		error = "thief!";
	else if (ftyp == W_CLIM)
		if (not vfy_integer(data))
			error = "price needed";
		else
			py->customer.creditlim = atoi(data) * 100;
	else if (ftyp == W_FNAME)
		strcpy(&py->customer.forenames[0], data);
	else if (ftyp == W_TERMS)
	{ if (py->customer.terms & P_DLTD and not old_bill_alt(py->customer.terms))
			error = cdel;
		else
		{ py->customer.terms = asc_to_terms(data);
			terms_to_asc(py->customer.terms, &this_sale_terms[0]);
			if (py->customer.terms & M_BILL)
			{ Id cid = py->customer.id;
      	Quot tms;
      	salert("Account to Bill");
      	error = py->customer.terms & M_BILL 
			                      		? get_acc_no(&py->customer.creditlim) :
      					get_cacc(&cid,&tms, whix) == 0 
                      					? null : ahnz;
			}
		}
	}
	else if (ftyp == W_INTEREST)
	{ char * s = data;
		double interest = atof(data);
		int intint = (int)(interest * 100.0 + 0.01);
		if			(interest < 0 or interest > 20.0)
			error = "Percentage Interest";
		else if (interest == 0)
		{ py->customer.interest_01 = 0;
		} 
		else
		{ Lra_t lra;
			Unique un = find_uniques(mainupn, &lra);
			py->customer.interest_01 = ((intint - un->interest_b_bp) / 5) | 1;
		}
		py->customer.props &= ~P_HIDEINT;
		for ( ; *s != 0; ++s)
			if (toupper(*s) == 'T')
				py->customer.props |= P_HIDEINT;
	}
	else if (ftyp == W_LEEWAY)
	{ int leeday = atoi(data);
		if (not vfy_integer(data) || leeday < 0)
			error = "Leeway days";
		else
			py->customer.loanlee = leeday;
	}
	else if (ftyp == W_TEL)
		strcpy(whix == IX_CUST ? &py->customer.tel[0]
                  				 : &py->supplier.tel[0], data);
	else if (ftyp == W_TT)
	{ Lra_t lrb;
		if (whix == IX_SUP)
			kv_t.integer = -kv_t.integer;
		tt_reg = (Tt_reg)ix_fetchrec(mainupn, IX_TT, &kv_t, &lrb, &tt_reg_t);

	{ char * e = extract_tt_ids(&tt_reg_t.tt_id[0], data, false);
    if      (e == NULL)
      error = "TT id comma FAC id";
		else if (tt_reg == null)
		{ tt_reg_t.cid = kv_t.integer;
    { Cc cc = i_Tt_reg_t(mainupn,&tt_reg_t,&lra);
      if (cc != SUCCESS)
        i_log(cc, "Corrupt TT Reg Area");
    }}
    else
    	(void)write_rec(mainupn,CL_TT_REG,lrb,(Char*)&tt_reg_t);
			
		py = null;
  }}
	else if (ftyp == W_ADDR)
		strcpy(whix == IX_CUST ? &py->customer.postaddr[0]
                  				 : &py->supplier.postaddr[0], data);
	else
		error = dtex;

	if (py != null)
	{ if (error != null)
			mend_fld();
		else
			(void)write_rec(mainupn,whix == IX_AGNT ? CL_AGNT :
                  						whix == IX_CUST ? CL_CUST : CL_SUP,lra,(Char*)py);
		if			(whix == IX_AGNT)
			put_agnt(py);
		else if (whix == IX_SUP)
			put_sup(py, tt_reg);
		else
		{ Id cid = py->id;
			Quot tms;
			terms_to_asc(py->customer.terms, &this_sale_terms[0]);
			put_cmr(py, tt_reg, get_cacc(&cid, &tms, IX_CUST));
		}
	}
	wr_unlock(mainupn);
	return (Char*)error;
}}

private Char * do_upd(whix, ds)
	 Quot 	whix;
	 Char * ds;
{ Int keytbl[60];
	Int i;
	for (i = 60; --i >= 0;)
		keytbl[i] = 0;

	if (whix == IX_CUST and disallow(R_CUST))
		return preq('C');
	if (whix == IX_SUP and disallow(R_GIN))
		return preq('N');

	int all = *ds == 'A' || *ds == 'a';
	if (all)
		++ds;

{ Key_t  kv_t;
	Cc cc = get_sname(ds, &kv_t);
	if (cc < OK)
		return illk;

	wopen(gentbl, whix == IX_AGNT ? SAover	:
            		whix == IX_CUST ? SI1over : SI2over);

{ Quot ix = whix == IX_AGNT ? IX_AGNTNM :
      			whix == IX_CUST ? IX_CUSTSN : IX_SUPDCN;

	Char * strm = ix_new_stream(mainupn, ix, cc == NOT_FOUND ? null : &kv_t);
	Party_t  py_t;
	Lra_t lra;
	Int lim = gentbl->tabledepth;
	Int lim_ = lim;
	Bool cont = true;
	Bool fst = true;
	const Char * error = null;

	Field fld = goto_fld(W_ID);

	while (cont)
	{ Party py;
    Tt_reg  tt_reg = null;

		while (lim > 0 and (py = (Party)ix_next_rec(strm, &lra, &py_t)) != null)
		{ keytbl[lim_-lim] = py->id;
      if (whix != IX_AGNT)
      { Key_t kvtt_t;
        Lra_t lrb;
        kvtt_t.integer = whix == IX_CUST ? py->id : -py->id;
        rd_lock(mainupn);
      { Tt_reg_t tt_reg_t;
        tt_reg = (Tt_reg)b_srchrec(mainupn,IX_TT,&kvtt_t,NULL);
        if (tt_reg != NULL)
        { tt_reg_t = *tt_reg;
          tt_reg = &tt_reg_t;
        }
        rd_unlock(mainupn);
      }}
			if			(whix == IX_CUST)
			{ Id cid = py->id;
      	Quot tms;
      	ths.acct.balance = get_cacc(&cid, &tms, IX_CUST);
  			if (!all && (py->customer.terms & P_DLTD) != 0)
					continue;
      	terms_to_asc(py->customer.terms, &this_sale_terms[0]);
			}
			else if (whix == IX_SUP)
			{ if (!all && (py->supplier.terms & P_DLTD) != 0)
					continue;

      	terms_to_asc(py->supplier.terms, &this_sale_terms[0]);
			}
			goto_fld(W_ID + lim_ - lim);
			if			(whix == IX_AGNT)
      	put_agnt(py);
			else if (whix == IX_SUP)
      	put_sup(py, tt_reg);
			else
      	put_cmr(py, tt_reg, ths.acct.balance);
			--lim;
		}

		if (fst)
		{ (void)goto_fld(W_ID);
			fst = false;
		}

		while (cont)
		{ if (error != null)
      	alert(error);

			error = null;
			cc = get_any_data( &ds[0], &fld);

			if (cc == HALTED)
			{ if (not (fld->props & FLD_LST))
      		error = nrsc;
      	else if (lim <= 0)
      	{ wscroll(enysz);
      		scroll_tbl(keytbl,lim_,1);
      		++lim;
      		break;
      	} 
			}
			else if (cc == PAGE_DOWN and py != null)
			{ wscroll((pty_sl-2)*enysz);
      	scroll_tbl(keytbl,lim_,lim_-2);
				(void)goto_fld(W_ID+2);
      	lim = lim_-2;
      	break;
			}
			else if (fld->id == W_CMD)
			{ Cc cc_ = sel_cmd(ds);
				error = "Help";
      	if (cc_ == CMD_HELP)
      	{ wclr();
					printf("\n\r\n\r"
"A      Post to Accounts\n\r"
"B      Do Takings\n\r"
"C      Edit Customers\n\r"
"D      Allow discounts\n\r"
"E      View Accounts\n\r"
"F      Make Duplicates\n\r"
"G      Report Statistics\n\r"
"H      Discount Invoices\n\r"
"I      Make Invoices\n\r"
"J      Change Invoice Prices\n\r"
"K      Change Stock Prices\n\r"
"L      See Supplier prices\n\r"
"M      Charge/Suggest interest\n\r"
"N      Goods Inwards\n\r"
"O      Edit Output\n\r"
"P      Configure Printers\n\r"
"Q\n\r"
"R      Make Credit Notes\n\r"
"S      Make Credit Sales\n\r"
"T      Tidy\n\r"
"U      Unix\n\r"
"V      Edit Stock\n\r"
"W      Minimum Rights\n\r"
"X\n\r"
"Y      Delete invoices\n\r"
							);
					hold();
					ds[0] = 0;
					cont = false;
    	  }
      	if (*strmatch(".", ds) == 0)
      		cont = false;
			}
			else if (cc == OK)
      	error = do_edit(whix, fld, keytbl, ds);
			else
      	error = "Help";
		}
		end_interpret();
	}
	ix_fini_stream(strm);

	return lim != pty_sl ? null : ngfd;
}}}

private Char * do_list(whix,	ds)
	 Quot 		whix;
	 Char * 	ds;
{ Key_t 	 kv_t;
	int all = *ds == 'A' || *ds == 'a';
	if (all)
		++ds;

	Cc cc = get_sname(ds, &kv_t);
	if (cc < SUCCESS)
		return illk;

	wopen(gentbl, whix == IX_AGNT ? SAover	:
            		whix == IX_CUST ? SI1over : SI2over);
{ Quot ix = whix == IX_AGNT ? IX_AGNTNM :
          	whix == IX_CUST ? IX_CUSTSN : IX_SUPDCN;

	Char * strm = ix_new_stream(mainupn, ix, cc != OK ? null : &kv_t);
	Lra_t lra;
	Party py;
	Int lim = pty_sl;
	Field fld = goto_fld(W_ID);

  while ((py = (Party)ix_next_rec(strm, &lra, null)) != null)
  { Tt_reg_t tt_reg_t;
	  Key_t kvi;	kvi.integer = py->id;
    Tt_reg tt_reg = null;
	{ Account acc = whix != IX_CUST
          			 ? null : (Account)ix_fetchrec(mainupn,IX_CACC,&kvi,null, null);
		Cash bal = acc == null ? 0 : acc->balance;

		if (whix != IX_AGNT)
    { Key_t kvtt_t;
      Lra_t lrb;
      kvtt_t.integer = whix == IX_CUST ? py->id : -py->id;
      rd_lock(mainupn);
      tt_reg = (Tt_reg)b_srchrec(mainupn,IX_TT,&kvtt_t,NULL);
      if (tt_reg != NULL)
      { tt_reg_t = *tt_reg;
        tt_reg = &tt_reg_t;
      }
      rd_unlock(mainupn);
    }

		if			(whix == IX_AGNT)
			put_agnt(py);
		else if (whix == IX_CUST)
		{ terms_to_asc(py->customer.terms, &this_sale_terms[0]);
			if (!all && (py->customer.terms & P_DLTD) != 0)
				continue;
			put_cmr(py, tt_reg, bal);
		}
		else
		{ if (!all && (py->supplier.terms & P_DLTD) != 0)
				continue;
			put_sup(py, tt_reg);
		}
		if (--lim > 0)
			down_fld(FLD_BOL, &fld);
		else
		{ Char kb_ch = salerth(crsd);
			if			(kb_ch == A_CR)
			{ wscroll(enysz);
      	lim = 1;
			}
			else if (kb_ch == ' ')
			{ wscroll((pty_sl-1)*enysz);
      	goto_fld(W_ID+1);
      	lim = pty_sl-1;
			}
			else
      	break;
		}
	}}

	salerth(lim == pty_sl ? ngfd : akey);
	ix_fini_stream(strm);
	return null;
}}

char * get_cust_sn(Id id, Surname tgt)

{ rd_lock(mainupn);
{ Lra_t lra;
	Customer_t py_t;
	Key_t  kv_t;	kv_t.integer = id;
{	int ix;
	Customer py = (Customer)b_fetchrec(mainupn, IX_CUST, &kv_t, &lra, &py_t);
	rd_unlock(mainupn);
	if (py == null)
		return NULL;

	strpcpy(tgt, py->surname, sizeof(Surname));
	return tgt;
}}}



char * get_email(Id cid, char * email, char * uses)

{ char * res = NULL;
	char * ln;
	char buf[140];
  FILE * ip = fopen("emails.txt", "r");
  if (ip == NULL)
    return NULL;
  
  while ((ln = fgets(buf, sizeof(buf), ip)) != NULL)
	{	if (atoi(ln) != cid)
			continue;
		
  	if (strlen(ln) <= EM_ADDR_OFFS)
  		continue;

	{	char * t = uses - 1;
		int clamp = 6;
	 	char * ue = ln + EM_USES_OFFS - 1;
	 	while (*++ue != 0 && *ue > ' ' && --clamp >= 0)
	 		*++t = *ue;
	 	*++t = 0;
  
  	res = strpcpy(email, ln + EM_ADDR_OFFS, 132);
	}}
	
	return res;
}



char * do_emails(const char * cname)

{ Key_t 	 kv_t;
	int all = *cname == 'A' || *cname == 'a';
	if (all)
		++cname;

	pty_sl = (g_rows - 12);
	gentbl = mk_form(SMform,pty_sl);

	SMover[3].row = 7 + pty_sl;
	wopen(gentbl, SMover);

{	Id s_id = 0;
	int num_new = 0;
	Int lim = gentbl->tabledepth;
	Int lim_ = lim;
	FILE * ip = popen("sort --key=1,2r emails.txt","r");
	FILE * op = fopen("/tmp/new_emails", "w");		/* must be made process unique */
	char buf[1024];
	char * ln;
	Bool cont = true;
	Bool fst = true;
	Id nid;
	char rights[8];
	char * emc = NULL;
	const Char * error = null;

	Field fld = goto_fld(W_ID);

	while (cont)
	{ Customer py;
		Id prev_id = 0;

	  while (lim > 0 && (ln = fgets(buf, sizeof(buf), ip)) != null)
		{	Surname sn;
			Id id = atoi(ln);
	  	int sl = strlen(ln);
	  	if (sl <= EM_ADDR_OFFS)
	  		continue;
	  	if (id == prev_id)
	  		continue;
	  		
			ln[sl-1] = 0;
	  	prev_id = id;
	  	if (s_id > 0)
	  	{ if (s_id != id)
		  		continue;
	  	}
	  	
		{	Surname sn_t;
			Char * sn = get_cust_sn(id, sn_t);
			if (sn == NULL)
				continue;
	  	
			if (s_id == 0)						// skipping to here
			{	int ix;
				for (ix = -1; cname[++ix] != 0 && toupper(cname[ix]) == toupper(sn[ix]); )
					;
	
			  if (cname[ix] != 0)
		  		continue;
		  }
  		
	  {	char * uses = ln + EM_USES_OFFS;
	  	char * ue;
	  	for (ue = uses - 1; *++ue != 0 && *ue > ' '; )
	  		;
	  	*ue = 0;

			goto_fld(W_ID + lim_ - lim);
	
			put_email(id, sn, uses, ln + EM_ADDR_OFFS);
			--lim;
		}}}

		if (fst)
		{ (void)goto_fld(W_NID);
			fst = false;
		}

		while (cont)
		{ if (error != null)
      	alert(error);

			error = null;
		{ Cc cc = get_any_data( &ds[0], &fld);

			if (cc == HALTED)
			{ if (not (fld->props & FLD_LST))
      		error = nrsc;
      	else if (lim <= 0)
      	{ wscroll(1);
       //	scroll_tbl(keytbl,lim_,1);
      		++lim;
      		break;
      	} 
			}
			else if (cc == PAGE_DOWN)
			{ wscroll((pty_sl-2)*1);
     //	scroll_tbl(keytbl,lim_,lim_-2);
				(void)goto_fld(W_ID+2);
      	lim = lim_-2;
      	break;
			}
			else if (fld->id == W_CMD)
			{ Cc cc_ = sel_cmd(ds);
				error = "Help";
      	if (cc_ == CMD_HELP)
      	{ wclr();
					printf("\n\r\n\r"
"I      Email Invoices\n\r"
"S      Email Statements\n\r"
							);
					hold();
					ds[0] = 0;
					cont = false;
    	  }
      	if (*strmatch(".", ds) == 0)
      		cont = false;
			}
			else if (cc != OK)
      	error = "Help";
			else if (fld->id == W_NID)
	    { nid = atoi(ds);
      	if (nid == 0)
      	{ error = "Need Customer Number";
      		continue;
      	}
	      	
			{	Surname sn_t;
				emc = get_cust_sn(nid, sn_t);
				if (emc == null)
      	{ error = "Cannot find customer";
					continue;
				}

      	goto_fld(W_NRGTS);
      	continue;
      }}
			else if (fld->id == W_NRGTS)
      {	if (emc == NULL)
      	{	goto_fld(W_NID);
      		continue;
      	}

			{	char * s = ds - 1;
				while (*++s != 0 && (toupper(*s) == 'I' || toupper(*s) == 'S'))
					;
				if (*s != 0)
				{ error = "I or S";
					continue;
				}

				strpcpy(rights, ds, sizeof(rights));

				goto_fld(W_NADDR);
				continue;
			}}
			else if (fld->id == W_NADDR)
      {	if (emc == NULL)
      	{	goto_fld(W_NID);
      		continue;
      	}

			{	int at = 0;
			  char * s = ds - 1;
				while (*++s > ' ')
				  if (*s == '@')
				  	at = 1;
				if (!at)
				{	error = "Need email address";
					continue;
				}
			{ char email[132];
				strpcpy(email, ds, sizeof(email));
				if (op != NULL)
				{	time_t tt = time(NULL); 
				 	struct tm * t = localtime(&tt);
				  ++num_new;
					fprintf(op,"%07d 20%02d%02d%02d%02d%2d %-5s %s\n", nid, 
									t->tm_year-100,t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min,
									rights, email);
				}
			{	Surname sn_t;
				Char * sn = get_cust_sn(nid, sn_t);
				if (sn != NULL)
				{	goto_fld(W_ID);
					put_email(nid, sn, rights, email);
					goto_fld(W_ID+1);
					put_eeol();
				}
				emc = NULL;
				goto_fld(W_NID);
				continue;
	    }}}}
		}}
		end_interpret();
	}

	if (ip != NULL)
		pclose(ip);
	if (op != NULL)
		fclose(op);
	if (num_new > 0)
	{	system("cat /tmp/new_emails >> emails.txt");
		system("rm  /tmp/new_emails");
	}
}}

const Char * do_prt_cmr(upn, ds, wh)
	 Id 			upn;
	 Char * 	ds;
	 Char 		wh;
{ Char str[50];
	Key_t 	 kv_t;

	Cc cc = get_sname(ds, &kv_t);
	Char * secname = skipspaces(&skipspaces(ds)[strlen(kv_t.string)]);
	if (*secname == '/')
		secname += 1;
	if (cc < SUCCESS)
		return illk;

	if (disallow(R_RPT))
		return preq('G');

{ Char * title = wh == 'D' ? "Debts from %s to %s at %s\n\n"
			                  	 : "Customers from %s to %s at %s\n\n";
	Prt chan = select_prt(0);
	if (chan == null)
		return aban;
	ipfld();
	sprintf(&msg[0], title, cc != OK			? "BEGINNING" : kv_t.string,
                  				*secname == 0 ? "END" 			: secname, todaystr);
{ Char * err = prt_one_report(chan, msg, "CUSTOMERS\n\n\n\n\n");
	if (err != null)
		return err;

	sprintf(&msg[0], "%s\n%s%s\n%s\n%s\n",
 " Id   Name                     Title", 
 wh != 'D' ? "     " : " Bal ", "        Tel. No. Post Code   Terms Credit Limit",
 "	 Address",	hline(75));
  if (wh != 'D')
    sprintf(&hhead[0], "%s\n%s\n%s\n",
 "  %5d   %r   %r",
 "               %12r       %8l         %12r          %12v",
 "                                                                           %78l"
         );
  else
    sprintf(&hhead[0], "%s\n%s\n%s\n",
 "  %5d   %r   %r",
 "               %12r       %8l         %12r          %12v        %9v",
 "                                                                           %78l"
         );
	prt_set_cols(msg, hhead);

{ Char * strm = ix_new_stream(upn, IX_CUSTSN, cc != SUCCESS ? null : &kv_t);
	Lra_t lra;
	fast Customer cu;
	Int ct = 0;
	Cash total = 0;

	while ((cu = (Customer)ix_next_rec(strm, &lra, null)) != null)
	{ Key_t kvi;	kvi.integer = cu->id;
		if (kvi.integer < 0)
			continue;
		if (*secname != 0 and strcmp(secname, cu->surname) < 0)
			break;
		if ((cu->terms & P_DLTD) and not old_bill_alt(cu->terms))
			continue;
		if (wh == 'D' and (cu->terms & M_BILL))
			continue;
	{ Cash cl = cu->creditlim;
		Account acc_ = (Account)ix_fetchrec(upn, IX_CACC, &kvi, null, null);
		Cash bal = acc_ == null ? 0 : acc_->balance;

		if (wh == 'D' and this_cut_date != 0)
		{ Cc cc = load_acc(upn, cu->id, &g_acc, &ths.acct, cu->terms, 0);
			if (cc == OK)
      	bal = g_acc.bal;
		}
		if (wh == 'D' and bal == 0)
			continue;
		if (wh == 'D' and (this_mindebt != 0 and bal <= this_mindebt))
			continue;
		if (wh == 'D' and this_min_excess != 0 and bal - cl < this_min_excess)
			continue;
		if (wh == 'D' and this_min_pcex != 0 and (bal-cl) * 100 < bal*this_min_pcex)
			continue;
		if (wh == 'D' and this_first_debt != 0)
		{ Cc cc = load_acc(upn, cu->id, &g_acc, &ths.acct, cu->terms, Q_UNPAID);
			if (this_first_due == 0 or this_first_due > this_first_debt)
      	continue;
		}

		ct += 1;
		if (ct == 1)
			prt_head();
		terms_to_asc(cu->terms, &str[0]);
		if (cu->terms & M_BILL)
		{ sprintf(&str[0], "Bill %d", cl);
			cl = 0;
		}
		if (wh != 'D')
			prt_row(0, cu->id, cu->surname, cu->forenames,
    					   cu->tel, cu->postcode, str, cl, cu->postaddr);
		else
			prt_row(0, cu->id, cu->surname, cu->forenames,
		    			   cu->tel, cu->postcode, str, cl, bal, cu->postaddr);
		total += bal;
	}}

	ix_fini_stream(strm);
	if (wh == 'D')
		prt_fmt("\nTotal of Balances				 %12v\n", total);

	prt_fmt(noes, ct);
	prt_fini_idle("");
	return ct > 0 ? null : ngfd;
}}}}

private const Char * do_partys(whix)
	 Quot 	whix;
{ if (whix != IX_AGNT and disallow(R_WEAK))
		return preq('W');

	if			(whix == IX_AGNT)
	{ SIsover[1].c = (Char*)agent;
		pty_sl = (g_rows - 7) / ENYDPTHA;
		enysz = ENYDPTHA;
		gentbl = mk_form(SAform,pty_sl);
	}
	else if (whix == IX_CUST)
	{ SIsover[1].c = (Char*)customer;
		pty_sl = (g_rows-6)/ENYDPTH;
		enysz = ENYDPTH;
		gentbl = mk_form(SIform,pty_sl);
	}
	else
	{ SIsover[1].c = (Char*)supplier;
		pty_sl = (g_rows - 6) / ENYDPTH2;
		enysz = ENYDPTH2;
		gentbl = mk_form(SI2form,pty_sl);
	}
	if (gentbl == null)
		return iner;

{ fast const Char * error = null;
	Bool leave = false;
	Bool refresh = true;

	while (not leave)
	{ if (refresh)
		{ const Char * s3over = optext[3];
			if (whix != IX_CUST)
      	optext[3] = null;
			refresh = false;
			wopen(seltbl, SIsover);
			put_choices(optext);
			optext[3] = s3over;
		}

		if (error != null)
			alert(error);
		error = dtex;

	{ Field fld;
		Cc cc = get_any_data(&ds[0], &fld);
		vfy_lock(mainupn);

		if (cc == HALTED)
		{ if (*ds == 0)
      	error = null;
		}
		else if (*strmatch(".", ds) == 0)
			leave = true;
		else
		{ Char * ds_ = &ds[0];
			Char ch = toupper(*ds_);
			if (in_range(ch, 'A', 'U'))
      	++ds_;
			refresh = true;
			switch (s_fid(fld)==W_SEL and ch==0 ? fld->id - W_SEL :
			      	in_range(ch, 'A', 'U')			? ch							: -1)
			{ case 0:
      	case 'A': error = do_add(whix,	ds_);
      	when 1:
      	case 'U': error = do_upd(whix,	ds_);
      	when 2:
      	case 'B': error = do_list(whix,  ds_);
      	when 3:
      	case 'L': error = whix != IX_CUST ? dtex : do_prt_cmr(mainupn,ds_,'C');
      						if (error != null)
      							salerth(error);
      						error = null;
      	when 4:
      	case 'D': error = whix != IX_CUST ? dtex : do_debts();
      	
      	when 5:
      	case 'M': error = do_emails(ds_);

      	otherwise refresh = false;
			      			error = illc;
			}
		}
	}}
	freem(gentbl);
	return null;
}}

static const char * g_eo_pfx = "QCGDLRE";

Cc print_eo_ids()

{ Char * strm = ix_new_stream(mainupn, IX_CUSTSN,null);
	Party py;
  Lra_t lra;

  while ((py = (Party)ix_next_rec(strm, &lra, null)) != null)
  { Tt_reg_t tt_reg_t;
	  Key_t kvi;	kvi.integer = py->id;
    Tt_reg tt_reg = null;
    Key_t kvtt_t;
    Lra_t lrb;
    kvtt_t.integer = py->id;
    rd_lock(mainupn);
  { Tt_reg tt_reg = (Tt_reg)b_srchrec(mainupn,IX_TT,&kvtt_t,NULL);
    if (tt_reg != NULL)
    { tt_reg_t = *tt_reg;
      tt_reg = &tt_reg_t;
    }
    rd_unlock(mainupn);
    if (tt_reg != NULL && strlen(tt_reg->tt_id) > 6)
    { Char tt_id[FIELD_TT+FIELD_FAC+2];
      (void)extract_tt_ids(tt_id,tt_reg == NULL ? NULL : &tt_reg->tt_id[0],true);
      printf("%d QCGDLRE%s\n", tt_reg->cid, tt_id);
    }
  }}
  
  return OK;
}

