#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "version.h"
#include "../h/defs.h"
#include "../h/errs.h"
#include "../h/prepargs.h"
#include "../form/h/form.h"
#include "../form/h/verify.h"
#include "../db/cache.h"
#include "../db/b_tree.h"
#include "../db/recs.h"
#include "domains.h"
#include "schema.h"
#include "rights.h"
#include "generic.h"

#define private 
#undef rnd

#define W_RND   (3 * ID_FAC)
#define W_DCN   (4 * ID_FAC)

#define W_TERMS (5 * ID_FAC)
#define W_DATE  (6 * ID_FAC)
#define W_PDATE (7 * ID_FAC)
#define W_SNAME (8 * ID_FAC)
#define W_FNAME (9 * ID_FAC)
#define W_CID   (10 * ID_FAC)
#define W_ADDR  (11* ID_FAC)

#define W_XID  (12 * ID_FAC)
#define W_XTYP (13 * ID_FAC)
#define W_XDAT (14 * ID_FAC)
#define W_XVAT (15 * ID_FAC)
#define W_XVAL (16 * ID_FAC)
#define W_XDUE (17 * ID_FAC)
#define W_XTOT (18 * ID_FAC)
#define W_TID  (19 * ID_FAC)

#define W_PC   (20 * ID_FAC)
#define W_OWES (21 * ID_FAC)
#define W_CLIM (22 * ID_FAC)
#define W_TEL  (23 * ID_FAC)

#define W_ADD  (25 * ID_FAC)
#define W_TOT  (26 * ID_FAC)
#define W_ODUE (27 * ID_FAC)
#define W_PDC  (28 * ID_FAC)
#define W_CONT (29 * ID_FAC)

#define W_STT  (5 * ID_FAC)
#define W_STP  (6 * ID_FAC)
//               was 13
#define SL (SCRN_LINES-12)

static const Char bal_msg[] = "Balance"; 
static       Char bnr[] = /* not const */
  " Id       Type     Date          Vat       Amount        Due     ";

static Over_t SIover [] = /* not const */
 {{0,1,1, 0,">"},
  {1,5,1, 0, bnr},
  {1,-7,1,15, hline(50)},
  {1,-7,1,0, ""},
  {0,-8,1,29, (char *)bal_msg},
  {0,-9,1,29, "Overdue"},
  {0,-10,1,29, "Held Cheques"},
  {-1,0,0,0,null}
 };

static const Fieldrep_t SIform [] = 
 {{W_MSG+1, FLD_RD+FLD_MSG,0,20,      60,  1, ""},
  {W_CMD,   FLD_WR,        1, 1,      20,  1, comd},
  {W_MSG,   FLD_RD+FLD_FD, 1,30,      30,  1, ""},
  {W_DATE,  FLD_WR,        1,68,      11,  1, stdt},
  {W_TERMS, FLD_WR,        2,59,       8,  1, "Terms"},
  {W_PDATE, FLD_WR,        2,68,      11,  1, "Unpaid date"},
  {W_CID,   FLD_WR,        3, 0,       8,  1, "Customer Id"},
  {W_SNAME, FLD_WR,        3, 9,  LSNAME,  1, "Name"},
  {W_FNAME, FLD_WR,        3,10+LSNAME,LFNAMES, 1, "Forenames"},
  {W_ADDR,  FLD_WR,        4,0,        LPADDR,  1, psta},

  {W_XID,   FLD_WR+FLD_BOL,6, 0,       8,  -SL, "Id"},
  {W_XTYP,  FLD_WR,        6, 9,       8,  -SL, "Type"},
  {W_XDAT,  FLD_WR,        6,18,      10,  -SL, da_te},
  {W_XVAT,  FLD_WR,        6,29,       9,  -SL, "Vat"},
  {W_XVAL,  FLD_WR,        6,39,      10,  -SL, amnt},
  {W_XDUE,  FLD_WR,        6,56,       9,  -SL, "Due"},
  {W_XTOT,  FLD_WR+FLD_EOL,6,66,      12,  -SL, bal_msg},

  {W_CONT,  FLD_RD,     -6, 9,   	8, 1, ""},
  {W_ADD,   FLD_WR+FLD_LST,-8,0,    25, 1, "Input"},
  {W_TOT,   FLD_RD,     -8, 42,      9, 1, ""},
  {W_ODUE,  FLD_RD,     -9, 42,      9, 1, ""},
  {W_PDC,   FLD_RD,     -10,42,      9, 1, ""},
  {0,0,0,0,0,1,null}
 };

#define SLr 5
#define ENYDPTH 3
#define SLb   SLr * ENYDPTH

static const Over_t SIrover [] = 
 {{0,1,1,0,">"},
  {0,1,1,71,"Start"},
  {1,3,1,0,
   " Id     Surname                          Postcode   Credit-limit   Owes"},
  {1,4,1,0,
   "       Forenames                             Terms    Telephone No "},
  {1,-6,1,0, "----------- Round "},
  {1,-6,1,18, hline(52)},
  {0,-8,1,49, (char*)bal_msg},
  {-1,0,0,0,"Round"}
 };

static const Fieldrep_t SIrform [] = 
 {{W_MSG+1, FLD_RD+FLD_MSG,0,20,      60,  1, ""},
  {W_CMD,   FLD_WR,        1, 1,      20,  1, comd},
  {W_MSG,   FLD_RD+FLD_FD, 1,30,      30,  1, ""},
  {W_RND,   FLD_WR,        2,0,        5,  1, "Round"},
  {W_DCN,   FLD_WR,        2,10,  LSNAME,  1, dcrn},
  {W_DATE,  FLD_WR,        2,68,      11,  1, stdt},

  {W_CID,   FLD_WR+FLD_BOL,5, 0,  4,   -SLr, "Customer Id"},
  {W_SNAME, FLD_RD,        5, 6,LSNAME,-SLr, "Name"},
  {W_PC,    FLD_RD,        5,40,LPCODE,-SLr, "Postcode"},
  {W_CLIM,  FLD_RD,        5,52,  8,   -SLr, "Credit limit"},
  {W_OWES,  FLD_RD,        5,64,  8,   -SLr, "Owes"},
  {W_FNAME, FLD_RD,        6, 8, 14,   -SLr, "Forenames"},
  {W_TEL,   FLD_RD,        6,23,LTEL,  -SLr,  teln},   
  {W_TERMS, FLD_RD,        6,24+LTEL,7,-SLr, "Terms"},
  {W_ADDR,  FLD_RD,        7, 4, 75,   -SLr,  psta},
  {W_BAN,   FLD_RD,	   -5,0,  80, 1,""},

  {W_TOT,   FLD_RD,    -5, 64,    8,  1, ""},
  {W_ODUE,  FLD_RD,    -8, 64,    8,  1, ""},
  {0,0,0,0,0,ENYDPTH,null}
 };

static const Over_t SIauover [] = 
{{0,1,1,0,">"},
 {1,3,1,0,  spaces(27)},
 {0,4,1,1, (char*)stdt},
 {0,5,1,1, "End Date"},
 {0,8,1,1, "Audit Files"},
 {1,15,1,0, "----- Audit Trail ---------"},
 {-1,0,0,0,null}
};

static const Fieldrep_t SIauform [] = 
 {{W_MSG+1, FLD_RD+FLD_MSG,0,20, 60,  1, ""},
  {W_CMD,   FLD_WR,        1, 1, 20,  1, comd},
  {W_MSG,   FLD_RD+FLD_FD, 1,30, 30,  1, ""},
  {W_STT,   FLD_WR,        4,20, 20,  1, stdt},
  {W_STP,   FLD_WR,        5,20, 20,  1, "End Date"},
  {W_DCN,   FLD_WR,        8,20, 20,  5, "Audit files"},
  {0,0,0,0,0,1,null}
 };

static const Over_t SIdover [] = 
{{0,1,1,0,">"},
 {1,3,1,0,  spaces(38)},
 {0,4,1,1, "Owes more than"},
 {0,5,1,3, "and"},
 {0,6,1,1, "Older than"},  {0,6,1,32, "(Takes Time)"},
 {0,7,1,3, "and"},
 {0,8,1,1, "Exceed Limit by"},
 {0,9,1,3, "and"},
 {0,10,1,1,"Exceed Limit by %"},
 {0,12,1,2,"At date"},  {0,12,1,32, "(Takes Time)"},
 {1,14,1,0, "-------- Debt Analysis  --------------"},
 {-1,0,0,0,null}
};

static const Fieldrep_t SIdform [] = 
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,  60,  1, ""},
   {W_CMD,   FLD_WR,        1, 1,  20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,30,  30,  1, ""},
   {W_XTOT,  FLD_WR,        4,19,  12,  1, "owed"},
   {W_STT,   FLD_WR,        6,19,  12,  1, da_te},
   {W_CLIM,  FLD_WR,        8,19,  12,  1, "Cash"},
   {W_OWES,  FLD_WR,       10,19,  12,  1, "Cash"},
   {W_DATE,  FLD_WR,	   12,19,  12,  1, da_te},

   {0,0,0,0,0,1,null}
  };

static const Over_t SIiover [] = 
{{0,1,1,0,">"},
 {0,3,1,1, "Generate Suggestions from"},
 {1,5,1,0, "-------- Interest Charging  -------------"},
 {0,6,1,1, "List from date"},
 {1,7,1,34, " Type y in the column to charge the interest"},
 {1,9,1,0, " Charge  Id         Name                From       To          Amount "},
 {0,24,1,1,  "Total Suggested"},
 {0,24,1,40, "Total Incurred"},
 {-1,0,0,0,null}
};

static const Fieldrep_t SIiform [] = 
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,  60,  1, ""},
   {W_CMD,   FLD_WR,        1, 1,  20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,30,  30,  1, ""},
   {W_DATE,  FLD_WR,	    3,30,  12,  1, da_te},
   {W_PDATE, FLD_WR,	    6,30,  12,  1, da_te},

   {W_CONT,  FLD_WR+FLD_BOL,10, 0,       1,  -SL, "Type y or d"},
   
   {W_XTYP,  FLD_RD,	    10, 2,	 1,  -SL, "T"},
   {W_XID,   FLD_RD,        10, 3,       8,  -SL, "Id"},
   {W_SNAME, FLD_RD,        10,13,      25,  -SL, "Name"},
   {W_XDAT,  FLD_RD,        10,40,       9,  -SL, "From"},
   {W_XDUE,  FLD_RD,        10,51,       9,  -SL, "To"},
   {W_XVAL,  FLD_RD,        10,62,      10,  -SL, amnt},

   {W_OWES,  FLD_RD,	    -11,17,   12,  1, "Total"},
   {W_XTOT,  FLD_RD,	    -11,58,   12,  1, "Total"},

   {0,0,0,0,0,1,null}
  };


static const Over_t SIpover [] = 
 {{0,0,1,50,"Enter Payment No or ."},
  {0,1,1,0,">"},
  {1,4,1,0, "  Id    Type  Customer Taking-Id    Date      Cheque-No  Amount "},
  {1,8,1,0, "----------- Payments "},
  {1,8,1,21, hline(46)},
  {-1,0,0,0,null}
 };


static const Fieldrep_t SIpform [] = 
 {{W_MSG+1, FLD_RD+FLD_MSG,0,20,  60,  1, ""},
  {W_CMD,   FLD_WR,        1, 1,  20,  1, ""},
  {W_MSG,   FLD_RD+FLD_FD, 1,30,  30,  1, ""},

  {W_XID,   FLD_RD,	   6, 0,   6,  1, ""},
  {W_XTYP,  FLD_RD,        6, 7,   8,  1, ""},
  {W_CID,   FLD_RD,        6,16,   5,  1, ""},
  {W_TID,   FLD_RD,	   6,22,   7,  1, ""},
  {W_XDAT,  FLD_RD,        6,30,  11,  1, ""},
  {W_XDUE,  FLD_RD,        6,41,  10,  1, ""},
  {W_XVAL,  FLD_RD,        6,52,  10,  1, ""},
  {0,0,0,0,0,1,null}
 };

#define MIN_ACC_SZ 2

typedef struct
{ Int      arr_sz;
  Int      curr_last;
  Int      curr_base;
  Id *     c;
  Bool     modified;
} Rnddescr_t, *Rnddescr;

static Round_t    this_round;

static Char       stt_date_str[12];
static Char       up_date_str[12];
static Bool       this_is_s;   /* this is a supplier */
static Quot       this_mode;    /* histogram or account */

static Rnddescr_t rnd;

Cash this_mindebt;
Date this_first_debt;
Cash this_min_excess;
Cash this_min_pcex;
Date this_debt_date;

