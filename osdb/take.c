#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "version.h"
#if XENIX
 #include <sys/ascii.h>
#endif
#include "../h/defs.h"
#include "../h/errs.h"
#include "../h/prepargs.h"
#include "../form/h/form.h"
#include "../form/h/verify.h"
#include "../db/cache.h"
#include "../db/page.h"
#include "../db/recs.h"
#include "../db/b_tree.h"
#include "domains.h"
#include "schema.h"
#include "rights.h"
#include "generic.h"

#define private

#define W_ID	(4 * ID_FAC)
#define W_NAME	(5 * ID_FAC)
#define W_TNO	(6 * ID_FAC)
#define W_DATE	(7 * ID_FAC)
#define W_STAT	(8 * ID_FAC)

#define W_CID	(9 * ID_FAC)
#define W_SNAME (10 * ID_FAC)
#define W_FNAME (11 * ID_FAC)
#define W_ADDR	(12 * ID_FAC)

#define W_XID  (13 * ID_FAC)
#define W_XTYP (14 * ID_FAC)
#define W_XNM  (15 * ID_FAC)
#define W_XVAT (16 * ID_FAC)
#define W_XVAL (17 * ID_FAC)
#define W_CHQN (18 * ID_FAC)

#define W_XCMD (21 * ID_FAC)
#define W_ADD  (22 * ID_FAC)
				/* must be contiguous descending: */
#define W_5000	(24 * ID_FAC)
#define W_2000	(25 * ID_FAC)
#define W_1000	(26 * ID_FAC)
#define W_500	(27 * ID_FAC)
#define W_200	(28 * ID_FAC)
#define W_100	(29 * ID_FAC)
#define W_50	(30 * ID_FAC)
#define W_20	(31 * ID_FAC)
#define W_5_10	(32 * ID_FAC)
#define W_1_2	(33 * ID_FAC)

#define W_MONEY (34 * ID_FAC)

#define W_NCHQ	(41 * ID_FAC)
#define W_PDC   (42 * ID_FAC)
#define W_CHQS	(43 * ID_FAC)
#define W_MATS	(44 * ID_FAC) 
#define W_EXP	(45 * ID_FAC)
#define W_TOT	(46 * ID_FAC)
#define W_OWE	(47 * ID_FAC)

#define W_STT	(50 * ID_FAC)
#define W_STP	(51 * ID_FAC)

#define SL (SCRN_LINES-13)

static int tak_sl;

static const Over_t SI1over [] = 
  {{0,1,1,0,">"},
   {1,5,1,0,
 " Id      Type     Name                          Vat      Amount  Cheque-No "},
// {1,SL+6,1,0, "-------- Takings "},
// {1,SL+6,1,17, hline(58)},
   {0,-9,1,53, wdtot},
   {0,-11,1,0, "#Chqs     Chq-Total            Expenses                Owe"},
   {-1,0,0,0, "Takings"}
  };

static const Fieldrep_t SI1form [] = 
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,   60,  1, ""},
   {W_CMD,   FLD_WR,	    1, 1,     20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD,1,30,    40,  1, ""},
   {W_STAT,  FLD_RD,	    1,71,	9,  1, ""},
   {W_ID,    FLD_RD,	    2,0,	8,  1, "Batch id"},
   {W_NAME,  FLD_WR,	    2,9,LSNAME,1, "Employee"},
   {W_TNO,   FLD_WR,	    2,50,	5,  1, "Taking No"},
   {W_DATE,  FLD_WR,	    2,61,     11,  1, da_te},

   {W_CID,   FLD_RD,	    3,0,	8,	1, ill},
   {W_SNAME, FLD_RD,	    3,9,LSNAME,1, ill},
   {W_FNAME, FLD_RD,	    3,10+LSNAME,LFNAMES,1, ill},
   {W_ADDR,  FLD_RD,	    4,0,LPADDR,1, ill},

   {W_XID,   FLD_WR+FLD_BOL,6, 0,	8,   -SL,"Delete, Money, PDate, chQ, or Trans"},
   {W_XTYP,  FLD_WR,	    6, 9,	 	8,    -SL, "Type"},
   {W_XNM,   FLD_WR,	    6,18,LSNAME,  -SL, "Customer"},
   {W_XVAT,  FLD_WR,	    6,18+LSNAME,8,-SL, "Vat"},
   {W_XVAL,  FLD_WR,	    6,27+LSNAME,8,-SL, amnt},
   {W_CHQN,  FLD_WR+FLD_EOL,6,36+LSNAME,10,-SL, "Cheque No/CR for CASH"},
   {W_BAN,   FLD_RD,	   -6,0,  74, 1,""},

   {W_XCMD,  FLD_WR+FLD_LST,-7,0,	 8, 1, "chQ, Money, Get, Exp"},
   {W_ADD,   FLD_WR,	    -7,9,	35, 1, "what/who"},
   {W_TOT ,  FLD_RD,	    -7,26+LSNAME,9,1, ill}, 

   {W_5000,  FLD_WR,	 -9,  0,	 9, 1, "œ50"},
   {W_2000,  FLD_WR,	 -9, 10,	 9, 1, "œ20"},
   {W_1000,  FLD_WR,	 -9, 20,	 9, 1, "œ10"},
   {W_500,   FLD_WR,	 -9, 30,	 9, 1, "œ5"},
   {W_MONEY, FLD_RD,	 -9, 59,	 9, 1, ill},
   {W_200,   FLD_WR,	 -10, 0,	 9, 1, "œ2"},
   {W_100,   FLD_WR,	 -10, 10,	 9, 1, "œ1 Coins"},
   {W_50,    FLD_WR,	 -10, 20,	 9, 1, "50p"},
   {W_20,    FLD_WR,	 -10, 30,	 9, 1, "20p"},
   {W_5_10,  FLD_WR,	 -10, 40,	 6, 1, "5p/10p"},
   {W_1_2,   FLD_WR,	 -10,	48,	 6, 1, "Bronze"},

   {W_NCHQ,  FLD_RD,	 -11, 5,	 3, 1, ill},
   {W_CHQS,  FLD_RD,	 -11,20,	 9, 1, ill},
   {W_EXP,   FLD_RD,	 -11,40,	 8, 1, ill},
   {W_OWE ,  FLD_RD,	 -11,59,	 9, 1, ill}, 
   {0,0,0,0,0,1,null}
  };

static const Over_t SI2over [] = 
{{0,1,1,0,">"},
 {1,5,1,0,    " Type     Number    Name/Description                  Amount "},
 {1,-7,1,0, "-------- Cheques "},
 {1,-7,1,17,hline(44)},
 {0,-8,1,55, wdtot},
 {0,-10,1,0,
"#Chqs  Post-dated  In-Date    Matured   Expenses     Total         Owe"},
 {-1,0,0,0,"Bank"}
};

static const Fieldrep_t SI2form [] = 
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,      60,  1, ""},
   {W_CMD,   FLD_WR,	    1, 1,      20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,30,      40,  1, ""},
   {W_TNO,   FLD_RD,	    2,50,	5,  1, "Bank Id"},
   {W_DATE,  FLD_RD,	    2,65,      11,  1, da_te},

   {W_XTYP,  FLD_WR+FLD_BOL,6, 0,	8,  -SL, "Type"},
   {W_CHQN,  FLD_WR,	    6,10,  10,  -SL, "Cheque No"},
   {W_XNM,   FLD_WR,	    6,21,LSNAME,-SL, "Customer"},
   {W_XVAL,  FLD_WR+FLD_EOL,6,23+LSNAME,8,-SL, amnt},
   {W_BAN,   FLD_RD,	   -6,0,  74, 1,""},

   {W_5000,  FLD_WR,	 -8,  0,	 9, 1, "œ50"},
   {W_2000,  FLD_WR,	 -8, 10,	 9, 1, "œ20"},
   {W_1000,  FLD_WR,	 -8, 20,	 9, 1, "œ10"},
   {W_500,   FLD_WR,	 -8, 30,	 9, 1, "œ5"},
   {W_MONEY, FLD_RD,	 -8, 64,	 9, 1, ill},
   {W_200,   FLD_WR,	 -9, 0,	 9, 1, "œ2"},
   {W_100,   FLD_WR,	 -9, 10,	 9, 1, "Pound Coins"},
   {W_50,    FLD_WR,	 -9, 20,	 9, 1, "50p"},
   {W_20,    FLD_WR,	 -9, 30,	 9, 1, "20p"},
   {W_5_10,  FLD_WR,	 -9, 40,	 6, 1, "5p/10p"},
   {W_1_2,   FLD_WR,	 -9, 48,	 6, 1, "Bronze"},

   {W_NCHQ,  FLD_RD,	 -11, 2,	3,  1, ""},
   {W_PDC,   FLD_RD,	 -11, 7,	9,  1, ""},
   {W_CHQS,  FLD_RD,	 -11,17,	9,  1, ""},
   {W_MATS,  FLD_RD,	 -11,27,	9,  1, ""},
   {W_EXP,   FLD_RD,	 -11,38,	8,  1, ""},
   {W_TOT ,  FLD_RD,	 -11,48,	9,  1, ""}, 
   {W_OWE ,  FLD_RD,	 -11,64,	9,  1, ""}, 
   {0,0,0,0,0,1,null}
  };

static const Over_t SI5over [] = 
{{0,1,1,0,">"},
 {1,5,1,0,    " Take     Employee                           Amount "},
 {1,-6,1,0, "-------- View Takings ------------------------------"},
 {-1,0,0,0,"Takings"}
};

static const Fieldrep_t SI5form [] = 
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,      60,  1, ""},
   {W_CMD,   FLD_WR,        1, 1,      20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,30,      30,  1, ""},
   {W_TNO,   FLD_WR,        2,50,       5,  1, "Bank Id"},
   {W_DATE,  FLD_WR,        2,65,      11,  1, da_te},

   {W_XID,   FLD_WR+FLD_BOL,6, 0,         8, -SL, "Take No"},
   {W_XNM,   FLD_WR,        6,12,    LSNAME, -SL, "Agent"},
   {W_XVAL,  FLD_WR,        6,15+LSNAME,  8, -SL, amnt},
   {W_BAN,   FLD_RD,	   -6,0,  74, 1,""},

   {W_TOT ,  FLD_RD,       -8,15+LSNAME,  9, 1,  ill}, 
   {0,0,0,0,0,1,null}
  };

static const Over_t SI3over [] = 
  {{0,1,1,0,">"},
   {1,4,1,0, " Transfer from Cash Account to Bank Account"},
   {0,7,1,30, "(Only the total (not the breakdown) is recorded)"},
   {0,8,1,6, "Money"},
   {0,12,1,20, amtot},
   {0,14,1,20, wdtot},
   {-1,0,0,0,null}
  };

static const Fieldrep_t SI3form [] = 
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,      60,  1, ""},
   {W_CMD,   FLD_WR,	    1, 1,      20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,30,      40,  1, ""},

   {W_5000,  FLD_WR,	 9,  0,	 9, 1, "œ50"},
   {W_2000,  FLD_WR,	 9, 10,	 9, 1, "œ20"},
   {W_1000,  FLD_WR,	 9, 20,	 9, 1, "œ10"},
   {W_500,   FLD_WR,	 9, 30,	 9, 1, "œ5"},
   {W_200,   FLD_WR,	10,  0,	 9, 1, "œ2"},
   {W_100,   FLD_WR,	10, 10,	 9, 1, "œ1 Coins"},
   {W_50,    FLD_WR,	10, 20,	 9, 1, "50p"},
   {W_20,    FLD_WR,	10, 30,	 9, 1, "20p"},
   {W_5_10,  FLD_WR,	10, 40,	 6, 1, "5p/10p"},
   {W_1_2,   FLD_WR,	10, 48,	 6, 1, "Bronze"},

   {W_MONEY, FLD_RD,	12, 35,	 9, 1, ill},
   {W_CHQS,  FLD_WR,	13, 35,	 9, 1, "Cheques"},
   {W_TOT ,  FLD_RD,	14, 35,	 9, 1, ill},

   {0,0,0,0,0,1,null}
  };

static const Over_t SI4over [] = 
{{0,1,1,0,">"},
  {0,3,1,0,"Bag No"},
  {1,5,1,0, " T -- View Takings"}, 
  {1,6,1,0, " L -- Local Takings"},
  {1,7,1,0, " C -- Close"},
  {1,8,1,0, " V -- View"}, 
  {1,9,1,0, " S -- Submit"},
  {1,10,1,0," P -- Print"},
  {1,11,1,0," B -- Bank Cash"},
 {-1,0,0,0,"Till"}
};

static const Fieldrep_t SI4form [] = 
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,      60,  1, ""},
   {W_CMD,   FLD_WR,	    1, 1,      20,  1, comd},
   {W_TNO,   FLD_WR,	    3,10,	5,  1, "Bank Id"},
   {W_STAT,  FLD_RD,	    3,40,	9,  1, ""},
   {W_DATE,  FLD_RD,	    3,60,      12,  1, ""},
   {W_BAN,   FLD_RD,	   13,0,  74, 1,""},
   
   {0,0,0,0,0,1, null}
  };

static const Over_t SI6over [] = 
{{0,1,1,0,">"},
 {1,3,1,0, "  SALES STATISTICS"},
 {1,3,1,18, spaces(26)},
 {0,4,1,1, stdt},
 {0,5,1,1, "End Date"},
 {1,16,1,0, "------- Select service "}, {1,16,1,23, hline(21)},
 {-1,0,0,0,null}
};

static const Fieldrep_t SI6form [] = 
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,      60,  1, ""},
   {W_CMD,   FLD_WR,	    1, 1,      20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,30,      30,  1, ""},
   {W_STT,   FLD_WR,	    4,15,      15,  1, stdt},
   {W_STP,   FLD_WR,	    5,15,      15,  1, "End Date"},
   {W_SEL,   FLD_WR+FLD_S+FLD_BOL,7,5, 40,  8, "Actions"},
   {0,0,0,0,0,1, null}
  };

static const Char * op6text[] = 
 { " 0 -- By Batches",
   " 1 -- By Batches Cash Sales",
   " 2 -- By Batches Account Sales",
   " 3 -- By Takings in Progress",
   " 4 -- Daily",
   " 5 -- By Cancelled Invoices",
   " 6 -- Monthly Sales, Payments by Acc",
   null
 };

static Tabled tbl1;		/* for SI1form */

static const Cash denom_to_val[] = { 5000, 2000, 1000, 500, 200, 100, 50, 20, 5, 1 };

       Bank_t	  this_bank;
       Set        real_agent_props;
static Bool	  in_local;	      /* in takings local to bank */
static Bool	  tbd_submit_local = false;

Takdescr_t tak_t;		/* these 3 used by recover.c */
Bankdescr_t bak_t;
Bagsdescr_t bags_t;

static Char xactn_typ;

#define Q_SAVE	 1
#define Q_CLOSE  2
#define Q_SUBMIT 3

static const Char ymbir[] = "You must be in revenue";
static const Char lted[] = "lost takings entry %d";
static const Char opiypci[] = "Others paid invoice(s) you paid, CHECK IT";  
static const Char tstus [][12] =
 { "NO ENTRYS", "ACTIVE", "CLOSED", "BOOKED", "IN-PROGRESS"};

static Bool doubt = false;		/* indicates OPIYP */

private void init_take()

{ static Bool it_done = false;
  if (it_done)
    return;

  today = asc_to_date("");

  tak_sl = g_rows - 12;
  it_done = true;
			/* debugging: set big enough to avoid extends! */
#define INIT_TSZ 100
#define INIT_BSZ 20
  tak_t.rev_sz = INIT_TSZ;
  tak_t.rev_c  = (Takitem)malloc(sizeof(Takitem_t) * (INIT_TSZ+1));
  tak_t.sal_sz = INIT_TSZ;
  tak_t.sal_c  = (Takitem)malloc(sizeof(Takitem_t) * (INIT_TSZ+1));
  bak_t.rev_sz = INIT_TSZ;
  bak_t.rev_c  = (Takitem)malloc(sizeof(Takitem_t) * (INIT_TSZ+1));
  bags_t.arr_sz = INIT_BSZ;
  bags_t.c     = (Bagsitem)malloc(sizeof(Bagsitem_t) * (INIT_BSZ+1));

  tbl1 = mk_form(SI1form,tak_sl);
}

private void init_cust()

{ this_custmr.id   = 0;
  this_custmr.surname[0]   = 0;
  this_custmr.forenames[0] = 0;
  this_custmr.postaddr[0]  = 0;
  this_custmr.postcode[0]  = 0;
}


private void init_tak(tak)
      Takdescr	tak;
{ Takitem rev_c = tak->rev_c;
  Takitem sal_c = tak->sal_c;
  Int rev_sz = tak->rev_sz;
  Int sal_sz = tak->sal_sz;

  *tak = null_sct(Takdescr_t);
  tak->rev_c = rev_c;
  tak->sal_c = sal_c;
  tak->rev_sz = rev_sz;
  tak->sal_sz = sal_sz;

  tak->rev_base  = 0;
  tak->rev_last  = -1;
  tak->sal_mode  = not in_local;
  tak->sal_base  = 0;
  tak->sal_last  = -1;
  tak->modified = false;
  doubt = false;
}


private void init_bank(bak,bags)
      Bankdescr  bak;
      Bagsdescr  bags;
{ bags->id = 0;
  bags->curr_base = 0;
  bags->curr_last = -1;

{ Takitem rev_c = bak->rev_c;
  Int rev_sz = bak->rev_sz;

  *bak = null_sct(Bankdescr_t);
  bak->rev_c = rev_c;
  bak->rev_sz = rev_sz;

  bak->curr_last = -1;
}}

private Char const * get_ttyp(props_)
	 Set  props_;
{ fast Set props = props_;
  return props & K_DEAD    ? "GONE"    :
	 props & K_SALE    ? 
	  props & K_ANTI   ? props & K_CASH ? "CRFND"    : 
			     props & K_UNB  ? "DISC"     : "CREDIT"  : 
			     props & K_CASH ? "CSALE"    : 
			     props & K_UNB  ? "BANK-CH"  : "INV"     : 
	 props & K_PAYMENT ? 
	  props & K_ANTI   ? props & K_PD   ? "PMT-PDC" :
			     props & K_UNB  ? "MAN-ADD" : "RTN-CHQ" :
			     props & K_PD   ? "MAT-CHQ" :
			     props & K_UNB  ? "MAN-SUB" : "PAYMENT" :
	 props & K_EXP	   ? "EXPENSE" :
	 props & K_CCHQ    ? "CHQ"     : ill;
}


private void put_item(it_, row)
	Takitem  it_;
	Int	 row;
{ fast Takitem  it = it_;
       Char numstr[20];
       Int spos = save_pos();
  fast Set  props = it->props;

  sprintf(&numstr[0],		props & K_CCHQ   ? props & K_PD ? "P%d" : "%d" :
       (props & K_SALE) and not (props & K_CASH) ? "BOOK"		       :
						   "CASH", it->chq_no); 
  form_put(T_INT+W_XID+row,     it->id,
           T_DAT+W_XTYP+ID_MSK, get_ttyp(props),
           T_DAT+W_XNM+ID_MSK,  it->name,
           T_CSH+W_XVAL+ID_MSK, it->val,
           T_DAT+W_CHQN+ID_MSK, numstr, 0);
  restore_pos(spos);
} 


private void put_rev(tak, ix)
	Takdescr tak;
	Int	 ix;
{ put_item(&tak->rev_c[ix], ix - tak->rev_base);
}

private void put_sale(tak, ix)
	Takdescr tak;
	Int	 ix;
{ Char numstr[14];
  Int spos = save_pos();
  fast Takitem it = &tak->sal_c[ix];
  fast Set  props = it->props;

  sprintf(&numstr[0], props & K_CCHQ ? "%d"   :
		      props & K_CASH ? "CASH" : "BOOK", it->chq_no);
  if ((props & 0x2000000) and this_take.version < Q_SUBMITV)
  { strcat(&numstr[0], "XX"); alert(""); }

  goto_fld(W_XID + ix - tak->sal_base); put_integer(it->id);
  goto_fld(W_XTYP+ID_MSK);		          put_data(get_ttyp(props));
  goto_fld(W_XNM+ID_MSK);		            put_data(it->name);
  goto_fld(W_XVAT+ID_MSK);		          put_cash(it->vat);
  goto_fld(W_XVAL+ID_MSK);		          put_cash(it->val);
  goto_fld(W_CHQN+ID_MSK);		          put_data(numstr);
/*
  form_put(T_INT+W_XID +(ix-tak->sal_base & 0x1f), it->id,
           T_DAT+W_XTYP+ID_MSK,            get_ttyp(props),
           T_DAT+W_XNM+ID_MSK,             it->name,
           T_CSH+W_XVAT+ID_MSK,            it->vat,
           T_CSH+W_XVAL+ID_MSK,	           it->val,
           T_DAT+W_CHQN+ID_MSK,            numstr, 0); */
  restore_pos(spos);
} 


private void put_vouch(bak, ix)
	Bankdescr bak;
	Int	  ix;
{ Int spos = save_pos();
  fast Takitem it = &bak->rev_c[ix];

  form_put(T_DAT+W_XTYP+ ix-bak->curr_base, get_ttyp(it->props),
           T_INT+W_CHQN+ID_MSK,             it->chq_no,
           T_DAT+W_XNM+ID_MSK,              it->name,
           T_CSH+W_XVAL+ID_MSK,             it->val, 0);
  restore_pos(spos);
} 

private void put_flag()

{ fld_put_data(W_XCMD, xactn_typ == 'Q' ? " CHEQUES" :
                       xactn_typ == 'M' ? " MONEY"   :
		       xactn_typ == 'G' ? " GET"     : " EXPENSE");
}				     


private void put_taksmy(bags, ix)
	Bagsdescr bags;
	Int	  ix;
{ Int spos = save_pos();
  fast Bagsitem it = &bags->c[ix];

  form_put(T_INT+W_XID+ ix - bags->curr_base, it->id,
           T_DAT+W_XNM+ID_MSK,                it->name,
           T_CSH+W_XVAL+ID_MSK,	              it->val, 0);
  restore_pos(spos);
} 

private Takitem extend_rev(tak)
	Takdescr   tak;
{ Int size = tak->rev_sz;
  tak->rev_last += 1; 			    /* more output */
  if (tak->rev_last >= size) 
  { size = size > 1000 ? size + 1000 : (size + 1) * 2;
    tak->rev_c  = (Takitem)realloc(tak->rev_c, (size + 1) * sizeof(Takitem_t));
    tak->rev_sz = size;
  }
  return &tak->rev_c[tak->rev_last];
}



private Takitem extend_sal(tak)
	Takdescr   tak;
{ Int size = tak->sal_sz;
  tak->sal_last += 1; 			    /* more output */
  if (tak->sal_last >= size)
  { size = size > 1000 ? size + 1000 : (size + 1) * 2;
    tak->sal_c  = (Takitem)realloc(tak->sal_c, (size + 1) * sizeof(Takitem_t));
    tak->sal_sz = size;
  }
  return &tak->sal_c[tak->sal_last];
}



private Takitem extend_bank(bank)
	Bankdescr   bank;
{ Int size = bank->rev_sz;
  bank->curr_last += 1;		  /* more output */
  if (bank->curr_last >= size)
  { size = size > 1000 ? size + 1000 : (size + 1) * 2;
    bank->rev_c  = (Takitem)realloc(bank->rev_c, (size+1) * sizeof(Takitem_t));
    bank->rev_sz = size;
  }
  return &bank->rev_c[bank->curr_last];
}



private Bagsitem extend_bags(bags)
	Bagsdescr   bags;
{ Int size = bags->arr_sz;
  bags->curr_last += 1;		/* more output */
  if (bags->curr_last >= size)
  { size = size > 1000 ? size + 1000 : (size + 1) * 2;
    bags->c      = (Bagsitem)realloc(bags->c, (size + 1) * sizeof(Bagsitem_t));
    bags->arr_sz = size;
  }
  return &bags->c[bags->curr_last];
}

private void put_details()

{      Int spos = save_pos();
       Char datestr[12]; datestr[0] = 0;
  if (this_take.tdate != 0)
    date_to_asc(&datestr[0], this_take.tdate, 0);

  form_put(T_DAT+W_STAT, tstus[this_take.id == 0             ? 0 : 
                               this_take.version < Q_CLOSEV  ? 1 : 
                               this_take.version < Q_SUBMITV ? 2 : 3],
           T_DAT+W_DATE, datestr, 
           T_INT+W_ID,   this_take.bankid,
           T_DAT+W_NAME, that_agent.name,
           T_INT+W_TNO,  this_take.id, 0);
  restore_pos(spos);
}



private Cash sum_owe(tak_)
	 Takdescr  tak_;
{ fast Takdescr  tak = tak_;
  return tak->tot_revenue + tak->tot_swap
    - (tak->tot_chqs + tak->tot_money + tak->tot_expense);
}

private void put_bdetails()

{ Int spos = save_pos();
  Char datestr[11];   date_to_asc(&datestr[0], this_bank.date, 0);
  
  form_put(T_DAT+W_DATE,  datestr,
           T_INT+W_TNO,   this_bank.id,
           T_DAT+W_STAT,  tstus[this_bank.id == 0             ? 0 : 
		                this_bank.version < Q_CLOSEV  ? 1 : 
		                this_bank.version < Q_SUBMITV ? 2 : 3], 0);
  restore_pos(spos);
}



private void put_totals(tk_)
      Takdescr	tk_;
{ fast Takdescr tk = tk_;
       Int spos = save_pos();
  form_put(T_CSH+W_MONEY, tk->tot_money,
           T_CSH+W_CHQS,  tk->tot_chqs,
           T_DAT+W_SNAME, tk->sal_mode ? "     *SALES*" : "     *REVENUE*",
           T_INT+W_NCHQ,  tk->nchq,
           T_CSH+W_EXP,   tk->tot_expense,
           T_CSH+W_TOT,   tk->tot_revenue,
           T_CSH+W_OWE,   sum_owe(tk), 0);
  restore_pos(spos);
} 


private void put_btotals(bak_)
      Bankdescr bak_;
{ fast Bankdescr bak = bak_;
       Int spos = save_pos();
  form_put(T_CSH+W_MONEY,  bak->tot_money,
           T_INT+W_NCHQ,   bak->nchq,
           T_CSH+W_PDC,    bak->tot_pdchqs,
           T_CSH+W_CHQS,   bak->tot_chqs - bak->tot_pdchqs,
           T_CSH+W_MATS,   bak->tot_matchqs,
           T_CSH+W_EXP,    bak->tot_expense,
           T_CSH+W_TOT,    bak->tot_revenue,
           T_CSH+W_OWE,    sum_owe(bak), 0);
  restore_pos(spos);
} 

private void put_denoms(take)
      Taking    take;			/* must be &this_take or &this_bank */
{      Int spos = save_pos();
  fast Int fpos;
  fast Short * m = &take->u5000;
  fast Cash const * den = &denom_to_val[0];
/*if (take != &this_take)			** a kludge (now obsolete) */
/*    --m; */
 
  for (fpos = W_5000; fpos <= W_1_2; fpos += ID_FAC)
  { if (goto_fld(fpos) != null)
      put_cash(*m * *den);
    ++m; ++den;
  }

  restore_pos(spos);
} 



private void put_custmr()

{ Int spos = save_pos();
  form_put(T_OINT+W_CID,  this_custmr.id,
           T_DAT+W_SNAME, this_custmr.surname,
           T_DAT+W_FNAME, this_custmr.forenames,
           T_DAT+W_ADDR,  this_custmr.postaddr, 0);
  restore_pos(spos);
}
					/* DETAIL */

private void sum_tots(tak_)
      Takdescr tak_;  
{ fast Takitem iip;
  fast Set props;
  
  fast Takdescr tak = tak_;
  memset((Char*)&tak->nchq, 0, 
		     (Int)&tak->tot_revenue + sizeof(Cash) - (Int)&tak->nchq);

  for (iip = &tak->rev_c[tak->rev_last]; iip >= tak->rev_c; --iip)
  { props = iip->props;

    if (props & K_DEAD)
      continue;
    if      (props & K_EXP)
    { tak->tot_expense += iip->val;
      if (props & K_VAT)
				tak->tot_expvat += iip->vat;
    }
    else if ((props & (K_CCHQ+K_ANTI+K_PD+K_UNB)) == K_CCHQ)
    { tak->nchq += 1;
      tak->tot_chqs += iip->val;
    }
    else if (props & K_PD)
    { if (props & K_ANTI)
				tak->tot_pdchqs += iip->val;
      else
				tak->tot_matchqs += iip->val;
    }
    if (((props & (K_PAYMENT + K_PD + K_ANTI)) == K_PAYMENT and iip->cid > 0 or
      	 (props & K_SALE)) and not (props & K_UNB))
      tak->tot_revenue += props & K_ANTI ? - iip->val : iip->val;
    if ((props & K_PAYMENT) and iip->cid == BANKACC)
      tak->tot_swap += props & K_ANTI ? - iip->val : iip->val;
    if ((props & (K_PAYMENT+K_UNB)) == K_PAYMENT+K_UNB)
      tak->tot_journal += props & K_ANTI ? -iip->val : iip->val;
  }

  for (iip = &tak->sal_c[tak->sal_last]; iip >= tak->sal_c; --iip)
  { props = iip->props;
    if (not (props & K_SALE) or (props & K_DEAD))
      continue;
    if (true)
      if (props & K_ANTI)
      { tak->noo_rets += 1;
				if (props & K_UNB)
				  tak->tot_disc += iip->val;
				else 
			 	  tak->tot_rets += iip->val;
				tak->tot_vatret  += iip->vat;
      }
      else 
      { tak->noo_sales += 1;
				if (props & K_UNB)
				  tak->tot_chg += iip->val;
				else 
			 	  tak->tot_sales += iip->val;
				tak->tot_vat   += iip->vat;
      }
 
    if (props & K_CASH)
      if (props & K_ANTI)
      { tak->noo_crets += 1;
				if (props & K_UNB)
				  ;
				else 
				  tak->tot_crets += iip->val;
				tak->tot_cvatret  += iip->vat;
      }
      else 
      { tak->noo_csales += 1;
				if (props & K_UNB)
				  ;
				else 
				  tak->tot_csales += iip->val;
				tak->tot_cvat   += iip->vat;
      }
  }
  tak->tot_rets -= tak->tot_vatret;
  tak->tot_sales -= tak->tot_vat;
  tak->tot_crets -= tak->tot_cvatret;
  tak->tot_csales -= tak->tot_cvat;
}

private void recalc(tak)
      Takdescr	tak;
{ sum_tots(tak);
  put_totals(tak);
}


private Cash sum_cash(take)
	 Taking  take;			/* must be &this_take or &this_bank */
{ fast Short * s = &take->u5000;
       Cash * den = &denom_to_val[0];
  fast Int tot = 0;
  fast Int ct = 10;
/*if (take != &this_take)			** a kludge (now obsolete) */
/*  --s; */

  while (--ct >= 0)
    tot += *s++ * *den++;
    
  return tot;
}

private Takitem add_teny(upn, tak, wh, kind, id, ip_)
	  Id	   upn;
	  Takdescr tak;
	  Kind	   wh;	/* K_SALE => add a sale, K_CASH => add a revenue */
	  Kind	   kind;
	  Id	   id;
	  Sorp	   ip_;    
{ fast Takitem iip = wh & K_CASH ? extend_rev(tak) : extend_sal(tak);
  tak->modified = true;
  iip->props  = kind;
  iip->id     = id;
  iip->cid    = ip_->paymt.customer;
  iip->val    = ip_->paymt.total;
  iip->vat    = not (ip_->paymt.kind & K_SALE) 
		   ? 0 : (ip_->sale.vattotal * ip_->sale.vat0_1pc + 500) / 1000;
  iip->chq_no = ip_->paymt.chq_no;
  iip->name[0]= 0;

  if (ip_->expense.kind & K_EXP)
  { strpcpy(&iip->name[0], ip_->expense.descn, sizeof(Surname)-1);
    iip->chq_no = 0;			/* to signify cash, probably obsolete */
    if (ip_->paymt.kind & K_VAT)
      iip->vat = ip_->expense.vat;
  }
  else 
  { Id cid = ip_->paymt.customer;
    Customer cu = get_cust(upn, IX_CUST, cid);
    strpcpy(&iip->name[0],  cid == 0   ? "For Cash" :
			      cu == null ? cdel
					 						 : &cu->surname[0], sizeof(Surname)-1);
    if (ip_->sale.kind & K_SALE)
    { if ((wh & (K_SALE+K_CASH)) == K_SALE+K_CASH)
      { Takitem oiip = iip;
				iip = extend_sal(tak);
				*iip = *oiip;
      }
    }
  }
  return iip;
}




private Takitem my_rev(tak, kind_, id)
	 Takdescr  tak;
	 Set	   kind_;
	 Id	   id;
{      Kind kind = kind_ & K_TPROPS;
  fast Takitem iip = &tak->rev_c[tak->rev_last+1];
       Takitem iiz = &tak->rev_c[0];

  while (--iip >= iiz)
    if ((iip->props & (K_TPROPS+K_DEAD)) == kind and iip->id == id)
      return iip;

  return null;
}


private Takitem my_sale(tak, kind_, id)
	 Takdescr  tak;
	 Set	   kind_; /* at present always K_SALE */
	 Id	   id;
{ /*   Kind kind = kind_ & K_TPROPS; */
  fast Takitem iip = &tak->sal_c[tak->sal_last+1];
       Takitem iiz = &tak->sal_c[0];

  while (--iip >= iiz)
    if (/* (iip->props & (K_TPROPS+K_DEAD)) == kind and */ iip->id == id)
      return iip;

  return null;
}



Int get_deliv_inv()

{ Int ct = 1000;
  Lra_t  lra;
  Key_t  kv_t;
  Cc cc = b_last(mainupn, IX_SALE, &kv_t, &lra);
  if (cc == OK)
    while (--ct > 0)
    { Sale sale = (Sale)ix_fetchrec(mainupn, IX_SALE, &kv_t, null, null);
      if (sale == null)
      { i_log(kv_t.integer, "NODELSAL");
        break;
      }
    /*i_log(sale->id, "Linv %d", sale->agent);*/
//    if (sale->agent == 16194 and sale->date == today)
//      return kv_t.integer;
      kv_t.integer -= 1;
    }

  return 0;
}

private Cc load_tak(upn, tak, id)
	 Id	  upn;	   /* read locked */
	 Takdescr tak;
	 Id	  id;
{ Taking_t takbuf;
  Set  tbpmask;
  Lra_t lra = 0;
  Key_t kv_t;  kv_t.integer = id;
{ Taking bkt = (Taking)ix_fetchrec(upn, IX_TAKE, &kv_t, &lra, &this_take);
  if (bkt == null)
    return NOT_FOUND;

  init_tak(tak);
  tbpmask = bkt->version < Q_SUBMITV ? -1 : ~K_TBP;
  tak->rw = tbpmask == -1 and not upn_ro &&
	   				(this_take.agid == this_agent.id || am_admin || this_take.agid == 0);
  tak->id = id;
  tak->tot_money = sum_cash(&this_take);

  while (s_page(lra))
  { // do_db(-1,-1);
    bkt = (Taking)read_rec(upn, CL_TAKE, lra, &takbuf);
    if (bkt == null)
      break;
  { Tak ip = takbuf.next & Q_EFLAG ? &takbuf.eny[-1-TAKHDR]
																   : &takbuf.eny[-1];

    while (++ip <= &takbuf.eny[TAKBUCKET-1] and ip->id != 0)
    { Kind kind = ip->kind & tbpmask;
      Kind wh = kind & K_SALE ? kind : K_CASH;
      if (kind == 0 or (kind & K_DEAD))
				continue;
      kv_t.integer = ip->id;
    { Quot ix = kind & K_SALE ? IX_SALE : 
								kind & K_EXP  ? IX_EXP  : IX_PMT;
      Lra_t  lra_;
      Sorp_t pay_t;
      Paymt ip_ = (Paymt)ix_fetchrec(upn, ix, &kv_t, &lra_, &pay_t);
      if (ip_ == null)
      { i_log(dbcc, lted, kv_t.integer);
        i_log(id, "was tid");
      }
      else 
      { int t = ix == IX_EXP ? ((Expense)ip_)->agent : ip_->agent;
				/*
        if (((t - id) & 0xffff) != 0)
				{ kind |= 0x2000000;
				  i_log(kind & 0xff, "Alien t.entry in %ld (%ld not %ld)",ip->id,t,id);
				}
				*/
				(void)add_teny(upn, tak, wh, kind, ip->id, (Sorp)ip_);
      }
    }}
    lra = takbuf.next & (Q_EFLAG-1);
  }}
  sum_tots(tak);
  tak->modified = false;
  return OK;
}}

			/* called with the following combinations:
			      id		0 or not
			      this_take.agid	0 or not
			*/
private Cc lddsp_tak(upn, tak, id, sho_items, persist)
	 Id	  upn;
	 Takdescr tak;
	 Id	  id;	      /* or 0 => current of age	*/
	 Bool	  sho_items, persist;
{ rd_lock(upn);
  if (persist)
  { init_tak(tak);
    tak->rw = not upn_ro;
    sum_tots(tak);
  }
{ Cc cc = id == 0 ? (persist ? OK : NOT_FOUND) : load_tak(upn, tak, id);

  rd_unlock(upn);

  if (cc == OK)
  { if (this_take.agid == 0)
      that_agent = this_agent;
    else
    { Char * err = find_that_agent(upn, this_take.agid);
      if (err != null)
				i_log(44, "Ag of Takings");
    }
    put_details();
    put_totals(tak);
    put_denoms(&this_take);
    wclrgrp();
    if (sho_items)
    { Int ix = tak->rev_base-1;
      while (ix - tak->rev_base < SL-1 and ix < tak->rev_last)
				put_rev(tak, ++ix);
    }
  }
  return cc;
}}

#if TAKTXT
X
XFILE * ttfd;

Xpublic Cc tt_init_rdwr(wh)
	Quot  wh;
X{ if (ttfd != null)
    fclose (ttfd);
  ttfd = fopen("/tmp/taks", wh == 0 ? "r" : "w");
  return ttfd != null;
X}


Xpublic Cc write_text_tak(tid)
				/* mainupn at least read locked */