Int first_match_grp;

static Tabled tbl1, tbl2;

static Bool ac_rd_only = false;

static const Char chna [] = "Customer has no account";

static int acc_sl,acc_r_sl;

Char * do_suggest(Id, Accdescr, char *);

private void init_forms()

{ acc_sl = g_rows-12;
  acc_r_sl = (g_rows-8)/ENYDPTH;
  if (tbl1 == null)
  { tbl1 = mk_form(SIform,acc_sl);
    tbl2 = mk_form(SIrform,acc_r_sl);
  }

  ths.acct = null_sct(Account_t);
  this_custmr = null_sct(Customer_t);

  today = asc_to_date("");
}



private void init_acc_cust()

{ this_custmr = null_sct(Customer_t);
}

private Char * get_atyp(props_, third)
	 Set  props_;
	 Bool third;
{ fast Set props = props_;
  Char *s = (props & K_DEAD) ? "GONE    gone    GONE" :
	    (props & K_SALE)    ? 
	    (props & K_ANTI)    ? 
	       (props & K_IP)    ? "cdt-ip  .?.?.   /?/?/"  :
	       (props & K_UNB)   ? "DISC    disc    DISCT"   :
	       (props & K_CASH)  ? "CRFND   crfnd   CREDIT" :
				   										 "CREDIT  credit  CREDIT" :
	       (props & K_IP)    ? "order   .?.?*   ORDER"  :
	       (props & K_UNB)   ? "BANK-CH bank-ch BANK-CH":
	       (props & K_CASH)  ? "INV     csale   INVOICE":
														   "INV     inv     INVOICE": 
	    (props & K_PAYMENT) ? 
	    (props & K_ANTI)    ? 
	       (props & K_INTCHG)? "INTR'ST intr'st INTR'ST" :
	       (props & K_INTSUG)? "INTSUG  intsug  INTR'ST" :
	       (props & K_PD)    ? "HELDCHQ heldchq HELDCHQ" :
	       (props & K_UNB)   ? "MAN-ADD man-add MAN-ADD" :
											         "RTN-CHQ rtn-chq RTN-CHQ" :
	       (props & K_PD)    ? "PD-CHQ  pd-chq  PD-CHQ"  :
	       (props & K_UNB)   ? "MAN-SUB man-sub MAN-SUB" :
	       (props & K_PAID)  ? "WRONG   receipt RECEIPT" : 
											         "ODD-PMT wrong   ODD-PMT" :
	   	(props & K_BFWD)    ? "BFWD    bfwd    BFWD"    :
												       "*?*?*   *?*?*   /?/?/";
  static Char gadesc[10];
  fast Char * t = &gadesc[0];
  if (third)
    return &s[16];
  else
  { for (s = &s[props & K_PAID ? 8 : 0]; *s > ' ' && t < gadesc+8; ++s)
      *t++ = *s;
    *t = 0;

    return &gadesc[0];
  }
}

private void put_acc_item(acc, ix, row)
	Accdescr acc;
	Int	 ix;
	Int      row;
{ Accitem it = &acc->c[ix];
  Int spos = save_pos();
  Char str[20];

  goto_fld( W_XID+row);
  wclreol();
  if (not (this_mode & K_HGRAM))
  {                  put_integer(abs(it->id));
    fld_put_data( W_XTYP+ID_MSK, get_atyp(it->props, false));
  }
  
  form_put(T_DAT+W_XDAT+ID_MSK, date_to_asc(&str[0], it->idate, 0),
           T_CSH+W_XVAT+ID_MSK, it->vat, 0);
{ Field fld = goto_fld(W_XVAL+ID_MSK);
  Int add = (((it->props & K_SALE) == 0) == ((it->props & K_ANTI) == 0)) * 6;
  if (not (this_mode & K_HGRAM))
    fld->pos += add;
		    put_cash(it->val);
  if (not (this_mode & K_HGRAM))
    fld->pos -= add;
  if (it->props & K_SALE)
    fld_put_data( W_XDUE+ID_MSK, date_to_asc(&str[0], get_idue(it), 0)) ;
  if (it->props & (K_INTCHG+K_INTSUG))
    fld_put_data( W_XDUE+ID_MSK, date_to_asc(&str[0], it->chq_no, 0)) ;

  if (not (this_mode & K_HGRAM))
  { goto_fld( W_XTOT+ID_MSK); put_cash(get_bal(acc, ix));
  }
  restore_pos(spos);
}}


private void put_cont(end)
	 Bool   end;
{ Int spos = save_pos();
  fld_put_data(W_CONT, end ? "" : "...");

  restore_pos(spos);
}

private void put_acc_cmr(upn, rnd, ix)
	Id       upn;
	Rnddescr rnd;
	Int      ix;
{      Cash bal = 0;
       Int spos = save_pos();
       Int row = ix - rnd->curr_base;

  init_acc_cust();
  if (rnd->c[ix] != 0)
  { rd_lock(upn);
    this_custmr.id = rnd->c[ix];
  { Id ix = this_custmr.id > 0 ? IX_CUST : IX_SUP;
    Customer ci = get_cust(upn, ix, this_custmr.id);
    if (ci == null)
      strpcpy(&this_custmr.surname[0], cdel, sizeof(Surname));
    else 
    { Key_t kvi;  kvi.integer = ci->id;
    { Lra_t  lraacc;
      Account acc = (Account)ix_srchrec(upn, IX_CACC, &kvi, &lraacc);
      bal = acc == null ? 0 : acc->balance;
    }}
    rd_unlock(upn);
  }}
  form_put(T_INT+W_CID+row,      abs(this_custmr.id),
           T_DAT+W_SNAME+ID_MSK, &this_custmr.surname[0],
           T_DAT+W_PC   +ID_MSK, &this_custmr.postcode[0],
           T_CSH+W_CLIM +ID_MSK, this_custmr.creditlim,
           T_CSH+W_OWES +ID_MSK, bal,
           T_DAT+W_FNAME+ID_MSK, &this_custmr.forenames[0],
           T_DAT+W_TEL  +ID_MSK, &this_custmr.tel[0],
           T_INT+W_TERMS+ID_MSK, this_custmr.terms,
           T_DAT+W_ADDR +ID_MSK, &this_custmr.postaddr[0], 0);
  restore_pos(spos);
}

private void put_acc_details()

{ Int spos = save_pos();
  if (ths.acct.props & K_GIN)
    strcpy(&this_custmr.forenames[0], "SUPPLIER");
  form_put(T_DAT+W_TERMS, this_sale_terms,
           T_DAT+W_DATE,  stt_date_str,
           T_DAT+W_PDATE, up_date_str,
           T_INT+W_CID,   abs(this_custmr.id),
           T_DAT+W_SNAME, this_custmr.surname,
           T_DAT+W_FNAME, this_custmr.forenames,
           T_DAT+W_ADDR,  this_custmr.postaddr, 0);
  restore_pos(spos);
}


private void put_rnd_details()

{ Int spos = save_pos();
  form_put(T_DAT+ W_DATE, stt_date_str,
           T_OINT+W_RND,  this_round.id,
           T_DAT+W_DCN,   this_round.name, 0);
  restore_pos(spos);
}



private void put_paymt_details(pmt)
	 Paymt   pmt;
{ Char dbuf[20];
  form_put(T_INT+W_XID,  pmt->id,
           T_DAT+W_XTYP, get_atyp(pmt->kind, false),
           T_INT+W_CID,  pmt->customer,
           T_INT+W_TID,  ((int)pmt->agent & 0xffff),
           T_DAT+W_XDAT, date_to_asc(&dbuf[0], pmt->date, 0),
           T_INT+W_XDUE, pmt->chq_no,
           T_CSH+W_XVAL, pmt->total, 0);
}

private void clr_acc_totals()

{ Int spos = save_pos();

  form_put(T_DAT+W_TOT,  "",
           T_DAT+W_ODUE, "",
           T_DAT+W_PDC,  "", 
           T_DAT+W_SNAME, "", 
           T_DAT+W_FNAME, "",
           T_DAT+W_ADDR,  "", 0);
  restore_pos(spos);
} 


private void put_acc_totals(acc)
      Accdescr  acc;
{ Int spos = save_pos();

  form_put(T_OCSH+W_ODUE, acc->odue,
           T_OCSH+W_PDC,  acc->pdc_bal,
           T_CSH+ W_TOT,  ths.acct.balance, 0);
  restore_pos(spos);
} 


private Int srch_acc(bal_ref, acc, date)
	 Cash  *  bal_ref;
	 Accdescr acc;
	 Date     date;		/* date to search for */
{ fast Accitem it = acc->c;
  fast Int ix = acc->curr_last; 

  if (date != 0);
    for ( ++ix; --ix >= 0; ++it)
      if (it->idate >= date)
        break;

  if (ix < MIN_ACC_SZ)
  { ix += MIN_ACC_SZ;
    it -= MIN_ACC_SZ;
    if (ix > acc->curr_last)
    { ix = acc->curr_last;
      it = acc->c;
    }
  }

  *bal_ref = get_bal(acc, ix);

#if 0
  return acc->curr_last - ix;
#else
{ Int res = acc->curr_last - ix;
  Int fmg = 255;  

  for ( ;--ix >= 0; ++it)
    if (it->match_grp != 0 and it->match_grp < fmg)
      fmg = it->match_grp;

  first_match_grp = fmg == 255 ? 0 : fmg;
  return res;
}
#endif
}

private Cc lddsp_acc(upn, acc, cid, terms)
	 Id       upn;
	 Accdescr acc;
 	 Id       cid;
 	 Set      terms;
{ wclrgrp();
  ipfld();
  clr_acc_totals();
  if (cid == 0)
    return NOT_FOUND;

{ Set cmd = this_mode & K_HGRAM ? Q_HGRAM : 0;
  Cc cc = load_acc(upn, cid, acc, &ths.acct, terms, cmd);
  if (cc == SUCCESS)
  { ac_rd_only = upn_ro;
    if (this_unpaiddate >= 0 and this_unpaiddate != ths.acct.unpaid_date)
    { ths.acct.unpaid_date = this_unpaiddate;
      wr_lock(upn);
      (void)write_rec(upn, CL_ACC, this_acclra, &ths.acct);      
      wr_unlock(upn);
    }

    init_curr(acc);
  { Cash bal;
    Int base = srch_acc(&bal, acc, ths.acct.prt_date);
    Int ix;
    date_to_asc(&stt_date_str[0], acc->c[base].idate, 0);
    date_to_asc(&up_date_str[0], ths.acct.unpaid_date, 0);

    acc->curr_base = base;
    acc->bfwd = ths.acct.bfwd;
    acc->bal = ths.acct.balance;

    for (ix = base; ix < base + SL and ix <= acc->curr_last; ++ix)
    {
      put_acc_item(acc, ix, ix - base);
    }
    put_cont(ix >= acc->curr_last);
    put_acc_details();
    put_acc_totals(acc);
  }}

  return cc;
}}



static Int dav_diff;

private Int diff_acct_version(upn, account)   
	 Id       upn;
	 Account  account;
{ Key_t kvi; kvi.integer = account->id;
  rd_lock(upn);
  dav_diff = 0;
{ Lra_t  lra;
  Account acct__ = (Account)ix_srchrec(upn, IX_CACC, &kvi, &lra);
  if (acct__ == null)
    i_log(dbcc, "lost an acc"); 
  else
    dav_diff = acct__->version - account->version;
  rd_unlock(upn);
  return dav_diff;
}}

#if ACCTXT

FILE * tafd;

public Cc ta_init_rdwr(wh)
	Quot  wh;
{ if (tafd != null)
    fclose (tafd);
  tafd = fopen("/tmp/accs", wh == 0 ? "r" : "w");
  return tafd != null;
}


public Cc write_text_acc(cid)

{ init_acc(&acc, &ths.acct);
{ Cc cc = load_acc(mainupn, cid, &acc, &ths.acct, 0, 0);
  fast Accitem iip = &acc.c[0];
       Accitem iiz = &acc.c[acc.curr_last];

  if (cc != OK)
    return cc;

  fprintf(tafd, "ACC %d %d %x %d\n",
		cid, ths.acct.version, ths.acct.props, ths.acct.balance);
  fprintf(tafd, "BFWD_DATE %d %d %d\n", ths.acct.bfwd_date,
  				      ths.acct.prt_date,
  				      ths.acct.bfwd);
  if (not (iip->props & K_BFWD))
    --iip;

  while (++iip <= iiz)
  { Id eid = iip->id;
    if (iip->props & K_PAID)
      eid |= 0x40000000;
    if (iip->props & K_PAYMENT)
      eid = -eid;
    fprintf(tafd, "%d ", eid);
  }

  fprintf(tafd, "0\n");
  return OK;
}}



public Cc read_text_acc()

{ init_acc(&acc, &ths.acct);
{ Int ct = fscanf(tafd, " ACC %d %d %x %d",
			      &ths.acct.id, &ths.acct.version, &ths.acct.props, &ths.acct.balance);
  if (ct <= 0)
    return NOT_FOUND;
  ct = fscanf(tafd, " BFWD_DATE %d %d %d", &ths.acct.bfwd_date,
  				            &ths.acct.prt_date,
  				            &ths.acct.bfwd);
  acc.id = ths.acct.id;
{ fast Accitem iip = &acc.c[-1];
  if (ths.acct.bfwd != 0)
  { ++iip;
    acc.curr_last += 1;
    iip->props = K_BFWD;
    iip->idate = ths.acct.bfwd_date;
    iip->val   = ths.acct.bfwd;
  }

  while (true)
  { Int eid;
    Int ct = fscanf(tafd, "%d", &eid);
    if (ct == 0)
      i_log(4, "rta");
    if (eid == 0)
      break;
    iip = extend_acc(&acc);
    *iip = null_sct(Accitem_t);
    iip->props = eid < 0 ? K_PAYMENT : K_SALE;
    iip->id    = eid < 0 ? -eid : eid;
    if (iip->id >= 0x40000000)		/* it's paid */
    { iip->props |= K_PAID;
      iip->id -= 0x40000000;
    }
  }
  recalc_acc(&g_acc);
  return OK;
}}}

#endif

					/* mark items paid */
public void match_credit(upn, cid)
	Id  upn;
	Id  cid;
{ Cc cc = cid == MAXINT ? 0 : load_acc(upn, cid, &g_acc, &ths.acct, 0, Q_UNPAID);
  if (cc != OK)
    i_log(cc, "C Create SAC for sale %ld", this_sale.id);
  else
  { Set set;
    Int matix = match_amount(&set, &g_acc, this_sale.total);
    if (matix >= MATCHSET)
    { Key_t kvi;  kvi.integer = this_sale.id;
    { Lra_t  lra;
      Sale_t sa_t;
      Sale sa = (Sale)ix_fetchrec(upn, IX_SALE, &kvi, &lra, &sa_t);
      if (sa != null)
      { sa->kind |= K_PAID;
				sa->match_grp = last_match_grp;
        (void)write_rec(upn, CL_SALE, lra, sa);      
      }
      this_sale.kind |= K_PAID;
    }}
  }
}



private Cash sum_cred(acc)
	Accdescr   acc;
{ Accitem iiz = &acc->c[0];
  Accitem iip = &acc->c[acc->curr_last+1];
  Cash res = 0;

  while (--iip >= iiz)
  { if (not (iip->props & (K_PAID + K_CASH + K_IP + K_PD)))
    { Cash v = iip->val;
      if (iip->props & (K_BFWD + K_SALE + K_GIN))
	v = -v;
      if (iip->props & K_ANTI)
	v = -v;
      if (v > 0)
	res += v;
    }
  }
  return res;
}

#define NO_BEFORE_PRT_DATE 2

typedef Char *  Chptr;

private Char * do_prt_acc(upn, acc, flagstr)
       Id       upn;
       Accdescr acc;
       Char *   flagstr;
{ Bool all = false;
  Int  spress = NO_BEFORE_PRT_DATE;
  Chptr parts[3];
	Char * p;
  Lra_t  lra; (void)find_uniques(upn, &lra);
	if (disallow(R_VACCT))
    return preq('E');

  while (*(flagstr = skipspaces(flagstr)) != 0)
  { if (toupper(flagstr[0]) == 'A')
      all = true;
    if (flagstr[0] == '-')
    { spress = atol(&flagstr[1]);
      while (in_range(flagstr[1], '0', '9'))
        ++flagstr;
    }
    ++flagstr;
  }

{ Char chead[250], cboard[LPADDR+4];
  Prt chan = select_prt('A');
  if (chan == null)
    return (char *)aban;

  chop3(&parts[0], strcpy(&cboard[0], this_custmr.postaddr), ',');
  fmt_data(&hhead[0], 
	"Osgood Smith %62!Account No %d\n"
	"98 Greenstead Rd.%20!%r %r\n"
	"Colchester       %20!%r%78!%l\n"
	"Essex   CO1 2SW. %20!%r\n%20!%r %r%78!%l\n",
     abs(this_custmr.id), this_custmr.forenames, this_custmr.surname, 
     parts[0], todaystr, parts[1], parts[2], 
     this_custmr.postcode, uniques.tel1);
{ Char * ln = hline(78);
  Char * err = prt_one_report(chan, hhead," CONTINUES\n\n\n\n\n\n\n\n\n\n\n");
  if (err != null)
    return err;

  sprintf(&chead[0], "%s\n%s\n%s\n",ln,
 "    Id     Type          Date        Vat      Debit     Credit    Balance", ln);
  prt_set_cols(chead, 
 "   %6d     %7l  %4r       %9r        %9v%r        %10v%r         %12v\n");

  prt_head();
  strcat(&hhead[0], " ACCOUNT CONTINUED\n");
  prt_set_title(hhead);  

{ Char paidstr[6], datestr[12], tostr[12]; 
  Cash bal = 0;
  Cash unalloc = sum_cred(acc);
  Vint supress = ths.acct.prt_date;		/* count of supressed entrys */
  Vint iter;
  Accitem iip;
  Accitem iiz = &acc->c[acc->curr_last];

  iter = supress == 0 ? 1 : 2; 
  supress = 0;
  				/* iter == 1 => supress goes up else down */
  for ( ;--iter >= 0; )
  {
    for (iip = &acc->c[-1]; ++iip <= iiz; )
    { fast Set props = iip->props;
  
      if (not (props & K_DEAD) and
      	  not (props & K_PD) and
  	  (all or (props & (K_SALE+K_CASH)) != (K_SALE+K_CASH)))
      { if (iter != 0)
        { if (iip->idate >= ths.acct.prt_date or iiz - iip <= 2)
      	    break;
      	  ++supress;
      	  continue;
	      }
      { Char * dscn = get_atyp(props, true);
    	  Char * mrk = props & K_CASH ? "CASH"  :
		                 props & K_PAID ? paidstr : "";
      	Bool add = ((props & K_PAYMENT) == 0) == ((props & K_ANTI) == 0);
    	  Char * spce  = add ? "" 	: spaces(10);
    	  Char * spce_ = add ? spaces(10) : "";
    	  Cash sval = props & K_CASH ? 0 : add ? iip->val : -(iip->val);
    	  Cash pbal = bal;
    	  Int match_grp = iip->match_grp - first_match_grp + 1;
    	  sprintf(paidstr, "*%02d*", match_grp<100 ? match_grp : match_grp-100);
      	bal += sval;
      	if (bal != iip->balance)
          i_log(acc->id, "pd %d %d %d %d", iip->id, iip->val, bal, iip->balance);
#if 0
      	if ((props & K_DUE) and mrk[0] == 0 and sval > 0 and sval > unalloc)
      	{ mrk = "DUE";
      	  unalloc -= sval;
      	}
#endif
      	if (supress <= 0)
      	{ if (supress == 0)
      	  { if (not (props & K_BFWD))
      	    { Char * spce  = pbal > 0 ? ""	   : spaces(10);
      	      Char * spce_ = pbal > 0 ? spaces(10) : "";
  
      	      date_to_asc(&datestr[0], iip[iip > acc->c ? -1 : 0].idate, 0);
	            prt_row(0, 0, "BfwD", "", datestr, 0, spce, pbal, spce_, pbal);
      	    }
      	  }
        { Cash val = iip->val;
          Cash vat = iip->vat;
          if (props & K_INTSUG) 
          { val = iip->vat;
            vat = 0;
          }
          if (props & (K_INTSUG+K_INTCHG)) 
          { if (val == 0)
              continue;
            if (props & K_INTSUG)
            { if (this_custmr.props & P_HIDEINT)
                continue;
            }
            else
            { strcpy(&tostr[0], "      ");
              date_to_asc(&tostr[2],iip->chq_no, 3);
              spce_ = tostr;
            }
          }

      	  prt_row(0, iip->id,dscn,mrk,date_to_asc(&datestr[0], iip->idate, 0),
		              vat,spce, val, spce_, bal);
          if (props & K_INTSUG)
          { char buf[30];
            int pc = iip->grace_ix / 10;
            prt_fmt("       @%d.%d%% From %r, Payable if overdue balance uncleared %r\n",
        	     pc, iip->grace_ix - pc * 10,
	             date_to_asc(&buf[0], iip->chq_no, 0),
	             date_to_asc(&datestr[0], today+uniques.loanlee, 0));
	        }
      	  if (props & K_ALIEN)
      	    prt_fmt("	       Paper Document: Ref.No. %d\n",iip->chq_no);
      	}}
      	--supress;
      }}
    }
    supress -= spress;
    if (supress < 0)
      supress = 0;
  }
  prt_need(7);
  prt_text(ln);
  prt_fmt("\n%35!Balance         %12v\n", bal);
  if (acc->odue > 0 and false)
    prt_fmt("%35!Due             %12v\n", acc->odue);

  prt_text("");
  prt_set_footer(ac_rd_only ? "PARTIAL ACCOUNT\n\n\n\n\n\n\n\n\n\n\n" :
			      "ACCOUNT\n\n\n\n\n\n\n\n\n\n\n");
  prt_fini_idle(NULL);
  return null;
}}}}

						/* write locked */

Cc do_upd_iips(Accdescr acc, Id id, Cash val)

{ Accitem iiz = &acc->c[acc->curr_last+1];
  Accitem iia = &acc->c[0];
  Accitem iip;

/*i_log(acc->id, "dui %d %d", id, val);*/

  for (iip = iiz; --iip >= iia; )
  { fast Set props = iip->props;
    if ((props & K_INTSUG) && iip->id == id)
      break;
  }
  if (iip < iia)
  { i_log(acc->id, "lost intchg %d", id);
    return -1;
  }

  iip->val = val;
  for (--iip; ++iip < iiz; )
    iip->balance += val;

{ Lra_t lra;
  Key_t kvi; kvi.integer = id;
{ Paymt_t pay_t;
  Paymt pay = (Paymt)ix_fetchrec(mainupn, IX_PMT, &kvi, &lra, &pay_t);
  if (pay->total != val)
  { i_log(id, "intchg changed %d %d", pay->chq_no, val);
    return -1;
  }
  else
  { pay->kind &= ~K_INTSUG;
    pay->kind |= K_INTCHG;
    (void)write_rec(mainupn, CL_PMT, lra, pay);
    do_adj_intbals(acc->id, id, val);
  }
  return OK;
}}}




private Cc do_unsugg(Id id)

{ Lra_t lra;
  Key_t kvi; kvi.integer = id;
{ Paymt pmt = (Paymt)ix_fetchrec(mainupn, IX_PMT, &kvi, &lra, null);
  if (pmt == null)
  { i_log(id, "Lost unsuggestion");
    return -1;
  }
  pmt->total = 0;
{ Cc cc = write_rec(mainupn, CL_PMT, lra, pmt);
  if (cc != OK)
    i_log(cc, "Wr Unsugg");
  return cc;
}}}



private Cc do_levy_int(Id id, Id cid, Date stt_date, Cash val)

{ Key_t kvi; kvi.integer = cid;
/*i_log(0, "do levy int %d %d %d", id, cid, val);*/
  wr_lock(mainupn);
{ Lra_t lra;
  Customer cust = (Customer)ix_fetchrec(mainupn, IX_CUST, &kvi, &lra, null);
  Accdescr_t ac_d;
  ac_d.c = null;
  init_acc(&ac_d, &ths.acct);
  ths.acct.id = cid;
{ Cc cc = cust==null ? -1 
                     : load_acc(mainupn, cid, &ac_d,&ths.acct, cust->terms,0);
  if (cc != OK)
    i_log(cc, "Failed reload levy acc");
  else
    cc = ac_d.odue == 0 ? HALTED : do_upd_iips(&ac_d, id, val);

  wr_unlock(mainupn);
  return cc;
}}}


					/* inherit this_custmr */