X{ Taking_t takbuf;
  Lra_t lra = 0;
  Key_t kv_t;  kv_t.integer = tid;
{ Taking bkt = (Taking)ix_fetchrec(mainupn, IX_TAKE, &kv_t, &lra, &this_take);
  if (bkt == null)
    return NOT_FOUND;
    
  fprintf(ttfd, "TAK %d %d %d %d %d\n",
	this_take.id, this_take.tdate, this_take.version, this_take.agid,
	this_take.bankid);
  fprintf(ttfd, "M %d %d %d %d %d %d %d %d %d\n",
	this_take.u5000,		this_take.u2000,this_take.u1000,
	this_take.u500,	this_take.u100,	
	this_take.u50,	this_take.u20,
	this_take.u5,	this_take.u1);

  while (s_page(lra))
  { bkt = (Taking)read_rec(mainupn, CL_TAKE, lra, &takbuf);
  { Tak ip = /* takbuf.next & Q_EFLAG  ? &takbuf.eny[-1-TAKHDR]
				    : */ &takbuf.eny[-1];
    if (bkt == null)
      break;

    while (++ip <= &takbuf.eny[TAKBUCKET-1] and ip->id != 0)
      fprintf(ttfd, "%x %x\n", ip->kind, ip->id);

    lra = takbuf.next & (Q_EFLAG-1);
  }}
    
  fprintf(ttfd, "0 0\n");
  return OK;
X}}



Xpublic Cc read__text_tak()

X{ this_take = null_sct(Taking_t);
X{ Int ct = fscanf(ttfd, " TAK %d %d %d %d %d",
	&this_take.id, &this_take.tdate, &this_take.version, &this_take.agid,
	&this_take.bankid);
  if (ct <= 0)
    return NOT_FOUND;
  ct = fscanf(ttfd, " M %d %d %d %d %d %d %d %d %d",
	&this_take.u5000,&this_take.u2000,&this_take.u1000,
	&this_take.u500,		  &this_take.u100,
	&this_take.u50,  &this_take.u20,
	&this_take.u5,	 &this_take.u1);

  (void)d_Taking_t(mainupn, &this_take);

{ Lra_t lra = 0;
  Cc cc = i_Taking_t(mainupn, &this_take, &lra);
  if (cc != OK)
    return cc;
 
  while (true)
  { Quot kind;
    Int eid;
    Int ct = fscanf(ttfd, "%lx %lx", &kind, &eid);
    if (ct == 0)
      i_log(4, "rta");
    if (eid == 0)
      break;
    (void)item_to_tak(this_take.id, MAXINT, kind, eid);
  }
{ Taking_t ta_t;
  Taking ta = (Taking)read_rec(mainupn, CL_TAKE, lra, &ta_t);
  if (ta != null)
  { ta->version = this_take.version;
    (void)write_rec(mainupn, CL_TAKE, lra, ta);
  }
  return OK;
X}}}}



Xpublic Cc read_text_tak()

X{ Cc cc = read__text_tak();
  if (cc < OK)
    i_log(cc, "RTXT:%d", this_take.id);

  return cc;
X}

#endif
					/* DDDDDDDDDDDDDDDDDDDDDDDDDDDDDD */

private Int same_version(upn, ix, taking)   
	 Id	  upn;
	 Id	  ix;		/* IX_TAKE, or IX_BANK */
	 Taking   taking;
{ Key_t kvi; kvi.integer = taking->id;
  if (kvi.integer == 0)
    return 0;
  rd_lock(upn);
{ Lra_t  lra;
  Taking take__ = (Taking)ix_srchrec(upn, ix, &kvi, &lra);
  Int res = take__ == null ? 0 : take__->version - taking->version;
  rd_unlock(upn);
  return res;
}}


private Char * load_newest(upn, tak, take)   /* advisory only */
	 Id	  upn;
	 Takdescr tak;
	 Taking   take;
{ Int vd = same_version(upn, IX_TAKE, take);
  if (vd == 0)
    return null;
{ Bool modified = tak->modified;
  Cc cc = lddsp_tak(upn, tak, take->id, true, false);
  tak->modified = modified;
  i_log(cc, "Loading Newer %ld by %ld", take->id, vd);
  return cc == OK ? "Others posted new items!" : sodi;
}}


					/* -1: not in the taking */
					/*  0: taking not found */
public Lra_t find_tak_t(take_ref, eny_ref, tid, kind_, id)
	 				/* write locked */
	 Taking  take_ref;
	 Tak   * eny_ref;
	 Id	 tid;
	 Quot	 kind_;
	 Id	 id;
{ Kind kind = kind_ & K_TPROPS;
  Lra_t lra = 0;
  Key_t kv_t;  kv_t.integer = tid;
{ Taking bkt_ = (Taking)ix_fetchrec(mainupn, IX_TAKE, &kv_t, &lra, take_ref);
  if (bkt_ == null)
    return 0;

  for (; lra != 0; lra = bkt_->next & (Q_EFLAG-1))
  { // do_db(-1,-1); 
    bkt_ = (Taking)read_rec(mainupn, CL_TAKE, lra, take_ref);
    if (bkt_ == null)
      break;
  { Tak ip = bkt_->next & Q_EFLAG ? &bkt_->eny[-1-TAKHDR]
																  : &bkt_->eny[-1];

    while (++ip <= &bkt_->eny[TAKBUCKET-1] and ip->id != 0)
      if ((ip->kind & (K_TPROPS+K_DEAD)) == kind and ip->id == id)
      { *eny_ref = ip;
				return lra;
      }
  }}

  return -1;
}}

private void add_chq(bak, iiip)
	 Bankdescr bak;
	 Takitem   iiip;
{ fast Takitem iib = extend_bank(bak);
  iib->id     = iiip->id;
  iib->cid    = iiip->cid;	     
  iib->props  = iiip->props;
  iib->chq_no = iiip->chq_no;
  strpcpy(&iib->name[0], &iiip->name[0], sizeof(Surname)-1);
  iib->val    = iiip->val;
}



private void add_takh(bags, tak)
	 Bagsdescr bags;
	 Takdescr  tak;
{ fast Bagsitem iip = extend_bags(bags);
  iip->props = 0;
  iip->id    = tak->id;
  iip->val   = tak->tot_revenue;
{ Key_t kv_t;  kv_t.integer = this_take.agid;
{ Lra_t lra_;
  Agent ag__ = (Agent)ix_srchrec(mainupn, IX_AGNT, &kv_t, &lra_);
  if (ag__ != null)
    strpcpy(&iip->name[0], ag__->name, sizeof(Surname)-1);
}}}



private Cc add_tak(upn, bak_, bags, tak_, tid, add_chqs)
	 Id	   upn;
	 Bankdescr bak_;
	 Bagsdescr bags;
	 Takdescr  tak_;
	 Id	   tid;
	 Bool      add_chqs;
{ fast Bankdescr bak = bak_;
  fast Takdescr tak = tak_; 
  Cc cc = load_tak(upn, tak, tid);  
  if (cc != OK)
    i_log(cc, "lost takings %d", tid);
  else 
  { add_takh(bags, tak);

  { Cash * pt, *ps = &tak->nchq;
    for (pt = &bak->nchq; pt <= &bak->tot_revenue; ++pt)
      *pt += *ps++;

  { Takitem iiip = &tak->rev_c[-1];
    Takitem iiiz = &tak->rev_c[tak->rev_last];

    if (add_chqs)
      while (++iiip <= iiiz)
				if (iiip->props & (K_CCHQ + K_EXP))
				  add_chq(bak, iiip);
  }}}
  return cc;
}

private Cc load_bags(upn, bak, bags, tak, bid)
	 Id	   upn;
	 Bankdescr bak;
	 Bagsdescr bags;
	 Takdescr  tak;
	 Id	   bid;
{ Lra_t  nlra, lra;
  Key_t kv_t;  kv_t.integer = bid;
  rd_lock(upn);
{ Bank bank = (Bank)ix_fetchrec(upn, IX_BANK, &kv_t, &lra, &this_bank);
  if (bank == null)
  { rd_unlock(upn); return NOT_FOUND; }

  init_bank(bak, bags);

  bak->tot_money = sum_cash(&this_bank);

  bags->id = bak->id = this_bank.id;
  bags->rw = bak->rw = this_bank.version < Q_SUBMITV and not upn_ro;

  while (s_page(lra) != 0)
  { // do_db(-1,-1);
    fast Int ix = -1;
    Bank_t  bank_ext;
    Bank bkt = (Bank)read_rec(upn, CL_BANK, lra, &bank_ext);
    if (bkt == null)
    { i_log(51, "Corrupt Bank");
      break;
    }
    while (++ix <= BANKBUCKET-1 and bkt->eny[ix] != 0)
      (void)add_tak(upn, bak, bags, tak, bkt->eny[ix], true);

    lra = bkt->next & (Q_EFLAG-1);
  }
  rd_unlock(upn);
  return OK;
}}

private Char * find_item(tak, ds_)
	 Takdescr tak;
	 Char *   ds_;
{ Char * ds = skipspaces(ds_);
  Cash amt = 0;
  fast Int ix;

  if (*ds == 0)
  { if (tak->sal_mode)
      tak->sal_base = 0;
    else 
      tak->rev_base = 0;
    return null;
  }
  if (get_cash(ds, &amt) <= ds)
    return "cash or nothing";

  if (tak->sal_mode)
  { for (ix = 0; ix <= tak->sal_last; ++ix)
      if (tak->sal_c[ix].val == amt)
      { tak->sal_base = ix;
				return null;
      }
  }
  else 
    for (ix = 0; ix <= tak->rev_last; ++ix)
      if (tak->rev_c[ix].val == amt)
      { tak->rev_base = ix;
				return null;
      }
  return ntfd;
}



public void add_cash(bank, take)
      Bank	bank;
      Taking	take;
{ fast Short * s = &take->u5000;
  fast Short * t = &bank->u5000;
  fast Int ct = 10;
  while (--ct >= 0)
    *t++ += *s++;
}

private void do_save_chqnos(upn, tak, act)
	 Id	  upn;
	 Takdescr tak;
	 Quot	  act;
{ Takitem iip = &tak->rev_c[-1];
  Takitem iiz = &tak->rev_c[tak->rev_last];

  while (++iip <= iiz)
  { // do_db(-1,-1); 
    if (not (iip->props & (K_DEAD+K_EXP)))
    { Quot ix = iip->props & K_PAYMENT ? IX_PMT	 :
    		iip->props & K_SALE    ? IX_SALE : 0;
      if (ix != 0)
      { Lra_t lra;
        Key_t kvi; kvi.integer = iip->id;
      { Sorp_t s_t;
        Sorp sa_ = (Sorp)ix_fetchrec(upn, ix, &kvi, &lra, &s_t);
        if (sa_ == null)
				  i_log(kvi.integer, "do_save_chqnos: 1");
        else
        { if      (ix == IX_SALE)
				    sa_->sale.chq_no = iip->chq_no;
          else
				    sa_->paymt.chq_no = iip->chq_no;
          write_rec(upn, ddict[ix].cl_ix, lra, sa_);
				}
      }}
    }
  }
}

public void do_commit_rev(upn, cid, paylra, pmt)
	Id	upn;	/* write locked */
	Id	cid;	/* not 0 */
	Lra_t	paylra;
	Paymt	pmt;
{ Cash v = pmt->kind & K_ANTI ? - pmt->total : pmt->total;
  Cc cc;
  Int tries = 2;

  while (--tries > 0)
  { cc = load_acc(upn, cid, &g_acc, &ths.acct, v, Q_RECENT | Q_MATCH);
    if (cc == OK)
      break;
    if (cc == NOT_FOUND)
      create_account(upn, cid);
  }
  if (cc != OK)
  { sprintf(&msg[0], "Cannot create account %ld", cid);
    i_log(cc, msg);
    return;
  }
			   /* the entry cannot be marked paid as it is not 
			      to be marked paid in the Post Dated Cheque Account
			    */
{ Paymt_t p_t;
  Paymt pmt__ = (Paymt)read_rec(upn, CL_PMT, paylra, &p_t);
  if (pmt__ != null)
  { pmt__->kind &= ~K_TBP;
    if	    (pmt->kind & K_PD)
    { if (not (pmt->kind & K_ANTI))
				pmt__->kind |= K_PAID;
    }
    else
    { if (pmt->kind & K_ASGD)
      	/*commit_match(Q_CHK, &acc, pmt->balance, -1, v) does nothing!*/;
      else
      { Set set;
				Int matix = match_amount(&set, &g_acc, v);
				if (matix >= MATCHSET)
				{ pmt__->kind |= K_PAID;
				  pmt__->match_grp = last_match_grp;
				}
      }
    }
    (void)write_rec(upn, CL_PMT, paylra, pmt__);
  }

  cc = sorp_to_account(0, cid, IX_PMT, pmt->id, -v);
  if (cc != OK)
  { sprintf(&msg[0], "Payment %ld to account %ld lost, Any Key", pmt->id, cid);
    i_log(cc, msg);
    alert(msg); hold();
    return;
  }
  pmt->balance = ths.acct.balance;

  if (pmt->kind & K_PD)
  { if (pmt->kind & K_ANTI)
    { cc = sorp_to_account(0, PDCHQACC, IX_PMT, pmt->id, -v);
      if (cc != OK)
      { sprintf(&msg[0], "Payment to POST-DATED Cheque Acc lost, Any Key", cid);
				i_log(cc, msg);
				alert(msg); hold();
      }
    }
    else
    { Lra_t mlra;
      Key_t kvi; kvi.integer = pmt->chq_no;   /* set by get_mat_chqs */
    { Cc cc = ix_search(upn, IX_PMT, &kvi, &mlra);
      if (cc == OK)
      { Paymt pt = (Paymt)read_rec(upn, CL_PMT, mlra, &p_t);
        if (pt != null)
				{ pt->kind |= K_PAID;
          (void)write_rec(upn, CL_PMT, mlra, pt);
        }
      }
    }}
  }
  at_xactn(pmt);
}}

private Char * do_post_pmts(upn, tak)
	 Id	  upn;	   /* write locked */
	 Takdescr tak;
{ Char * res = null;
  Lra_t elra;
  Takitem iip = &tak->sal_c[-1];
  Takitem iiz = &tak->sal_c[tak->sal_last];
  Int iter;
  
  while (++iip <= iiz)
  { // do_db(-1,-1); 
    Key_t kvi; kvi.integer = iip->id;
    Sorp_t eny_t;
    Sorp eny = (Sorp)ix_fetchrec(upn, IX_SALE, &kvi, &elra, &eny_t);
    if (eny == null)
      i_log(66, "do_post_pmts: 3");
    else
    { if ((iip->props & K_DEAD)) /* or not (iip->props & K_TBP) */
				continue;
      if (eny->sale.terms & M_ELEC)
				save_extinv((Sale)eny);
    }
  }

  for (iter = 2; --iter >= 0; )
  { iip = &tak->rev_c[-1];
    iiz = &tak->rev_c[tak->rev_last];

    while (++iip <= iiz)
    { // do_db(-1,-1); 
      Set props = iip->props;
      Quot  ix = props & K_SALE	   ? IX_SALE :
	         props & K_PAYMENT ? IX_PMT  : IX_EXP;
      Key_t kvi; kvi.integer = iip->id;
      
      if (iter ? ix != IX_SALE : ix == IX_SALE)
				continue;
      if ((props & K_DEAD) or not (props & K_TBP))
				continue;

      iip->props &= ~K_TBP;
    { Sorp_t eny_t;
      Sorp eny = (Sorp)ix_fetchrec(upn, ix, &kvi, &elra, &eny_t);
      if (eny == null)
      { i_log(66, "do_post_pmts: 2");
				continue;
      }

      if (ix == IX_EXP)
      { at_xactn(eny);
				continue;
      }
    { Quot terms;
      Id cid = eny->sale.customer;
      Cash bal = get_cacc(&cid, &terms, IX_CUST);
 
      if      ((props & K_PAYMENT) and cid != 0)
				do_commit_rev(upn, cid, elra, (Paymt)eny);
      else if (props & K_SALE)
      { Cash v = eny->sale.kind & K_ANTI ? -eny->sale.total : eny->sale.total;

				if ((eny->kind & K_PAID) == 0)
        { Sale_t s_t;
          Sale sa__ = (Sale)read_rec(upn, CL_SALE, elra, &s_t);
          if (sa__ != null)
				  { sa__->kind   |= K_PAID | K_CASH | (iip->props & K_CCHQ);
				    sa__->balance = bal - v;
				    at_xactn(sa__);
				  { Cc cc = sorp_to_account(Q_UPDATE, cid, IX_SALE, eny->sale.id, -v);
				    if (cc != OK)
				      i_log(cc, "Sale %d no longer on Account %d", iip->id, cid);
				    (void)write_rec(upn, CL_SALE, elra, sa__);
				  }}
				}
				else
				{ Takitem iis = my_sale(tak, props, iip->id);	/* sales side */
				  if (iis != null)
				    iis->props &= ~(K_TBP+K_CASH+K_PAID);
				  iip->props = K_DEAD;				/* revenue side */
				{ Paymt_t pmt;
				  Taking_t take;
				  Tak teny;
				  Lra_t lra = find_tak_t(&take, &teny, this_take.id, props, iip->id);
				  if (lra <= 0)
				    i_log(iip->id, "LOST TENY");
				  else
				  { teny->kind &= ~(K_CASH+K_PAID+K_CCHQ);
				    (void)write_rec(upn, CL_TAKE, lra, &take);
				  }
				  pmt.kind     = K_PAYMENT | (iip->props & (K_CCHQ+K_ANTI));
				  pmt.id       = last_id(upn, IX_PMT) + 1;
				  pmt.customer = cid;
				  pmt.cat      = 0;
				  pmt.date     = today;
				  pmt.total    = eny->sale.total;
				  pmt.chq_no   = iip->chq_no;		 /* this is OK. You put the */
				{ Lra_t  lra;				 /* chq_no on the sale      */
				  Cc cc = item_to_tak(this_take.agid, Q_SUBMITV-1, pmt.kind, pmt.id);
				  pmt.agent    = this_take.id;
				  pmt.balance  = bal - v;
				  at_xactn(&pmt);
				  if (cc == OK)
				    cc = i_Paymt_t(upn, &pmt, &lra);
				  if (cc != OK)
				  { i_log(cc, "Do post_pmts");
				    continue;
				  }
				  do_commit_rev(upn, cid, lra, &pmt);

				  (void)add_teny(upn, tak, K_CASH, pmt.kind, pmt.id, &pmt);

				  res = opiypci;
				  sprintf(&ds1[0], "Invoice %d already paid, Press CR", eny->sale.id);
				  salerth(ds1);
				  salert("");
				  i_log(iip->id, eny->kind & K_CASH ? "OPIYPIMP" : "OPIYP");
				  doubt = true;
				}}}
      }
    				/* the following is only done against crashes */
    { Taking_t take;
      Tak teny;
      Lra_t lra = find_tak_t(&take, &teny, this_take.id, props, iip->id);
      if (lra > 0)
      { teny->kind &= ~K_TBP;
				(void)write_rec(upn, CL_TAKE, lra, &take);
      }
    }}}}
  }

  iip = &tak->sal_c[-1];
  iiz = &tak->sal_c[tak->sal_last];

  while (++iip <= iiz)
  { // do_db(-1,-1); 
    Key_t kvi; kvi.integer = iip->id;
    if (not (iip->props & K_TBP))
      continue;
    iip->props &= ~K_TBP;
  { Sale_t sale_t;
    Sale sale__ = (Sale)ix_fetchrec(upn, IX_SALE, &kvi, &elra, &sale_t);
    if	    (bad_ptr(sale__))
      i_log(66, "do_post_pmts: 1");
    else if (sale__->kind & K_MOVD)
    { Quot terms;
      Id cid = sale__->customer;
      Cash bal = get_cacc(&cid, &terms, IX_CUST);
      sale__->balance = bal;
      at_xactn(sale__);
    }
  }}

  sum_tots(tak);
  return res;
}