private Char * do_levy_acc(upn, acc, src, undo)
       Id       upn;
       Accdescr acc;
       Char *   src;
       Bool     undo;			/* dont levy but instead remove */
{ Id id = atoi(src);
  char ch = *skipspaces(src);
  if (ch != 0 && !vfy_integer(src))
    return "Syntax";
  if (disallow(R_ITRST))
    return preq('M');
  if (this_custmr.props & P_HIDEINT)
    return "Clear the t";

{ Accitem iip = &acc->c[acc->curr_last+1];
  Accitem iiz = &acc->c[0];
  Int ct = 0;

  for ( ; --iip >= iiz; )
  { // do_db(-1,-1); 
    fast Set props = iip->props;
    if ((props & K_INTSUG) && iip->vat != 0 && 
        (id == 0 || id < 0 ? ct < -id : iip->id == id))
    { Cash val = iip->vat;
      if (!undo && acc->odue == 0)
        return "Sorry, Balance paid in time";
      wr_lock(mainupn);
      if (undo)
        (void)do_unsugg(iip->id);
      else
      { Cc cc = lddsp_acc(mainupn, acc, acc->id, this_custmr.terms);
        if (cc != OK)
          i_log(cc, "Failed to reload acc");
        else
          cc = do_upd_iips(acc, id, val); 
        iip->val = val;
        iip->props &= ~K_INTSUG;
        iip->props |=  K_INTCHG;
      }
      wr_unlock(mainupn);
      iip->vat = 0;
      ++ct;
    { FILE * op = fopen("custint", "a");
      if (op != NULL)
      { Char buf[30];
        Cash itrst100 = val / 100;
        fprintf(op, "%s Cust Id %d %s\t: %d.%d%s", date_to_asc(&buf[0], today, 0),
      				              acc->id, this_custmr.surname, 
			       					      itrst100, val - itrst100*100,
      			 					      !undo ? "" : " WITHDRAWN");
        fclose(op);
      }
    }}
  }
  return null;
}}

private Char * do_pay(upn, cat, date, amount, chq_no, acc)
	Id	 upn;		       /* write locked */
	Quot	 cat;
	Date	 date;
	Cash	 amount;
	Int	 chq_no;
	Accdescr acc;
{ Id id = last_id(upn, IX_PMT) + 1;
  Int samt = cat & K_ANTI ? -amount : amount;
  Set set;
  Int matix = match_amount(&set, acc, samt);
  if (matix >= MATCHSET)
    cat |= K_PAID;

{ Paymt_t pmt;
  pmt.kind      = cat;
  pmt.id        = id;
  pmt.customer  = ths.acct.id;
  pmt.match_grp = set == 0 ? 0 : last_match_grp;
  pmt.agent     = 0;
  pmt.cat       = 0;
  pmt.date      = date;
  pmt.total     = amount;
  pmt.chq_no    = chq_no;
{ Cc cc = eny_to_accs(&pmt);

  if (cc == OK)
  { fast Accitem item  = &acc->c[acc->curr_last+1];
    item->props	    = cat;
    item->id	    = id;
    item->balance   = ths.acct.balance;
    item->match_grp = pmt.match_grp;
    pmt.balance   = ths.acct.balance;
    acc->odue -= samt; 
    if (acc->odue < 0)
      acc->odue = 0;
    at_xactn(&pmt);
  }
  return cc == OK ? null : iner;
}}}


private Char * do_foreign(upn, acc)
	 Id	  upn;			/* write locked */
	 Accdescr acc;
{ this_sale.id = ths.acct.props & K_GIN ? first_neg_id(upn, IX_SALE) - 1
					 : last_id(upn, IX_SALE) + 1;
  if (this_sale.id == 0)
    return "Int.Err.";
  if (this_sale.kind & K_ANTI)
    match_credit(upn, MAXINT);	
{ Cc cc = eny_to_accs(&this_sale);
  if (cc == OK)
  { fast Accitem item = &acc->c[acc->curr_last+1];
    item->props   = this_sale.kind;
    item->id	  = this_sale.id;
    item->balance = ths.acct.balance;
    this_sale.balance = ths.acct.balance;
    at_xactn(&this_sale);
  }
  return cc == OK ? null : (Char*)iner;
}}



private Id do_interest(upn, cid, from, to, int_rate, amount)
	 Id	  upn;			/* write locked */
	 Id	  cid;
	 Date     from, to;
	 double   int_rate;
	 Cash     amount;
{ wr_lock(upn);
{ Taking_t stake = this_take;
  Id id = last_id(upn, IX_PMT) + 1;
  Paymt_t pmt;
  pmt.kind      = K_PAYMENT+K_ANTI+K_INTSUG;
  pmt.id        = id;
  pmt.customer  = cid;
  pmt.match_grp = 0;
  pmt.agent     = 0;
  pmt.cat       = (Sbyte)((int_rate + 0.0004) * 1000);
/*i_log(0, "int_rate %8.4f %d", int_rate, pmt.cat);*/
  pmt.date      = to;
  pmt.total     = amount;
  pmt.chq_no    = from;

  this_take.id = INTEREST_AGT;
  
{ Cc cc = eny_to_accs(&pmt);

  this_take = stake;
  wr_unlock(upn);
  return cc != OK ? -1 : id;
}}}

private void get_sdetails(kind, amount, chq_no_ref)
	 Set	 kind;
	 Cash	 amount;
	 Int	*chq_no_ref;
{ this_sale = null_sct(Sale_t);
  this_sale.kind     = kind;
  this_sale.customer = ths.acct.id;
  this_sale.terms    = this_custmr.terms;
  this_sale.date     = today;
  this_sale.tax_date = today;
  this_sale.total    = amount;
{ Int spos = save_pos();
  Cash vat;
  Cc cc;
  clr_item(W_XID+acc_sl-2);
  clr_item(W_XID+acc_sl-1);
  goto_fld(W_XVAL+acc_sl-1);	       put_cash(amount);

  goto_fld(W_ADD);   
{ Int cat;
  salert("Sales Category");
  while ((cc = get_integer(&cat)) != OK)
    salert("Number 0 - 1000");

  this_sale.cat = cat;

  salert("Enter Reference No");
  while ((cc = get_integer(chq_no_ref)) != OK)
    salert("Ref No: Try Again");

  this_sale.chq_no   = *chq_no_ref;
  if (not (kind & K_UNB) and uniques.vatrate != 0)
  { this_sale.vat0_1pc = uniques.vatrate;
    salert("Enter Amount of VAT");
    while ((cc = get_money(&vat)) != OK)
      salert("VAT: Try Again");
    this_sale.vattotal = (vat * 1000) / uniques.vatrate;
  }
  restore_pos(spos);
}}}

private const Char * do_adj_acc(upn, acc, amount, wh)
	 Id	  upn;
	 Accdescr acc;
	 Cash	  amount;
	 Quot	  wh;
{ if (this_agent.id != 1)
    return ymba;
  if (wh & K_BFWD)
  { salert("Press / to clear all entries in Account");
    if (hold() != '/')
      return ndon;
    wr_lock(mainupn);
  { Cc cc = del_acc(upn, ths.acct.id);
    if (cc == OK)
      cc = create_account(upn,ths.acct.id);
    wr_unlock(mainupn);
    if (cc != OK)
      return iner;
  }}

  wr_lock(upn);
{ Key_t kvi; kvi.integer = ths.acct.id;
{ Lra_t  lra;
  Account ac__ = (Account)ix_fetchrec(upn, IX_CACC, &kvi, &lra, null);
  const Char * err;
  if (ac__ == null)
    err = ntfd;
  else 
  { if (wh & K_BFWD)
    { ac__->balance   = amount;
      ac__->bfwd      = amount;
      ac__->bfwd_date = today;
    }
    else
    { Cash inc = amount - ac__->balance;
      ac__->balance += inc;
      ac__->bfwd    += inc + this_a_diff;
      (void)update_bfwd(lra, inc + this_a_diff);
    }
  { Cc cc = write_rec(upn, CL_ACC, lra, ac__);
    if (cc == OK)
      cc = lddsp_acc(upn, acc, ths.acct.id, this_custmr.terms);
    err = cc == OK ? null : iner;
  }}
  wr_unlock(upn);
  return err;
}}}

#define K_ASALE (K_SALE+K_ALIEN)

private Char * do_acc_item(upn, acc, data)
	 Id	  upn;
	 Accdescr acc;
	 Char *   data;
{ static const Char * cmd[] = { "CHQ", "CASH",  "CHARGE", "CRE", 
				"ADD", "ADJ", "B52", "BOUNCE", 
				"DISC", "INV",	"SUB" };
  static const Set code[] = 
    { K_PAYMENT+K_CCHQ, K_PAYMENT, K_ASALE+K_UNB, K_ASALE+K_ANTI,
      K_PAYMENT+K_UNB+K_CCHQ+K_ANTI, K_TAKE, K_BFWD, K_PAYMENT+K_CCHQ+K_ANTI,
      K_ASALE+K_UNB+K_ANTI, K_ASALE, K_PAYMENT+K_UNB+K_CCHQ };
 
  Set cat = 0;
  Int ix;
  for (ix = sizeof(cmd) / sizeof(Char*); --ix >= 0; )
    if (*strmatch(cmd[ix], data) == 0)
    { cat = code[ix];
      break;
    }
{ Char * ds = skipalnum(data);
  Cash amount;
  Char * data_ = get_cash(ds, &amount);
  if (cat == 0 or data_ == ds)
    return "(add,bounce,cash,charge,chq,cre,disc,inv) amount"; 
  if (ths.acct.id == 0)
    return "Select an account first";
  if (this_custmr.terms & P_DLTD)
    return "Account is closed";
  if ((cat & (K_PAYMENT+K_UNB)) == K_PAYMENT+K_UNB and this_agent.id != 1)
    return ymba;
  if (disallow(R_ACCT) or cat == K_ASALE+K_UNB and disallow(R_DISC))
    return "Permits A, D Required";
{ Int chq_no = cat & K_CCHQ ? 0 : -1;  
  (void)get_qty(data_, &chq_no);

  if (cat & K_SALE)
    get_sdetails(cat, amount, &chq_no);

{ Char * res = null;
  if      (ths.acct.version != 0 and diff_acct_version(upn, &ths.acct) != 0)
    res = (Char *)-1;
  else if (cat & (K_TAKE | K_BFWD))
    res = (Char*)do_adj_acc(upn, acc, amount, cat);
  else 
  { fast Accitem item = extend_acc(acc);
    item->props  = cat;
    item->id	 = 0;
    item->idate  = today;
    item->vat	 = not (cat & K_SALE) 
		    ? 0 : (this_sale.vattotal * this_sale.vat0_1pc + 500)/ 1000;
    item->val	 = not (cat & K_SALE) ? amount : this_sale.total;
    item->chq_no = chq_no;
    item->balance= 0;
    clr_item(W_XID+acc_sl-2);
    put_acc_item(acc, acc->curr_last, acc_sl - 1);
    acc->curr_last -= 1;
    salert(pstc);
  { Char rdch;
    while ((rdch = hold()) != '/' and rdch != '.')
      ttbeep();
    clr_alert();
    clr_item(W_XID+acc_sl-1);
    if (rdch == '.')
      alert(aban);
    else 
    { wr_lock(upn);
      res = cat & K_SALE ? do_foreign(upn, acc) 
												 : do_pay(upn, cat, today, amount, chq_no, acc);

/*  	if (cat & (K_PAYMENT+K_UNB+K_CCHQ) = K_PAYMENT+K_UNB+K_CCHQ)
    	{ Accdescr acc_;

    	} */
    
      wr_unlock(upn);
      acc->curr_last += 1;
    }
  }}
  if (res == (Char *)-1 or 
      ths.acct.version != 0 and diff_acct_version(upn, &ths.acct) != 0)
  { (void)lddsp_acc(upn, acc, ths.acct.id, this_custmr.terms);
    i_log(ths.acct.id, "OPNI %ld %ld", ths.acct.version, dav_diff);
    return "Others posted new items!";
  }

  put_acc_totals(acc);
  return res;
}}}}

private Char * find_acc_item(acc, ds_)
	 Accdescr acc;
	 Char *   ds_;
{ if (not vfy_ointeger(&ds_[4]))
    return nong;
{ fast Int ix;
  Id id = atoi(&ds_[4]);
  if (id == 0)
  { acc->curr_base = 0;
    return null;
  }

  for (ix = acc->curr_base; ix <= acc->curr_last; ++ix)
    if (acc->c[ix].id == id or acc->c[ix].val == id)
    { acc->curr_base = ix;
      return null;
    }
  return ntfd;
}}



private Char * set_stt_date(ftyp, ds)
	 Quot    ftyp;
	 Char *  ds;
{ Char * date_str = ftyp == W_DATE ? stt_date_str : up_date_str;
  Date date;
  if (ds[0] == 0)
  { date = 0;
    date_str[0] = 0;
  }
  else 
  { date = asc_to_date(ds);
    if (date <= 0)
      return illd;
    date_to_asc(&date_str[0], date, 0);
    put_data(date_str);
  }

  wr_lock(mainupn);
{ Key_t kvi;  kvi.integer = this_custmr.id;
{ Lra_t  lraacc;
  Account_t ac_t;
  Account acct = (Account)ix_fetchrec(mainupn, IX_CACC, &kvi, &lraacc, &ac_t);
  if (acct != null)
  { if (ftyp == W_DATE)
      acct->prt_date = ths.acct.prt_date = date;
    else
      acct->unpaid_date = ths.acct.unpaid_date = date;
    (void)write_rec(mainupn, CL_ACC, lraacc, acct);
  }
  wr_unlock(mainupn);
  return null;
}}}

private Char * do_edit_adetails(upn, fld, acc, ds)
	 Id	  upn;
	 Field	  fld;	    /* W_SNAME or W_CID or W_DATE */
	 Accdescr acc;
	 Char *   ds;
{ Quot ftyp = fld->id;
  if	  (ftyp == W_CID)
  { Bool ok = vfy_integer(ds);
    if (not ok)
      return nreq;

  { Id cid = this_is_s ? -atoi(ds) : atoi(ds);
    Id ix = cid < 0 ? IX_SUP : IX_CUST;
    Customer cu = get_cust(upn, ix, cid);
    if (cu == null)
      this_custmr.id = cid;
    else
    { if (bill_alt(cu->terms))
      { sprintf(&ds[0], "Account is with %d", cu->creditlim);
	return ds;
      }
      terms_to_asc(this_custmr.terms, &this_sale_terms[0]);
    }
  }}
  else if (ftyp == W_SNAME)
  { Set props = (not this_is_s ? 0 : P_SUPPLR) + P_ACCONLY;
    this_custmr.id = get_one_cust(upn,ds, props, put_acc_details);
    vfy_lock(upn);
  }
  else 
  { Char * res = set_stt_date(ftyp, ds);
    if (res != null or this_custmr.id == 0)
      return res;
  }
{ Cc cc = lddsp_acc(upn, acc, this_custmr.id, this_custmr.terms);
  return cc == SUCCESS ? null : chna;
}}

Path at_arr[5];

					/* update account using an audit file */
private Char * au_ac(upn, cid, acc)
	 Id	    upn, cid;
	 Accdescr   acc;
{ Tabled tblat = mk_form(SIauform,0);
  if (tblat == null)
    return iner;
  strpcpy(&at_arr[0][0], this_at, sizeof(Path));
  
{ const Char * error = null;
  Date stt = 0;
  Date stp = today;
  Int i;

  wopen(tblat, (Over)SIauover);

  for (i = 0; i <= 4; ++i)
  { form_put(T_DAT+W_DCN + i, at_arr[i], 0);
  }

  goto_fld(W_STT);  

  while (true)
  { if (error != null)
      alert(error);
    error = null;

    ds[0] = 0;
  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);

    if	    (cc == OK and in_range(fld->id, W_STT, W_STP))
    { Date date = asc_to_date(ds);
      error = date > 0 ? null : illd;
      if (error == null)
				if (fld->id == W_STT)
				  stt = date;
				else 
				  stp = date;
		}
    else if (fld->id == W_CMD)
    { cc = sel_cmd(ds);
      error = "Go";
      
      if      (cc == CMD_GO)
      { Int ix;
				init_acc(acc, &ths.acct);
				ths.acct.id      = cid;
				ths.acct.version = 0;
				ac_rd_only = true;

				for (ix = 0; ix <= 4; ++ix)
				{ error = at_arr[ix][0] == 0 ? null : audit_acc(upn,cid,acc,at_arr[ix]);
				  if (error != null)
				  { goto_fld(W_DCN + ix);
				    break;
				  }
				}	   
      }
      else if (cc == CMD_DOT)
				break;	      
      else if (*ds == 0)
				error = null;
    }
    else if ((fld->id & ~ID_MSK) == W_DCN)
    { Int ix = fld->id & ID_MSK;
      strpcpy(&at_arr[ix][0], ds, sizeof(Path));
      put_data(at_arr[ix]);
    }
  }}
  ths.acct.balance = acc->bal;
  end_interpret();
  free(tblat);
  return null;
}}

private Char * mark_paid_up(it, cid, cmd)
	 					/* write locked */
	 Accitem it;
	 Id	 cid;
	 Char *  cmd;
{ Quot pup = cmd[0] != 'P' ? 0 : K_PAID;
  Int match_grp = atoi(&cmd[1]);
  Quot typ = it->props & K_BFWD    ? IX_CACC :
	     it->props & K_PAYMENT ? IX_PMT  : IX_SALE;
  Lra_t lra;
  Key_t kv_t; kv_t.integer = typ == IX_CACC ? cid : it->id;
{ Account_t sorp_t;
  fast Paymt ip__= (Paymt)ix_fetchrec(mainupn, typ, &kv_t, &lra, &sorp_t);
  if (ip__ == null)
    i_log(kv_t.integer, "Pay or Sale (%x) ntfd", typ);
  else 
  { Set styp = (typ == IX_CACC ? ((Account)ip__)->props : ip__->kind) & K_PAID;
    if (match_grp == 0)
      match_grp = next_match_grp();
    it->props &= ~K_PAID;
    pay_acceny(ths.acct.id, (it->props & (K_SALE+K_PAYMENT+K_BFWD)) | pup, 
							 it->id, match_grp);
    it->props |= pup;
    it->match_grp = match_grp;

    if (pup == 0 and styp == 0)
      return "Not Paid";
    if (pup != 0 and styp != 0)
      return alrp;

  }
  return null;
}}


					/* NOT_FOUND => call me back */
private Cc do_acc(upn, acc, ds)
	 Id 	upn;
	 Accdescr acc;
	 Char * 	ds;
{ Field fld;
	const Char * error = null;
	Int sl = g_rows - 12;

	init_acc_cust();
	init_acc(acc, &ths.acct);

	SIover[3].c = this_is_s ? "------ Supplier Account "
				: "------ Account ";
	wopen(tbl1, SIover);
	fld = goto_fld(W_SNAME);

	if (*ds != 0)
	{ goto_fld(W_CID);
		ac_rd_only = upn_ro;
		error = do_edit_adetails(upn, fld, acc, ds);
	}
{ Int row = sl - 1;
	Cc contcc = SUCCESS;

	while (contcc == SUCCESS)
	{
		while (row < sl-1 and ++row + acc->curr_base <= acc->curr_last)
		{
			put_acc_item(acc, acc->curr_base + row, row);
		}
		put_cont(acc->curr_base + row >= acc->curr_last);

		if (error != null)
			alert(error);
		error = null;

	{ Cc cc = get_any_data(&ds[0], &fld);
		Quot fld_typ = fld->id & ~ID_MSK;
		vfy_lock(upn);

		if (cc == PAGE_UP or cc == PAGE_DOWN)
		{ Int mv = cc == PAGE_DOWN ? sl - 1 : 1 - sl;
			Short base = acc->curr_base + mv;
			if (base + sl - 1 > acc->curr_last)
				base = acc->curr_last - sl + 2;
			if (base < 0)
				base = 0;
			mv = base - acc->curr_base;
			acc->curr_base = base;
			if (mv != 0)
			{ row = -1;
				wclrgrp();
			}
		}
		else if (cc == HALTED)
		{ if			(fld->props & FLD_FST)
			{ if (acc->curr_base > 0)
				{ wclrgrp();
					acc->curr_base -= 1;
					row = - 1;
				} 
			}
			else if (acc->curr_base + sl - 1 < acc->curr_last)
			{ wscroll(1);
				acc->curr_base += 1;
				row -= 1;
			}
		}
		else if (cc != OK)
			; 
		else if (fld_typ == W_SNAME or fld_typ == W_CID or 
			 fld_typ == W_DATE or fld_typ == W_PDATE)
		{ ac_rd_only = upn_ro;
			error = do_edit_adetails(upn, fld, acc, ds);
			if (error == null)
				row = sl - 1;
		}
		else if (this_mode & K_HGRAM)
			error = "This is a summary";
		else if (fld_typ == W_CMD)
		{ Cc cc_ = sel_cmd(ds);
			if			(cc_ == CMD_DOT)
				contcc = HALTED;
			else if (cc_ == CMD_PRINT)
				error = do_prt_acc(upn, acc, &ds[5]);
			else if (cc_ == CMD_LEVY)
				error = do_levy_acc(upn, acc, &ds[4], false);
			else if (cc_ == CMD_SUGGEST || cc_ == CMD_INTEREST)
				error = do_suggest(upn, acc, ds);
			else if (cc_ == CMD_UNSUGG)
				error = do_levy_acc(upn, acc, &ds[6], true);
			else if (cc_ == CMD_CHOP)
			{ if	(disallow(R_ACCT) or disallow(R_TIDY))
					error = "Permits A,T Required";
				else if (ac_rd_only)
					error = rdoy;
				else
				{ Lra_t  lra;
					Int b = atoi(&ds[5]);
					(void)find_uniques(mainupn, &lra);
					if (b == 0)
						b = uniques.grace[6] + 1;
					if (b <= uniques.grace[6])
					{ sprintf(&ds1[0], "At least %d days please", uniques.grace[6]);
						error = ds1;
					}
					else
					{ /* error = do_adj_acc(upn, acc, acc->bal, K_TAKE);
						if (error == null) */
						{ Int x = do_compact_acc(ths.acct.id, acc, today-b);
							sprintf(ds1, "%d Entries removed", x);
							salert(ds1); 
							acc->curr_base = 0;
							init_curr(acc);
							row = -1;
							wclrgrp();
						}
					}
				}
			}
			else if (cc_ == CMD_STATS)
			{ sprintf(ds1, "At %d out of %d Entries", acc->curr_base, acc->curr_last);
					salert(ds1); 
			}
			else if (cc_ == CMD_DELETE)
			{ if			(acc->id == 0)
					error = "No Account";
				else if (acc->bal != 0)
					error = "Balance must be zero";
				else
				{ salert("Press 'D' to delete account");
					Char ch = ttgetc(0);
					if (ch != 'D')
						salert("Cancelled");
					else
					{ Int x = do_delete_acc(acc);
						salert("Done");
					}
				}
			}
			else if (cc_ == CMD_FIND)
			{ error = find_acc_item(acc, ds);
				row = -1;
				if (error == null or row == -1)
				{ wclrgrp();
					(void)goto_fld(W_XID);
				}
			}
			else if (cc_ == CMD_CREATE)
			{ error = this_custmr.id == 0 			? "No Customer" 	 :
						ths.acct.version != 0 				? arth			 :
						create_account(upn,this_custmr.id) != OK	? "Error Creating"
																					:	null;
				if (error == null)
				{ Cc cc = lddsp_acc(upn, acc, this_custmr.id, this_custmr.terms);
					if (cc != OK)
						error = iner;
				}
			}
			else if (cc_ == CMD_AUDIT)
			{ Screenframe_t stk;
				push_form(&stk);
				error = this_custmr.id == 0 ? "No Customer"
																		: au_ac(upn, this_custmr.id, acc);
				pop_form(&stk);
				if (ac_rd_only)
				{ acc->curr_base = 0;
					init_curr(acc);
					row = -1;
					wclrgrp();
					put_acc_totals(acc);
				}
			}
			else if (cc_ == CMD_HELP)
			{ wclr();
				printf("\n\r\n\r"
							 "Audit 					Recover Audit Records\n\r"
							 "Create					Create Account\n\r"
							 "Delete					Delete account with zero balance\n\r"
							 "Find #					Amount in pence or Invoice/Payment number\n\r"
							 "Levy						Convert suggested interest payments into actual\n\r"
							 "Interest {date} show what an interest payment would be\n\r"
							 "								(prompted for date if none given and none existing)\n\r"
							 "Sugg {date} 		Suggest an interest payment\n\r"
							 "								(prompted for date if none given and none existing)\n\r"
							 "Unsugg {{-}#} 	Clear a suggested interest payment\n\r"
							 "		(nothing: all, -ve: last #, +ve: number of entry\n\r"
							 "Print {A}{-#} 	Print {All} {# entries from latest}\n\r"
							 "Chop #					Chop so that only # days remain in records\n\r"
							 "Stats\n\r"
							 "Press CR");
				hold();
				ds[0] = 0;
				return NOT_FOUND;
			}
			else 
				error = !*ds ? null : "Help,Create,Delete,Find,Levy,Sugg {date},Print {A}{-#},Chop{days}";
		}
		else if (fld_typ == W_ADD)
		{ error = ac_rd_only ? rdoy 			 : do_acc_item(upn, acc, ds);
			if (error == null)
			{ Int scroll_dist = acc->curr_last - acc->curr_base - sl + 1;
				if (scroll_dist > 0)
				{ wscroll(scroll_dist);
					acc->curr_base += scroll_dist;
				}
				row = - 1;
			}
		}
		else if (fld_typ == W_XID)
		{ Int irow = (fld->id & ID_MSK) + acc->curr_base;
			ds[0] = toupper(ds[0]);
			if			(ds[0] != 'P' and ds[0] != 'U')
				error = "Pn/U to mark it paid/unpaid";
			else if (irow > acc->curr_last)
				error = ngth;
			else if (ac_rd_only)
				error = rdoy;
			else 
			{ wr_lock(mainupn);
				error = mark_paid_up(&acc->c[irow], ths.acct.id, ds);
				wr_unlock(mainupn);
				if (error == null)
				{ put_acc_item(acc, irow, (fld->id & ID_MSK));
					if (not (fld->props & FLD_LST))
						down_fld(FLD_WR, &fld);
				}
			}
		}
		else 
			error = cndo;
	}}
	end_interpret();
	return contcc;
}}

				       /* Rounds Menu */