private Char * save_tak(upn, tak, act)
	 Id	  upn;
	 Takdescr tak;
	 Quot	  act;		      /* save, close, submit */
{ if (this_take.id == 0)
    return ngth;
  if (act == Q_SAVE and not tak->modified)
    return "No changes to save";
  if (act != Q_CLOSE and not tak->rw)
    return "Not writeable";
  if (act == Q_CLOSE and this_take.version >= Q_CLOSEV)
    return "Already closed";
  if (act == Q_SUBMIT and sum_owe(tak) != 0)
    return "Not Balanced";
  if (act != Q_SAVE and tak->tot_journal != 0)
  { sprintf(&ds1[0], "Manual ADD/SUB unbalanced by %ldp", tak->tot_journal);
    return &ds1[0];
  }

{ Cash pjschk = sum_cash(&this_take) - tak->tot_money;
  if (pjschk != 0)
  { i_log(pjschk, "PJSCHK %ld %ld", tak->id, tak->tot_money);
    dump_blk((char*)&this_take, (sizeof(this_take) + 31) / 32);
    return "Internal Error, PLEASE write down what you did";
  }

  if (act == Q_SUBMIT and in_local)
    act = Q_SAVE;

{ Key_t kvi;  kvi.integer = this_take.id;
  tak->id = this_take.id;
  wr_lock(upn);
{ Char * res = load_newest(upn, tak, &this_take);
  Lra_t  lra;
  Cc cc = ix_search(upn, IX_TAKE, &kvi, &lra);
  if (cc != OK and res == null)
    res = "Lost the takings";
  if (res == null)
  { Taking_t t_t;
    Taking take__ = (Taking)read_rec(upn, CL_TAKE, lra, &t_t);
   /* Taking take__ = (Taking)rec_mark_ref(upn, CL_TAKE, lra);*/
    if (take__ != null)
    { memcpy(&take__->u5000, &this_take.u5000,
				(Int)&this_take.eny[0]-(Int)&this_take.u5000);

      pjschk = sum_cash(&this_take) - tak->tot_money;
      if (pjschk != 0)
				i_log(pjschk, "Zeroing (%d) Caught on %ld", tak->tot_money, this_take.id);
      if (act == Q_CLOSE and take__->version < Q_CLOSEV)
      { take__->version = Q_CLOSEV;
				this_take.version = take__->version;
      }
      do_save_chqnos(upn, tak, act);
      if (act == Q_SUBMIT)
      { res = do_post_pmts(upn, tak);
				tak_to_bank(upn, in_local ? Q_SUBMITV : Q_CLOSEV - 1, &this_take);
				kvi.integer = this_bank.id;
				take__->tdate	= today;
				take__->version = Q_SUBMITV;
				take__->bankid	= this_bank.id;
				this_take = *take__;
			  
				tak->rw = false;
				at_take(&this_take);
				this_take.id = 0;
				wclrgrp();
      }
      (void)write_rec(upn, CL_TAKE, lra, take__);
    }
    if (act != Q_SAVE)
    { kvi.integer = this_take.agid;
    { Agent_t a_t;
      Agent ag__ = (Agent)ix_fetchrec(upn, IX_AGNT, &kvi, &lra, &a_t);
      if (ag__ != null)
      { ag__->tak_id = 0;
				this_agent = *ag__;
        (void)write_rec(upn, CL_AGNT, lra, ag__);
      }
    }}
  }
  wr_unlock(upn);

  return res;
}}}}


extern Id global_lock();
extern Id global_unlock();



private Cc save_tt_tak(tak)
	 Takdescr tak;
{ FILE * op = NULL;

	Takitem iip = &tak->sal_c[tak->sal_base-1];
  Takitem iiz = &tak->sal_c[tak->sal_last];

	while (++iip <= iiz)
	{ Char fnbuff[256];
	  Char * fname = prepare_dfile(&fnbuff[0], "tt_inv", iip->id);
	  if (fname == null || file_exists(fname) == 0)
			continue;

		if (op == null)
		{ (void)global_lock();
			op = fopen("tt_inv/tt_list.dat","a+");
			if (op == null)
			{	global_unlock();
				return -1;
			}
		}

		fprintf(op,"%d\n", iip->id);
	}

	if (op != null)
	{ fclose(op);
		global_unlock();
	}

	return OK;
}

private Char * do_edit_details(upn, fld, tak, rw, ds)
	 Id	  upn;
	 Field	  fld;	      /* W_ID, W_NAME, or W_TNO only */
	 Takdescr tak;
	 Bool	  rw;		/* acts as a mask */
	 Char *   ds;
{ Id tid;

  if (tak->rw and tak->modified)
    return "Save or Drop first";

  if (fld->id == W_NAME)
  { Agent_t sagent; sagent = that_agent;
    if (get_one_agent(upn, ds, 0, put_details) == 0)
    { that_agent = sagent;
      put_details();
      return null;
    }
    tid = that_agent.tak_id;
  }
  else if (fld->id == W_TNO)
  { tid = atol(ds); 
    if (not vfy_integer(ds))
      return nreq;
  }
  else 
    return cndo;

{ Cc cc = lddsp_tak(upn, tak, tid, false, false);
  if (cc != SUCCESS)
    return ntfd;

  if (am_admin and this_agent.id != that_agent.id)
    this_agent = that_agent;

  tak->rw = tak->rw and rw;
  return null;
}}

private Accitem my_acc_sale(acc, id)
	 Accdescr acc;
	 Id	  id;
{ fast Accitem it = acc->c;
  fast Int ix;
  for (ix = acc->curr_last+1; --ix >= 0; )
  { if ((it->props & K_SALE) and it->id == id)
      return it;
    ++it;
  }
  
  return null;
}

static Id   item_inv;	      /* local to prep_tak_item, do_tak_item */
static Cash item_amt;
static Descn exp_descn;
static Int  adjino;

static const Char inmg[] = "Invoice %d %s";

static const Char qmsg[] = "inv-no or cust-name or # cust-no or ## bank-no or ME ";
static const Char mmsg[] = "inv-no or cust-name or # cust-no";
static const Char imsg[] = "inv-no";
static const Char emsg[] = "Description";

static Takitem_t ti_buff;
static Set inv_set = 0;
static Cash inv_tot = 0;


private Char * prep_tak_list(tak, row, d)
	 Takdescr tak;
	 Short	  row;
	 Char *   d;
{ item_inv = 0;
  item_amt = 0;
  inv_set = 0;
  this_custmr.id = 0;

  if (xactn_typ == 'G')
    return cndo;
    
  while (true)
  { // do_db(-1,-1); 
    d = skipspaces(d);
  { Id inv_id = atoi(d);
    if (inv_id < adjino - 1000)
      inv_id += adjino;
    if (not in_range(*d, '0', '9'))
      break;
    if      (this_custmr.id == 0)
    { Key_t kvi; kvi.integer = inv_id;
    { Sale_t ss_t;
      Sale sa = (Sale)ix_fetchrec(mainupn, IX_SALE, &kvi, null, &ss_t);
      if (sa == null)
      { sprintf(&ds1[0], inmg, kvi.integer, ntfd);
				return ds1;
      }
    { Quot terms;
      Id cid = sa->customer;
      (void)get_cacc(&cid, &terms, IX_CUST);
      this_custmr.id = cid;
    { Cc cc = load_acc(mainupn,cid, &g_acc, &ths.acct, 0,Q_UNPAID/*|Q_RECENT*/);
      if (cc != OK)
				return ltcr;    
    }}}}
  { Accitem iis = my_acc_sale(&g_acc, inv_id);
    if (iis == null or (iis->props & K_PAID))
    { sprintf(&ds1[0], inmg,inv_id, iis != null ? alrp : "different customer");
      return ds1;
    }
  { Int offs = iis - g_acc.c;
    if (offs > 31)
      return "Too many unpaid invoices";
    
    inv_set |= 1 << offs;
    item_amt += iis->props & K_ANTI ? -iis->val : iis->val;
    d = strchr(d, '+');
    if (d == null)
      break;
    ++d;
  }}}}
  if (item_amt <= 0)
    return "Must add up positive";
  fmt_data(&ds1[0], "Add up to %v, Press / to confirm", item_amt);
  return salerth(ds1) == '/' ? null : aban;
}

void hide_bank_acc(char * buf)

{ const char tr[] = "753869421";
  int ix;

  while (! in_range(*buf, '1', '9') && *buf != 0)
    ++buf;

  for (ix = -1; buf[++ix] != 0; )
    if (in_range(buf[ix], '1', '9'))
      buf[ix] = (tr[buf[ix]-'1']-'1'+ix) % 9 + '1';
}


void unhide_bank_acc(char * buf)

{ const char tr[] = "983765146";
  int ix;

  while (! in_range(*buf, '1', '9') && *buf != 0)
    ++buf;

  for (ix = -1; buf[++ix] != 0; )
    if (in_range(buf[ix], '1', '9'))
      buf[ix] = tr[(buf[ix]-'1'-ix+99) % 9];
}


void get_bank_details()

{ if (this_custmr.id == 0 or
      this_custmr.bank_acc != 0 and this_custmr.bank_sort_code != 0 and
      not in_range( (get_rand() & 0xfff), 1234, 1264))
    return;
{ Customer_t newcust;
  char buf[100];

  while (true)
  { salert("Bank Sort Code");
  { Cc cc = get_data(buf);
    Int ix;
    for (ix = -1; buf[++ix] != 0 and cc == OK; )
    { if      (buf[ix] == '-')
      { strcpy(&buf[ix], &buf[ix+1]);
        --ix;
      }
      else if (not in_range(buf[ix] , '0','9'))
        break; 
    }
    if (cc == OK and buf[ix] == 0)    
    { Char * d_ = get_qty(buf, &newcust.bank_sort_code);
      if (d_ > buf and in_range(newcust.bank_sort_code, 011111, 999999))
        break;
    }
    alert("Should be like 12-34-56");
  }}

  while (true)
  { salert("Bank Acc Number");
  { Cc cc = get_data(buf);
    Int ix;
    for (ix = -1; buf[++ix] != 0 and cc == OK; )
    { 
      if (not in_range(buf[ix] , '0','9'))
        break; 
    }
    hide_bank_acc(buf);
    if (cc == OK and buf[ix] == 0)    
    { Char * d_ = get_qty(buf, &newcust.bank_acc);
      if (d_ > buf and newcust.bank_acc > 1000000)
        break;
    }
    alert("Should have at least 7 digits");
  }}
    
  if (this_custmr.bank_sort_code == newcust.bank_sort_code and
      this_custmr.bank_acc       == newcust.bank_acc)
    return;

{ Cc yncc = OK; 
  if (this_custmr.bank_acc != 0)
  { char ubuf[20], mbuf[40];
    sprintf(&ubuf[0], "%d", this_custmr.bank_acc);
    unhide_bank_acc(ubuf);
    unhide_bank_acc(buf);
    sprintf(&mbuf[0], "Changed, was: %d %s  now: %d %s. Overwrite?", 
       			this_custmr.bank_sort_code, ubuf,
       			newcust.bank_sort_code,      buf);
    alert(mbuf);
    yncc = get_data(buf);
    if (yncc != OK or toupper(buf[0]) != 'Y')
    { salert("Not Changed");
      return;
    }
  }
  
  if (this_custmr.bank_acc != 0)
  { FILE * op = fopen("oldbkaccs", "a");
    if (op != NULL)
    { sprintf(buf, "Cust Id %d, SC %d, No %d", this_custmr.id,
    					       this_custmr.bank_sort_code,
       					       this_custmr.bank_acc);
      fprintf(op, "%s\n", buf);
      fclose(op);
    }
  }

{ Key_t  kv_t;  kv_t.integer = this_custmr.id;
  wr_lock(mainupn);
{ Lra_t lra;
  Customer_t py_t;
  Customer py = (Customer)ix_fetchrec(mainupn, IX_CUST, &kv_t, &lra, &py_t);
  if      (py == null)
    i_log(0, "Lost Cust Upd BA %d", this_custmr.id);
  py->bank_sort_code = newcust.bank_sort_code;
  py->bank_acc = newcust.bank_acc;
  
  (void)write_rec(mainupn, CL_CUST,lra,(Char*)py);

  kv_t.integer = this_custmr.bank_acc;
  if (kv_t.integer != 0)
  { Cc cc = ix_delete(mainupn, IX_BACC, &kv_t, &lra);
    if (cc != OK)
      i_log(this_custmr.id, "Couldnt Delete bA ix");
  }

  if (py != null)
    this_custmr = *py;

  kv_t.integer = this_custmr.bank_acc;
  if (kv_t.integer != 0)
    (void)ix_insert(mainupn, IX_BACC, &kv_t, &lra);

  wr_unlock(mainupn);
}}}}}
  

static const Char paop[] = "Positive amounts only please";

private Char * prep_tak_item(tak, row, data)
	 Takdescr tak;
	 Short	  row;
	 Char *   data;
{ Key_t kvi;
  Set cat = K_PAYMENT | K_TBP;
  Int  bank_no = 0;
  Int  chq_no = -1;

  if (not tak->rw)
    return rdoy;

  this_custmr.id = 0;
  this_custmr.surname[0] = 0;
  item_inv = 0;
  inv_set = 0;

{ Bool getp2 = true;
  Char * d = skipspaces(data);
  Char * d_ = xactn_typ == 'E'	      ? &d[strlen(d)]		           :
	      in_range(*d, '0', '9')  			? get_qty(d, &item_inv)	     :
	      *d == '#'  ? /* d[1] == '#'   ? get_qty(&d[2],&bank_no)    : */
	                                  		get_qty(&d[1],&this_custmr.id) :
																				get_b_sname(d,&this_custmr.surname[0]);
  if (d_ <= d)
    return xactn_typ == 'Q' ? qmsg : 
				   xactn_typ == 'M' ? mmsg :
				   xactn_typ == 'G' ? imsg : emsg;
	   
  if (item_inv != 0)
  { if (item_inv < adjino - 1000)
      item_inv += adjino;
				/* invno+invno+invno */
    if (strchr(d, '+') != null)
    { Char * res = prep_tak_list(tak, row, d);
      if (res != null)
				return res;
      cat |= K_ASGD;			/* overwritten below! */
      sprintf(&d_[0], "%d", item_amt);
    }
  }
  else
    if (xactn_typ == 'G')
      return "You can only get invoices";

  if	  (*strmatch("ME", this_custmr.surname) == 0 and
   			   this_custmr.surname[2] == 0)
  { if (xactn_typ != 'Q')
      return "ME only valid with cheQues";
    this_custmr.id = 0;
    strcpy(&this_custmr.surname[0], "For Cash");
  } 
  else if (xactn_typ == 'E')
  { cat = K_EXP;
    strpcpy(&exp_descn[0], d, sizeof(Descn));
  }
  else if (item_inv != 0)
  { if (item_inv == 999999)
    { item_inv = get_deliv_inv();
      if (item_inv == 0)
        return "All submitted";
      getp2 = false;
    }
    kvi.integer = item_inv;
  { Sale sa = (Sale)ix_fetchrec(mainupn, IX_SALE, &kvi, null, null);
    if (sa == null)
      return "invoice not there";

    item_amt = sa->total;
    this_custmr.id = sa->customer;
    cat = sa->kind & (K_SALE+K_ANTI+K_CASH+K_CCHQ);
    if (cat & K_CCHQ)
      chq_no = sa->chq_no;
    if (sa->kind & (K_PAID+K_CASH))
    { alert("Warning: Already paid");
      hold();
    }
  }}
  else if (this_custmr.id == 0) 
  { if (bank_no != 0 and false)
    { kvi.integer = bank_no;
    { Customer cust = (Customer)ix_fetchrec(mainupn,IX_BACC,&kvi, null, &this_custmr);
      if (cust != null)
      { if (was_mult())
          return "Not Unique";
      }
      else
      { salert("Which customer is this?");
        get_data(data);
        get_b_sname(data,&this_custmr.surname[0]);
      }
    }}
    this_custmr.id = get_one_cust(mainupn, this_custmr.surname,
                      						P_ACCONLY+P_NOSPEC, put_custmr);
    if (this_custmr.id == 0)
      return "Nobody Found";
    if (bank_no != 0 and false)
    { wr_lock(mainupn);			/* record the bank_acc */
    { Key_t kvi; kvi.integer = this_custmr.id;
    { Lra_t lra;
      Customer_t c_t;
      Customer cu__ = (Customer)ix_fetchrec(mainupn, IX_CUST, &kvi, &lra, &c_t);
      if (cu__ != null)
      { cu__->bank_acc = bank_no;
        (void)write_rec(mainupn, CL_CUST, lra, cu__);
				kvi.integer = bank_no;
				(void)ix_insert(mainupn, IX_BACC, &kvi, &lra);
      }
      wr_unlock(mainupn);
    }}} 
  }

  if (this_custmr.id != 0 and this_custmr.surname[0] == 0)
  { if (get_cust(mainupn, IX_CUST, this_custmr.id) == null)
      return ltcr;
    if (this_custmr.id < 0 and
    	this_custmr.id != BANKACC)
      return "Only the Receipts account allowed";
    if (item_inv == 0)
    { if (bill_alt(this_custmr.terms))
      { sprintf(&ds[0], "Account is with %d", this_custmr.creditlim);
				return &ds[0];
      }
      if (this_custmr.terms & P_DLTD)
				return "Account is closed";
    }
  /*i_log(this_custmr.id, "takcust");*/
    put_custmr();
  }
  clr_item(W_XID+row);
{ Sid spos = save_pos();
  fld_put_data(W_XNM+row, xactn_typ == 'E' ? exp_descn : this_custmr.surname);
  restore_pos(spos);

{ Cash vat = 0;
  Cash amount;
  if (*d_ == 0)
    salert(amnt);
  
  if (getp2)
  while (true)
  { Cc cc = *d_ != 0 ? SUCCESS : get_data(d_);
    Char * d__ = get_qty(d_, &amount);
    if	    (d__ <= d_)
      alert(amip);
    else if (item_inv != 0 and item_amt != amount)
    { static Char amt_msg[30];
      fmt_data(&amt_msg[0], "Amount of invoice      %10v", item_amt);
      return amt_msg;
    }
    else if (amount <= 0 and item_inv == 0)
      return paop;
    else
    { d_ = d__;
      break;
    }
    d_ = data;
    *d_ = 0;
  }
  if (item_inv != 0)
  { goto_fld(W_XVAL+row);   put_cash(item_amt);
    restore_pos(spos);
  }

  if (xactn_typ == 'E')
  { cat = K_EXP + K_TBP;
    vat = amount * uniques.vatrate / (1000 + uniques.vatrate);

    fmt_data(&ds1[0], "VAT? (       %10v)", vat);
    if (*d_ == 0)
      salert(ds1);
    while (true)
    { Cc cc = *d_ != 0 ? OK : get_data(d_);
      Char * d__ = get_cash(d_, &vat);
      if	    (d__ < d_)
				alert(amip);
      else if (vat < 0)
				return paop;
      else
      { d_ = d__;
				break;
      }
      d_ = data;
      *d_ = 0;
    }
    if (vat != 0)
      cat |= K_VAT;
  }

  if (xactn_typ == 'Q' and (cat & (K_ANTI+K_SALE)) != K_ANTI+K_SALE 
		       and chq_no == -1)
  { cat |= K_CCHQ;
    get_bank_details();  
    chq_no = 0;
    if (*d_ == 0)
      salert("Cheque No please");
    while (true)
    { if (*d_ == 0)
        get_data(d_);
    { Char * d__ = *d_ == 0 ? &d_[1] : get_qty(d_, &chq_no);
      if (d__ <= d_)
				alert(nong);
      else 
				break;
      d_ = data;
      *d_ = 0;
    }}
  }
  clr_item(W_XID+row); 
{ Takitem it = &ti_buff;
  it->props  = cat;
  it->id     = item_inv;
  it->cid    = this_custmr.id;
  strpcpy(&it->name[0], xactn_typ == 'E' ? exp_descn : this_custmr.surname, 
							    sizeof(Surname)-1);
  it->vat    = vat;
  it->val    = amount;
  it->chq_no = chq_no;
  return null;
}}}}}