private void init_round(rnd)
      Rnddescr rnd;
{ rnd->curr_base = 0;
  rnd->curr_last = -1;
  rnd->modified  = false;
  wclrgrp();
}



private void clr_init_rnd(rnd)
      Rnddescr rnd;
{ init_round(rnd);
  this_round = null_sct(Round_t);
  put_rnd_details();
}




private Id *  extend_rnd(rnd, iip)
	Rnddescr   rnd;
	Id *	   iip;
{ Int offs = iip - rnd->c;
  Int size = rnd->arr_sz;
  if (rnd->curr_last < rnd->arr_sz)
    return iip+1;
  size = size > 1000 ? size + 1000 : (size + 1) * 2;
  rnd->arr_sz = size;
  rnd->c      = (Id *)realloc(rnd->c, (size + 1) * sizeof(Id));
  return &rnd->c[offs];
}

private Bool round_lock(yn, upn, id)
      Bool   yn;
      Id     upn, id;
{ Key_t kv_t;  kv_t.integer = id;
  wr_lock(upn);
{ Lra_t lra;
  Round rd__ = (Round)ix_fetchrec(upn, IX_ROUND, &kv_t, &lra, null);
  if (rd__ == null)
  { wr_unlock(upn); return false; }

{ Bool lkd = (rd__->kind & K_LOCK) != 0;
  if (yn)
    rd__->kind |= K_LOCK;
  else 
    rd__->kind &= ~K_LOCK;
  (void)write_rec(upn, CL_ROUND, lra, rd__);
  wr_unlock(upn);
  return lkd != (yn != 0);
}}}



private const Char * mark_rnd(upn, rnd)
      Id	upn;
      Rnddescr	rnd;
{ const Char * err = null;

  if (not rnd->modified)
  { if (this_round.id != 0 and not round_lock(true,upn,this_round.id))
    { err = seyl;
      if (this_agent.id == 1)
      { if (salerth("Somebody else editting, override?, CR") == A_CR)
	  err = null;
      }
    }
    if (err == null)	    
      rnd->modified = true;
  }
  return err;
}

				/* Auxilliary results */
Cash round_bal;


private const Char * lround(upn, rnd, id)	     /* must be locked */
	 Id	  upn;
	 Rnddescr rnd;
	 Id	  id;
{ Cash tot_bal = 0;
  Key_t kv_t;  kv_t.integer = id;
{ Lra_t lra;
  Round rd = (Round)ix_fetchrec(upn, IX_ROUND, &kv_t, &lra, &this_round);
  if (rd == null)
    return ntfd;

  init_round(rnd);
  if ((this_round.kind & K_LOCK) and this_agent.id != 1)
    return ssee;

{ Id * iip = &rnd->c[-1];
  iip[1] = 0;

  while (s_page(lra) != 0)
  { // do_db(-1,-1); 
    Round_t rnd_t;
    Round bkt_ = (Round)read_rec(upn, CL_ROUND, lra, &rnd_t);
    if (bkt_ == null)
    { i_log(0, "Corr. Round Area");
      return iner;
    }
    lra = bkt_->next & (Q_EFLAG-1);
    
  { Id * ip_ = &bkt_->customer[-1];

    while (++ip_ <= &bkt_->customer[RNDBUCKET-1] and *ip_ != 0)
    { Key_t kvi;  kvi.integer = *ip_;
    { Lra_t  lraacc;
      Account acct = (Account)ix_srchrec(upn, IX_CACC, &kvi, &lraacc);
      if (acct != null)
        tot_bal += acct->balance;
      rnd->curr_last += 1;    /* more output */
      iip = extend_rnd(rnd, iip);
      *iip = *ip_;
    }}
  }}
  put_rnd_details();
{ Int ix = rnd->curr_base-1;
  while (ix - rnd->curr_base < SLr-1 and ix < rnd->curr_last)
    put_acc_cmr(upn, rnd, ++ix);
  
  round_bal = tot_bal;
  return null;
}}}}



private const Char * load_rnd(upn, rnd, id)
	 Id	  upn;
	 Rnddescr rnd;
	 Id	  id;
{ rd_lock(upn);
{ const Char * res = lround(upn, rnd, id);
  rd_unlock(upn);
  return res;
}}

private const Char * get_one_round(upn, rnd)
	 Id	  upn;
	 Rnddescr rnd;
{ Char * strm = ix_new_stream(upn, IX_ROUND, null);
   Lra_t lra;
   Char rd_ch = '.';
   Round rd;

   while ((rd = (Round)ix_next_rec(strm, &lra, &this_round)) != null)
   { // do_db(-1,-1); 
     put_rnd_details();

     rd_ch = salerth(sc2d);
     if (rd_ch == A_CR)
       rd_ch = hold();
     if (rd_ch != ' ')
       break;
   }

   ix_fini_stream(strm);
   return rd_ch == A_CR ? null : load_rnd(upn, rnd, this_round.id);
}

private Char * save_rnd(upn, rnd, no)
	 Id	  upn;
	 Rnddescr rnd;
	 Int	  no;
{ if (not rnd->modified)
    return "nothing to save";
  this_round.next = 0;			/* for 2 arms below */
  wr_lock(upn);
{ Bool newrnd = this_round.id == 0;
  if (newrnd)
    this_round.id = no != 0 ? no : last_id(upn, IX_ROUND) + 1;

  this_round.kind &= ~K_LOCK;
{ Key_t kv_t; kv_t.integer = this_round.id;
{ Lra_t lra; 
  Cc cc = ix_search(upn, IX_ROUND, &kv_t, &lra);
  if (cc != OK)
  { lra = 0;
    cc = i_Round_t(upn, &this_round, &lra);
    if (cc != SUCCESS)
    { wr_unlock(upn); return "Corrupt Rounds area"; 
    }
  }
  else 
    if (no != 0)
    { wr_unlock(upn); return arth; }

{ fast Id * iip = &rnd->c[-1];
       Id * iiz = &rnd->c[rnd->curr_last];

  while (iip <= iiz)
  { // do_db(-1,-1); 
    Round_t rnd_t;
    Round bkt_ = (Round)read_rec(upn, CL_ROUND, lra, &rnd_t);
    fast Id * ip_ = &bkt_->customer[-1];

    if (bkt_ == null)
    { wr_unlock(upn); return "Corrupt Rounds area"; }

    memcpy(&bkt_->name[0], this_round.name, sizeof(Surname));
    bkt_->kind = this_round.kind;

    while (ip_ < &bkt_->customer[RNDBUCKET-1])
    { ++iip;			  /* more input */
      if (iip > iiz)
      { *(++ip_) = 0;
				break;
      }
      if (*iip != 0)
				*(++ip_) = *iip;
    }

    if (iip <= iiz and s_offs(bkt_->next) == 0)
    { Lra_t  lra_;
      this_round.customer[0] = 0;
      cc = new_init_rec(upn, CL_ROUND, &this_round, &lra_);
	    if (cc != OK)
  	    i_log(cc, "inierr"); 
      bkt_->next = lra_ | Q_EFLAG;
    }	     

    cc = write_rec(upn, CL_ROUND, lra, bkt_);
    if (cc != OK)
      i_log(cc, "rnderr"); 
    lra = bkt_->next & (Q_EFLAG-1);
  }
  wr_unlock(upn);
  if (newrnd)
  { goto_fld(W_RND);   put_integer(this_round.id);
    salerth("Note Round Number");
  }
  return null;
}}}}}

private const Char * do_add_call(upn, rnd, ix_, data)
	 Id	  upn;
	 Rnddescr rnd;
	 Int	  ix_;
	 Char *   data;
{ if (not vfy_nninteger(data))
    return "customer number required";

{ const Char * res = null;
  Int num = atoi(data);
  Customer cu = get_cust(upn, IX_CUST, num);
  if (cu == null)
    return ntfd;

  res = mark_rnd(upn, rnd);
  if (res == null)
  { (void)extend_rnd(rnd,null);

    rnd->curr_last += 1;
  { Int ix = ix_ < rnd->curr_last ? ix_ : rnd->curr_last;
    Int sz = rnd->curr_last - ix;
    Id * item = &rnd->c[ix];

    if (sz > 0)
      memcpy(&item[1], item, sz*sizeof(Int));

    *item = num;   
  }}
  return res;
}}

private const Char * do_edit_rdet(upn, fld, rnd, ds)
	 Id	  upn;
	 Field	  fld;
	 Rnddescr rnd;
	 Char *   ds;
{ const Char * res = null;
  Quot kind = s_fid(fld);
  switch (kind)
  { case W_DCN:   if (*skipspaces(ds) == 0)
								    res = "Blank is illegal";
								  else 
								  { strpcpy(&this_round.name[0], ds, sizeof(Surname));
								    res = mark_rnd(upn, rnd);
								  }
	  break;
    when W_RND:   res = not vfy_integer(ds) ? nreq  :
									rnd->modified	    ? dsfst : null;
								  if (res == null)
								    res = load_rnd(upn, rnd, atoi(ds));

	  break;
    when W_DATE:  res = set_stt_date(kind, ds);
  }
  if (res != null)
    put_rnd_details();
  return res;
}

private int prt_acc(upn, cid, acc, ct)
	 Id	  upn;
	 Id	  cid;
	 Accdescr acc;
	 Int      ct;
{ Char duemsg[40];
  Customer cu = get_cust(upn, IX_CUST, cid);
  if (cu == null)
  { init_acc_cust();
    sprintf(&this_custmr.surname[0], "Customer %d not found", cid);
    cu = &this_custmr;
  }
  ths.acct.balance = 0; 
{ Cc cc = load_acc(upn, cid, acc, &ths.acct, this_custmr.terms, 0);

  fmt_data(&duemsg[0],  cc == OK ? "       %11v" : "NO ACCOUNT", acc->odue);

  if (ct == 0)
    prt_head();
  prt_row(0, cu->id, cu->surname, ths.acct.balance, duemsg, cu->postaddr, 
	   cu->creditlim,  cu->tel, cu->postcode);
  return ct + 1;
}}



#define match_acc(acct) true

private Char * do_prt_rnd(upn, rnd, acc)
	 Id	  upn;
	 Rnddescr rnd;
	 Accdescr acc;
{ Prt  chan = select_prt(0);
  if (chan == null)
    return (Char*)aban;

{ Cash tot_bal = 0;
  Cash tot_due = 0;
  Char * lines =    "----------------------------       -----------";
  Int ct = 0;
  fmt_data(&msg[0], "%r\n                        %26l             %11r\n%r\n\n",
	                         lines, this_round.name, todaystr, lines);
{ Char * err = prt_one_report(chan, msg, "ROUND\n\n\n");
  if (err != null)
    return (Char*)err;

  sprintf(&msg[0], "%s%48.48s%9.9s\n %s\n%s\n%s\n", 
 " Id   Name", "Balance", "Due",
 adss,
 "  Credit Limit      Telephone Number    Post Code",
	  hline(79));
  prt_set_cols(msg,
 "  %5d                           %30l                 %11v         %11r\n"
 "%r\n"
 "           %9v                 %12r             %9l\n");
  
  if (rnd != null)
  { Id * iip = &rnd->c[-1];
    Id * iiz = &rnd->c[rnd->curr_last];

    while (++iip <= iiz)
    { ct = prt_acc(upn, *iip, acc, ct);
      tot_bal += ths.acct.balance;
      tot_due += acc->odue;
    }
  }
  else
  { rd_lock(upn);
    Char * strm = ix_new_stream(upn, IX_CUSTSN, null);
    Customer cust;
    Lra_t  lra;

    while ((cust = (Customer)ix_next_rec(strm, &lra, null)) != null)
    { Key_t kvi; kvi.integer = cust->id;
    { Account acct = (Account)ix_fetchrec(upn, IX_CACC, &kvi, &lra, null);

      if (acct != null and match_acc(acct))
      { ct = prt_acc(upn, acct->id, acc, ct);
        tot_bal += ths.acct.balance;
        tot_due += acc->odue;
      }

      rd_unlock(upn); rd_lock(upn);
    }}
    rd_unlock(upn);
    ix_fini_stream(strm);
  }

  prt_fmt(noes, ct);
  prt_fmt("\nTotal Owed          %10v\nTotal Overdue       %10v\n",
            tot_bal, tot_due);
  prt_fini(NULL);

  return ct != 0 ? null : (Char*)ngfd;
}}}

private Char * find_call(rnd, ds_)
	 Rnddescr rnd;
	 Char *   ds_;
{ Char * key = skipspaces(&ds_[4]);

  if (*key == 0)
  { rnd->curr_base = 0;
    return null;
  }
  if (not vfy_integer(key))
    return nreq;

{ Id id = atoi(key);
  Int ix;
  for (ix = 0; ix <= rnd->curr_last; ++ix)
    if (id == rnd->c[ix]) 
    { rnd->curr_base = ix;
      return null;
    }
  return ntfd;
}}



private Cc do_rnd(upn, rnd, acc, ds)
	 Id	  upn;
	 Rnddescr rnd;
	 Accdescr acc;
	 Char *   ds;
{ wopen(tbl2, (Over)SIrover);
  goto_fld(W_CMD);
  clr_init_rnd(rnd);

{      Int ix = rnd->curr_base-1;
       Cc contcc = SUCCESS;
  fast const Char * error = null;

  while (contcc == SUCCESS)
  { while (ix - rnd->curr_base < tbl2->tabledepth and ix < rnd->curr_last)
      put_acc_cmr(upn, rnd, ++ix);

    if (error != null)
      alert(error);
    error = dtex;

  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);
    Cc cm = sel_cmd(ds);
    vfy_lock(upn);
    
    if	    (fld->id == W_CMD)
    { Int ix_ = rnd->curr_base-1;
      if      (cm ==CMD_DOT or cm == CMD_SELECT)
      { ix_ = 0;
				if      (rnd->modified)
				  error = dsfst;
				else if (cm == CMD_DOT)
				  contcc = HALTED;
				else 
				{ error = get_one_round(upn, rnd);
				  if (error == null and this_round.id != 0)
				    error = load_rnd(upn, rnd, this_round.id);
				  
				  if (error == null)
				  { Int spos = save_pos();
				    goto_fld(W_TOT);
				    put_cash(round_bal);
				    restore_pos(spos);
				  }
				}
      }
      else if (cm == CMD_SAVE)
      { error = save_rnd(upn, rnd, atoi(&ds[4]));
				if (error == null)
				  clr_init_rnd(rnd);
			}
      else if (cm == CMD_DROP)
      { if (rnd->modified and this_round.id > 0)
				  (void)round_lock(false, upn, this_round.id);
				clr_init_rnd(rnd);
				error = null;
      }
      else if (cm == CMD_PRINT)
				error = do_prt_rnd(upn, rnd, acc);
      else if (cm == CMD_DELETE)
      { error = this_agent.id != 1                       ? ymba :
                this_round.id == 0                       ? ngth :
				del_round(upn, this_round.id) == SUCCESS ? null : "Failed";
				clr_init_rnd(rnd);
      }
      else      if (cm == CMD_FIND)
      { error = find_call(rnd, ds);
				if (error == null or ix == -1)
				  (void)goto_fld(W_CID);
      }
      else 
      { error = *ds == 0 ? null : "Select, Save, Save n, Print, Drop, Delete, or Find";
				ix_ = ix;
      }
      ix = ix_;
    }
    else if (cc ==PAGE_UP or cc == PAGE_DOWN)
    { Short base = rnd->curr_base + (cc == PAGE_DOWN ? 1 : -1)*tbl2->tabledepth;
      if (base + SLr - 1 > rnd->curr_last)
				base = rnd->curr_last - SLr +  2;
      if (base < 0)
				base = 0;      
      if (rnd->curr_base != base)
      { rnd->curr_base = base;
				ix = base - 1;
				wclrgrp();
      }
      error = null;
    }
    else if (cc == HALTED)
      if      (fld->props & FLD_FST)
				error = nrsc;
      else if (ix < rnd->curr_last)
      { wscroll(3);
				rnd->curr_base += 1;
				error = null;
      }   
	    else if (cc == OK)
	      if      (in_range(fld->id, W_RND, W_DATE))
					error = do_edit_rdet(upn, fld, rnd, ds);
	      else if (s_fid(fld) == W_CID)
	      { Int ix_ = rnd->curr_base + (fld->id & (ID_FAC - 1));
					if	(*ds == 0)
					{ error = ix_ <= rnd->curr_last ? null : ngth;
					  if (error == null)
					  { Int fid = fld->id;
					    sprintf(ds, "%d", rnd->c[ix_]);
					    while (do_acc(upn, acc, ds) == NOT_FOUND)
					      ;
					    ix = rnd->curr_base - 1;
					    wopen(tbl2, (Over)SIrover);
					    goto_fld(fid);
					    put_rnd_details();
					  }
					}
					else if (cm == CMD_D)
					{ error = ix_ <= rnd->curr_last ? null : ngth;
					  if (error == null)
					  { memcpy(&rnd->c[ix_],&rnd->c[ix_+1],(rnd->curr_last-ix_)*sizeof(Id));
					    rnd->curr_last -= 1;
					    wclrgrp();
					    ix = rnd->curr_base - 1;
					  }
					}
					else if (cm == CMD_I)
					  if (this_round.name[0] == 0)
					    error = "Description First";
					  else 
					  { salert("Customer No. to insert?");
					    get_data(&ds[0]);
					    error = do_add_call(upn, rnd, ix_, ds);
					    if (error == null)
				  	    ix = ix_ - 1;
					  }
					else if (toupper(ds[0]) == 'P')
					{ Id cid = rnd->c[ix_];
					  Customer cu = get_cust(upn, IX_CUST, cid);
					  Cc cc = load_acc(upn, cid, acc, &ths.acct, cu->terms, 0);
					  error = cc != OK ? chna
													   : do_prt_acc(upn, acc, &ds[1]);
					}
					else 
					  error = "(CR), (I)nsert number, (D)elete, (P)rint {A}";
      	}
	  }}
  end_interpret();
  return contcc;
}}

const Char * do_accounts(which)
	 Quot which;
{ this_mode = which;
  this_is_s = (which & K_GIN) != 0;
  this_take = null_sct(Taking_t);

  if (disallow(R_VACCT))
    return preq('E');

{ Char * vat = "";
  if (which & K_HGRAM)
  { for (vat = &bnr[0]-1; *++vat != 0;)
      if (*vat == 'V')
      { memcpy(&vat[0], "Payments  Sales    ", 18);
        break;
      }
  }

  init_forms();

  *ds1 = 0;
  while (do_acc(mainupn, &g_acc, ds1) == NOT_FOUND)
    ;
  if (vat[0] != 0)
    memcpy(&vat[0], "Vat       Amount    ", 18);
  return null;
}}  



const Char * do_rounds()

{ Id arr[40+1];
  rnd.arr_sz = sizeof(arr)/sizeof(arr[0]) - 1;
  rnd.c      = &arr[0];

  init_forms();
  do_rnd(mainupn, &rnd, &g_acc, ds1);

  return null;
}  

private const Char * do_debts()

{ fast const Char * error = null;
  Tabled  tbldbt = mk_form(SIdform,0);
  wopen(tbldbt, (Over)SIdover);
  goto_fld(W_XTOT); 

  while (true)
  { Int spos = save_pos();
    ds[0] = 0;
    if (this_first_debt != 0)
      date_to_asc(&ds[0], this_first_debt, 0);
    ds1[0] = 0;
    if (this_debt_date != 0)
      date_to_asc(&ds1[0], this_debt_date, 0);

    form_put(T_CSH+W_XTOT, this_mindebt,
             T_DAT+W_STT,  ds,
             T_CSH+W_CLIM, this_min_excess,
             T_INT+W_OWES, this_min_pcex,
             T_DAT+W_DATE, ds1, 0);

    restore_pos(spos);

    if (error != null)
      alert(error);
    error = null;

  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);
    Quot fld_typ = fld->id & ~ID_MSK;
    Cc cm = sel_cmd(ds);

    if	    (fld_typ == W_CMD)
    { if      (cm == CMD_DOT)
      	break;
      else if (cm == CMD_PRINT)
      { this_cut_date = this_debt_date;
      	error = do_prt_cmr(mainupn, &ds[5], 'D');
      	this_cut_date = 0;
      }
      else
      	error = "Print { start name { / stop name } }";
    }
    else if (cc == SUCCESS)
    { if      (fld_typ == W_XTOT)
      { Char * data_ = get_cash(ds, &this_mindebt);
      	if (data_ <= ds)
      	  error = amip;
      }
      else if (fld_typ == W_STT or fld_typ == W_DATE)
      { Date date = ds[0] == 0 ? 0 : asc_to_date(ds);
      	if (date < 0)
      	  error = illd;
      	else 
      	  if (fld_typ == W_STT)
      	    this_first_debt = date;
      	  else
      	    this_debt_date = date;
      }
      else if (fld_typ == W_CLIM)
      { Char * data_ = get_cash(ds, &this_min_excess);
				if (data_ <= ds)
				  error = amip;
      }
      else if (fld_typ == W_OWES)
      { Cc cc = to_integer(ds, &this_min_pcex);
				if (cc != OK)
				  error = "percentage";
      }
    }
  }}
  end_interpret();
  freem(tbldbt);
  return null;
}


void put_levy_item(Paymt pmt, Int row)

{ Int spos = save_pos();
  Char buf1[30], buf2[30], buf3[30], buf4[30];

  Key_t kv_t;  kv_t.integer = pmt->customer;
{ Lra_t lra;
  Customer_t cu_t;
  Customer cu = (Customer)ix_fetchrec(mainupn, IX_CUST, &kv_t, &lra, &cu_t);
  if (cu == null)
    sprintf(&cu_t.surname[0], "Cust No %d", kv_t.integer);

  goto_fld( W_XID+row); 

  form_put(T_INT+W_XID+ID_MSK, pmt->id,
           T_DAT+W_XTYP+ID_MSK, cu_t.props * P_HIDEINT ? "T" : "",
           T_DAT+W_SNAME+ID_MSK, cu_t.surname,
           T_DAT+W_XDAT+ID_MSK, date_to_asc(&buf1[0], pmt->chq_no, 0),
           T_DAT+W_XDUE+ID_MSK, date_to_asc(&buf2[0], pmt->date, 0),
           T_CSH+W_XVAL+ID_MSK, pmt->total, 0);
  restore_pos(spos);
}}



Char * do_all_add_int(Id, Date);


private Char * do_run_int()