private Char * do__tak_item(upn, tak)
	 Id	   upn;  /* write locked */
	 Takdescr  tak;
{ Char * res = load_newest(upn, tak, &this_take);
  if (res != null)
    return res;

  if (item_inv != 0)
  { Lra_t lra;
    Key_t kvi;	kvi.integer = item_inv;
  { Sale_t  sale_t;
    Sale sa = (Sale)ix_fetchrec(upn, IX_SALE, &kvi, &lra, &sale_t);
    if (sa == null)
      return "invoice lost";
    if (sa->kind & K_DEAD)
      return "Invoice Deleted";
    if (ti_buff.chq_no >= 0)
      sa->chq_no = ti_buff.chq_no;
  { Kind nkind = (sa->kind & (K_SALE+K_ANTI)) 				   | 
            		 (xactn_typ=='G' and not (sa->kind & K_CASH) ? 0 : K_CASH) |
    	           (xactn_typ=='G' or      (sa->kind & K_CASH) ? 0 : K_TBP)  |
            		 (ti_buff.props & K_CCHQ);
    Kind wh = nkind & (K_SALE+K_CASH);
    Takitem td = xactn_typ == 'G' ? my_sale(tak, sa->kind, sa->id)
											  				  : my_rev (tak, sa->kind, sa->id);
    if (td != null)
      return xactn_typ == 'G' ? "Its yours already" : alrp;

    if (xactn_typ != 'G' and my_sale(tak, sa->kind, sa->id) != null)
      return "It's yours, M or Q in Id Field";
 
		kvi.integer = (int)sa->agent;
	{ Id tid = 0;
		Lra_t lraa;
		Agent_t ag_t;
		Agent ag = (Agent)ix_fetchrec(upn, IX_AGNT, &kvi, &lraa, &ag_t);
    Kind that_kind;
    int iter;
    char * ustrm = NULL;
    for (iter = -1; ++iter < 100; )
	  { 
			if (ag != null)
			{ tid = ag->tak_id;
		  {	Cc cc = item_from_tak(upn, tid, sa->kind, sa->id, &that_kind);
		    if (cc == OK)
		    	break;
		  }}
	  	if (ustrm == NULL)
	   	  ustrm = new_stream(upn, CL_AGNT);
		  ag = (Agent)next_rec(ustrm, &lraa, &ag_t);
		  if (ag == NULL)
	    	break;
	  }
	  
    if      (ag == NULL)
      return "Already submitted";
    else if ((sa->kind & K_CASH) and xactn_typ == 'G')
      alert("Warning: The other taking paid it");

	{ Cc cc = item_to_tak(this_agent.id, Q_SUBMITV-1, nkind, sa->id);
    if (cc != OK)
      i_log(32, "Lost your takings");
    tak->id = this_take.id;
//  sa->kind |= K_MOVD;
//  sa->agent = this_take.id & 0xffff;
    sa->chq_no = ti_buff.chq_no;
    (void)write_rec(upn, CL_SALE, lra, sa);

    (void)add_teny(upn, tak, wh, nkind, sa->id, sa);
  { Taking_t take;
    Tak teny;
    Lra_t lra = find_tak_t(&take, &teny, this_take.id, K_SALE, sa->id);
    if (lra > 0)
    { teny->kind |= nkind;
      (void)write_rec(upn, CL_TAKE, lra, &take);
    }
  }}}}}}
  else

  { Takitem it = extend_rev(tak);
    *it = ti_buff;
    it->props |= K_TBP;
    tak->modified = true;
    tak->rev_last -= 1;			/* roll back */

    if (this_take.id == 0)	      /* open the takings to set this_take.id */
    { Taking_t stake; stake = this_take;
      item_to_tak(this_agent.id, Q_CLOSEV-1, 0, 0);
      memcpy(&this_take.u5000, &stake.u5000,
				(Int)&this_take.eny[0]-(Int)&this_take.u5000);
      tak->id = this_take.id;
    }

    if	    (ti_buff.props & K_EXP)
    { Expense_t exp;
      exp.kind	   = K_EXP;
      exp.id	   = last_id(upn, IX_EXP) + 1;
      exp.date	   = today;
      exp.total    = it->val;
      exp.vat      = it->vat;
      strpcpy(&exp.descn[0], exp_descn, sizeof(Descn));
      it->id = exp.id;

    { Cc cc = item_to_tak(this_agent.id, Q_SUBMITV-1, K_EXP, exp.id);
      if (cc == OK)
      { Lra_t  lra;
        exp.agent  = this_take.id;
        cc = i_Expense_t(upn, &exp, &lra);
      }
      if (cc != OK)
				return "Corrupt Expense Area";
    }}
    else 
    { Paymt_t pmt;
      pmt.kind	   = ti_buff.props;
      pmt.id	   = last_id(upn, IX_PMT) + 1;
      pmt.customer = ti_buff.cid;
      pmt.date	   = today;
      pmt.total    = ti_buff.val;
      pmt.chq_no   = ti_buff.chq_no;
      pmt.balance  = inv_set;
      it->id = pmt.id;
    { Cc cc = item_to_tak(this_agent.id, Q_SUBMITV-1, it->props, pmt.id);
      if (cc != OK)
				return "Corrupt Pai Area";
      if (cc == OK)
      { Lra_t  lra;
        pmt.agent = this_take.id;
        cc = i_Paymt_t(upn, &pmt, &lra);
        if (cc != OK)
				  return "Corrupt Pay Area";
      }
    }}
    tak->rev_last += 1;
  }
  return null;
}

private Char * do_tak_item(upn, tak)
	 Id	   upn;
	 Takdescr  tak;
{ wr_lock(upn);
{ Char * res = do__tak_item(upn, tak);
  wr_unlock(upn);
  return res;
}}



private Char * do_cash_item(upn, fldtyp, tak, take, ds)
	 Id	  upn;
	 Id	  fldtyp;
	 Takdescr tak;	 /* or Bankdescr */
	 Taking   take;
	 Char *   ds;
{ if (not tak->rw)
    return rdoy;
  ds = skipspaces(ds);
{ Quot denom = (fldtyp - W_5000) >> LOG_ID_FAC;
  Short * p = &((Short*)&take->u5000)[denom];
  Cash unit = denom_to_val[denom];
  Int sign = *ds == '+' ? 1  : 
				     *ds == '-' ? -1 : 0;
  Cash amt;
  Char * d = sign != 0 ? ds+1 : ds;
  Char * dd = get_cash(d, &amt);
  if (dd <= d)
    return amip;

  if (amt % unit != 0)
    return "Cannot be";
  
  amt /= unit;
  
  if	  (sign == 0)
    *p = amt;
  else 
    *p += amt * sign;
  tak->tot_money = sum_cash(take);
  tak->modified = true;
  put_denoms(take);
  if (take == &this_take)
    put_totals(tak);
  else
    put_btotals(tak);
  return null;
}}

Cc do_tak(Id, Takdescr, Bool, Char *) forward;
Char * do_get_mats(Id, Date, Takdescr) forward;


private Char * do_v_tak(upn, tak, tid, ds)
	 Id	  upn;
	 Takdescr tak;
	 Id	  tid;
	 Char *   ds;
{ wopen(tbl1, SI1over);
  init_tak(tak);
{ Cc cc = lddsp_tak(upn, tak, tid, false, false);
  if (cc != SUCCESS)
     return ntfd;
  tak->rw = false;
  (void)do_tak(upn, tak, false, ds);
  return null;
}}



public const Char * do_takings()

{ real_agent_props = this_agent.props;
  rd_lock(mainupn);
  get_taking(&this_agent, this_agent.id, false);
  if (not in_local)
    this_bank.id = 0;
  rd_unlock(mainupn);

  if (this_take.id != 0 and (this_agent.id != this_take.agid or
											       this_take.version >= Q_SUBMITV))
  { Lra_t aglra;				/* temp patch pjs */
    Key_t kvi; kvi.integer = this_agent.id;
    wr_lock(mainupn);
  { Agent_t a_t;
    Agent ag__ = (Agent)ix_fetchrec(mainupn, IX_AGNT, &kvi, &aglra, &a_t);
    if (ag__ != null)
    { ag__->tak_id = 0;
      (void)write_rec(mainupn, CL_AGNT, aglra, ag__);
    }
    wr_unlock(mainupn);
    i_log(this_take.agid, "Wrong Take %d", this_take.id);
  }}

{ Agent_t sag; sag = this_agent;	/* precautionary only ? */
  init_take();
  init_tak(&tak_t);
  tak_t.rw = not upn_ro;
  wopen(tbl1, SI1over);

{ Cc cc = lddsp_tak(mainupn, &tak_t, this_take.id, false, true);
#if 0
  if (in_local and tak_t.rw)
  { Int delay = date_to_day(today) == 5 ? 3 :
	        date_to_day(today) == 6 ? 2 :
					  1;
    Char * res = do_get_mats(mainupn, today + delay, &tak_t);
    if (res != null)
    { alert(res); hold(); }
    clr_alert();
  }
#endif
{ Char * res = do_tak(mainupn, &tak_t, true, ds) == OK ? null : "Failed";
  if (sag.id != this_agent.id)
    this_agent = sag;
  return res;
}}}}

private Char * do_edit_chqn(upn, tak, ix, ds)
	 Id	  upn;
	 Takdescr tak;
	 Int	  ix;
	 Char *   ds;
{ Takitem ti = tak->sal_mode ? &tak->sal_c[ix]
			     : &tak->rev_c[ix];
  Set props = ti->props;
  Lra_t  lra_;
  Key_t kv_t; kv_t.integer = ti->id;

  if (not vfy_ointeger(ds))
    return nong;

  if (not ((props & K_PAYMENT) or 
	   (props & K_SALE) and (props & K_CASH)))
    return "Must be a payment";

  wr_lock(upn); 
{ Quot dix = props & K_SALE ? IX_SALE : IX_PMT;
  Sorp_t ss_t;
  Sorp sp = (Sorp)ix_fetchrec(upn, dix, &kv_t, &lra_, &ss_t);
  Char * res = sp != null ? null : "lost it";
  if (res == null)
  { Int chq_no = *skipspaces(ds) == 0 ? -1 : atoi(ds);
    ti->chq_no = chq_no;
    ti->props  = chq_no == -1 ? props & ~K_CCHQ : props | K_CCHQ;
    recalc(tak);
    sp->paymt.kind &= ~K_CCHQ;
    sp->paymt.kind |= (ti->props & K_CCHQ);
    sp->paymt.chq_no = chq_no;
    write_rec(upn, ddict[dix].cl_ix, lra_, sp);
  { Taking_t take;
    Tak teny;
    Lra_t lra = find_tak_t(&take, &teny, this_take.id, props, kv_t.integer);
    if (lra > 0)
    { teny->kind &= ~K_CCHQ;
      teny->kind |= (chq_no == -1 ? 0 : K_CCHQ);
      (void)write_rec(upn, CL_TAKE, lra, &take);
    }
  { Takitem td = tak->sal_mode ? my_rev(tak, props, kv_t.integer) 
			       : my_sale(tak, props, kv_t.integer);
    if (td != null)
    { td->props &= ~ K_CCHQ;
      td->props |= (chq_no == -1 ? 0 : K_CCHQ);
      td->chq_no = chq_no;
    }
  }}}
  wr_unlock(upn);
  if (tak->sal_mode)
    put_sale(tak, ix);
  else
    put_rev(tak, ix);
  return res;
}}

private void kill_my_rev(tak, kind, id)
	 Takdescr tak;
	 Kind	  kind;
	 Int	  id;
{ Takitem ip = &tak->rev_c[-1];
  Takitem iz = &tak->rev_c[tak->rev_last];

  while (++ip <= iz)
    if ((ip->props & (K_SALE + K_PAYMENT + K_EXP)) ==  
	     (kind & (K_SALE + K_PAYMENT + K_EXP)) 
	 and ip->id == id)
      ip->props |= K_DEAD;
}




private Char * do_payi(upn, tak, ix, ch)
	 Id	  upn;
	 Takdescr tak;
	 Int	  ix;
	 Char	  ch;
{ Int row = ix - tak->sal_base;
  Int chq_no = -1;
  Sale_t  sa_t;
  Sale sa;

  if (tak->sal_mode)
  { Lra_t lra;
    Key_t kvi;  kvi.integer = tak->sal_c[ix].id; 
    sa = (Sale)ix_fetchrec(upn, IX_SALE, &kvi, null, &sa_t);
    if (sa == null)
      return "invoice lost";
    if (sa_t.kind & (K_CASH + K_PAID))
      return alrp;
  }

  ds[0] = 0;
  if (ch == 'Q')
  { salert("Cheque No");
    goto_fld(W_CHQN+row); 
    get_data(&ds[0]);
    chq_no = atol(ds);
    goto_fld(W_XID+row);
  }
  if (not tak->sal_mode)
    return do_edit_chqn(upn, tak, ix, ds);
  if (my_rev(tak, K_SALE, sa_t.id) != null)
    return alrp;

{ Kind nkind = (sa_t.kind & (K_SALE+K_ANTI)) | K_CASH | K_TBP |
    		 	     (ch != 'Q' ? 0 : K_CCHQ);
  Takitem iip = add_teny(upn, tak, K_CASH, nkind, sa_t.id, &sa_t);
  iip->chq_no = chq_no;
  iip = my_sale(tak, nkind, sa_t.id);
  if (iip != null)
  { iip->props |= nkind;
    iip->chq_no = chq_no;
  }
  wr_lock(upn);
{ Taking_t take;
  Tak teny;
  Lra_t lra = find_tak_t(&take, &teny, this_take.id, K_SALE, sa_t.id);
  if (lra > 0)
  { teny->kind |= nkind;
    (void)write_rec(upn, CL_TAKE, lra, &take);
  }
  wr_unlock(upn);
  put_sale(tak, ix);
  recalc(tak);
  return null;
}}}

private Char * do_s_xfer(upn, tak, ix, aid)
	 Id	  upn;
	 Takdescr tak;
	 Int	  ix;
	 Id	  aid;
{ if (aid == 0 or not tak->sal_mode)
    return "You must be assigning in sales";

  wr_lock(upn); 
{ Set props;
  Id id = tak->sal_c[ix].id;
  Cc cc = item_from_tak(upn, this_take.id, tak->sal_c[ix].props, id, &props);
  if (cc == NOT_FOUND)
   	i_log(this_take.id, "Invoice %d not in tak", id);

  if (cc == SUCCESS)
  { Taking_t stake; stake = this_take;
    this_take.id = 0;
  { Cc cc = item_to_tak(aid, Q_CLOSEV-1, props, id);
    if (cc != OK)
      i_log(cc, "itt %ld", id);  
    tak->sal_c[ix].props |= K_DEAD;
    kill_my_rev(tak, props, id);
  { Lra_t lra;
    Key_t key;	key.integer = id;
  { Quot ix = props & K_SALE ? IX_SALE :
	      props & K_EXP  ? IX_EXP  : IX_PMT;
    Int spos = save_pos();
    Field fld;
    Sorp_t s_t;
    Sorp eny__ = (Sorp)ix_fetchrec(upn, ix, &key, &lra, &s_t);
    if (eny__ != null)
    { eny__->sale.kind |= K_MOVD;
      eny__->sale.agent = this_take.id;
      (void)write_rec(upn, ddict[ix].cl_ix, lra, eny__);
    }
    right_fld(0, &fld); 
    put_data("GONE");
    recalc(tak);
    restore_pos(spos);
    this_take = stake;
  }}}}
  wr_unlock(upn);
  return cc == SUCCESS ? null : "Failed to remove"; 
}}

private Char * del_sorp(upn, tak, iix)
	 Id	  upn;
	 Takdescr tak;
	 Int	  iix;
{ Char reason[30];
  Takitem ip = tak->sal_mode ? &tak->sal_c[iix] : &tak->rev_c[iix];
  Set props = ip->props;
  Char * res = null;
  Lra_t lra;
  Key_t key; key.integer = ip->id;

  if ((props & K_PAYMENT) and (not (props & K_TBP) or
  				    props & (K_HASPD | K_PD)))
    return "Too late";

  if (tak->sal_mode and disallow_r(R_BODY))
    return preq('Y');
    
  sprintf(ds, tak->sal_mode ? "Delete Invoice %d, Type CR"
  		         					    : "Cancel revenue %d, Type CR", key.integer);
  if (salerth(ds) != A_CR)
    return ndon;

  if (tak->sal_mode)
  { Int spos = save_pos();
    alert("Reason?");
    goto_fld(W_CMD); get_data(&reason[0]);
    restore_pos(spos);
  }
  wr_lock(upn);
{ Cash v = 0;
  Sorp_t srec;
  Taking_t take;
  Tak teny;
  Lra_t tlra = find_tak_t(&take, &teny, tak->id, props, key.integer);
  if      (tlra <= 0)
    res = sodi;
  else
  { Set props = teny->kind;
    Quot ix = props & K_SALE ? IX_SALE :
      	      props & K_EXP  ? IX_EXP  : IX_PMT;
    Sorp sorp = (Sorp)ix_fetchrec(upn, ix, &key, &lra, &srec);
    if (sorp == null)
    { i_log(key.integer, "DSORP");
      res = "Error";
    }
    else
    { if      ((props & K_DEAD) or
	       (props & (K_SALE+K_CASH)) == K_SALE and not tak->sal_mode)
        res = aldo;
      else if (tak->sal_mode)
      { if (srec.kind & (K_CASH+K_PAID) == K_PAID)
				  res = "Somebody else paid it";
      } 
      else
      { if ((props & (K_EXP + K_TBP)) == 0)
				  res = "Cash was taken";
      }
      if (res == null)
      { srec.kind &= ~(K_CASH+K_CCHQ);
				if (tak->sal_mode or not (props & K_SALE))
				  srec.kind |= K_DEAD;
				srec.sale.chq_no = 0;
				if (ix == IX_SALE and ip->cid != 0)
				{ v = (srec.kind & K_ANTI ? -1 : 1) * ip->val;
				  if (tak->sal_mode)			/* deleting invoice */
				  { Takitem ir = my_rev(tak, props, ip->id);
				    if (ir != null)
			    	    { ir->props |= K_DEAD;
				      ir->props &= ~(K_TBP + K_CASH + K_CCHQ);
				    }
				    if (not (props & K_TBP) and (props & K_CASH))
				    { i_log(ip->id, "Deleting with 0");
				      v = 0;
				    }
				    else
				      v = -v;
				    at_xactn(sorp);
				  }
				  else					/* deleting revenue on invoice*/
				  { Takitem is = my_sale(tak, props, ip->id);
				    if (is != null)
				      is->props &= ~(K_TBP + K_CASH + K_CCHQ);

				    if (props & K_TBP)
				      v = 0;
				  }
				  if (v != 0)
				  { Set terms;
				    Id cid = ip->cid;
				    Cash bal = get_cacc(&cid, &terms, IX_CUST);

				    Cc cc = sorp_to_account(Q_UPDATE, cid, IX_SALE, ip->id, v);
				    if (cc != OK)
				      i_log(cc, "STA %d %d", ip->cid, ip->id);
				    srec.sale.balance = ths.acct.balance;
				  }
				}
				(void)write_rec(upn, ddict[ix].cl_ix, lra, &srec);
				teny->kind &= ~(K_CASH+K_CCHQ+K_PAID);
				if (tak->sal_mode or not (props & K_SALE))
				  teny->kind |= K_DEAD;
				(void)write_rec(upn, CL_TAKE, tlra, &take);
				if (tak->sal_mode)
				  undo_invo(srec.sale.id, &reason[0]);
      }
    }
  }

  wr_unlock(upn);

  if (res == null)
  { Field fld;
    Short spos = save_pos();
    ip->props |= K_DEAD;
    ip->props &= ~(K_TBP + K_CASH + K_CCHQ);
    ip->chq_no = 0;
    right_fld(0, &fld); 
    put_data(tak->sal_mode ? "DELETED" : "REV-GONE");
    i_log(ip->id, "Deleted %s %d", tak->sal_mode ? "INV" : "REV", v);
    recalc(tak);
    restore_pos(spos);
    if (tak->sal_mode  && (props & K_DEAD) ==0 && (srec.sale.kind & K_TT))
    { this_sale = srec.sale;
      Cc cc = save_tt_inv();
      if (cc != OK)
        i_log(cc, "Track and trace error");
    }
  }
  return res;
}}

private Char * get_tgt(upn, tak, ds_, xferag_ref)
	 Id	   upn;
	 Takdescr  tak;         
	 Char *    ds_;
	 Agent_t  *xferag_ref;
{ Char * ds; 
  while ((ds = skipspaces(ds_))[0] == 0)
  { salert("Agent?");
    get_data(ds_);
  }
  rd_lock(upn);
  
{ Key_t key;
  Lra_t trash;
  Cc cc = get_sname(ds, &key);
  Agent ag__ = cc != OK ? null : (Agent)ix_srchrec(upn,IX_AGNTNM,&key, &trash);
  Char * error = not tak->sal_mode	    ? "Only in sales"	:
										 ag__ == null		    ? "Agent not found" :
										 ag__->id == this_take.agid ? "Transfer to yourself?!" :
																						      null;
  if (error == null)
    *xferag_ref = *ag__;
  rd_unlock(upn);
  return error;
}}



private void get_cdesc(upn, id, len, rstr1, rstr2)
	  Id	  upn;
	  Id	  id;
	  Int	  len;
	  Char	 *rstr1;   /* inout */
	  Char	 *rstr2; 
{ Key_t kvi; kvi.integer = id;
  rd_lock(upn);
  rstr2[0] = 0;
{ Customer cu__ = (Customer)ix_srchrec(upn, IX_CUST, &kvi, &trash);
  if (cu__ != null)
  { if (cu__->forenames[0]!=0 and strlen(rstr1)+strlen(cu__->forenames) <=len-2)
    { strcat(&rstr1[0], ", ");
      strcat(&rstr1[0], cu__->forenames);
    }
    strpcpy(&rstr2[0], cu__->postaddr, len);
  }
  rd_unlock(upn);
}}

private void do_hl_line(txt, stt, offs)
	 Char * txt;
	 Int    stt, offs;
{ sprintf(&msg[0], "%s\n", hline(78));
  msg[stt] = ' ';
  strcpy(&msg[stt+1], txt);
  msg[strlen(msg)] = ' ';
  prt_text(&msg[offs]);
}



private void prt_saletots(tak)
	 Takdescr  tak;
{ prt_need(11);
  prt_text( "\n      Sales   Excl.Vat     Vat     Returns  Excl.Vat     Vat\n");
{ Char * row_fmt = 
	      "        %5d       %9v       %9v      %5d       %9v       %9v\n";
  prt_text("Cash\n");
  prt_fmt(row_fmt, tak->noo_csales, tak->tot_csales, tak->tot_cvat, 
	   	   tak->noo_crets,  tak->tot_crets,  tak->tot_cvatret);
  prt_text("Book\n");
  prt_fmt(row_fmt,
				   tak->noo_sales - tak->noo_csales, 
				   tak->tot_sales - tak->tot_csales,
				   tak->tot_vat - tak->tot_cvat, 
				   tak->noo_rets - tak->noo_crets, 
				   tak->tot_rets - tak->tot_crets,
				   tak->tot_vatret - tak->tot_cvatret);

  prt_fmt(row_fmt,
				   tak->noo_sales, tak->tot_sales, tak->tot_vat, 
				   tak->noo_rets, tak->tot_rets, tak->tot_vatret);

  prt_text("\n  Discount    Charges     Expenses     (Vat)\n");
  row_fmt =  "       %9v       %10v         %10v       %9v\n";
  prt_fmt(row_fmt, tak->tot_disc, tak->tot_chg,
		   tak->tot_expense, tak->tot_expvat);
}}


private void prt_money(den)
	 Short   den[];
{ Cash val[10];
  fast Int i;
  for (i = 10; --i >= 0; )
    val[i] = den[i] * denom_to_val[i];
  prt_need(13);
  do_hl_line("Money", 7, 0);
  prt_fmt(
  " Notes  (50)       %9v\n"
  "        (20)       %9v\n"
  "        (10)       %8v\n"
  "        (5)        %8v\n"
  " Coin   200p       %8v\n"
  "        100p       %8v\n"
  "         50p       %8v\n"
  "         20p       %8v\n"
  "       5/10p       %8v\n"
  "          1p       %8v\n\n",
    val[0],val[1],val[2],val[3],val[4],val[5],val[6],val[7],val[8],val[9]);
}

  static const Char  t_hdr0[] = 
 "   Id     Type  Customer No, Name                   ";
  static const Char  t_hdr1[] = 
 "  Vat    Amount  Cheque-No";
#define t_hdr1a &t_hdr1[9]
#define t_hdr1b &t_hdr1[5]

  static const Char row_fmt[] = 
 "   %6d     %7l   %5d                           %30l\n"
 "                                                %31l  %9v       %9v      %9r\n";

  static Char tot_fmt[] =
 "Chqs%3d Chq-Tot      %10v         - Future Cheques      %10v         %10v\n"
 "     +  Money        %10v    then + Mature Cheques                   %10v\n"
 "              =      %10v           Bankable tot                     %10v\n"
 "     + Expenses       %8v\n"
 "     - Swaps          %8v\n"
 "     + Owe           %10v\n"
 "             =       %10v\n";

private Char * do_prt_take(tak, dup)
	 Takdescr tak;
	 Bool     dup;
{ Prt chan = select_prt(0);
  if (chan == null)
    return msgpa;

{ Char * sln = "---- Sales ";
  Char who1[31], who2[31];
  if (tak->id == 0)
    tak->id = this_take.id;
  	      
  date_to_asc(&msg[0], this_take.tdate, 0);
  fmt_data(&hhead[0],
			     "  %5d                %16l            %11r      %5r       %10r\n",
				   tak->id, that_agent.name, 
				   this_take.version >= Q_SUBMITV ? dup ? dupmsg :
				   doubt ? "DOUBTFULL" : "SUBMITTED" : tstus[4],
		       date_to_weekday(this_take.tdate), msg);
{ Char * err = prt_one_report(chan, hhead, " TAKINGS\n\n\n\n\n\n\n\n\n\n\n");
  if (err != null)
    return err;

  sprintf(&msg[0], "%s\n%s%s\n%s%s\n",hline(79), t_hdr0, t_hdr1,
	    			 "---- Revenue ", hline(66));
  prt_set_cols(msg, row_fmt);
  prt_head();
  strcat(&hhead[0], " TAKINGS CONTINUED\n");
  prt_set_title(hhead);  

{ Cash mtotal = sum_cash(&this_take);
  Cash owe = sum_owe(tak);
  fast
  Takitem iip = &tak->rev_c[-1];
  Takitem iiz = &tak->rev_c[tak->rev_last];
  Bool have_pdchqs = false;
  Bool have_matchqs = false;
  Char str[40];

  while (++iip <= iiz)
  { Set  props = iip->props;
    Char * dscn = get_ttyp(props);

    if (props & K_DEAD)
      continue;
    if (props & K_PD)
    { if (props & K_ANTI)
				have_pdchqs = true;
      else
				have_matchqs = true;
      continue;
    }
    if (props & K_CASH)
      continue;
    if ((props & K_PAYMENT) and iip->cid == 0)
      dscn = "EXCHANGE";
    sprintf(&str[0], (props & K_CCHQ) == 0 ? "CASH" :
		     props & K_PD          ? "P%d"  : "%d", iip->chq_no);
    strpcpy(&who1[0], iip->name, LSNAME);
    if (props & K_EXP)
      who2[0] = 0;
    else
      get_cdesc(mainupn, iip->cid, LSNAME, &who1[0], &who2[0]);
    prt_row(0, iip->id, dscn, iip->cid, who1, who2, iip->vat, iip->val, str);
  }

  if (have_pdchqs)
  { iip = &tak->rev_c[-1];
    iiz = &tak->rev_c[tak->rev_last];

    do_hl_line("Post Dated Cheques", 4, 0);

    while (++iip <= iiz)
    { Set  props = iip->props;
      Char * dscn = get_ttyp(props);

      if (props & K_DEAD)
				continue;
      if ((props & (K_ANTI+K_PD)) == (K_ANTI+K_PD))
      { strpcpy(&who1[0], iip->name, LSNAME);
				sprintf(&str[0], "P%d", iip->chq_no);
				get_cdesc(mainupn, iip->cid, 30, &who1[0], &who2[0]);
				prt_row(0, iip->id, dscn, iip->cid, who1, who2, iip->vat, iip->val, str);
      }
    }
  }
  if (have_matchqs)
  { iip = &tak->rev_c[-1];
    iiz = &tak->rev_c[tak->rev_last];

    do_hl_line("Matured Cheques", 4, 0);

    while (++iip <= iiz)
    { Set  props = iip->props;
      Char * dscn = get_ttyp(props);

      if (props & K_DEAD)
				continue;
      if ((props & (K_ANTI+K_PD)) == K_PD)
      { strpcpy(&who1[0], iip->name, LSNAME);
				sprintf(&str[0], "P%d", iip->chq_no);
				get_cdesc(mainupn, iip->cid, LSNAME, &who1[0], &who2[0]);
				prt_row(0, iip->id, dscn, iip->cid, who1, who2, iip->vat, iip->val, str);
      }
    }
  }
  prt_off_cols();  
  prt_money(&this_take.u5000);
  prt_need(9);

{ Cash btot = tak->tot_chqs+mtotal-tak->tot_pdchqs+tak->tot_matchqs;
  prt_fmt(tot_fmt,
					tak->nchq, tak->tot_chqs,tak->tot_pdchqs,  btot - tak->tot_matchqs,
					mtotal,               tak->tot_matchqs,  tak->tot_chqs+mtotal, btot,
					tak->tot_expense,
					tak->tot_swap,
					owe,
					tak->tot_revenue);
}
  prt_newpage();
  sprintf(&msg[0], "%s\n%s%s\n%s%s\n",hline(79), t_hdr0,t_hdr1, sln, hline(68));
          prt_set_cols(msg, row_fmt);
  prt_head();
  iip = &tak->sal_c[-1];
  iiz = &tak->sal_c[tak->sal_last];

  while (++iip <= iiz)
    if (not (iip->props & K_DEAD) and (iip->props & K_CASH))
    { Set  props = iip->props;
		  Char * dscn = props & K_SALE ? 
								    props & K_ANTI ? "CRFND" : "CSALE"
							   									 : ill;
      sprintf(&str[0], (props & K_CCHQ) == 0 ? "CASH" :
			props & K_PD	     ? "P%d"  : "%d", iip->chq_no);
      strpcpy(&who1[0], iip->name, LSNAME);
      get_cdesc(mainupn, iip->cid, LSNAME, &who1[0], &who2[0]);
      prt_row(0, iip->id, dscn, iip->cid, who1, who2, iip->vat, iip->val, str);
    }

  iip = &tak->sal_c[-1];
  iiz = &tak->sal_c[tak->sal_last];

  while (++iip <= iiz)
    if (not (iip->props & (K_CASH + K_DEAD)))
    { Set  props = iip->props;
      if ((props & K_ANTI) == 0)
      { strpcpy(&who1[0], iip->name, LSNAME);
				get_cdesc(mainupn, iip->cid, LSNAME, &who1[0], &who2[0]);
				prt_row(0, iip->id, props & K_SALE ? "INV" : "ERROR", iip->cid,
								who1,who2, iip->vat, iip->val, "BOOK");
      }
    }

  iip = &tak->sal_c[-1];
  iiz = &tak->sal_c[tak->sal_last];

  while (++iip <= iiz)
    if ((iip->props & (K_CASH + K_DEAD + K_SALE + K_ANTI)) == (K_SALE + K_ANTI))
    { strpcpy(&who1[0], iip->name, LSNAME);
      get_cdesc(mainupn, iip->cid, LSNAME, &who1[0], &who2[0]); 
      prt_row(0, iip->id,"CREDIT",iip->cid, who1,who2, iip->vat, iip->val, "BOOK");
    }

  prt_saletots(tak);
  prt_text(sln);
  prt_text(hline(68));
  prt_text("\n\n");
  prt_fini_idle(NULL);
  return null;
}}}}

private Char * do_prt_ssheet(tak)
	 Takdescr tak;
{  Char * row_fmt = 
 "   %6d     %7l   %5d                           %30l\n"
 "                                               %30l  __________________\n";
   Prt chan = select_prt(0);
   if (chan == null)
     return msgpa;

{ Char * lines =     "-----------------------------------         -----------";
  Char who1[31], who2[31];
  sprintf(&msg[0], "%s\n%s\n%s\n\n",
          lines, "  %5d             %16l         %11l               %11r", lines);
  fmt_data(&hhead[0], msg, this_take.id, that_agent.name,
				   this_take.version >= Q_SUBMITV ? "SUBMITTED" : tstus[4], 
           date_to_asc(&who1[0], this_take.tdate, 0));

{ Char * err = prt_one_report(chan, hhead, " SIGNING SHEET\n\n\n\n\n\n\n\n\n\n\n");
  if (err != null)
    return err;

  sprintf(&msg[0], "%s\n%s%s\n%s%s\n", hline(76), t_hdr0, "  Signature",
			 "---- Round ",hline(65));
  prt_set_cols(msg, row_fmt);
  prt_head();
{ fast Takitem iip = &tak->sal_c[-1];
       Takitem iiz = &tak->sal_c[tak->sal_last];

  while (++iip <= iiz)
  { strpcpy(&who1[0], iip->name, LSNAME);
    get_cdesc(mainupn, iip->cid, LSNAME, &who1[0], &who2[0]);
    prt_row(0, iip->id, get_ttyp(iip->props), iip->cid, who1, who2);
  }
  prt_fini_idle(NULL);
  return null;
}}}}

private Char * do_edit_chq(upn, bak, ix, wh)	/* postdate a cheque */
	 Id	   upn;
	 Bankdescr bak;
	 Int	   ix;
	 Quot	   wh;
{ Takitem ib = &bak->rev_c[ix];
  Int spos = save_pos();
  Date date;
  Key_t kvi; kvi.integer = ib->id;

  if (not (ib->props & K_CCHQ))
    return "Must be a cheque";

  salert("Date on cheque? or CR to quit");
  while (true)
  { goto_fld(W_ADD);
    get_data(&ds[0]);
    if (ds[0] == 0)
      return ndon;
    date = asc_to_date(ds);
    if (date > 0)
       break;
    salert(illd);
  }

  restore_pos(spos);

  wr_lock(upn);
{ Char * res = null;
  Lra_t lra;
  Kind dix = ib->props & K_SALE ? IX_SALE : IX_PMT;
  if (dix == IX_SALE)
    return "No Longer Possible";

  Paymt_t pmt_t;
  Paymt pt__ = (Paymt)ix_fetchrec(upn, dix, &kvi, &lra, &pmt_t);
  if      (pt__ == null)
    i_log(dbcc, lted, kvi.integer);
  else if (pt__->kind & (K_HASPD + K_PD))
    res = "Already postdated";
  else 
  { Date stoday = today;
    today = date;
    item_inv = 0;
    ti_buff = *ib;
    ti_buff.props  = K_PAYMENT + K_ANTI + K_CCHQ + K_PD + K_TBP;
    ti_buff.cid    = pt__->customer;

    res = do__tak_item(upn, bak);
    today = stoday;
    if (res == null)
    { Taking_t take;
      Tak teny;
      Lra_t lra = find_tak_t(&take, &teny, this_take.id, pt__->kind, pt__->id);
      if (lra > 0)
      { teny->kind |= K_HASPD;
				(void)write_rec(upn, CL_TAKE, lra, &take);
      }
    { Paymt_t p_t;
      Paymt pt__ = (Paymt)read_rec(upn, ddict[dix].cl_ix, lra, &p_t);
      if (pt__ != null)
      { pt__->kind |= K_HASPD;
        (void)write_rec(upn, ddict[dix].cl_ix, lra, pt__);
      }
      
      ib->props |= K_HASPD;
      bak->nchq -= 1;
      bak->tot_pdchqs += ib->val;
    }}
  }
  wr_unlock(upn);
  if (wh == K_TAKE)
    put_totals(bak);
  else 
    put_btotals(bak);
  put_rev(bak, ix);        
  return res;
}}