{ Date list_date = today - 90;
  Int row = - 1;
  Char st_arr[SL];
  Date date_arr[SL];
  Id id_arr[SL];
  Id cid_arr[SL];
  Cash val_arr[SL];
  fast const Char * error = null;
  Tabled  tbldbt = mk_form(SIiform,0);
  Date from_date = today-30;

  if (disallow(R_ITRST))
    return preq('M');

  wopen(tbldbt, (Over)SIiover);
  date_to_asc(ds, list_date, 0);
  form_put(T_DAT+W_PDATE, ds, 0);

{ Lra_t lra;
  Unique un = find_uniques(mainupn, &lra);
  Key_t kvi; kvi.integer = uniques.first_sugg;
{ Char * strm = ix_new_stream(mainupn, IX_PMT, null);
  Paymt_t p_t;
  Paymt pmt = (Paymt)4;
  Id first_pid = 0;
  Cash gtotal = 0;
  Cash etotal = 0;		/* too early */
  Date allow_date = today - uniques.loanlee;
  Bool redo_first = false;
  Id fsugg = 0; /* uniques.first_sugg;*/

  salert("In Progress");

  while (true)
  { // do_db(-1,-1); 
    rd_lock(mainupn);
    pmt = (Paymt)ix_next_rec(strm, &lra, &p_t);
    rd_unlock(mainupn);
    if (pmt == null)
      break;
    if ((pmt->kind & K_INTSUG) == 0)
      continue;
    if (pmt->chq_no == 0)
    { i_log(pmt->id, "False IntSug");
      pmt->kind &= ~K_INTSUG;
      wr_lock(mainupn);
      write_rec(mainupn, CL_PMT, lra, pmt);
      wr_unlock(mainupn);
      continue;
    }
    if (first_pid == 0)
      first_pid = pmt->id;
    gtotal += pmt->total;
    if      (pmt->date < allow_date)
      etotal += pmt->total;
  } 

  if (first_pid != 0)
    first_pid -= 1;
  kvi.integer = first_pid;
  ix_fini_stream(strm);
     
  form_put(T_CSH+W_XTOT, gtotal, T_CSH+W_OWES, gtotal-etotal, 0);
  goto_fld(W_DATE); 
  
  strm = ix_new_stream(mainupn, IX_PMT, &kvi);
  pmt = (Paymt)4;

  while (true)
  { salert("In Progress");
    while (row < SL-1 && pmt != null)
    { // do_db(-1,-1); 
      rd_lock(mainupn);
      pmt = (Paymt)ix_next_rec(strm, &lra, &p_t);
      if (pmt == null)
      { rd_unlock(mainupn);
        break;
      }
      
      if ((pmt->kind & K_INTSUG) && pmt->total != 0 && pmt->date >= allow_date)
      { st_arr[++row] = 0;
        id_arr[row] = pmt->id;
        date_arr[row] = pmt->date;
        cid_arr[row] = pmt->customer;
        val_arr[row] = pmt->total;
        put_levy_item(pmt, row);
      }
      rd_unlock(mainupn);
    }
    salert("Ready");
    
  { Int spos = save_pos();
    date_to_asc(&ds[0], from_date, 0);

    form_put(T_DAT+W_DATE+ID_MSK, ds, 0);

    restore_pos(spos);

    if (error != null)
      alert(error);
    error = null;

  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);
    Quot fld_typ = fld->id & ~ID_MSK;
    Cc cm = sel_cmd(ds);

    if (cc == PAGE_DOWN)
    { Int ix;
      for (ix = -1; ++ ix < SL; )
        clr_item(W_CONT+ix);
      row = -1;
    }
    else if (fld_typ == W_CMD)
    { if      (cm == CMD_DOT)
      	break;
      else if (cm == CMD_SUGGEST)
      { error = do_all_add_int(mainupn, from_date);
        row = -1;
        ix_fini_stream(strm);
        strm = ix_new_stream(mainupn, IX_PMT, null);
      }
      else
      	error = "Sugg";
    }
    else if (cc == SUCCESS)
    { if (fld_typ == W_DATE || fld_typ == W_PDATE)
      { Date date = ds[0] == 0 ? 0 : asc_to_date(ds);
      	if (date < 0)
      	  error = illd;
      	else
      	  if (fld_typ == W_DATE)
      	  { if (date > today - 5)
      	    { error = "Too late";
      	      date = from_date;
      	    }
      	    from_date = date;
      	  }
      	  else
      	  { strm = ix_new_stream(mainupn, IX_PMT, null);
      	    row = -1;
      	  }
      	date_to_asc(ds, date, 0);
      	form_put(T_DAT+fld_typ+ID_MSK, ds);
      }
      else if (fld_typ == W_CONT)
      { Int irow = (fld->id & ID_MSK);
        if (irow <= row)
        { Cc cc = OK;
          if      (st_arr[irow])
            salert("Already Done");
          else if (toupper(ds[0]) == 'Y')
          { cc = do_levy_int(id_arr[irow],date_arr[irow],cid_arr[irow],val_arr[irow]);
            salert(cc == OK     ? "OK" 			:
                   cc == HALTED ? "Sorry, paid in time" : "Error");
            if (id_arr[irow] - fsugg > 1000)
              redo_first = true;
            st_arr[irow] = 'I';
          }
          else if (toupper(ds[0]) == 'D')
          { wr_lock(mainupn);
            cc = do_unsugg(id_arr[irow]);
      	    wr_unlock(mainupn);
      	    if (cc != OK)
      	      salert("Failed");
      	    else
              st_arr[irow] = 'D';
          }
      	  if (st_arr[irow] != 0)
      	  { goto_fld( W_CONT+irow); 
      	    form_put(T_DAT+W_CONT+ID_MSK, st_arr[irow] == 'I' ? "Y" : "D");
      	  }
        }
      }
    }
  }}}
  
  ix_fini_stream(strm);

  if (redo_first)
  { kvi.integer = fsugg;
    fsugg = 0;
    strm = ix_new_stream(mainupn, IX_PMT, &kvi);
    rd_lock(mainupn);
    while ((pmt = (Paymt)ix_next_rec(strm, &lra, &p_t)) != null)
    { // do_db(-1,-1); 
      if ((pmt->kind & K_INTSUG) && pmt->total != 0)
      { fsugg = pmt->id;
        break;
      }
    }
    if (pmt == null)
    { Cc cc = b_last(mainupn, IX_PMT, &kvi, &lra);
      if (cc == OK)
        fsugg = kvi.integer;
      i_log(cc, "No interest suggestions exist");
    }
    rd_unlock(mainupn);
    wr_lock(mainupn);
    (void)find_uniques(mainupn, &lra);
    uniques.first_sugg = fsugg;
    write_rec(mainupn, CL_UNIQ, lra, &uniques);
  /*i_log(0, "FSUG %d", fsugg);*/
    wr_unlock(mainupn);
  }

  end_interpret();
  freem(tbldbt);
  return null;
}}}

public const Char * do_sho_paymt()

{ Tabled  tbl = mk_form(SIpform,0);
  wopen(tbl, (Over)SIpover);

  while (true)
  { const Char * error = null;
    goto_fld(W_CMD);
    get_data(&ds[0]);
    if (ds[0] == '.')
      break;
    if (not vfy_integer(ds))
      error = "Payment No or .";
    else
    { Key_t kvi;  kvi.integer = atoi(ds);
    { Paymt pmt = (Paymt)ix_fetchrec(mainupn, IX_PMT, &kvi, null, null);
      if (pmt == null)
				error = ntfd;
      else
				put_paymt_details(pmt);
    }}
    if (error != null)
      alert(error);
  }

  end_interpret();
  freem(tbl);
  return null;
}

#define MAX_FREE_CREDIT 51

Cash sum_int(upn, acc, int_rate, stt_date_ref/*inout*/, end_date)
	 Id       upn;
	 Accdescr acc;
	 double   int_rate;
	 Date    *stt_date_ref;/*can be 0, result can be later by K_INTSUG..*/
	 Date     end_date;
{ Cash loandays = 0;
  Cash loandays_M = 0;

  Date stt_date = *stt_date_ref;
  fast Accitem iiz = &acc->c[0];
       Accitem iip = &acc->c[acc->curr_last+1];

  while (--iip >= iiz)
  { Id eid = iip->id;
    Cash val = iip->val;
    Date stt = get_idue(iip);
    if (stt_date < stt && (iip->props & (K_INTCHG+K_INTSUG)) && 
        iip->vat+iip->val != 0)
    { stt_date = stt;
      *stt_date_ref = stt_date;
    }

    if      (iip->props & K_PAYMENT)
      val = -val;
    else if (iip->props & (K_SALE+K_BFWD))
      ;
    else
      continue;
    if (iip->props & K_ANTI)
      val = -val;

    if (stt < stt_date)
    { if (stt + MAX_FREE_CREDIT < stt_date)
      { val = iip->balance;
        iip = iiz;
      }
      stt = stt_date;
    }

    if (stt < end_date)
    { loandays += (val / 100) * (end_date - stt);
    /*i_log(0, "ld %d %d V %d", loandays, end_date - stt, val);*/
      if (loandays >= 0x40000000)
      { loandays -=  0x40000000;
        loandays_M += 1;
      }
      if (loandays <= -0x40000000)
      { loandays +=  0x40000000;
        loandays_M -= 1;
      }
    }
  }
  
/*i_log(0, "LD %d V %4.2f", loandays, s_interest_rate(cu));*/

  return (Cash)((double)(loandays / 3.65) * int_rate);
}



#define MIN_ITRST 50

Char * do_suggest(Id upn, Accdescr acc, char * src)

{ Char * error = null;
  Customer cu = &this_custmr;
  Int soffs = toupper(src[0]) == 'S' ? 4 : 8;
  Bool store = !disallow(R_ITRST) && toupper(src[0]) == 'S' && s_interest_on(cu);
  Date stt_date = asc_to_date(src+soffs);
  if (stt_date < 0 or *skipspaces(src+soffs) == 0)
    stt_date = 0;

  do /* once */
  { double int_rate = s_interest_rate(cu);
    if (cu->id == 0)
      break;
    if (!store)
    { do
      { salert("What interest rate WOULD apply?");
        get_data(&ds[0]);
        int_rate = atof(ds);
        if (int_rate != 0.0)
          break;
      } while (1);
      int_rate /= 100.0;
    }
    
  { char buf[30];
    Cash itrst;
    Cc cc = lddsp_acc(upn, acc, cu->id, cu->terms);
    if (cc != OK)
    { i_log(cc, "Sum Int");
      return "error";
    }

    do
    { itrst = sum_int(upn, acc, int_rate, &stt_date, today);
/*    buf[0] = 'X'; buf[1] = 0;
      date_to_asc(buf, stt_date, 0);
      salert(buf);*/
      if (stt_date > 0)
        break;
      salert("Date for interest to start from or Q to quit");
      get_data(&ds[0]);

      if (ds[0] == 'Q' or ds[0] == 'q')
        return aban;
      stt_date = asc_to_date(ds);
    } while (1);

  { Char from[30], to[30];
    if (stt_date == 0)
      strcpy(&from[0], "-");
    else
      date_to_asc(&from[0], stt_date, 0);
    if (itrst < MIN_ITRST)
    { error = "Too small";
      break;
    }
    if (!store)
    { fmt_data(&buf[0], "Interest from %r would be %v", from, itrst);
      salert(buf);
      continue;
    }
    fmt_data(&buf[0], "Interest from %r is %v", from, itrst);
    salert(buf);
  { fast Accitem item = extend_acc(acc);
    item->props  = K_PAYMENT+K_ANTI+K_INTSUG;
    item->idate  = today;
    item->vat	 = itrst;
    item->val	 = 0;
    item->chq_no = stt_date;
    item->balance= acc->bal;
    item->id	 = do_interest(upn, cu->id, stt_date, today, 
				 s_interest_rate(cu), itrst);
  { FILE * op = fopen("custsug", "a");
    if (op != NULL)
    { Cash itrst100 = itrst / 100;
      if (stt_date == 0)
        strcpy(&from[0], "-");
      else
        date_to_asc(&from[0], stt_date, 0);
      fprintf(op,"%s : %s Cust Id %d %s\t: %d.%d",
      		 from, date_to_asc(&to[0], today, 0), cu->id, cu->surname, 
       					      itrst100, itrst - itrst100*100);
      fclose(op);
    }
  }}}}}
  while (0);

  return error;
}


Char * do_all_add_int(upn, stt_date/*not used*/)
	 Id       upn;
	 Date     stt_date;
{ Char * strm = ix_new_stream(upn, IX_CUSTSN, null);
  Lra_t lra;
  fast Customer cu;
  Int ct = 0;
  Cash total = 0;
  FILE * op = fopen("custsug", "a");

  while ((cu = (Customer)ix_next_rec(strm, &lra, null)) != null)
  { // do_db(-1,-1); 
    if (cu->id < 0)
      continue;
    if (!s_interest_on(cu))
      continue;

  { Accdescr_t acc_t;
    acc_t.c = null;
    init_acc(&acc_t, &ths.acct);
  {	Cc cc = load_acc(upn, cu->id, &acc_t, &ths.acct, 0, 0);
    if (cc != OK)
    { i_log(cc, "Sum Int");
      return "error";
    }

  { Date stt_date_ = stt_date;
    Cash itrst = sum_int(upn, &acc_t, s_interest_rate(cu), &stt_date_, today);
    if (itrst < MIN_ITRST)
      continue;
      
    (void)do_interest(upn, cu->id, stt_date, today, s_interest_rate(cu), itrst);
    if (op != NULL)
    { Char from[30], to[30];
      Cash itrst100 = itrst / 100;
      if (stt_date_ == 0)
        strcpy(&from[0], "-");
      else
        date_to_asc(&from[0], stt_date_, 0);
      fprintf(op,"%s : %s Cust Id %d %s\t: %d.%d",
      		 from, date_to_asc(&to[0], today, 0), cu->id, cu->surname, 
       					      itrst100, itrst - itrst100*100);
    }
  }}}}

  if (op != NULL)
    fclose(op);

  return null;
}