private Cc do_rev_or_sal(upn, tak, rw, ds_)
	 Id	  upn;
	 Takdescr tak;
	 Bool	  rw;
	 Char *   ds_;
{ Int ix = tak->sal_mode ? tak->sal_base-1 : tak->rev_base-1;
  Cc contcc = SUCCESS;
  Char * error = null;
  Agent_t xferag; xferag.id = 0;

  if (not tak->sal_mode)
  { put_denoms(&this_take);
    put_details();
  }
  init_cust();
  put_flag();
  goto_fld(W_CMD);

  while (contcc == SUCCESS)
  { if (tak->sal_mode) 
      while (ix - tak->sal_base < tak_sl-1 and ix < tak->sal_last)
				put_sale(tak, ++ix);
    else 
      while (ix - tak->rev_base < tak_sl-1 and ix < tak->rev_last)
				put_rev(tak, ++ix);

    if (error != null)
      alert(error);
    error = null;

  { Field fld;
    Cc cc = get_any_data(&ds_[0], &fld);
    Id fld_typ = fld->id & ~ID_MSK;
    Char * ds = skipspaces(ds_);
    Int * base_p = tak->sal_mode ? &tak->sal_base : &tak->rev_base;
    Int * last_p = tak->sal_mode ? &tak->sal_last : &tak->rev_last;
    vfy_lock(upn);

    if (cc != OK)
    { if (cc == PAGE_UP or cc == PAGE_DOWN)
      { Short base = *base_p + (cc == PAGE_DOWN ? 1 : -1) * tak_sl;
				if (base + tak_sl - 1 > *last_p)
				  base = *last_p - tak_sl +  2;
				if (base < 0)
				  base = 0;
				if (*base_p != base)
				{ *base_p = base;
				  ix = base - 1;
				  wclrgrp();
				}
      }
      else if (cc == HALTED)
      { if (fld->props & FLD_FST)
				{ if (*base_p > 0)
				  { wclrgrp();
				    *base_p -= 1;
				    ix = *base_p - 1;
				  } 
				}
				else 
				  if (*base_p + tak_sl - 1 < *last_p)
				  { wscroll(1);
				    *base_p += 1;
				  }
      }
    }

    else if (fld_typ == W_CMD)
    { switch (cc = sel_cmd(ds))
      { case CMD_CLOSE:
          error = save_tak(upn, tak, Q_CLOSE);
				  if (error == null) 
				  { put_details();
				    salerth("Note Taking No, Any Key");
				  }
        when CMD_SUBMIT:
				  ipfld();
				  error = save_tak(upn, tak, Q_SUBMIT);
				  if (error == null or error == opiypci)
				  { error = do_prt_take(tak, false);
					{	Cc cc = save_tt_tak(tak);
				    init_tak(tak);  
				    wclrgrp();
				  }}
        when  CMD_DOT:
				  if (tak->modified and this_take.id != 0)
				  {
        case  CMD_SAVE:
				    error = save_tak(upn, tak, Q_SAVE);
				  }
				  if (error == null and cc == CMD_DOT)
				    contcc = HALTED;

        when CMD_ADJ:
				  adjino = atoi(&ds[3]);
        when CMD_FLIP:
				  wclrgrp();
				  tak->sal_mode = not tak->sal_mode;
				  base_p = tak->sal_mode ? &tak->sal_base : &tak->rev_base;
				  last_p = tak->sal_mode ? &tak->sal_last : &tak->rev_last;
				  put_totals(tak);
				  contcc = NOT_FOUND;  
            
        when  CMD_PRINT:
				  error = ds[5]==' ' and toupper(ds[6])=='S' ? do_prt_ssheet(tak) 
																				             : do_prt_take(tak, true);
        when  CMD_FIND:
				  error = find_item(tak, &ds[4]);
				  ix = tak->sal_mode ? tak->sal_base-1 : tak->rev_base-1;
				  if (error == null or ix == -1)
				  { wclrgrp();
				    goto_fld(W_XID);
				  }

        when  CMD_ASSIGN:
				  error = get_tgt(upn, tak, &ds[6], &xferag);
				  if (error == null)
				    salert("Select documents to transfer with a T");
        otherwise 
				  error = *ds==0  ? null				              : 
									tak->rw ? "Adj,Close,Submit,Print,Assign,Save,Find,Flip, or ."
												  : "Print,Find, Flip, or .";
      }
    }

    else if (fld_typ == W_XCMD)
    { Char ch = toupper(*ds);
      error = ch=='Q' or ch=='M' or ch=='G' or ch == 'E' ? null:"Q, M, E, or G";
      if (error == null)
				xactn_typ = ch;
      put_flag();
      goto_fld(W_ADD);
    }
    else if (fld_typ == W_ADD)
    { if (*last_p - *base_p > tak_sl - 2)
      { Short scroll_dist = *last_p - *base_p - tak_sl + 3;
				wscroll(scroll_dist);
				*base_p += scroll_dist;
      }
      error = prep_tak_item(tak, *last_p + 1 - *base_p, ds);
      if (error == null)
      { Char rdch = '/';
				if (tak->sal_mode ? not (ti_buff.props & K_SALE) 
												  : xactn_typ == 'G' and not (ti_buff.props & K_CASH))
				{ put_item(&ti_buff, *last_p + 1 - *base_p); 
				  salert(pstc);
				  while ((rdch = hold()) != '/' and rdch != '.')
				    ttbeep();
				  clr_item(W_XID + *last_p + 1 - *base_p);
				  clr_alert();
				}
				if (rdch == '/') 
				  error = do_tak_item(upn, tak);
				if (error == null and rdch == '/')
				{ recalc(tak);
				  ix = ix == *last_p - 1 ? ix : *base_p - 1;
				}
      }
    }
    else if (in_range(fld_typ, W_ID, W_TNO))
    { error = do_edit_details(upn, fld, tak, rw, ds);
      if (error == null)
				ix = tak->sal_mode ? tak->sal_base-1 : tak->rev_base-1;
    }
    else
    { error = load_newest(upn, tak, &this_take);
      if (error != null)
				continue;

      if      (in_range(fld_typ, W_5000, W_1_2))
      { error = do_cash_item(upn, fld_typ, tak, &this_take, ds);
				if (error == null)
				  right_fld(FLD_WR, &fld);
      }
      else if (fld_typ == W_XID or fld_typ == W_CHQN)
      { Int irow = (fld->id & ID_MSK) + *base_p;
				if      (not tak->rw)
				  error = rdoy;
				else if (not in_range(irow, 0, *last_p) or
				         (tak->sal_mode ? tak->sal_c[irow].props & K_DEAD
						        : tak->rev_c[irow].props & K_DEAD))
				  error = ngth;
				else
				{ Char ch = toupper(ds[0]);
				  error = fld_typ==W_CHQN       ? do_edit_chqn(upn, tak, irow, ds)    :
								  ch == 'T'	     			  ? do_s_xfer(upn,tak, irow, xferag.id) :
								  ch == 'M' or 
								  ch == 'Q'	    		    ? do_payi(upn, tak, irow, ch)         :
								  ch == 'D'	      		  ? del_sorp(upn, tak, irow)	       :
								  ch == 'P' and 
								  toupper(ds[1]) == 'D' ? tak->sal_mode ? ymbir	       :
																             do_edit_chq(upn,tak,irow,K_TAKE) :
								  tak->sal_mode          ? "D, M, Q, or T"		       :
															  	  		   "D, M, Q, or PD";
#if 0
				  if (error == null and not (fld->props & FLD_LST))
				    down_fld(FLD_WR, &fld);
#endif
				}
      }
      else 
				error = cndo;
    }
  }}
  end_interpret();
  return contcc;
}

private Cc do_tak(upn, tak, rw, ds)
	 Id	  upn;
	 Takdescr tak;
	 Bool	  rw;
	 Char *   ds;
{ Cc cc = SUCCESS;
  xactn_typ = 'G';
  adjino = 0;
  while (cc != HALTED)
    cc = do_rev_or_sal(upn, tak, rw, ds);

  return SUCCESS;
}



private Char * do_vtaks(bags, bak, tak, ds)
	Bagsdescr  bags;
	Bankdescr  bak;
	Takdescr   tak;
	Char *	   ds;
{ Bool cont = true;
  Tabled tbl5;

  tbl5 = mk_form(SI5form,tak_sl);
  bags->curr_base = 0;

  while (cont)
  { wopen(tbl5, SI5over);
    put_bdetails();
    goto_fld(W_TOT);  put_cash(bak->tot_revenue);
    goto_fld(W_XID);

  { Int ix = bags->curr_base-1;
    Char * error = null;

    while (true)
    { while (ix - bags->curr_base < tak_sl-1 and ix < bags->curr_last)
				put_taksmy(bags, ++ix);

      if (error != null)
				alert(error);
      error = null;

    { Field fld;
      Cc cc = get_any_data(&ds[0], &fld);
      Cc cm = sel_cmd(ds);
      vfy_lock(mainupn);

      if (cm == CMD_DOT)
      { cont = false;
        break;
      }
      if (fld->id == W_CMD)
				if (cm == CMD_FIND)
				{ bags->curr_base = 0;
				  ix = bags->curr_base-1;
				  if (error == null or ix == -1)    /* I think this is obsolete */
				    goto_fld(W_XTYP);
				}
				else 
				  error = *ds == 0 ? null : "Find";
			      else if (cc == HALTED)
				if (fld->props & FLD_FST)
				  error = nrsc;
				else 
				{ wscroll(1);
				  bags->curr_base += 1;
				}	  
	      else if (cc != SUCCESS)
					;
      else if ((fld->id & ~ID_MSK) == W_XID)
      { Int ix_ = bags->curr_base + s_frow(fld);
				if (ix_ <= bags->curr_last) 
				{ error = do_v_tak(mainupn, tak, bags->c[ix_].id, ds);
				  break;		/* to refresh screen */
				}
      }
      else 
				error = cndo;
    }}
    end_interpret();
  }}
  freem(tbl5);
  return null;
}

private Char * load_new_bank(upn, bags, bak, tak)
	 Id	   upn;
	 Bagsdescr bags;
	 Bankdescr bak;
	 Takdescr  tak;
{ if (same_version(upn, IX_BANK, &this_bank) != 0)
  { (void)load_bags(upn, bak, bags, tak, this_bank.id);
    return "Others posted new items (Not closed)";
  }
  return null;
}



private Char * do_cbank(ltyp, bags, bak, tak)
	 Int	   ltyp;
	 Bagsdescr bags;
	 Bankdescr bak;
	 Takdescr  tak;
{ if (upn_ro)
    return rdoy;
  if (this_bank.version >= ltyp)
    return aldo;
  sprintf(&ds1[0], "Press CR to %s bank", ltyp==Q_CLOSEV ? "close" : "submit");
  alert(ds1);  
  if (hold() != A_CR)
    return ndon; 
  wr_lock(mainupn);
{ Char *  res = load_new_bank(mainupn, bags, bak, tak);
  while (res == null)				/* once only */
  { // do_db(-1,-1); 
    init_tak(tak);
    tak->rw = not upn_ro;
  { Agent_t sag; sag = this_agent;
    get_taking(&this_agent, BANK_AGT, false);
    if (ltyp == Q_SUBMITV and this_take.id != 0 and this_take.version < Q_SUBMITV)
    { Cc cc = add_tak(mainupn, bak, bags, tak, this_take.id, false);
      res = cc != OK ? "take err" : save_tak(mainupn, tak, Q_SUBMIT);
    }
    this_agent = sag;  
    if (res != null)
      break;
  { Key_t kvi;  kvi.integer = this_bank.id;
  { Lra_t lra;
    Bank_t ba_t;
    Bank bk__ = (Bank)ix_fetchrec(mainupn, IX_BANK, &kvi, &lra, &ba_t);
    if (bk__ == null)
    { res = ngth; break; }
    if (ba_t.version >= ltyp)
    { res = "Somebody else did it"; break; }
  
    this_bank.date    = today;
    this_bank.version = ltyp;
    ba_t.date    = today;
    ba_t.version = ltyp;
    (void)write_rec(mainupn, CL_BANK, lra, &ba_t);
  
    if (ltyp == Q_SUBMITV)
    { Paymt_t pmt;
      pmt.kind     = K_PAYMENT+K_ANTI;
      pmt.id       = last_id(mainupn, IX_PMT) + 1;
      pmt.customer = BANKACC;
      pmt.agent    = 0;
      pmt.date     = today;
      pmt.total    = sum_cash(&this_bank)
              			+bak->tot_chqs-bak->tot_pdchqs+bak->tot_matchqs;
      pmt.chq_no   = 0;
    { Cc cc = i_Paymt_t(mainupn, &pmt, &lra);
      if (cc == OK)
        cc = sorp_to_account(0, BANKACC, IX_PMT, pmt.id, pmt.total);
      if (cc != OK)
        res = "Error with bank";
    }}
    break;
  }}}}
  wr_unlock(mainupn);
  return res;
}}

private void save_bank(upn, bak)
	 Id	     upn;
	 Bankdescr   bak;
{ Lra_t lra;
  Key_t kvi; kvi.integer = this_bank.id;
  wr_lock(upn);
{ Bank_t b_t;
  Bank bank__ = ix_fetchrec(upn, IX_BANK, &kvi, &lra, &b_t);
  if (bank__ != null)
  { memcpy(&bank__->u5000, &this_bank.u5000,
    			      (Int)&this_bank.eny[0]-(Int)&this_bank.u5000);
    (void)write_rec(upn, CL_BANK, lra, bank__);
    bak->modified = false;
  }
  wr_unlock(upn);
}}



private Char * do_submit_bank(bags, bak, tak)
	 Bagsdescr bags;
	 Bankdescr bak;
	 Takdescr  tak;
{ if (upn_ro)
    return rdoy;
  if (this_bank.version >= Q_SUBMITV)
    return aldo;
#if 0		    
  if (salerth("Press CR to fetch matured cheques") == A_CR)
  { error = do_ltak(bags, bak, tak, ds);
    if (error != null)
      return error;
  }
#endif
{ Char * res = do_cbank(Q_SUBMITV, bags, bak, tak);
  if (res == null)
  { save_bank(mainupn, bak);
    bags->rw = bak->rw = false;
  }
  return res;
}}



private Char * do_chqs(bag, bak_, ds)
	 Bagsdescr bag;
	 Bankdescr bak_;
	 Char *    ds;
{ Tabled tbl2;
  Bankdescr bak = bak_;
  Agent_t sag; sag = this_agent;

  this_agent.id = BANK_AGT;
  this_agent.tak_id = 0;
  strcpy(&this_agent.name[0], "Bank");
  this_take = null_sct(Taking_t);	     /* a local takings has no agent */
  this_take.agid = this_agent.id;
  
  tak_sl = g_rows - 12;
  tbl2 = mk_form(SI2form,tak_sl);
  wopen(tbl2, SI2over);
  put_bdetails();
  put_denoms(&this_bank);
  put_btotals(bak);

  goto_fld(W_CMD);
{ Int ix = bak->curr_base-1;
  Char * error = null;

  while (true)
  { while (++ix - bak->curr_base < tak_sl and ix <= bak->curr_last)
      put_vouch(bak, ix);

    if (error != null)
      alert(error);
    error = null;

  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);
    vfy_lock(mainupn);

    if (cc == PAGE_UP or cc == PAGE_DOWN)
    { Short base = bak->curr_base + (cc == PAGE_DOWN ? tak_sl - 1 : 1 - tak_sl);
      if (base + tak_sl - 1 > bak->curr_last)
				base = bak->curr_last - tak_sl + 2;
      if (base < 0)
				base = 0;
      if (base != bak->curr_base)
      { bak->curr_base = base;
				ix = base - 1;
				wclrgrp();
      }
    }
    else if (cc == HALTED)
    { if (fld->props & FLD_FST)
      { if (bak->curr_base > 0)
				{ wclrgrp();
				  bak->curr_base -= 1;
				  ix = bak->curr_base - 1;
				} 
      }
      else 
				if (bak->curr_base + tak_sl - 1 < bak->curr_last)
				{ wscroll(1);
				  bak->curr_base += 1;
				}
    }	  
    else if (fld->id == W_CMD)
      if (*strmatch(".", ds) == 0)
				break;
      else if (*strmatch("FIND", ds) == 0)
      { bak->curr_base = 0;
				ix = bak->curr_base-1;
				if (error == null or ix == -1)	  /* I think this is obsolete */
				  goto_fld(W_XTYP);
      }
      else 
				error = *ds == 0 ? null : "Find";
	  else if (in_range(fld->id, W_XID, W_ADD-1))
	  { Int ix_ = bak->curr_base + s_frow(fld);
			if (ix_ <= bak->curr_last) 
      { error =  toupper(ds[0]) != 'P' or 
								 toupper(ds[1]) != 'D' ? "PD is the only command" :
							                            do_edit_chq(mainupn, bak, ix_, 0);
				put_vouch(bak, ix_);
				if (ix_ == tak_sl - 1)
				{ wscroll(1);
				  bak->curr_base += 1;
				}
      }
    }
    else if (in_range(fld->id, W_5000, W_1_2))
      error = do_cash_item(mainupn, fld->id, (Takdescr)bak, &this_bank, ds);
    else 
      error = cndo;
  }}
  end_interpret();
  this_agent = sag;
  freem(tbl2);
  return null;
}}

#if 0

private Char * do_get_mats(upn, date, tak)
	 Id	  upn;
	 Date	  date;
	 Takdescr tak;
{ if (salerth("Press CR to get matured cheques") != A_CR)
    return ndon;
  wr_lock(upn);
{ Char * res = null;
  Cc cc = load_acc(upn, PDCHQACC, &acc, &ths.acct, 0);
  if (cc != OK)
    i_log(cc, "C Create PDCA");
{ Accitem iip = &acc.c[-1];
  Accitem iiz = &acc.c[acc.curr_last];

  while (++iip <= iiz)
  { // do_db(-1,-1); 
    if (not (iip->props & K_PAID) and iip->idate <= date)
    { Key_t kvi; kvi.integer = iip->id;
    { Lra_t lra;
      Paymt pt__ = (Paymt)ix_fetchrec(upn, IX_PMT, &kvi, &lra, null);
      if (pt__ == null)
      	i_log(dbcc, "Lost PDCA entry %d", kvi.integer);
      else
      { Customer cu = get_cust(upn, IX_CUST, pt__->customer);
				Takitem it = &ti_buff;
				it->props  = K_PAYMENT + K_CCHQ + K_PD;
				it->cid    = pt__->customer;
				strpcpy(&it->name[0], cu == null ? cdel : cu->surname, LSNAME);
				it->vat    = 0;
				it->val    = pt__->total;
				it->chq_no = iip->id;	  /* store payment id here for do_commit_rev */
      { Char *	res_ = do__tak_item(upn, tak);
				if (res_ != null)
				  res = res_;
				else
				  add_chq(&bak_t, &tak->rev_c[tak->rev_last]);/* sorry about bak_t !!*/
      }}
    }}
  }
  wr_unlock(upn);
  recalc(tak);
  return res;
}}}

#endif

private Char * do_ltak(bags, bak, tak, ds)
	Bagsdescr  bags;
	Bankdescr  bak;
	Takdescr   tak;
	Char *	   ds;
{ Agent_t sag; sag = this_agent;
  this_agent.id = BANK_AGT;
  this_agent.tak_id = 0;
  strcpy(&this_agent.name[0], "Bank");
  this_take = null_sct(Taking_t);	     /* a local takings has no agent */
  this_take.agid = this_agent.id;

  in_local = true;
{ Char * res = do_takings();
  in_local = false;
  this_agent = sag;
  return res;
}}

static const Char * hdrchqs[] = 
{ "Expense Vouchers", "Matured Cheques", "Future Cheques", "Cheques", 
  "Returned Cheques", "Journal"};

private Char * do_prt_bank(bags, bak_)
	 Bagsdescr bags;
	 Bankdescr bak_;
{ fast Bankdescr bak = bak_;
       Char * row_fmt = 
 "   %6d      %8l   %4d                           %30l       %11v        %10d\n";
      Char * row_fmt1 = 
 "   %6d                              %30l         %11v\n";
      Prt  chan = select_prt(0);
      if (chan == null)
	return msgpa;

{ Char datestr[30];
  Char * lines =  "---------------------------     -----------------";

  sprintf(&msg[0], "%s\n%s\n%s\n\n",
	   lines, "  %5d                %11r          %5r      %10r", lines);
  fmt_data(&hhead[0], msg, this_bank.id, this_bank.id > 0 ? "" : tstus[4],
 			  	 date_to_weekday(this_bank.date), 
           date_to_asc(&datestr[0], this_bank.date, 0));
{ Char * err = prt_one_report(chan, hhead, " BANK\n\n\n\n\n\n\n\n\n\n\n");
  if (err != null)
    return err;

  sprintf(&msg[0], "%s\n%s%s\n", hline(74), t_hdr0, t_hdr1b);
  prt_set_cols(msg, row_fmt);
  prt_head();
  strcat(&hhead[0], " BANK CONTINUED\n");
  prt_set_title(hhead);  

{ Cash mtotal = sum_cash(&this_bank);
  Cash owe = sum_owe(bak);
  Int iter;
  for (iter = 6; --iter >= 0; )
  { Bool hdr = false;
    Takitem iip = &bak->rev_c[-1];
    Takitem iiz = &bak->rev_c[bak->curr_last];
    
    while (++iip <= iiz)
    { fast Set  props = iip->props;
      if (iter==5 ? (props &(K_PAYMENT+K_UNB))		 == K_PAYMENT + K_UNB :
				  iter==4 ? (props &(K_PD+K_EXP+K_HASPD+K_ANTI+K_UNB)) == K_ANTI      :
				  iter==3 ? (props &(K_PD+K_EXP+K_HASPD+K_ANTI+K_UNB)) == 0           :
				  iter==2 ? (props &(K_PD+K_EXP+        K_ANTI+K_UNB)) == K_PD+K_ANTI :
			    iter==1 ? (props &(K_PD+K_EXP+        K_ANTI+K_UNB)) == K_PD	      :
     	  			      (props & K_EXP))
      { if (not hdr)
				{ hdr = true;
				  do_hl_line(hdrchqs[iter], 8, 4);
				}
				prt_row(0, iip->id, iter == 3 ? "CHQ" : get_ttyp(props), iip->cid,
  					    iip->name, iip->val, iip->chq_no);
      }
    }
  }
  prt_off_cols();
  prt_money(&this_bank.u5000);
  prt_need(9);

  prt_fmt(tot_fmt,
					bak->nchq, bak->tot_chqs,bak->tot_pdchqs, bak->tot_chqs - bak->tot_pdchqs,
			    mtotal,               bak->tot_matchqs,
			    bak->tot_chqs+mtotal, bak->tot_chqs+mtotal-bak->tot_pdchqs+bak->tot_matchqs,
			    bak->tot_expense,
			    bak->tot_swap,
			    owe,
			    bak->tot_revenue);
  prt_saletots(bak);

  prt_newpage();

  sprintf(&msg[0], "%s\n%s%s%s\n%s%s\n", hline(55),
 "    Id     Name", spaces(31), amnt,
 "---- Takings ", hline(42));
  prt_set_cols(msg, row_fmt1);
{ Bagsitem iip = &bags->c[-1];
  Bagsitem iiz = &bags->c[bags->curr_last];

  while (++iip <= iiz)
    prt_row(0, iip->id, iip->name, iip->val);

  prt_need(8);
  prt_text(strcat(strcpy(&msg[0], spaces(40)), hline(15)));
  prt_fmt("\n                                                 %10v\n",
			bak->tot_revenue);
  do_hl_line(" Bank ", 8, 4);
  prt_fini_idle(NULL);
  return null;
}}}}}

private Char * do_send_cash()

{ Tabled tbl = mk_form(SI3form,0);
  wopen(tbl, SI3over);
  
{ Char * error = null;

  init_tak(&tak_t);
  tak_t.rw = true;
  this_take = null_sct(Taking_t);

  while (true)
  { put_totals(&tak_t);
    goto_fld(W_TOT); put_cash(tak_t.tot_money+tak_t.tot_chqs);
    if (error != null)
      alert(error);
    error = null;

  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);
    if (cc == OK)
    { if (fld->id == W_CMD)
      { cc = sel_cmd(ds);
        if      (cc == CMD_DOT)
        { error = aban;
          break;
        }
        else if (cc == CMD_SUBMIT)
        { 
          break;
        }
        else
          salert("Submit");
      }
      if      (in_range(fld->id, W_5000, W_1_2))
        error = do_cash_item(mainupn, fld->id, &tak_t, &this_take, ds);
      else if (fld->id == W_CHQS)
      { Cash amt;
				Char * dd = get_cash(ds, &amt);
				if (dd <= ds)
				  error = amip;
				else
				  tak_t.tot_chqs = amt;
      }
    }
  }}
  freem(tbl);
  return error;
}}

private const Char * do_till()

{ Char * error = null;
  Bool first = true;
  Tabled tbl4;

  if (disallow(R_BANK))
    return preq('B');

  init_take();
  tbl4 = mk_form(SI4form,0);
  init_bank(&bak_t, &bags_t);
{ Key_t kvi; kvi.integer = last_id(mainupn, IX_BANK);
  (void)ix_fetchrec(mainupn, IX_BANK, &kvi, null, &this_bank);

  while (true)
  { wopen(tbl4, SI4over);
    if (first)
      salert("Press CR to load till");
    put_bdetails();
    goto_fld(first ? W_TNO : W_CMD);
    first = false;

    if (error != null)
      alert(error);
    error = null;

  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);
    vfy_lock(mainupn);
    if (fld->id == W_TNO)
      if (not vfy_ointeger(ds)) 
				error = "Must be an integer";
      else 
      { Id bid = ds[0] == 0 ? this_bank.id : atoi(ds);
				if (load_bags(mainupn, &bak_t, &bags_t, &tak_t, bid) != SUCCESS)
				  error = ntfd;
      }
    else 
    { Char ch = toupper(*ds);
      if      (ch == '.')
				break;
      else if (bags_t.id == 0 and ch != 'B') 
				error = "Select one first";
      else 
				switch (ch)
				{ when 'T': error = do_vtaks(&bags_t,&bak_t, &tak_t, ds);
				  when 'L': error = do_ltak(&bags_t,&bak_t, &tak_t, ds);
				  when 'C': error = do_cbank(Q_CLOSEV, &bags_t,&bak_t, &tak_t);
				  when 'V': error = do_chqs(&bags_t, &bak_t, ds);
				  when 'S': error = do_submit_bank(&bags_t, &bak_t, &tak_t); 
					    if (error != null)
					      break;
				  case 'P': error = do_prt_bank(&bags_t, &bak_t);
				  when 'B': error = do_send_cash();
				}
    }
  }}
  if (bak_t.modified)
    save_bank(mainupn, &bak_t);

  this_bank.id = 0;
  freem(tbl4);
  return null;
}} 
/*********************************************************/

private Char * check_annnce(upn, cmd, stt, stp, ds)
	 Id	   upn;
	 Char	   cmd;
	 Date	   stt, stp;
	 Char *    ds;
{ Char title[80];

  if (stt > stp)
    return "End before start";

  date_to_asc(&ds[0], stt, 0);
  if (stt == 0)
    strcpy(&ds[0], "BEGINNING");

  if (disallow(R_RPT))
    return preq('G');
{ Prt chan = select_prt(0);
  if (chan == null)
    return msgpa;
  sprintf(&title[0], "%s sales from %s to %s Inclusive\n\n", 
	 		     cmd == 0 ? wdtot     :
  		     cmd == 1 ? "Cash"    : 
  		     cmd == 2 ? "Account" : 
  		     cmd == 3 ? "In Progress" : 
  		     cmd == 4 ? "Daily"   : 
  		     cmd == 9 ? "Denominations" : "Cancelled",
           ds,
           date_to_asc(&ds[30], stp, 0));
 return prt_one_report(chan, title, "SALES STATISTICS\n\n\n\n\n\n\n\n\n\n\n");
}}

private void prt_salestots(cmd, id, dat, bak)
	 Int       cmd, id;
	 Char *    dat;
	 Bankdescr bak;
{ if      (cmd == 0 or cmd == 3)
    prt_row(0, id,dat, bak->noo_sales, bak->tot_sales, bak->tot_vat,
    		    bak->noo_rets,  bak->tot_rets,  bak->tot_vatret,
    		    bak->tot_expense, bak->tot_expvat
           );
  else if (cmd == 1)
    prt_row(0, id,dat, bak->noo_csales, bak->tot_csales, bak->tot_cvat,
    		    bak->noo_crets,  bak->tot_crets,  bak->tot_cvatret);
  else
    prt_row(0, id,dat, bak->noo_sales - bak->noo_csales,
		  	    bak->tot_sales - bak->tot_csales,
	  		    bak->tot_vat - bak->tot_cvat,
				    bak->noo_rets - bak->noo_crets,
				    bak->tot_rets - bak->tot_crets, 
				    bak->tot_vatret - bak->tot_cvatret,
				    bak->tot_expense,
				    bak->tot_expvat
      	   );
}


public Char * do_do_over(upn, cmd, bags, bak, tak, stt, stp, ds)
	 Id	   upn;
	 Char	   cmd;
	 Bagsdescr bags;
	 Bankdescr bak;
	 Takdescr  tak;
	 Date	   stt, stp;
	 Char *    ds;
{ Char * err = check_annnce(upn, cmd, stt, stp, ds);
  if (err != null )
    return err;
  ipfld();
{ Lra_t lra;
  Key_t kvi;  kvi.integer = 1;
{ Char * strm = ix_new_stream(upn, cmd == 3 ? IX_TAKE : IX_BANK, &kvi);  
  fast Bank bk;
  Int bagct = 0;
  Bankdescr_t tots; 
  Cash agg[10];
  tots = null_sct(Bankdescr_t);
  memcpy(&agg[0], &tots, sizeof(agg));

  prt_set_cols(
	  cmd == 9 ?
"    Date  Up-to-50p  Pounds      5's      10's       20's       50's   Cheques\n",
"      %9r     %8v      %9v      %10v      %10v       %11v       %11v       %11v\n"
		   :
"   Id   Date     Sales  ( Value      Vat) Returns (Value   Vat) Exp (Value Vat)\n",
"  %5d       %9r  %4d       %10v       %9v  %4d      %9v     %7v       %8v     %7v\n");
  prt_head();

  while ((bk = (Bank)ix_next_rec(strm, &lra, null)) != null)
    if (in_range(bk->date, stt, stp))
    { Char dat[30]; date_to_asc(&dat[0], bk->date, 0);
      bagct += 1;
    { Cash *pt, *ps;
      if (cmd == 3)
      { if (bk->version >= Q_SUBMITV)
				  continue;
				load_tak(upn, tak, ((Taking)bk)->id);
				ps = &tak->noo_sales;
      }
      else
      { load_bags(upn, bak, bags, tak, bk->id);
				ps = &bak->noo_sales;
      }
      for (pt = &tots.noo_sales; pt <= &tots.tot_expvat; ++pt)
				*pt += *ps++;
      if (cmd == 9)
      { Cash p50 = bk->u1 + bk->u5*5L+ bk->u20*20L+ bk->u50*50L;
        prt_row(0, dat, p50, bk->u100*100L,   bk->u500*500L, bk->u1000*1000L, 
						    bk->u2000*2000L, bk->u5000*5000L,
						    bak->tot_chqs);
				agg[0] += bk->u5000;
				agg[1] += bk->u2000;
				agg[2] += bk->u1000;
				agg[3] += bk->u500;
				agg[4] += bk->u100;
				agg[5] += p50;
				agg[6] += bak->tot_chqs;
      }
      else
				prt_salestots(cmd, bk->id, dat, cmd == 3 ? (Bankdescr)tak : bak);
    }}

  ix_fini_stream(strm);
{ Char totbar[81];
  memset(&totbar[0], '=', 79);
  totbar[79] = '\n';
  totbar[80] = 0;
  prt_text(totbar);
  if (cmd == 9)
    prt_row(0, "", agg[5], agg[4]*100L, agg[3]*500L, agg[2]*1000L, 
						agg[1]*2000L, agg[0]*5000L, agg[6]);
  else
    prt_salestots(cmd, 0, "", &tots);
  prt_fini_idle(NULL);
  return null;
}}}}

typedef struct
{ Cash	  noo_sales;
  Cash	  tot_sales;
  Cash	  tot_vat;
  Cash	  noo_rets;
  Cash	  tot_rets;
  Cash	  tot_vatret;
  Cash	  tot_expense;
  Cash	  tot_expvat;
} Statsdescr_t, *Statsdescr;


private Char * do_do_sales(upn, cmd, stt, stp, ds)
	 Id	   upn;
	 Quot	   cmd;
	 Date	   stt,	stp;
	 Char *    ds;
{ Char * err = check_annnce(upn, cmd, stt, stp, ds);
  if (err != null )
    return err;
  ipfld();
  if (cmd == 5)
    prt_set_cols("   Date    Inv.No.  Value inc. Vat        Reason\n",
    		 "      %9r     %7d           %10v              %16r\n");
  else  
    prt_set_cols(
"   Date    Sales  (Inc.Vat    Vat)  Returns (Inc.Vat  Vat) Exp (Inc.Vat  Vat)\n",
"      %9r  %5d       %11v      %10v %4d      %10v      %9v      %9v       %10v\n");
  prt_head();

{ Int i;
  Statsdescr tots = (Statsdescr)malloc((stp-stt+1)*sizeof(Statsdescr_t));
  Statsdescr_t gtot; gtot = null_sct(Statsdescr_t);
  memset((Char*)&tots[0], 0, (stp-stt+1)*sizeof(Statsdescr_t));

{ Set sprops = this_agent.props;
  Sale_t sale;
  Quot pat = cmd != 5 ? 0 : K_DEAD;
  Char * strm = ix_new_stream(upn, IX_SALE, null);
  Date olddate = 0;
  this_agent.props |= R_SCRT;

  rd_lock(upn);
  while (ix_next_rec(strm, &trash, &sale) != null)
  { // do_db(-1,-1);
    if (in_range(sale.tax_date, stt, stp) and pat == (sale.kind & K_DEAD)
					  and not (sale.kind & K_GIN))
    { fast Statsdescr tot = &tots[sale.tax_date - stt];
	   Cash vat = (sale.vattotal * sale.vat0_1pc + 500) / 1000;

      if (sale.kind & K_ANTI)
      { tot->noo_rets   += 1;
				tot->tot_vatret += vat;
				tot->tot_rets   += sale.total;
      }
      else 
      { tot->noo_sales += 1;
				tot->tot_vat   += vat;
				tot->tot_sales += sale.total;
      }
      if (cmd == 5)
      { ds[0] = 0;
        if (sale.tax_date != olddate)
				  date_to_asc(&ds[0], sale.tax_date, 0);
				olddate = sale.tax_date;
      { Char * r = reason_invo(sale.id, &ds1[0], sizeof(ds1));
        prt_row(0, ds, sale.id, sale.total, r);
      }}
    }
    rd_unlock(upn); rd_lock(upn);
  }

  rd_unlock(upn);
  ix_fini_stream(strm);

  if (pat == 0)
  { Expense_t exp;
    strm = init_rec_strm(upn, CL_EXP);
    rd_lock(upn);
    while (next_rec(strm, &trash, &exp) != null)
    { // do_db(-1,-1);
      if (in_range(exp.date, stt, stp))
      { /*i_log(exp.id, "Exp Val %d", exp.total);*/
        tots[exp.date - stt].tot_expense += exp.total;
				tots[exp.date - stt].tot_expvat += exp.vat;
      }

      rd_unlock(upn); rd_lock(upn);
    }

    rd_unlock(upn);
    fini_stream(strm);
  }
{ Bool lastvoid = true;
  Date day;
  for (day = stt; day <= stp; ++day)
  { fast Statsdescr tot = &tots[day-stt];
    if (tot->noo_sales == 0 and tot->noo_rets == 0 and tot->tot_expense == 0)
    { if (not lastvoid)
				prt_text("\n");
      lastvoid = true;
    }
    else 
    { Cash * t = (Cash*)&gtot;
      Cash * s = (Cash*)tot;
      while (s < (Cash*)&tot[1])
				*t++ += *s++;
      if (cmd != 5)
      { lastvoid = false;
				prt_row(0, date_to_asc(&ds[0], day, 0),
				        tot->noo_sales, tot->tot_sales, tot->tot_vat,
								tot->noo_rets, tot->tot_rets, tot->tot_vatret,
								tot->tot_expense, tot->tot_expvat);
      }
    }
  }
{ Char totbar[82];
  memset(&totbar[0], '=', 80);
  totbar[80] = '\n';
  totbar[81] = 0;
  prt_text(totbar);

  if (cmd == 5)
    prt_row(0, "", gtot.noo_sales, gtot.tot_sales, "");
  else
    prt_row(0, "", gtot.noo_sales, gtot.tot_sales, gtot.tot_vat,
	          gtot.noo_rets, gtot.tot_rets, gtot.tot_vatret,
	          gtot.tot_expense, gtot.tot_expvat);
  prt_fini_idle(NULL);
  free((char*)tots);
  this_agent.props = sprops;
  return null;
}}}}}

public const Char * do_overall()

{ Bool quit = false;
  init_take();
  while (not quit)
  { Date stt = 0;
    Date stp = today;
    Bool refresh = true;
    Char * error = null;
    Tabled tbl6 = mk_form(SI6form,0);
    wopen(tbl6, SI6over);
    goto_fld(W_STT);  

    while (true)
    { if (refresh)
      { Int spos = save_pos();
        put_choices(op6text);
        fld_put_data(W_STP, date_to_asc(&ds[0], stp, 0));
        restore_pos(spos);
      }
      refresh = false;
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
      { quit = true;
        break;
      }
      else if  (cc == SUCCESS and in_range(fld->id, W_STT, W_STP))
      { Date date = asc_to_date(ds);
        error = date > 0 ? null : illd;
        if (error == null)
				  if (fld->id == W_STT)
				    stt = date;
				  else 
				    stp = date;
      }
      else 
      { Int x = get_fopt(ds, fld);
        switch(x)
        { case 0:
				  case 1:
				  case 2: 
				  case 3:
				  case 9: error = do_do_over(mainupn, x,
								&bags_t, &bak_t, &tak_t, stt, stp, ds);
				  when 4: 
				  case 5: error = do_do_sales(mainupn, x, stt, stp, ds);
				
				  when 6: error = do_accounts(K_HGRAM);
        }
        refresh = true;
        if (x == 6)
          break;
      }
    }}
    freem(tbl6);
  }
  return null;
}

#if 0

public void cvt_cash(whix, id)
	Quot  whix;
	Id    id;
{ Lra_t lra;
  Key_t kv_t; kv_t.integer = id;
{ Char tank[BLOCK_SIZE];
  Taking ta = (Taking)ix_fetchrec(mainupn, whix, &kv_t, &lra, tank);
  if (ta != null)
  {      Cash * den = whix == IX_TAKE ? &ta->u5000 : &((Bank)ta)->u5000;
    fast Int ct = 10;
    while (--ct >= 0)
      den[ct] /= denom_to_val[ct];
    (void)write_rec(mainupn, ddict[whix].cl_ix, lra, ta);
  }
}}

#endif
