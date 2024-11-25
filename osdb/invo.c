#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include "version.h"
#include "../h/defs.h"
#include "../h/errs.h"
#include "../form/h/form.h"
#include "../form/h/verify.h"
#include "../db/b_tree.h"
#include "../db/recs.h"
#include "../db/cache.h"
#include "domains.h"
#include "schema.h"
#include "rights.h"
#include "generic.h"
#include "../env/tcap.h"

#define private 

extern Int     an_n;
extern Double  an_a, an_b;
extern Double  an_rate;		/* per day at uniques.lst_anal */


#define MAX_IBANTXT 200
#define LETOFFCT 4

#define W_INO   (3 * ID_FAC)
#define W_AG    (4 * ID_FAC)
#define W_DATE  (5 * ID_FAC)
#define W_TERMS (6 * ID_FAC)
#define W_CAT	(7 * ID_FAC)
#define W_ID    (8 * ID_FAC)
#define W_SNAME (9 * ID_FAC)
#define W_FNAME (10 * ID_FAC)
#define W_CST   (11 * ID_FAC)
#define W_ADDR  (12* ID_FAC)
#define W_STT   (10* ID_FAC)
#define W_STP   (11* ID_FAC)

#define W_QTY  (13 * ID_FAC)
#define W_SUP  W_QTY
#define W_KEY  (14 * ID_FAC)
#define W_PROP (15 * ID_FAC)
#define W_PACK (16 * ID_FAC)
#define W_TKT  (17 * ID_FAC)
#define W_DSCN (18 * ID_FAC)
#define W_PRC  (19 * ID_FAC)
#define W_STK  (20 * ID_FAC)
#define W_ORD  (21 * ID_FAC)
#define W_OTW  (22 * ID_FAC)
#define W_MAR  (23 * ID_FAC)

#define W_ADD  (25 * ID_FAC)
#define W_TOT  (26 * ID_FAC)
#define W_DRTE (27 * ID_FAC)
#define W_VDISC (28 * ID_FAC)
#define W_DISC (29 * ID_FAC)
#define W_VRTE (30 * ID_FAC)
#define W_VBLE (31 * ID_FAC)
#define W_VAT  (32 * ID_FAC)
#define W_CT   (33 * ID_FAC)
#define W_STATUS (34 * ID_FAC)
#define W_PBLE (35 * ID_FAC)

#define W_DMSG (36 * ID_FAC)

#define LTERMS 15

#define LUI 50

#define SL (SCRN_LINES-14)
#define SL24 (24-14)

static int inv_sl;

static const Over_t SIover [] =
  {{0,1,1,0,">"},
   {0,1,1,59,"Inv.No."},
   {0,2,1,59,"Terms"},
   {0,3,1,59,"Category"},
   {0,4,1,59,"Pref"},
   {1,3,1,0,
 " Acc-No    Account-Name                  Title           "},
/* {0,3,1,0,
 "ÉÍÍÍÍÍ»\n"
 "º     º\n"
 "ÈÍÍÍÍÍ¼\n"},
*/
   {1,7,1,0,
 " Qty     Code       Pack Ticket Description                   Price  "},
   {-1,0,0,0,"Invoicing"}
  };

static const Fieldrep_t SIform [] =
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,      60,  1, ""},
   {W_CMD,   FLD_WR,        1, 1,      20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,30,      29,  1, ""},
   {W_INO,   FLD_WR,	    1,72,       8,  1, "Invoice no"},

   {W_AG,    FLD_RD,	    2, 0,       5,  1, ""},
   {W_DATE,  FLD_WR,	    2,47,   LDATE,  1, "Invoice date"},
   {W_TERMS, FLD_WR,	    2,65,   LTERMS, 1, "Terms"},
   {W_CAT,   FLD_WR,	    3,72,       8,  1, "Category"},
   {W_ID,    FLD_WR,	    4,0,        8,  1, "Customer id"},
   {W_SNAME, FLD_WR,	    4,9,   LSNAME,  1, "Name"},
   {W_FNAME, FLD_WR,	    4,10+LSNAME,LFNAMES, 1, "Title"},
   {W_CST,   FLD_WR,	    4,72,       8,  1, "Constraint"},
   {W_ADDR,  FLD_WR,	    5,0,   LPADDR,  1, psta},

   {W_QTY,   FLD_WR+FLD_BOL,8, 0,         5,      -SL, "Quantity"},
   {W_KEY,   FLD_WR,	    8, 6,        LSTKKEY, -SL, pcde},
   {W_PACK,  FLD_WR,	    8, 7+LSTKKEY, 3,      -SL, pck},
   {W_TKT,   FLD_WR,	    8,11+LSTKKEY, 4,      -SL, retl},
   {W_DSCN,  FLD_WR,	    8,16+LSTKKEY, LDSCN,  -SL, dcrn},
   {W_PRC,   FLD_WR+FLD_EOL,8,17+LSTKKEY+LDSCN, 7,-SL, pric},

   {W_BAN,   FLD_RD,	 -8,0,                72, 1, ""},

   {W_ADD, FLD_WR+FLD_LST+FLD_PEN,-9,0,      50, 1, "Input"},
   {W_TOT,   FLD_RD,	 -9, 16+LSTKKEY+LDSCN, 8, 1, ""},
   {W_DMSG,  FLD_RD,	 -10,13,               9, 1, ""},
   {W_DRTE,  FLD_WR,	 -10,27,               7, 1, "Drate"},
   {W_VDISC, FLD_RD,	 -10,7+LSTKKEY+LDSCN,  8, 1, ""},
   {W_DISC,  FLD_RD,	 -10,16+LSTKKEY+LDSCN, 8, 1, ""},
   {W_VRTE,  FLD_RD,	 -11,17,              30, 1, ""},
   {W_VBLE,  FLD_RD,	 -11,7+LSTKKEY+LDSCN,  8, 1, ""},
   {W_VAT,   FLD_RD,	 -11,16+LSTKKEY+LDSCN, 8, 1, ""},
   {W_CT,    FLD_RD,	 -12,2,                4, 1, ""},
   {W_STATUS,FLD_RD,	 -12,20,              30, 1, ""},
   {W_PBLE,  FLD_RD,	 -12,16+LSTKKEY+LDSCN, 8, 1, ""},
   {0,0,0,0,0,1,null}
  };

static const Char nadt [] = "Next Arrival Date";
#define ardt (&nadt[5])
static const Char samg [] = "Safety margin";

static const Over_t SPover [] =
  {{0,1,1,0,">"},
   {0,3,1,2,"Order No"},
   {0,3,1,30,"Supplier Id"},
   {0,3,1,54,"Percent Overbuy"},
   {0,4,1,2,ardt},
   {0,5,1,2,nadt},
   {1,6,1,0,
" Source  Code   Props  Pack Description             Price   Stk OTW Order Margin"},
   {-1,0,0,0,null}
  };

static const Fieldrep_t SPform [] =
  {{W_MSG+1,FLD_RD+FLD_MSG,0,20,58,  1, ""},
   {W_CMD,  FLD_WR,	   1, 1, 28, 1, comd},
   {W_MSG,  FLD_RD+FLD_FD, 1,30, 40, 1, ""},
   {W_INO,  FLD_WR,	   3,20,  8, 1, "Our Order Ref"},
   {W_ID,   FLD_WR,	   3,44,  8, 1, "Supplier Number"},
   {W_TERMS,FLD_WR,	   3,70,  8, 1, samg},
   {W_STT,  FLD_WR,	   4,20, 30, 1, ardt},
   {W_STP,  FLD_WR,	   5,20, 30, 1, nadt},

   {W_SUP,  FLD_WR+FLD_BOL,7,0,8,SL, splr},
   {W_KEY,  FLD_WR,	   7, 9,  5, SL, pcde},
   {W_PROP, FLD_WR,	   7,15,  6, SL, prty},
   {W_PACK, FLD_WR,	   7,22,  3, SL, "case"},
   {W_DSCN, FLD_WR,	   7,26,LDSCN-3,SL,dcrn},
   {W_PRC,  FLD_WR,	   7,53,  6, SL, pric},
   {W_STK,  FLD_WR,	   7,60,  4, SL, "Stock before arrival"},
   {W_OTW,  FLD_RD,	   7,65,  4, SL, "on the way"},
   {W_ORD,  FLD_WR,	   7,70,  4, SL, "order"},
   {W_MAR,  FLD_WR,	   7,75,  4, SL, samg},
   {W_BAN, FLD_RD,      -7,0,  72,  1, ""},
   {W_ADD, FLD_WR,      -8,0,  50,  1, "Input"},
   {W_STATUS, FLD_RD,  -10,20, 40,  1, ""},
   {0,0,0,0,0,1,null}
  };

static const Over_t DDover [] =
  {{0,1,1,0,">"},
   {0,3,1,0,"Delivery note No."},
   {0,4,1,0,"Barcode"},
   {0,6,0,0,"Delivery Note"},
   {-1,0,0,0,null}
  };

static const Fieldrep_t DDform [] =
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,      60,  1, ""},
   {W_CMD,   FLD_WR,        1, 1,      20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,30, 40, 1, ""},
   {W_INO,   FLD_WR,	   		3,20,  8, 1, "D Note #"},
   {W_ID,    FLD_WR,	    	4,20, 30, 1, "Barcode"},
   {0,0,0,0,0,1,null}
  };

static const Char enoi [] = "Enter Number of items";
static const Char crsq [] = "CR to select, . to quit";

typedef struct
{ Set    props;
  Descn  descn;
  Id     id;
  Stkkey key;		
  Int    sup;
  Int    line;
  Int    qty;
  short  cas;
  short  pack;
  short  ticket;
  Cash   price[2];			/* second is higher */
  Int    altref;
  Int    tt_ix;
} Invoitem_t, *Invoitem;

typedef struct
{ float   an_a;
  float   an_b;
  Int     stock;	/* stock just before arrival */
  Int     stocknow;
  Int     otway;
  Int     sale;		/* sale arrival to next arrival */
  Int     qty;
} Stockanal_t, * Stockanal;


typedef struct
{ Int       arr_sz;
  Int       curr_last;
  Int       curr_base;
  Bool      modified;
  Invoitem  c;
  Stockanal o;
} Invdescr_t, *Invdescr;


static Invdescr_t inv;

		        	/* these amounts are stored permanently */
Sale_t   this_sale;
		        	/* these amounts are inferred from this_sale */
static Id     this_sale_acc;

typedef struct
{ Cash    gross;
  Cash    disct;
  Cash    gross_vatable;
  Cash    vattotal;
  Cash    total;
} Twinsale_t, *Twinsale;

static Twinsale_t this_s[2];			/* 0: charged, 1: shown */

static Short  this_alt;				/* alternative price (higher) */

#define  this_s0  this_s[0]
#define  this_s1  this_s[1]
#define  this_sa  this_s[this_alt]

static float  this_drate;  /* percent charged */
static Int    this_ct;
static Int    this_ln;

static Id       this_sale_cat;
static Shortset this_constraint;

static Set  dflt_kind;
static Bool g_ordering = false;
static Bool redisplay;           /* redisplay after a stock choice */

static Double def_margin = 1.2;

static Tabled tbl1;

static Char vatban[26];

static Char g_cust_TT_id[LTTID+1];

typedef struct Tt_ui_s
{ char c[LUI+1];
  char wh;
  Cash price;
} Tt_ui_t, *Tt_ui;

static Tt_ui g_tt_uis;
static int g_tt_num_ui;
static int g_tt_max_ui;


private Stockitem get_si() forward;
private const Char * lsale(Invdescr inv, Int id) forward;

private void mk_banner(Char * title)

{ 
}

					/* share the buffer for orders */
#define INV_O_BUFF_IX ((sizeof(biglistbuff)/sizeof(Invoitem_t) - 1) / 2)


private void fini_sale(inv)
      Invdescr inv;
{ if ((Byte*)inv->c != &biglistbuff[0] and
             inv->c != null)		/* this condition redundant! */
  { free(inv->c);
    inv->c = null;
  }
  if ((Byte*)inv->o != &biglistbuff[0] and
             inv->o != null)
  { free(inv->o);
    inv->o = null;
  }

  inv->arr_sz = 0;
}



private void init_sale(inv)
      Invdescr inv;
{ today = asc_to_date("");      // eliminate this global variable!
  this_sale	     = null_sct(Sale_t);
  this_sale.kind     = (K_SALE + K_IP) | dflt_kind;
  this_sale.tax_date = today;
  this_sale.vat0_1pc = uniques.vatrate;
  this_sale.terms    = to_rgt(8+'G');
  this_sale.cat	     = this_sale_cat;
  this_constraint    = 0;

  strcpy(&this_sale_terms[0], "CASH");
  sprintf(&vatban[0], "VAT at    %1.2f%%    on", (float)this_sale.vat0_1pc/10);

  if (g_ordering)
  { this_sale.tax_date = today + 7;
    this_sale.terms    = today + 14;
  }

  g_tt_num_ui = 0;
  this_custmr = null_sct(Customer_t);
  this_sale_acc = 0;
  memset(&this_s, 0, sizeof(this_s));
  this_drate = 1.0;
  this_ct = 0;
  this_ln = 0;

  inv->curr_base = 0;
  inv->curr_last = -1;
  inv->modified  = false;

  inv_sl = g_rows-14;
  if (tbl1 == null)
    tbl1 = mk_form(SIform,inv_sl);

  fini_sale(inv);

  if (inv->o != null)
    free(inv->o);
    
  if (inv->c != null && inv->c != (Invoitem)&biglistbuff[0])
    free(inv->c);

  inv->o = null;
  inv->c = (Invoitem)&biglistbuff[0];
  inv->arr_sz = sizeof(biglistbuff)/sizeof(Invoitem_t) - 1;

  if ((dflt_kind & K_GIN) != 0)
    inv->o = (Stockanal)malloc((inv->arr_sz + 1) * sizeof(Stockanal_t));
}



private Invoitem extend_inv(inv)
	Invdescr   inv;
{ fast Int size = inv->arr_sz;
  if ((inv->curr_last += 1) >= size)
  { size += 100;

  { Invoitem tbl = (Invoitem)malloc((size + 1) * sizeof(Invoitem_t));
    if (tbl == null)
      return null;
    memcpy(&tbl[0], inv->c, inv->arr_sz * sizeof(Invoitem_t));
    if ((Byte*)(inv->c) != &biglistbuff[0])
      free((char*)inv->c);
    inv->c  = tbl;
    inv->arr_sz = size;

    if ((dflt_kind & K_GIN) != 0)
    { Stockanal t = (Stockanal)malloc((size + 1) * sizeof(Stockanal_t));
      if (t == null)  
        return null;
      memcpy(&t[0], inv->o, inv->arr_sz * sizeof(Stockanal_t));
      if ((Byte*)inv->o != null)
        free((char*)inv->o);
      inv->o  = (Stockanal)t;
    }
  }}

  return &inv->c[inv->curr_last];
}



private void scroll_istk(keytbl, n)
	Id    keytbl[SL24+1];
	Int   n;
{ Char * s = (Char *)&keytbl[n];
  fast Int i;

  memcpy(&keytbl[0], s, (Char *)&keytbl[SL24+1] - s);
  for (i = n; --i >= 0;)
    keytbl[SL24 - i - 1] = 0;
}

private void put_iitem(inv, ix)
	Invdescr inv;
	Int	 ix;
{      Int spos = save_pos();
  fast Invoitem it = &inv->c[ix];

  if (goto_fld(W_QTY + ix - inv->curr_base) == null)
    return;

  wclreol();

  if (it->props & P_DESCN)
    fld_put_data(W_DSCN+ID_MSK, it->descn);
  else
  { if	    (it->qty == 0)
      put_data("****");
    else if (it->qty == 1 and (it->props & P_DISC))
      put_data("");
    else
      put_integer(it->qty);
    fld_put_data(W_KEY+ID_MSK, &it->key[0]);
    goto_fld(W_PACK+ID_MSK);
    if (it->props & P_DISC)
			put_free_data(" BONUS  ");
    else
    {	put_integer(it->pack * it->cas);
      fld_put_int(W_TKT+ID_MSK, it->ticket);
    }
    form_put(T_DAT+W_DSCN+ID_MSK, it->descn,
             T_CSH+W_PRC+ID_MSK,  it->price[1/*this_alt*/], 0);
  }
  restore_pos(spos);
}



private void put_isi(si)
	Stockitem si;
{      Int spos = save_pos();
       Char linestr[18];  bc_to_asc(si->supplier, si->line, &linestr[0]);

  form_put(T_INT+W_QTY+ID_MSK,  si->stock,
           T_DAT+W_KEY+ID_MSK,  linestr,
           T_INT+W_PACK+ID_MSK, si->pack * (s_case(si) < 2 ? 1 : s_case(si)),
           T_INT+W_TKT+ID_MSK,  si->ticket,
           T_DAT+W_DSCN+ID_MSK, si->descn,
           T_CSH+W_PRC+ID_MSK,  si->price, 0);
  restore_pos(spos);
}

private void put_oitem(row, it, sa)
	 Int row;
	 Invoitem  it;
	 Stockanal sa;
{      Int spos = save_pos();
       Char linestr[30];    bc_to_asc(0, it->line, &linestr[0]);
#define line2 (&linestr[15])

  goto_fld(W_SUP+row);

  if	  (it->props & P_OBS)
  { wclreol();		         put_data("DELETED");
  }
  else if (it->props & P_DESCN)
    fld_put_data(W_DSCN+ID_MSK, it->descn);
  else
  {			         put_ointeger(it->sup);
    line2[0] = 0;
    if (it->props & 0xffffff00)
      rgt_to_asc(it->props >> 8,  &line2[0]);
    form_put(T_DAT+W_KEY+ID_MSK,   linestr,
             T_DAT+W_PROP+ID_MSK,  line2,
             T_INT+W_PACK+ID_MSK,  it->pack * it->cas,
             T_DAT+W_DSCN+ID_MSK,  it->descn,
             T_CSH+W_PRC+ID_MSK,   it->price[1/*this_alt*/],
             T_INT+W_STK+ID_MSK,   sa->stock,
             T_INT+W_OTW+row,      sa->otway,
             T_INT+W_ORD+row,      it->qty,
             T_INT+W_MAR+row,      sa->stock + sa->qty - sa->sale, 0);
  }
  restore_pos(spos);
}


private void put_ordstk(row, si)
	 Int       row;
	 Stockitem si;
{ if (not g_ordering)
    put_isi(si);
  else
  { Invoitem_t it_t;
    it_t.props = 0;
    it_t.line  = si->line;
    it_t.sup   = si->supplier;
    it_t.cas = s_case(si) < 2 ? 1 : s_case(si);
    it_t.pack  = si->pack * it_t.cas;
    strpcpy(&it_t.descn[0], si->descn, sizeof(it_t.descn));
    it_t.price
     [this_alt]= si->price;
    it_t.qty   = 0;

    put_oitem(row, &it_t, &null_sct(Stockanal_t));
  }
}



private void put_details_order()

{      Int spos = save_pos();
       Char str[60];
       Char * m =  this_sale.kind & K_ORD ? "ON ORDER"      :
        	         this_sale.kind & K_DED ? "DELIVERY NOTE" :
                        				            "NOT IN EFFECT";
  sprintf(&str[0], "%1.3f", (def_margin - 1.0) * 100.0);
  date_to_asc(&str[20], this_sale.tax_date, 0);
  date_to_asc(&str[40], this_sale.terms, 0);
  form_put(T_DAT+W_TERMS,    str,
           T_OINT+W_INO,     -this_sale.id,
           T_OINT+W_ID,      this_custmr.creditlim, /* barcode */
           T_DAT+W_STT,      &str[20],
           T_DAT+W_STP,      &str[40],
           T_DAT+W_STATUS,   m, 0);
  restore_pos(spos);
}

private void put_details_invo()

{      Int spos = save_pos();
       Char str[100];
  sprintf(&str[0],"%*s", LTERMS, this_sale_terms);

  form_put(T_OINT+W_INO, abs(this_sale.id),
           T_OINT+W_AG,  this_sale.agent,
           T_OINT+W_ID,  dflt_kind & K_GIN
                            				   ? this_custmr.creditlim : this_custmr.id,
           T_DAT+W_TERMS,str, 0);

  goto_fld(W_CAT);	
  if (this_sale.kind & K_DEAD)
			put_data("CANCEL'D");
  else
			put_ointeger((Int)this_sale.cat );
			
  date_to_asc(&str[0], this_sale.tax_date, 0);
  sprintf(&str[20], "%s%s%s", (this_sale.kind & K_GIN   ? "GOODS-IN "   : ""),
                              (this_sale.kind & K_ANTI  ? "CREDIT-NOTE" : ""),
                              (this_sale.kind & K_IP    ? " In Progress": ""));

  form_put(T_DAT+W_DATE,   str,
           T_DAT+W_SNAME,  this_custmr.surname,
           T_DAT+W_FNAME,  this_custmr.forenames,
           T_DAT+W_ADDR,   this_custmr.postaddr,
           T_DAT+W_STATUS, &str[20], 
           T_DAT+W_CST,    rgt_to_asc(this_constraint, &str[80]), 0);

  restore_pos(spos);
}



private void put_totals_invo()

{ Int spos = save_pos();
      Char discban[40];

  discban[0] = 0;

  if (this_drate < 0.9999)
  { sprintf(&discban[0],"Discount Rate %1.2f%%  Vatable Discount",
						(1.0 - this_drate) * 100);
    form_put(T_FAT+W_DMSG,   discban,
             T_OCSH+W_VDISC, this_s1.gross_vatable - this_s1.vattotal,
             T_OCSH+W_DISC,  -this_sale.disct, 0);
  }

  form_put(T_OCSH+W_TOT,  this_s1.gross,
           T_DAT+W_VRTE,  vatban,
           T_OCSH+W_VBLE, this_s1.vattotal,
           T_OCSH+W_VAT,  (this_s1.vattotal * this_sale.vat0_1pc + 500)/1000,0);

  if (this_sale.id != 0 and not (this_sale.kind & K_IP))
  { goto_fld(W_CT);   put_ointeger(this_ct); 
  }
  goto_fld(W_PBLE);   put_ocash(this_s1.total);
  restore_pos(spos);
}

private Char * get_pling_qty(str, qty_ref)	/* advanced => good */
	 Char *   str;
	 Int	 *qty_ref;
{ fast Char * s = str;

  while (*s == ' ')
    ++s;

  if (isdigit(*s))
  { Int qty = *s++ - '0';
    while (isdigit(*s))
      qty = qty * 10 + *s++ - '0';

    if (*s == '!' or *s == '.')
    { *qty_ref = qty;
      return &s[1];
    }
    if (*s == '@')
    { *qty_ref = qty;
      return s;
    }
  }
  return str;
}

private void update_sale()

{ Int ix;
  for (ix = 2; --ix >= 0; )
  { fast Twinsale p = &this_s[ix];
    p->vattotal = p->gross_vatable * this_drate + 0.5;
    p->disct    = p->gross * (1.0000001 - this_drate);
    p->total    = p->gross - p->disct +
            			  (p->vattotal * this_sale.vat0_1pc + 500)/ 1000;
  }
  
  this_sale.vattotal = this_s1.vattotal;
  this_sale.disct    = this_s1.disct;
  this_sale.total    = this_s1.total;
}


private void add_product(iip)
	Invoitem  iip;
{ Cash val = iip->price[0] * iip->qty;

  this_s0.gross += val;
  if (iip->props & P_VAT)
    this_s0.gross_vatable += val;
      
  val = iip->price[1] * iip->qty;
  this_s1.gross += val;
  if (iip->props & P_VAT)
    this_s1.gross_vatable += val;
}


				/* writes this_s[*].(gross,gross_vatable) */

private void recalc_invo(inv)
      Invdescr	inv;
{ fast Invoitem iip = &inv->c[-1];
       Invoitem iiz = &inv->c[inv->curr_last];
  this_ct = 0;
  this_ln = 0;
  memset(&this_s, 0, sizeof(this_s));

  while (++iip <= iiz)
    if (iip->props & P_ITEM)
    { add_product(iip);
      if (not (iip->props & P_DISC))
      { this_ct += iip->qty;
    	  if (iip->qty > 1)
	        this_ln += 1;
      }
    }
}



private void add_to_invo(inv, item)
      Invdescr	inv;
      Invoitem	item;
{ Int prv = inv->curr_last;
  if (prv >= 0 and inv->c[prv].id==item->id and 
      inv->c[prv].price[0] == item->price[0])
  { if (inv->c[prv].qty > 0  and not (item->props & P_DISC))
      this_ln += 1;
    inv->c[prv].qty += item->qty;
  }
  else
  { if (item->qty > 1  and not (item->props & P_DISC))
      this_ln += 1;
    inv->curr_last += 1;
  }
  if (not (item->props & P_DISC))
    this_ct += item->qty;

  add_product(item);

  update_sale();
  if (not g_ordering)
    put_totals_invo();
}

private Bool sale_lock(yn, id)
      Bool   yn;
      Id     id;
{ Key_t kv_t;  kv_t.integer = id;
  wr_lock(mainupn);
{ Lra_t lra;
  Sale_t sa_t;
  Sale sa = (Sale)ix_fetchrec(mainupn, IX_SALE, &kv_t, &lra, &sa_t);
  if (sa == null)
  { wr_unlock(mainupn); return false; }

{ Bool lkd = (sa_t.kind & K_LOCK) != 0;
  if (yn)
    sa_t.kind |= K_LOCK;
  else
    sa_t.kind &= ~K_LOCK;
  (void)write_rec(mainupn, CL_SALE, lra, &sa_t);
  wr_unlock(mainupn);
  return yn != lkd;
}}}


				/* care, after calling this, the old inv->c,
				   inv->o may be invalid */
private const Char * mark_inv(inv, try)
				/* must not be read locked */
      Invdescr	inv;
      Bool      try;
{ if (this_sale_acc == 0)
    return "Select Customer first";
  if (not inv->modified)
  { if (not (this_sale.kind & K_IP) or upn_ro)
/*    and ((this_sale.kind & K_PAID) or not (this_sale.kind & K_GIN))) */
      return rdoy;
    if (this_sale.id != 0 and not sale_lock(true, this_sale.id) and
      	this_agent.id != 1)
    { fini_sale(inv);
      lsale(inv, this_sale.id);
      wreopen(tbl1);
      return seyl;
    }
    if (try)
      inv->modified = true;
  }
  return null;
}

private void do_sa(si, sa, qty)
      	 Stockitem si;
         Stockanal sa;
         Int	   qty;
{ Stockitem_t si_t;

  if (si->props & P_SUBS)
  { Lra_t lra;
    Key_t kvi;  kvi.integer = si->stk_alias;
    si = (Stockitem)ix_fetchrec(mainupn,IX_STK, &kvi,&lra,&si_t);
    if (si == null)
      i_log(93, "SALIAS");
  }

  today = asc_to_date("");
  analyse(si);
  sa->qty	= qty;
  sa->stocknow  = si->stock;
  sa->an_a	= an_a;
  sa->an_b	= an_b;
{ Int gotnow = si->stock < 0 ? sa->otway : sa->otway+si->stock;  
  Double rate1 = an_rate + an_b * 
          		 (today + this_sale.tax_date - 2 * uniques.lst_anal) / 2.0;
  Double rate2 = an_rate + an_b * 
        	    (this_sale.tax_date + this_sale.terms - 2 * uniques.lst_anal) / 2.0;
  Int openv = gotnow - rnd(rate1 * dbl(this_sale.tax_date-today));

  Double sale = rate2 * dbl(this_sale.terms-this_sale.tax_date+1);
  Int case_ = s_case(si);
  sa->stock = openv < 0 ? 0 : openv;
  sa->sale  = rnd(sale + 0.4);
  if (sa->sale < 0)
    sa->sale = 0;
  sa->otway = si->order;
}}


private const Char * lsale(inv, id)
	 Invdescr inv;
	 Int	  id;
{ Key_t kv_t;  kv_t.integer = id;
  init_sale(inv);
  g_tt_num_ui = 0;
{ Lra_t lra;
  Sale sa = (Sale)ix_fetchrec(mainupn, IX_SALE, &kv_t, null, &this_sale);
  if (sa == null)
    return ntfd;

 /*this_sale.alt_terms = this_sale.terms; 	for now */

  terms_to_asc(this_sale.terms, &this_sale_terms[0]);

  this_s0.gross = this_sale.total + this_sale.disct
		    - (this_sale.vattotal * this_sale.vat0_1pc + 500)/ 1000;
  this_s0.disct = this_sale.disct;
  this_drate = this_s0.gross == 0 ? 1.0
			          : 1.0000001 - this_s0.disct / this_s0.gross;
  this_s0.gross_vatable = (this_sale.vattotal - 0.5) / this_drate;
  this_s0.vattotal = this_sale.vattotal;
  this_s0.total = this_sale.total;

  this_s1 = this_s0;			/* initialise it to same */

  if ((this_sale.kind & K_LOCK) and this_agent.id != 1)
    return ssee;
  if (this_sale.kind & K_IP)
    this_sale.vat0_1pc = uniques.vatrate;
  sprintf(&vatban[0], "VAT at %1.2f%% on", (double)this_sale.vat0_1pc / 10);

  if (this_sale.customer != 0)
  { Id ix = this_sale.kind & K_GIN ? IX_SUP : IX_CUST;
    kv_t.integer = this_sale.customer;
  { Customer cu = (Customer)ix_fetchrec(mainupn, ix, &kv_t, null, &this_custmr);
    if (cu == null)
    { this_custmr.id = kv_t.integer;
      strcpy(&this_custmr.surname[0], cdel);
    }
  }}

  rd_lock(invupn);
{ fast Invoitem iip = &inv->c[-1];
  iip[1].descn[0] = 0;

  if (not (this_sale.kind & K_ALIEN))
  { kv_t.integer = id;
    if (ix_search(invupn, IX_INV, &kv_t, &lra) != SUCCESS)
    { rd_unlock(invupn);
      return "$Invoice body deleted";
    }

    memset(&this_s, 0, sizeof(this_s));
  }
  else
  { ++iip;
    inv->curr_last += 1;
    iip->props = P_DESCN;
    sprintf(&iip->descn[0], "Paper Invoice, Ref No %d", this_sale.chq_no);
    lra = 0;
  }

  while (s_page(lra) != 0)
  { do_db(-1,-1);
    Invoice_t bkt_t;
    Invoice bkt = (Invoice)read_rec(invupn, CL_INV, lra, &bkt_t);
    if (bkt == null)
      break;
  { Item_u ip = &bkt->items[-1];

    while (++ip <= &bkt->items[INVBUCKET-1] and ip->d.props != 0)
      if (ip->d.props & P_EXTN)
      { Char * s = &ip->d.descn[0];
      	Char * slim_ = (Char *)&ip[1];
      	Char * t = strchr(iip->descn,0);
      	Char * tlim = &((Char *)&iip[1])[-1];
      	while (t < tlim and s < slim_ and *s != 0)
      	  *t++ = *s++;
      	*t = 0;
      }
      else	
      { iip	   = extend_inv(inv);
      	iip->props = ip->d.props & ((1 << 11) - 1);
      	if (ip->i.props & P_DESCN)
      	  strpcpy(&iip->descn[0], &ip->d.descn[0], sizeof(Dbucket) + 1);
      	else
      	{ if (iip->props & P_TT)
      	    g_tt_num_ui = -1;
      	  iip->qty      = (((short)ip->i.props) >> 11) + (ip->i.qty & 0xffff);
      	  iip->price[1] = ip->i.price;
      	  iip->price[0] = ip->i.price;/* + ip->i.alt_p_diff */
      	  iip->id       = ip->i.id;
      	  kv_t.integer = iip->id;
      	{ Stockitem si = (Stockitem)ix_fetchrec(mainupn,IX_STK,&kv_t,&lra,null);

          if (si != null and (si->props & P_OBS) and (this_sale.kind & K_IP))
          { Stockitem sii = get_si(mainupn, si->supplier, si->line, &lra);
            if (sii != null)
            { si = sii;
              iip->id = si->id;
            }
          }
            
      	  if (si == null)
      	  { sprintf(&iip->key[0], "I%d", iip->id);
      	    iip->descn[0] = 0;
	          iip->pack	  = 0;
      	    iip->ticket   = 0;
	        }
      	  else
      	  { strpcpy(&iip->descn[0], si->descn, sizeof(Descn));
	          bc_to_asc(si->supplier, si->line, &iip->key[0]);
      	    iip->sup	= si->supplier;
      	    iip->line	= si->line;
      	    iip->cas = s_case(si) < 2 ? 1 : s_case(si);
	          iip->pack	= si->pack * iip->cas;
      	    iip->ticket = si->ticket;
	          iip->altref = si->profit * (256*256L) + si->altref;
	        }
      	  if ((dflt_kind & K_GIN) and inv->o != null)
	          do_sa(si, &inv->o[inv->curr_last], iip->qty);
      	}}
      }
      lra = bkt->next & (Q_EFLAG-1);
    }}
  rd_unlock(invupn);

  if (not (this_sale.kind & K_ALIEN))
  { recalc_invo(inv);
    this_drate = this_s0.gross == 0 ? 1.0 :
        1.0 - (double)this_s0.disct / (double)this_s0.gross - 0.00000001;
    update_sale();
  }
  if ((this_sale.kind & K_GIN) and this_s0.gross != 0 and disallow(R_SCRT))
  { init_sale(inv);
    return preq('L');
  }
  if ((this_sale.kind & (K_GIN + K_DED)) == K_GIN and not g_ordering)
  { init_sale(inv);
    return "Accept it first";
  }
  return null;
}}}



private const Char * load_sale(inv, id)
	 Invdescr inv;
	 Id	  id;
{ rd_lock(mainupn);
{ const Char * res = lsale(inv, id);
  
  wreopen(tbl1);
    
  rd_unlock(mainupn);
  return res;
}}




private const Char * ld_sale(id)
	 Id	  id;
{ rd_lock(mainupn);
{ const Char * res = lsale(&inv, id);
  rd_unlock(mainupn);
  return res;
}}

private void deduct_stock(inv, ikind, ord)
					/* write locked */
      Invdescr inv;
      Set      ikind;
      Int      ord;
{ Int sign_ = ikind == 0 ? 0 : ikind & K_GIN ? -1 : 1;
  Int sign  = ikind & K_ANTI ? -sign_ : sign_;

  Invoitem iip = &inv->c[-1];
  Invoitem iiz = &inv->c[inv->curr_last];

  Stockanal isa = inv->o;

  while (++iip <= iiz)
  { if (iip->props & P_ITEM)
    { Lra_t lra;
      Key_t ki; ki.integer = iip->id;
    {	Stockitem_t si_t;
      fast Stockitem si =(Stockitem)ix_fetchrec(mainupn,IX_STK, &ki,&lra,&si_t);
      if (si == null)
      	i_log(dbcc, "Stockitem deleted");
      else
      { if (si->props & P_SUBS)
    	  { ki.integer = si->stk_alias;
    	    si = (Stockitem)ix_fetchrec(mainupn,IX_STK, &ki,&lra,&si_t);
	        if (si == null)
    	      i_log(93, "SALIAS");
	      }
	      if (ord != 0)
      	{ Int o = ord * iip->qty;
      	  si_t.order += o;
      	  if (si_t.order < 0)
      	    si_t.order = 0;
      	  if (isa != null)
      	  { isa->stock += o;	/* stock just before arrival */
	          isa->stocknow += o;
      	    isa->otway -= o;
      	  }
      	}
      	if (ikind & K_DED)
      	{ Int q =  sign * iip->qty;
      	  si_t.stock -= q;
      	  si_t.props &= ~P_OOS;
      	  if (not (ikind & K_GIN))
      	  { si_t.week_sale += q;
      	    if (not (ikind & K_ANTI))
      	      si_t.sale_date = asc_to_date("");
      	  }
	      }
	      if (iip->props & P_TT)
	        si_t.props |= P_HAS2D;
      	(void)write_rec(mainupn, CL_SI, lra, &si_t);
      }
    }}
    if (isa != null)
      ++isa;
  }
}

private Id first_pos_key(upn,ix)
	  Id   upn;		/* write locked */
	  Int  ix;
{ Lra_t lra;
  Key_t  kv_t;	kv_t.integer = 0;
{ Char * strm = ix_new_stream(upn, ix, &kv_t);
  (void)next_of_ix(strm, &kv_t, &lra);
  ix_fini_stream(strm);
  return kv_t.integer;
}}


private Cc save_body(id, save_all, inv)
	 Id       id;
	 Bool	  save_all;
	 Invdescr inv;
{ Cc cc = OK;
  Lra_t lra;
  Key_t kv_t; kv_t.integer = id;
  wr_lock(invupn);
{ Cc cc1 = ix_search(invupn, IX_INV, &kv_t, &lra);
  if (cc1 == SUCCESS)
  { while (s_page(lra) != 0)
    { Invoice_t inv_t;
      Invoice iv__ = (Invoice)read_rec(invupn, CL_INV, lra, &inv_t);
      if (iv__ == null)
      	break;
      if (del_rec(invupn, CL_INV, lra) != OK)
      	break;
      lra = iv__->next & (Q_EFLAG-1);
    }
    if (s_page(lra) != 0)
      i_log(54, "save_body: Inv bkt lost");
  }
  wr_unlock(invupn);
  wr_lock(invupn);

  if (save_all)
  { fast Invoitem iip = &inv->c[-1];
    Invoitem iiz = &inv->c[inv->curr_last];
    Char * restdesc = "";
    Int secmrk = 0;
    Lra_t fstlra, prevlra;

    while (iip <= iiz)
    { Invoice_t bkt;
      bkt.id   = this_sale.id;
      bkt.next = secmrk;
    { Item_u ip = &bkt.items[-1];

      while (ip < &bkt.items[INVBUCKET-1])
      { if (*restdesc == 0)
      	{ ++iip;		      /* more input */
      	  if (iip > iiz)
      	    break;
      	  if (iip->props & P_DESCN)
	          restdesc = &iip->descn[0];
      	}
      	++ip;
      	ip->d.props = iip->props | ((iip->qty >> 16) << 11);
      	if (*restdesc == 0)
      	{ ip->i.qty        = iip->qty;
      	  ip->i.id         = iip->id;
      	  ip->i.price      = iip->price[1];
      	/*ip->i.alt_p_diff = iip->price[0] - iip->price[1];*/
	      }
    	  else
      	{ Char * t = &ip->d.descn[0];
      	  Int n = sizeof(Dbucket);
      	  if (restdesc != &iip->descn[0])
      	    ip->d.props |= P_EXTN;
      	  while (--n >= 0 and *restdesc != 0)
	          *t++ = *restdesc++;
      	  if (n >= 0)
      	    *t = 0;
      	}
      }
      if (ip < &bkt.items[INVBUCKET-1])
      	(++ip)->i.props = 0;

    { Cc cc = new_init_rec(invupn, CL_INV, (char*)&bkt, &lra);
      if (cc != OK)
      { wr_unlock(invupn);
      	i_log(cc, "save_i_bkts: db wr fail");
      	return cc;
      }
      if (secmrk == 0)
      	fstlra = lra;
      else
      { Invoice iv = (Invoice)read_rec(invupn, CL_INV, prevlra, null);
      	iv->next  = (iv->next & Q_EFLAG) + lra;
      	(void)write_rec(invupn, CL_INV, prevlra, iv);
      }
      prevlra = lra;
      secmrk = Q_EFLAG;
    }}}
    cc = cc1 == NOT_FOUND ? ix_insert(invupn, IX_INV, &kv_t, &fstlra)
			                    : ix_replace(invupn, IX_INV, &kv_t, &fstlra);
    if (cc1 == NOT_FOUND and chan_filesize(invupn) > 0x4000)
    { Key_t kv_t_; kv_t_.integer = first_pos_key(invupn, IX_INV);
      if (kv_t.integer - kv_t_.integer > 0x4000)
      { Invoice_t inv_t; inv_t.id = kv_t_.integer;
      	cc = ix_search(invupn, IX_INV, &kv_t_, &prevlra);
      	cc = cc != OK ? cc : d_Invoice_t(invupn, &inv_t);
    } }
  }
  wr_unlock(invupn);
  return cc;
}}



public void save_extinv(sale)
	 Sale  sale;
{ const Char * err = lsale(&inv, sale->id);
  if (err != null)
    i_log(sale->id, "SAVEEXTINV: %s", err);
  else
  { char fn[80]; 
    sprintf(&fn[0], "edi/%d.dat", sale->id);
  { FILE * op = fopen(fn, "w");
    if (op == null)
      i_log(3, "Unable to save EDI %d", sale->id);
    else
    {      Char strdate[LDATE+3], strcode[18], comment[16];
      fast Invoitem iip = &inv.c[-1];
           Invoitem iiz = &inv.c[inv.curr_last];
      comment[0] = 0;

      for (; ++iip <= iiz; )
        if (iip->props & P_LINK)
        { strpcpy(&comment[0], iip->descn, sizeof(comment));
          break;
        }

      fprintf(op, "1%c%6.6s%-20d%6.6s%08.8s%15d%15.15s\r\n", 
                   (sale->kind & K_ANTI ? 'C' : 'I'),
                   OSGOOD_SUGRO_NO,
                   sale->customer, 
                   skipspaces(to_client_ref(&strcode[0], this_custmr.bank_acc)),
		               skipspaces(date_to_asc(&strdate[0], sale->date, 2)),
            		   sale->id, comment
             );
      
      for (iip = &inv.c[-1]; ++iip <= iiz; )
      { Int rate = 0;
       /* Int v = 0;*/
        char buf[22];
        if (not (iip->props & P_ITEM) or iip->qty == 0)
          continue;
        if (iip->props & P_VAT)
        { rate = this_sale.vat0_1pc;
        /*v = (iip->qty * iip->price[1] * rate + 500) / 1000;*/
        }
        sprintf(&buf[0], "%d", iip->altref);
        fprintf(op, "2%-20.20s%-20.20s%06d00+%08d%08d+00000000+%04d\r\n",
              /*      ProdCd ProdCd2 Qty  Price lineVal      VATRate */
        		    iip->key,
      	  	    buf,
      		      iip->qty,
      		      iip->price[1],
      		      iip->qty * iip->price[1], rate * 10);
      }
    { Int vat = (this_s1.vattotal * this_sale.vat0_1pc + 500) / 1000;
      fprintf(op, "3%08d+%08d+%08d+\r\n",
      		    this_s1.total-vat,
      		    vat,
      		    this_s1.total
             );
    }}
    fclose(op);
  }}
}



public Cc save_tt_inv()

{ Char fnbuff[256];
  Char * fname = prepare_dfile(&fnbuff[0], "tt_inv", this_sale.id);
  if (fname != null)
  { Tt_ui ttp = g_tt_uis - 1;
    Tt_ui ttz = &g_tt_uis[g_tt_num_ui-1];
    int del_ct = 0;
    while (++ttp <= ttz)
      if (strcmp(ttp->c, "DELETED") == 0)
        ++del_ct;
      
  { FILE * op = del_ct >= g_tt_num_ui ? NULL : fopen(fname, "w");
    if (op != null)
    { ttp = g_tt_uis - 1;
      ttz = &g_tt_uis[g_tt_num_ui-1];

      char dbuff[20];
      date_to_asc(dbuff,this_sale.tax_date,2);
      dbuff[8] = '-';
      dbuff[9] = dbuff[2]; dbuff[10] = dbuff[3];
      dbuff[11] = '-';
      dbuff[12] = dbuff[0]; dbuff[13] = dbuff[1];
      dbuff[14] = 0;
      
      char totbuf[20];
      sprintf(totbuf,"%10.2f", (float)this_sale.total / 100.0);
      
      fprintf(op,"%s %d Size %d ", this_sale.kind & K_GIN  ? "GIN" : "INV",
                                   this_sale.id, g_tt_num_ui - del_ct);
      fprintf(op,"EO_ID %s %s",*g_cust_TT_id == 0 ? "?,?" : g_cust_TT_id,
                                   this_sale.kind & K_DEAD ? "CANCEL" : ".");
      fprintf(op," Total %s Date %s", totbuf,&dbuff[4]);
      fprintf(op," Cid %d\n", this_sale.customer);
               
      while (++ttp <= ttz)
      { if (strcmp(ttp->c, "DELETED") == 0)
          continue;
        fprintf(op,"%c%s %d\n", ttp->wh, ttp->c,ttp->price);
      }

      fprintf(op, "END\n");
      fclose(op);
      
      if (g_ordering)
      { (void)global_lock();
  			op = fopen("tt_inv/tt_list.dat","a+");
	  		if (op != null)
      	{	fprintf(op,"%d\n", this_sale.id);
          fclose(op);
        }
        (void)global_unlock();
      }
      return OK;
    }
  }}
  i_log(this_sale.id, "unable to save tt");
  return -1;
}


private Cc save_tt_dnote(char * bc, int d_note_no)

{ 
{ Char fnbuff[256];
  Char * fname = prepare_dfile(&fnbuff[0], "tt_inv", d_note_no);
  if (fname != null)
  { FILE * op = fopen(fname, "a+");
  	if (op != NULL)
    { char dbuff[20];
      date_to_asc(dbuff, asc_to_date(""),2);
      dbuff[8] = '-';
      dbuff[9] = dbuff[2]; dbuff[10] = dbuff[3];
      dbuff[11] = '-';
      dbuff[12] = dbuff[0]; dbuff[13] = dbuff[1];
      dbuff[14] = 0;
    	fprintf(op,"DNOTE %d %s Date %s\n", d_note_no, bc, dbuff);
    	fclose(op);
    }
    (void)global_lock();
  	op = fopen("tt_inv/tt_list.dat","a+");
	  if (op != null)
    {	fprintf(op,"%d\n", d_note_no);
      fclose(op);
    }
    (void)global_unlock();
  }

	return OK;
}}

public Cc load_tt_inv(inv_id, inv)
	 Id       inv_id;
	 Invdescr inv;
{ Char fnbuff[256];
  Char * fname = prepare_dfile(&fnbuff[0], "tt_inv", inv_id);
  while (fname != null)             // Once only
  { FILE * input = fopen(fname, "r");
    if (input == null)
    { i_log(inv_id, "unable to load tt");
      return -1;
    }
  { Char tt_id[LTTID+1];
    Char ui_id[LUI+1];
    Id  id;
    fscanf(input,"Id %d Size %d EO_ID %s\n", &id, &g_tt_num_ui, &tt_id);

    if (strcmp(g_cust_TT_id,tt_id) != 0)
      i_log(0, "Cust EO_id changed");
    if (id != inv_id)
      i_log(0, "Inv id changed");
    
    if (g_tt_num_ui > g_tt_max_ui || g_tt_uis == null)
    { if (g_tt_uis != null)
        free(g_tt_uis);
      g_tt_max_ui = g_tt_max_ui + 64;
      g_tt_uis = (Tt_ui)malloc((g_tt_max_ui + 1) * sizeof(Tt_ui_t));
    }
  { char buff[LUI+1];
  	int ct = g_tt_num_ui;
    Tt_ui ttp = g_tt_uis - 1;
    char * s = null;

    while (--ct >= 0)
    { s = fgets(buff, 100, input);
      if (s == null)
        break;
      ++ttp;
      ttp->wh = *s;
			while (*++s != ' ' && *s != 0)
			  ;
			ttp->price = atoi(s);
			*s = 0;
			strpcpy(&ttp->c[0], buff, LUI);
    }
    if (s == null)
    { if (ct < 0)
      { s = fgets(&buff[0], LUI, input);
        if (s != null && strcmp(buff, "END") == 0)
          break;
      }
    }
    i_log(ct,"UI file truncated");
    return -1;
  }}}
  
{ fast Invoitem iip = &inv->c[-1];
  Invoitem iiz = &inv->c[inv->curr_last];

  int ct = g_tt_num_ui;
  int tt_ix = 0;

  while (iip < iiz)
 	{ ++iip;		      /* more input */
    if ((iip->props & P_TT) == 0)
      continue;

  { int to_get = iip->qty;
    if (to_get > ct)
    { i_log(0,"Run out of uis");
      break;
    }

    ct -= to_get;

    iip->tt_ix = tt_ix;
    
    tt_ix += to_get;
  }}
}}


public void undo_invo(id, reason)
					/* mainupn write_locked */
	Id     id;
	Char * reason;
{ const Char * err = lsale(&inv, id);
  if (err == null)
  { deduct_stock(&inv, (this_sale.kind ^ K_ANTI) | K_DED, 0);
  { Invoitem iip = extend_inv(&inv);
    iip->props = P_DESCN;
    strcpy(&iip->descn[0], "Deleted: ");
    strpcpy(&iip->descn[8], reason, sizeof(iip->descn) - 9);
    (void)save_body(id, true, &inv); 
  }}
}




public Char * reason_invo(id, reason, reason_len)
					/* mainupn read_locked */
	Id     id;
	Char * reason;
	Int    reason_len;
{ const Char * err = lsale(&inv, id);
  Invoitem iip = &inv.c[inv.curr_last];
  strpcpy(&reason[0], err != null ? "???" : &iip->descn[8], reason_len);
  return reason;
}

private Char * save_sale(inv, save_all, props)
	 Invdescr inv;
	 Bool	  save_all;
	 Set	  props;
{ if (not inv->modified and (props & (K_ORD+K_DED+K_IP)) == K_IP)
    return "No changes to save";
  if (not (this_sale.kind & K_IP) and props == K_IP)
    return null;
  if ((this_sale.kind & K_SALE) and not (this_sale.kind & K_IP))
    return not disallow(R_DUP) ? null : "Invoice already printed";
  if (this_sale.customer == 0 and
      not (g_ordering or (props & K_IP)))
    return "No account to book to";
  if ((this_sale.kind & K_GIN) and (props & K_CASH))
    return "Cash illegal for Goods-in";
  if ((props & K_ORD) and (this_sale.kind & K_ORD))
    return "Already ordered";

  today = asc_to_date("");

  if (props & (K_IP+K_ORD))
  { this_sale.total = 0;
    this_sale.vattotal = 0;
  }
  if (this_sale.date == 0)
    this_sale.date = today;
    
  if (g_tt_num_ui > 0)      // Not a problem when sales loses last TT item
    this_sale.kind |= K_TT;

  inv->modified = false;
  wr_lock(mainupn);
  if      (props & K_ORD)
    deduct_stock(inv, 0, 1);
  else if (props & K_DED)
  { Set dprops = (this_sale.kind & (K_SALE + K_GIN + K_ANTI)) +
		  (~this_sale.kind & K_DED);
    deduct_stock(inv, dprops, 0);
  }

{ Bool newinv = this_sale.id == 0;
  if (newinv)
    this_sale.id = dflt_kind & K_GIN ? first_neg_id(mainupn, IX_SALE) - 1
			                        	     : last_id(mainupn, IX_SALE) + 1;
  if (dflt_kind & K_GIN)
    this_sale.kind |= K_SALE;

  this_sale.kind &= ~(K_LOCK + K_IP);  /* admin can break locks */
  this_sale.kind |= props;
  if (not ((this_sale.kind & K_GIN) or (this_sale.kind & K_IP)))
  { /*i_log(this_take.id, "stt invt");*/
    xactn_to_taking((Sorp)&this_sale);			  /* set this_take.id */
    /*i_log(this_take.id, "fin invt");*/
  }

  this_sale.agent = this_take.id;

{ Cc cc;
  Lra_t lra;
  Key_t kv_t; kv_t.integer = this_sale.id;
  cc = newinv ? NOT_FOUND : ix_search(mainupn, IX_SALE, &kv_t, &lra);
  cc = cc != OK ? i_Sale_t(mainupn, &this_sale, &lra)
            		: write_rec(mainupn, CL_SALE, lra, (Char *)&this_sale);

  if (this_sale_acc == 0)
    ths.acct.balance = 0;
  else
  { sorp_to_account(0, this_sale_acc, IX_SALE, this_sale.id,
    this_sale.kind & (K_CASH + K_IP) ? 0		     :
	  this_sale.kind & K_ANTI	  ? -this_sale.total : this_sale.total);
  }
  if (not (this_sale.kind & K_IP))
  { this_sale.balance = ths.acct.balance;	
    at_xactn((Sorp)&this_sale);
  }
  if (newinv)		  /* writing terminal with locks on for performance */
  { if (g_ordering)
      put_details_order();
    else
      put_details_invo();
    salert("Note Serial No; Any key");
  }
  if (cc == OK)
    cc = save_body(this_sale.id, save_all, inv);
  wr_unlock(mainupn);
  if (newinv)
    hold();

  clr_alert();
  return cc == OK ? null : "error saving";
}}}

private Char * do_prt_invo(cm, upn,inv,oprops, alt_prc)
	 Cc       cm;
	 Id       upn;
	 Invdescr inv;
	 Set      oprops;
	 Bool     alt_prc;			/* 0 : the lower price */
{ static Char * iban_txt = null;
  static Char * iban_0 = "";
  static Char * iban_1 = "";
        Bool del_n = /* alt_prc and this_sale.alt_terms != this_sale.terms */
                     false;
        Char area[100];
        Char * parts[3];
	Char * row1_fmt =
		          "%30!                           %30l\n";
	Char * row2_fmt =
		          "%30!  Relates to Invoice:       %9l\n";
	 Char dstr [LDATE]; date_to_asc(&dstr[0], this_sale.tax_date, 0);

  if (this_prt.path[0] == 0)
    return "You have no listing device";

  if (iban_txt == null)
  { iban_txt = malloc(MAX_IBANTXT+1);
    if (iban_txt != null)
    { Int fid = open("inv_msg", O_RDONLY, 0644);
      if (fid >= 0)
      { Int ct = read(fid, iban_txt, MAX_IBANTXT);
        if (ct > 0)
        { Char * s;
          iban_txt[ct] = 0;
          iban_0 = iban_txt;
          for (s = iban_txt; *s != 0; ++s)
            if (*s == '\n')
            { *s = 0;
              if (iban_1[0] == 0)
                iban_1 = s+1;
            }
        }
        close(fid);
      }
    }
  }

  chop3(&parts[0], strcpy(&area[0], this_custmr.postaddr), ',');
  sprintf(&msg[0], "%s\n%s",
          dflt_kind & K_GIN ? "Buying Note No %d%f" : 
          del_n             ? "Delivery Note %d%f"  :
                              "Invoice No %d %61!Vat No %r",
          "%21!%r %r%78!%l\n"
          "Osgood Smith%21!%r\n"
          "98 Greenstead Rd%21!%r%61!Tel %r\n"
          "Colchester%21!%r %r%61!    %r\n"
          "Essex   CO1 2SW.%61!Fax %r\n");

  fmt_data(&hhead[0], msg, abs(this_sale.id), uniques.vatno,
           this_custmr.forenames, this_custmr.surname, dstr, parts[0], parts[1],
           uniques.tel1, parts[2], this_custmr.postcode,
           uniques.tel2,uniques.fax);
{ Char * err = prt_one_report(&this_prt, hhead, " CONTINUES\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
  if (err != null)
    return err;

  if (not alt_prc)
    prt_fmt("COMPANY CONFIDENTIAL\n"
            "COMPANY CONFIDENTIAL\n");

  sprintf(&msg[0], "%s\n%s%s\n%s\n",
          hline(80),
          cm == CMD_DNOTE
            ? " Code    Qty  Pack  Ticket  Description"
            : " Qty  Code        Pack  Ticket  Description               Retail",
          del_n ? "" :  "  W/sale   Cost",
          hline(80));
  strcpy(&area[0], 
          cm == CMD_DNOTE
? "  %5d     %3d %l      %9r                           %30l   %6v   %6v      %9v\n"
: "%3d          %13r %l      %9r                           %30l   %6v   %6v      %9v\n");
  if (del_n)
  { area[67] = '\n';
    area[68] = 0;
  }
  prt_set_cols(msg, area);
  prt_head();
  strcat(&hhead[0], "CONTINUED\n");
  prt_set_title(hhead);

{ Short salt = this_alt;
  this_alt = alt_prc;

{ Cash vat = (this_sa.vattotal * this_sale.vat0_1pc + 500)/ 1000;
  Cash rettot = 0;
  Invoitem iip = &inv->c[-1];
  Invoitem iiz = &inv->c[inv->curr_last];
  Set  last_props = 0;
  Bool casht = (this_sale.terms & (1 << (8+'G'-'@'))) != 0;

  while (++iip <= iiz)
  { if (iip->props & P_ITEM)
    { Cash ret = iip->cas * iip->pack * iip->ticket;
      if (iip->qty > 0)
      { Cash prc = iip->price[alt_prc];
      	fmt_data(&area[0], iip->props & P_DISC ? " BONUS " : "%3d   %5v",
		     				 iip->pack * iip->cas, iip->ticket);

        if (cm == CMD_DNOTE)
      	  prt_row(0, iip->altref, iip->qty, iip->props & P_VAT ? " " : "Z",
		              area, iip->descn);
        else
      	  prt_row(0, iip->qty, iip->key, iip->props & P_VAT ? " " : "Z",
		              area, iip->descn, ret, prc, prc * iip->qty);
      }
      if (not (iip->props & P_DISC))
      	rettot += ret * iip->qty;
    }
    else
    { if ((iip->props & P_NA) and not (last_props & P_NA))
      	prt_text("         ********** Not Available *>>>>****************\n");

      prt_fmt(iip->props & P_LINK ? row2_fmt : row1_fmt, iip->descn);
    }
    last_props = iip->props;
  }

  prt_need(7);
  prt_fmt(del_n ? "------%f\n%3d\n" : "------%71!%r\n%3d%70!      %10v\n", 
          hline(9), this_ct, this_sa.gross);

  if (this_sa.disct != 0 and not alt_prc)
  { Cash dis = -this_sa.disct;
    prt_fmt(&area[0], "%36!Discount Rate  %4dpc                     %10v\n",
	    	    (int)((1.0 - this_drate) * 100 + 0.5), dis
           );
    prt_fmt("%71!%r\n", hline(9));
    prt_fmt(&area[0], "%70!      %10v\n", this_sa.gross - dis);
  }
  if (not del_n)
    prt_fmt("%34!Vat at %vpc on         %10v           %10v\n%71!%r\n",
			      this_sale.vat0_1pc * 10, 
			      this_sa.vattotal, vat, hline(9));
  strcat(strcpy(&area[0], "        %10r        %11l%62!"),
    	   del_n                           ? "%9v\n"                :
	       this_sale.terms & to_rgt(8+'P') ? "%9v           %10v\n"
				                            	   : " %f             %10v\n");
  prt_fmt(area,
        not(oprops & K_IP) ? dupmsg : this_sale.kind & K_CASH ? "CASH":"TO-ACCOUNT",
        this_sale.kind & K_ANTI ? "CREDIT-NOTE": del_n ? "DEL NOTE" : "INVOICE", 
        rettot,this_sa.total);
  fmt_data(&hhead[0],
           dflt_kind & K_GIN  ? "Record of Prices Paid" :
           dflt_kind & K_ANTI ? "E. & O.E.\n\n\n\n"   :
 "           --- Parcels -------------- Signature --------------   Please check\n"
 "           l             %9l                                 l   retail prices.\n"
 "           --- Cash ------ Cheque --- Signature --------------\n"
 " INVOICE   l           l           l                         l\n"
 "           %r\n"
 "%r\n"
 "No claims accepted for shortage unless notified immediately.\n%r\n%r\n\n",
      	  casht ? "CASH-ONLY" : spaces(9),
          hline(51),
      	  casht ? "" : strcat(strcpy(&msg[0],
 " ACCOUNT   l           l           l                         |\n"
 "           "),
		       hline(51)),
           iban_0, iban_1		   
		   );
  strcat(&hhead[0], "\n\n\n\n\n\n\n\n\n\n\n\n");
  if (iban_0[0] != 0)
    strcat(&hhead[0], "\n");
  if (iban_1[0] != 0)
    strcat(&hhead[0], "\n");
  
  prt_set_footer(hhead);
  prt_fini();
  this_alt = salt;
  return null;
}}}}

private const Char * do_prt_order(upn,inv, props)
	 Id       upn;
	 Invdescr inv;
	 Set      props;
{ Char * row1_fmt="%21!                            %30l\n";
  Char str1 [LDATE], str2[LDATE]; date_to_asc(&str1[0], this_sale.tax_date, 0);
				  date_to_asc(&str2[0], this_sale.terms, 0);
{ Prt chan = select_prt(0);
   if (chan == null)
     return msgpa;

  strcpy(&msg[0], "Order No    %d\n"
    	  "Supplier No %d\n"
    	  "Date Today                 %l\n"
		    "Arrival Date               %l\n"
  		  "Following Delivery         %l\n\n");

  fmt_data(&hhead[0], msg, abs(this_sale.id), abs(this_custmr.id),
		       todaystr, str1, str2);
{ Char * err = prt_one_report(chan, hhead, " CONTINUES\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
  if (err != null)
    return err;

  sprintf(&msg[0], "%s\n%s\n%s\n", hline(79),
 " Qty Supplier Code Case  Description              S-Now S-Arr Sale Buffer Price",
				   hline(79));
  prt_set_cols(msg, 
 "%3d     %7d   %5r %3d                         %28l  %4d  %4d  %4d  %4d     %7v\n");
  prt_head();
  strcat(&hhead[0], "CONTINUED\n");
  prt_set_title(hhead);

{ Stockanal isa = &inv->o[-1];
  Invoitem iip = &inv->c[-1];
  Invoitem iiz = &inv->c[inv->curr_last];
  Int ivl = this_sale.terms - this_sale.tax_date;
  ivl = ivl < 1 ? 1 : ivl;

  while (++iip <= iiz)
  { ++isa;
    if      (not (iip->props & P_ITEM))
      prt_fmt(row1_fmt, iip->descn);
    else /* if (iip->qty > 0) */
    { Int sale = isa->sale * 28 / ivl;
      bc_to_asc(0, iip->line, &msg[0]);
      prt_row(0, iip->qty, iip->sup, msg, iip->pack * iip->cas, iip->descn, 
              isa->stocknow,isa->stock,
              sale, isa->stock+iip->qty-isa->sale, iip->price[0]);
    }
  }
  prt_set_footer(props & K_DED ? "Order Accepted\n\n" : "Order Placed\n\n");
  prt_fini();
  return null;
}}}}

private Cash offer(upn, terms, id, std_price)
	 Id     upn;
	 Quot   terms;
	 Id     id;
	 Cash   std_price;
{ Key_t  kvi;  kvi.integer = id;
  /*i_log(4, "Finding offers for %d", id);*/
{ Lra_t  lra;
  Set which = terms >> LOG_B_SR;
  Cash prc = MAXINT;
  Char * strm = ix_new_stream(upn,IX_SR, &kvi);
  fast Stockred sr;

  while ((sr = (Stockred)ix_next_rec(strm, &lra, null)) != null)
  { Id sid = sr->id;
    Set wh = sr->which;
    Cash pr = sr->price;

  /*i_log(4, "Try %d %x %d", sid, wh, pr);*/
    
    if (sid != id)
      break;
    if      (wh & which & 1)
    { if (((wh ^ which) & 0xff0000) == 0)
      { prc = pr;
        break;
      }
    }
    else if ((wh & which) and pr < prc)
      prc = pr;
  }

  return prc == MAXINT ? std_price : prc;
}}

private Stockitem get_si(upn, sup, line, lra_ref)
	 Id      upn;              /* locks on */
	 Id      sup, line;
	 Lra_t  *lra_ref;
{ Key_t kv_t;
  static Stockitem_t si_t;
  static Stockitem_t sibuf;

  if (sup != 0)
  { kv_t.string = bc_to_asc(sup, line, &ds1[0]);
    return (Stockitem)ix_fetchrec(upn, IX_STKBC, &kv_t, lra_ref, &si_t);
  }
  else
  { Int spos = save_pos();
    kv_t.integer = line;
  { Stockitem res__ = (Stockitem)ix_fetchrec(upn,IX_STKLN,&kv_t,lra_ref,&si_t);
    if (res__ == null or not was_mult())
      return res__;
    redisplay = true;
  { Char * strm = ix_new_stream(upn,IX_STKLN, &kv_t);
    Int lim = SL;
    Bool cont = true;
    Int ct = 0;
    Int rawct = 0;
    Bool fst = true;
    const Char * error = crsq;
    Field fld;

    Int i;
    Id keytbl[SL+1];
    for (i = SL+1; --i >= 0;)
      keytbl[i] = 0;
    rd_unlock(upn);                       /* because its locked */

    sibuf.constraint = 0;

    while (cont)
    { Stockitem si;
      while (lim > 0 and
      	    (si = (Stockitem)ix_next_rec(strm, lra_ref, &si_t)) != null)
      { /*i_log(0, "NR %d %d", si->line, si->supplier);*/
        ++rawct;
        if (si->line != line)
        { si = null;
          break;
        }
        if (this_constraint != 0 and (this_constraint & si->constraint) == 0)
          continue;
        keytbl[SL-lim] = si->id;
        if      (ct == 0)
        { sibuf = *si;
          ct = 1;
          --lim;
          continue;
        }
        if (ct == 1)
        { wclrgrp();
          goto_fld(W_KEY);
      	  put_ordstk(0, &sibuf);
      	  down_fld(0, &fld);
        }
        put_ordstk(SL-lim, si);
        if (--lim > 0)
      	  down_fld(0, &fld);
        ++ct;
      }

      if (si == null and ct <= 1)
      { rd_lock(upn);
        ix_fini_stream(strm);
        restore_pos(spos);
        return ct == 0 and (rawct == 0 or this_constraint == 0) ? null 
                                                  							: &sibuf;
      }

      if (fst)
      { (void)goto_fld(W_KEY);
      	fst = false;
      }

      while (cont)
      { if (error != null)
      	  alert(error);
      	error = null;

      { Cc cc = get_any_data(&ds1[0], &fld);
      	Int dist = 0;

      	if	(cc == PAGE_DOWN)
      	  dist = SL-1;
      	else if (cc == HALTED and in_range(fld->id, W_QTY, W_PRC))
      	  if (not (fld->props & FLD_LST))
	          error = nrsc;
	    	  else
  	  	    dist = 1;
      	else if (ds1[0] == '.' or
            		 ds1[0] == 0 /* and gad_ct > 0 */ or  
								 in_range(ds1[0], '1', '9'))
      	  cont = false;
      	else
      	  error = crsq;
      	if (dist != 0)
      	{ wscroll(dist);
      	  scroll_istk(keytbl,dist);
      	  lim += dist;
      	  while (--dist > 0)
	        up_fld(0, &fld);
      	  break;
      	}	
      }}
    }
    ix_fini_stream(strm);
    wclrgrp();

    rd_lock(upn);
    restore_pos(spos);
    kv_t.integer = keytbl[s_frow(fld)];
    return ds1[0] != 0 or not in_range(fld->id, W_QTY, W_PRC) or
      	   kv_t.integer == 0
	     ? null : (Stockitem)ix_fetchrec(upn,IX_STK,&kv_t,lra_ref,&si_t);
  }}}
}

private const Char * shop(si, inv, qty, uniq)
	 Stockitem si;
	 Invdescr  inv;
	 Int	   qty;
	 Bool    uniq;
{ Invoitem item = &inv->c[inv->curr_last+1];
  Stockanal sa =  &inv->o[inv->curr_last+1];
  Invoitem it = item;

  if (uniq)
    while (--it >= &inv->c[0])
      if ((it->props & (P_ITEM + P_OBS)) == P_ITEM)
        if (it->sup == si->supplier and it->line == si->line)
        	return arth;
  do_sa(si, sa, qty);
  if (qty == MININT)
  { Int case_ = s_case(si);
    qty   = dbl(sa->sale - sa->stock) * def_margin + 0.4;
    qty   = case_ <= 0 ? qty : ((qty + case_ - 1) / case_) * case_;
    if (qty < 0)
      qty = 0;
  }
  sa->qty = qty;
  return null;
}

private void get_offer(item, si)
	Invoitem item;
	Stockitem si;
{ item->id     = si->id;
  item->props  = P_ITEM | (si->props & (P_VAT + P_DISC));
  item->sup    = si->supplier;
  item->line   = si->line;
  item->altref = si->profit * (256*256L) + si->altref;
  item->cas    = s_case(si) < 2 ? 1 : s_case(si);
  item->pack   = si->pack * item->cas;
  item->ticket = si->ticket;
  item->price[0] = dflt_kind & K_GIN	? 0	    :
		   not (si->props & P_SR) ? si->price :
			  offer(mainupn, this_sale.terms, si->id,si->price);
  item->price[1] = item->price[0];
  if (this_sale.alt_terms != this_sale.terms and
      (dflt_kind & K_GIN) == 0               and
      (si->props & P_SR) 
     )
    item->price[1] = offer(mainupn, this_sale.alt_terms, si->id,si->price);

  strpcpy(&item->descn[0], si->descn, sizeof(Descn));
  bc_to_asc(si->supplier, si->line, &item->key[0]);
}

static char * charlie_barcodes[] =
{ "010500014391647810400646992150015324014231245",
  "010500014392578410400960872151196324014231248",
  "0105000431028449210DURXQ8UWK0824002166504",
  "01050003930829092155198K520326323130024010047228",
  "01050003930834872163118EL23171557730024010041523",
  "010500043102729921XLVFLU8MKNGM24002166290",
  "01050003930832412163B28MT15186454030024010047209",
  "010506007778625721T63BGCYTZGQP240FA068924.02"
};


public Char * get_core_code(const Char * data, Char * ui, int * outer)

{ static Char local_data[14];
  *ui = 0;
  *outer = 0;
  
  if (data[0] == ';' && in_range(data[1], '1', '8'))
    data = charlie_barcodes[data[1] - '1'];

  if (data[0] == '_') 
    *outer = 1;
  
  if (strlen(data) > 16 || *outer != 0)
  { strpcpy(&local_data[0], data+3+*outer*2, 13);
    strpcpy(&ui[0],data/*+12*/,LUI+100);
    data = local_data;
  }
  
  return (char *)data;
}


private const Char * do_add_item(upn,  inv, data)
	 Id       upn;
	 Invdescr inv;
	 Char *   data;
{ static Char ui[LUI+100]; ui[0] = 0;
  int outer;
  const Char * res = mark_inv(inv, true);
  if (res != null)
    return res;

  if (data[0] != '/')
  { data = get_core_code(data, ui, &outer);
    if (ui[0] != 0 && g_tt_uis != NULL)
    { Tt_ui s;
      for ( s = &g_tt_uis[g_tt_num_ui]; --s >= g_tt_uis; )
        if (strcmp(&s->c[0],ui) == 0)
          break;
      if (s >= g_tt_uis)
        return "Alreay Scanned";
    }
    if (outer != 0 && g_ordering == 0)
      return "Cannot sell cases";
  }

{ Invoitem item = extend_inv(inv);

  if (data[0] == '/')
  { item->id = -1;
    item->props = P_DESCN;
    strpcpy(&item->descn[0], &data[1], sizeof(Descn));
  }
  else
  { inv->curr_last -= 1;
  { Int qty = MININT;
    Char * data_ = get_pling_qty(data, &qty);
    Id sup, line;
    Char * d = get_bc(data_, &sup, &line);
    if (d <= data_)
      return *data == 0 ? tcr : illco;

    rd_lock(upn);
  { Lra_t lra;
    Stockitem si_ = get_si(upn,  sup, line, &lra);
    if      (si_ == null)
    { sprintf(&ui[0],"Not found %s, Type CR",data);
      res = ui;
    }
    else if ((this_sale.terms & to_rgt(8+'R')) and 
             not (si_->props & to_rgt(8+'R')))
      res = "Not restricted range, Type CR";
    else if (this_constraint != 0 and (this_constraint & si_->constraint) == 0)
      res = "Not as preferred, Type CR";
    else if (ui[0] == 0 && (si_->props & P_HAS2D))
      res = "Scan the 2D code";
    else
    { item->qty  = qty == MININT ? 1 : qty;
      get_offer(item, si_);
      
      if ((*d == '/' or *d == ' ') and in_range(d[1], '1', '9'))
      { (void)get_cash(&d[1], &item->price[0]);
        item->price[1] = item->price[0];
      }
      if (item->props & P_DISC)
      { item->price[0] = -item->price[0];
        item->price[1] = item->price[0];
      }

      if (g_ordering)
        res = shop(si_, inv, qty, ui[0] == 0);
      if (res == null)
      { add_to_invo(inv, item);
        if (ui[0] != 0)
        { item->props |= P_TT;
          ++g_tt_num_ui;
          if (g_tt_num_ui > g_tt_max_ui)
          { Tt_ui nui = (Tt_ui)malloc(sizeof(Tt_ui_t) * (g_tt_max_ui + 66));
            if (nui != null)
            { if (g_tt_uis != null)
              { memcpy(nui, g_tt_uis, sizeof(Tt_ui_t) * (g_tt_num_ui - 1));
                free(g_tt_uis);
              }
              g_tt_uis = nui; 
              g_tt_max_ui += 64;
            }
          }
          if (g_tt_num_ui <= g_tt_max_ui)
          { g_tt_uis[g_tt_num_ui - 1].wh = item->price[0] == 0 ? 'C' :
                                           s_case(si_) <= 1    ? 'P' : 'C';
            strpcpy(&g_tt_uis[g_tt_num_ui - 1].c[0], ui, LUI+1);
            g_tt_uis[g_tt_num_ui - 1].price = item->price[0];
          }
        }
      }
    }
    rd_unlock(upn);
  }}}
  return res;
}}

private const Char * do_add_na(upn,  inv, data)
	 Id       upn;
	 Invdescr inv;
	 Char *   data;
{ const Char * res = null;
  salert(this_sale.kind & K_ANTI ? "Type Related Invoice numbers, . to finish"
                                 : "Type Items Not Available, . to finish");
  goto_fld(W_ADD);

  while (res == null)   /* and not break */
  { get_data(&data[0]);
    if (data[0] == 0 or data[0] == '.')
      break;

  { Invoitem item = extend_inv(inv);
    item->props   = P_DESCN + P_NA + (this_sale.kind & K_ANTI ? P_LINK : 0);
    strpcpy(&item->descn[0], data, sizeof(Descn));
    res = mark_inv(inv, true);
    if (res != null)
      inv->curr_last -= 1;
  }}
  return res;
}

private const Char * do_q_stock(upn,  inv, ds)
	 Id       upn;
	 Invdescr inv;
	 Char *   ds;
{ Id keytbl[100];
  Int i;
  Key_t  kv_t;
  Id  sup, line;
  Char * ccstr = get_bc(ds, &sup, &line);
  redisplay = true;
  if (ccstr < ds)
    return illco;

  kv_t.string = bc_to_asc(sup, line, &ds[0]);
  if (sup == 0)
    kv_t.integer = line;

  for (i = 100; --i >= 0; )
    keytbl[i] = 0;

{ Int spos = save_pos();
  Quot ix = sup == 0 ? IX_STKLN : IX_STKBC;
  Char * strm = ix_new_stream(upn, ix, ccstr == ds ? null : &kv_t);
  Lra_t lra;
  Int lim = inv_sl;
  Bool cont = true;
  Bool fst = true;
  const Char * error = null;
  Field fld = goto_fld(W_KEY);

  salert(crsq);

  while (cont)
  { Stockitem si;
    while (lim > 0 and (si = (Stockitem)ix_next_rec(strm, &lra, null)) != null)
      if (this_constraint == 0 or (this_constraint & si->constraint) != 0)
      { keytbl[inv_sl-lim] = si->id;
        put_ordstk(inv_sl-lim, si);
        if (--lim > 0)
      	  down_fld(0, &fld);
      }

    if (fst)
    { (void)goto_fld(W_KEY);
      fst = false;
    }

    while (cont)
    { if (error != null)
      	alert(error);
      error = null;

    { Cc cc = get_any_data(&ds[0], &fld);
      Int dist = 0;

      if      ((ds[0] & 0x1f) == ('L' - '@'))
      	dist = inv_sl-1;
      else if (cc == HALTED and in_range(fld->id, W_QTY, W_PRC))
      	if (not (fld->props & FLD_LST))
      	  error = nrsc;
      	else
      	  dist = 1;
      else if (ds[0] == '.' or ds[0] == 0 or in_range(ds[0], '1', '9'))
      	cont = false;
      else
      	error = crsq;
      if (dist != 0)
      { wscroll(dist);
      	scroll_istk(keytbl,dist);
      	lim += dist;
      	while (--dist > 0)
      	  up_fld(0, &fld);
      	break;
      }
    }}
  }
  ix_fini_stream(strm);
  wclrgrp();
  restore_pos(spos);
  if (ds[0] == '.' or not in_range(fld->id, W_QTY, W_PRC))
    error = "";
  else
  { Key_t kvi;	kvi.integer = keytbl[s_frow(fld)];

  { Stockitem si = kvi.integer == 0
              		? null : (Stockitem)ix_fetchrec(upn, IX_STK, &kvi, null,null);
    error = si == null ? "Nothing There, Type CR" : mark_inv(inv, true);
    if (error == null)
    { Invoitem item = extend_inv(inv);
      item->qty = ds[0] != 0 ? atoi(ds) :
								  g_ordering ? MININT   : 1; 
      inv->curr_last -= 1;
      get_offer(item, si);

      if (item->props & P_DISC)
      { item->price[0] = -item->price[0];
        item->price[1] = -item->price[1];
      }
      if (g_ordering)
      	error = shop(si, inv, item->qty);
      if (error == null)
      	add_to_invo(inv, item);
    }
  }}
  return error;
}}

private void query_c_details(ds)
	 Char * ds;
{ this_sale.customer = 0;
  this_custmr.id = 0;
  strcpy(&this_custmr.surname[0], ds);
  goto_fld(W_FNAME);  get_data(&this_custmr.forenames[0]);
  goto_fld(W_ADDR);   get_data(&this_custmr.postaddr[0]);
  goto_fld(W_SNAME);
}


#ifndef atof
extern double atof();
#endif

private const Char * do_edit_invo(upn,  fld, inv, ds)
	 Id	  upn;
	 Field	  fld;
	 Invdescr inv;
	 Char *   ds;
{ today = asc_to_date("");
{ Bool markit = false;
  Quot kind = s_fid(fld);
  const Char * res = (kind == W_TERMS or kind == W_DATE  or
    		              kind == W_DRTE  or kind == W_CAT)
            		       and not (this_sale.kind & K_IP) ? rdoy : null;
  switch (kind)
  { case W_TERMS: if (res == null)
            		  { this_sale.terms = asc_to_terms(ds) & ~M_BILL;
              	    markit = this_sale.id != 0;
            		  }
    when W_DATE:{ Date date = asc_to_date(ds);
            		  if (date < 0)
            		    res = illd;
            		  if (res == null)
            		  { this_sale.tax_date = date;
            		    markit = this_sale.id != 0;
            		  }
            		}
    when W_DRTE:
      res = disallow(R_DBILL) and not (dflt_kind & (K_ANTI+K_GIN))
		                           	? preq('H') :
     			  not vfy_real(ds)	  ? nreq		  : res;
		  if (res == null)
		  { this_drate = 1.0 - atof(ds) / 100.0;
		    if (this_drate < 0.89 or this_drate > 1.0)
		      res = "discounts up to 10%";
		    else
		    { recalc_invo(inv);
		      update_sale();
		      markit = this_sale.id != 0;
		    }
		  }
    when W_CAT:
    { Id cat = atol(ds);
      if (ds[0] != 0 and
	  	     not (vfy_nninteger(ds) and in_range(cat, 0, 255)))
		    res = "Number 0 to 255";
  		if (res == null)
	  	{ this_sale_cat = cat;
	  	  this_sale.cat = cat;
	  	  markit = this_sale.id != 0;
	  	}
	  }
    when W_CST:
    { Set cst = asc_to_rgt(ds, false);
      if ((unsigned)cst > 0xffff)
        res = "only letters A-O";
      else
        this_constraint = cst;
    }

    when W_INO:   res = not inv->modified     ? res  : "Invoice in Progress";
    case W_ID:	  res = vfy_integer(ds)       ? res  : nreq;
    case W_SNAME: res = this_custmr.surname[0] == 0 or kind == W_INO
		                              			      ? res  : "not changeable";
		  if (res == null)
		  { Id id = atoi(ds);
		    init_sale(inv);
		    if	    (kind == W_INO)
		    { res = load_sale(inv, dflt_kind & K_GIN ? -id : id);
		      if (res == null or res[0] == '$')
      		{	this_custmr.id = this_sale.customer;
        		if (g_tt_num_ui < 0)
        		  (void)load_tt_inv(this_custmr.id, inv);
          }
		    }
		    else if (kind == W_SNAME)
		    { Set who = P_LITFST+P_NOSPEC
				          + (dflt_kind & K_GIN ? P_SUPPLR : 0);
		      Id id = get_one_cust(upn, ds, who, put_details_invo);
		      if (id == 0)
			      query_c_details(ds);
		    }
		    else
		    { Id ix = dflt_kind & K_GIN ? IX_MANBC : IX_CUST;
		      Key_t k_t;k_t.integer = id;

		    { Customer cu = (Customer)ix_fetchrec(upn,ix, &k_t, null, &this_custmr);
		      if (cu == null)
      			res = ntfd;
		      if ((this_custmr.terms & P_DLTD) and
		           this_custmr.terms != -1)
		        res = cdel;
		    }}
		    this_sale_acc = this_custmr.id;
		    this_sale.customer = this_sale_acc;
		    if (this_sale_acc != 0)
		      (void)get_cacc(&this_sale_acc, &this_sale.terms,
				dflt_kind & K_GIN ? IX_SUP : IX_CUST);
		    if (kind != W_INO and this_sale.terms & (to_rgt(8+'W') << 8)
		                      and this_sale.terms != -1)
		    { init_sale(inv);
		      res = "Customer on Hold";
		    }
		    
		    this_sale.alt_terms = this_sale.terms;
		    if ((this_sale.terms & P_AT) and 
		        in_range((this_sale.alt_terms & 0xff000000),
		                  0x1000000,0x3000000))
        { this_sale.terms &=  0xffffffL;
          this_sale.terms |= 0x3000000L;/* cheapest fixed terms */
        }
        
      { Key_t  kv_t;
			  kv_t.integer = this_sale_acc;
			  rd_lock(mainupn);
			{ Lra_t lrb;
			  Tt_reg_t tt_r;
		    Tt_reg tt_reg = (Tt_reg)ix_fetchrec(mainupn, IX_TT, &kv_t, &lrb, &tt_r);
			  rd_unlock(mainupn);
			  g_cust_TT_id[0] = 0;
			  if (tt_reg != null)
			    strpcpy(g_cust_TT_id, tt_reg->tt_id, sizeof(g_cust_TT_id));
		  }}}
		break;
    otherwise	    res = "Not Editable";
  }
  if (markit)
    res = mark_inv(inv, true);
  if (res == null or res[0] == '$')
  { put_totals_invo();
    terms_to_asc(this_sale.terms, &this_sale_terms[0]);
  }
  put_details_invo();
  return res;
}}

private const Char * find_i_item(inv, ds_)
	 Invdescr inv;
	 Char *   ds_;
{ Char * bc = skipspaces(&ds_[4]);
  Int ix;

  if (*bc == 0)
  { inv->curr_base = 0;
    return null;
  }

  for (ix = 0; ix <= inv->curr_last; ++ix)
    if (strcmp(bc, inv->c[ix].key) == 0)
    { inv->curr_base = ix;
      return null;
    }
  return ntfd;
}

private const Char * do_edit_iitem(upn,  fld, inv, ix, data)
	 Id	  upn;
	 Field	  fld;
	 Invdescr inv;
	 Int	  ix;
	 Char *   data;
{ Invoitem item = &inv->c[ix];
  const Char * res = mark_inv(inv, false);
  if (res != null)
    return res;
  if (not (item->props & P_ITEM))
    return cndo;

  if      (s_fid(fld) == W_QTY)
  { res = vfy_integer(data) ? null : nreq;
    if (res == null)
      item->qty = atoi(data);
  }
  else if (s_fid(fld) == W_KEY)
    res = cndo;
  else if (s_fid(fld) == W_PRC)
  { res = disallow(R_CUT) and not (dflt_kind & (K_GIN+K_ANTI))
			                      			   ? preq('J')           :
      	  not vfy_cash(data)			   ? "price needed"	 :
      	  item->props & P_DISC			 ? "Bonuses are fixed" : null;
    if (res == null)
    { item->price[0] = atoi(data);
      item->price[1] = item->price[0];
    }
  }
  else
    res = cndo;
  if (res == null)
  { recalc_invo(inv);	
    update_sale();
    put_totals_invo();
    res = mark_inv(inv, true);
  }
  return res;
}

private Cc do_invo(upn, inv, ds)
	 Id	  upn;
	 Invdescr inv;
	 Char *   ds;
{ clr_alert();
/*flush_pen(); */
  goto_fld(this_sale_cat == 0 ? W_SNAME : W_CAT);
{ Int ix = -1;
  Cc contcc = SUCCESS;
  const Char * error = null;
  Int sl = g_rows - 14;

  while (contcc == SUCCESS)
  { while (ix - inv->curr_base < sl-1 && ix < inv->curr_last)
      put_iitem(inv, ++ix);

    if (error != null)
    { if (error[0] != '$')
        alert(error);
      else
        salert(&error[1]);
    }
    error = "HOME to quit";
    redisplay = false;

  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);
    Char *d = skipspaces(&ds[0]);
    vfy_lock(upn);

    if (cc == PAGE_UP or cc == PAGE_DOWN)
    { Short base = inv->curr_base + (cc == PAGE_DOWN ? sl - 1 : 1 - sl);
      if (base + sl - 1 > inv->curr_last)
      	base = inv->curr_last - sl + 2;
      if (base < 0)
      	base = 0;
      if (inv->curr_base != base)
      { inv->curr_base = base;
        ix = base - 1;
      	wclrgrp();
      }
      error = null;
    }
    else if (cc == HALTED)
    { if (fld->props & FLD_FST)
      { if (inv->curr_base > 0)
      	{ wclrgrp();
    	    inv->curr_base -= 1;
	        ix = inv->curr_base - 1;
    	  }
      }
      else if (inv->curr_base + sl - 1 < inv->curr_last)
  	  { wscroll(1);
	      inv->curr_base += 1;
    	}
      error = null;
    }	
    else if (fld->id == W_CMD)
    { Int newix = inv->curr_base-1;
      Cc cm = sel_cmd(d);
      if      (cm == CMD_DOT)
      { if (inv->modified)
      	  error = dsfst;
      	else
      	  contcc = HALTED;
      }
      else if (cm == CMD_CHECK)
      { Int diff = atoi(&d[5]) - this_ct;
      	if (diff == - this_ct)
      	{ salert(enoi);
      	  put_data("");
      	  cc = get_data(&ds[0]);
      	  diff = atoi(ds) - this_ct;
      	}
      	if (diff == 0)
      	{ error = null;
      	  goto_fld(W_ADD);
      	}
      	else
      	{ sprintf(&ds[0], in_range(diff,-2,2) ? "Multiples %d, Count is %d" :
			            this_ct != this_ln  ? "Multiples %d"
                            					: "Wrong",this_ln, this_ct);
       	  error = (const char*)&ds[0];
     	  }
      	newix = ix;
      }
      else if (cm == CMD_EDI)
      { if (this_sale.kind & K_IP)
          error = "Must be complete";
        else
          save_extinv(&this_sale);
      }
      else if (cm == CMD_PRINT or cm == CMD_DNOTE or cm == CMD_CASH or cm == CMD_SAVE)
      { Set oprops = this_sale.kind;
        Set props = (toupper(*d) == 'C' ? K_CASH + K_PAID : 0) +
		                (toupper(*d) == 'S' ? K_IP		  : K_DED);
        Bool alt_prc = toupper(*d) != 'P' or 
                       toupper(skipspaces(&d[5])[0]) != 'R' or
                       toupper(skipspaces(&d[5])[1]) != 'R';

        if      (not alt_prc and disallow(R_SCRT))
          error = preq('L');
        else if (props == 0 and disallow(R_SALE))
          error = preq('S');
        else
        { if	  (this_sale.kind & K_GIN)
	          error = null;
        	else if (not (oprops & K_IP))
        	  error = props & K_DED ? null : rdoy;
        	else if (props & K_IP)
        	  error = null;
        	else
        	{ Int diff = atoi(&ds[5]) - this_ct;
        	  if (diff == - this_ct and (oprops & K_IP))
        	  { if (this_ct <= LETOFFCT)
        	      diff = 0;
        	    else
        	    { salert(enoi);
        	      put_data("");
        	      cc = get_data(&ds[0]);
        	      diff = atoi(ds) - this_ct;
        	    }
        	  }
            if (diff != 0 and (oprops & K_IP))
        	  { sprintf(&ds[0], in_range(diff,-2,2) ? "Multiples %d, Count is %d" :
			                this_ct != this_ln  ? "Multiples %d"		:
					                                  "Wrong",this_ln, this_ct);
      	      error = (const char *)&ds[0];		
        	    goto_fld(W_ADD);
        	  }
        	  else
        	  { if (props & K_CASH)
      	      { salert("Enter Cheque Number or CR for cash");
                put_data("");
             		cc = get_data(&ds[0]);
             		this_sale.chq_no = ds[0] == 0 ? -1 : atoi(&d[0]);
             		if (this_sale.chq_no >= 0)
             		  props |= K_CCHQ;
	            }
      	      if (this_sale.kind & K_ANTI)
      	      { Char ch = salerth("This is a Credit Note, Type Y");
           		  error = toupper(ch) == 'Y' ? null : "Use Drop";
             		if (error == null)
        	      { Char ch = salerth("Goods to be resold(Y/N)?");
		              if (toupper(ch) != 'Y')
             		    props &= ~K_DED;
            		}
  	          }
      	      error = do_add_na(upn, inv, ds);
    	      }
    	    }
        	if (error == null and (oprops & K_IP))
	          error = save_sale(inv, true, props);
      	  if (error == null)
         	{ if (g_tt_num_ui > 0)
              (void)save_tt_inv();

         	  if (not (props & K_IP))
      	      error = do_prt_invo(cm, upn, inv, oprops, alt_prc);
		  	    	/* only now is biglistbuff free */
  	        if (this_sale.kind & K_ANTI)
        	  { wr_lock(mainupn);
       	      match_credit(mainupn, this_sale_acc);
       	      wr_unlock(mainupn);
       	    }
	          init_sale(inv);
	          wreopen(tbl1);
            goto_fld(W_SNAME);
	        }
	      }
     	  newix = ix;
      }
      else if (cm == CMD_DROP)
      { if (inv->modified and this_sale.id != 0)
       	  (void)sale_lock(false, this_sale.id);
       	init_sale(inv);   wreopen(tbl1);   error = null;
      	goto_fld(W_CMD);
      }
      else if (cm == CMD_STOCK)
      { do_stock();
       	wreopen(tbl1);
       	put_details_invo();
  				/* the discount banner must be redisplayed */
       	put_totals_invo();
       	goto_fld(W_ADD);
       	error = null;
      }
      else if (cm == CMD_FIND)
      { error = find_i_item(inv, d);
       	if (error == null)
       	{ Int pos = 0;
       	  if (inv->curr_base >= 2)
       	  { pos = 2; inv->curr_base -= pos; }
          wclrgrp();
          goto_fld(W_KEY + pos);
       	  newix = inv->curr_base - 1;
        }
      }
      else
      { error = *d == 0 	 ? null :
               	dflt_kind & K_GIN ? "Print, Save, Drop, Stock, or Find" :
			                          "Print, Cash chq_no, Save, Drop, Stock, or Find";
       	newix = ix;
      }
      ix = newix;
    }
    else if (cc == SUCCESS)
    { if      (in_range(fld->id, W_INO, W_ADDR) or fld->id == W_DRTE)
      { error = do_edit_invo(upn, fld, inv, ds);
      	if (fld->id == W_INO and error == null)
       	  ix = inv->curr_base - 1;
       	goto_fld(W_ADD);
       }
       else
       { if (fld->id == W_ADD)
       	{ error = d[0] == '?' ? do_q_stock(upn, inv, &d[1])
		                          : do_add_item(upn, inv, d);
       	  if (error != null)
       	  { alert(error);  error = null;
       	    while (true)
       	    { get_data(&d[0]);
       	      if (d[0] == 0)
       	        break;
       	      alert("Type CR");
       	    }
       	    clr_alert();
       	  }
       	  else
       	  { Int scroll_dist = inv->curr_last - inv->curr_base - sl + 1;
       	    if (scroll_dist > 0)
       	    { if (scroll_dist < 3)
             		scroll_dist = 3;
       	      wscroll(scroll_dist);
       	      inv->curr_base += scroll_dist;
       	    }
       	    ix = ix >= inv->curr_last-1 ? inv->curr_last-1 : inv->curr_base-1;
       	  }
       	  if (redisplay)
       	    ix = inv->curr_base - 1;
       	}
       	else if (in_range(fld->id, W_QTY, W_PRC+sl))
       	{ Int ix_ = inv->curr_base + (fld->id & (ID_FAC - 1));
       	  if (ix_ <= inv->curr_last)
       	  { error = do_edit_iitem(upn,	fld, inv, ix_, ds);
       	    put_iitem(inv, ix_);
       	  }
       	}
      }
    }
  }}
  end_interpret();
  return contcc;
}}

public const Char * do_invoices(which)
	Set  which;
{ if ((which == K_SALE) 	 and disallow(R_INV) or
      (which == K_SALE + K_ANTI) and disallow(R_RTN) or
      (which & K_GIN)		 and disallow(R_GIN))
    return "Permits required";

  dflt_kind = which;
  g_ordering = which & K_GIN ? true : false;

  init_pen();
  if (which & K_TT)
  { Tabled tbl = mk_form(DDform,0);
		char d_barcode[80];
		d_barcode[0] = 0;
  	wopen(tbl,DDover);
	  goto_fld(W_ID);
	{ Cc contcc = SUCCESS;
	  const Char * error = null;
	  while (contcc == SUCCESS)
  	{ if (error != null)
  	    salert(&error[0]);
	  { Field fld;
  	  Cc cc = get_any_data(&ds[0], &fld);
	    Char *d = skipspaces(&ds[0]);
	    if 			(fld->id == W_CMD)
	    { if (*strmatch("DO", d) == 0)
				{ if (d_barcode[0] == 0)
					{ error = "Nothing to save";
						continue;
					}
					wr_lock(mainupn);
				{ Id dn_id = 0;
					Lra_t lra;
				  Unique un = find_uniques(mainupn, &lra);
				  if (un != null)
					{ dn_id = (un->last_dnote += 1);
				    write_rec(mainupn, CL_UNIQ, lra, un);
					}

  				wr_unlock(mainupn);
	    		(void)save_tt_dnote(d, -1000000 - dn_id);
					contcc = HALTED;
	    	}}
	    	else
					error = "Do";
	    }
	    else if (fld->id == W_ID)
	    { strpcpy(d_barcode, d, sizeof(d_barcode));
	    }
	  }}
  }}
  else
  {	init_sale(&inv);
	  select_io(this_tty, 'I');
	  wopen(tbl1,SIover);
	  this_take = null_sct(Taking_t);

	  while (true)
		{ Cc ccc = do_invo(mainupn, &inv, ds);
		  if (ccc == HALTED)
    	  break;
	  }
	  fini_sale(&inv);
  }

  return null;
}

						  /* Order Recommendations */
private const Char * redo_order(upn, inv)
	 Id	   upn;
	 Invdescr  inv;
{ if (this_sale.id != 0)
    return rdoy;
{ Invoitem iip = &inv->c[-1];
  Invoitem iiz = &inv->c[inv->curr_last];
  Stockanal sa = &inv->o[-1];
  Int ct = -1;

  while (++iip <= iiz)
  { ++sa; ++ct;
    if (iip->props & P_ITEM)
    { Stkkey sk;
      Lra_t lra;
      Key_t kv_t; kv_t.string = bc_to_asc(iip->sup, iip->line, &sk[0]);
    { Stockitem si = (Stockitem)ix_srchrec(upn, IX_STKBC, &kv_t, &lra);
      if (si == null)
      { strcpy(&iip->descn[0], "Obsolete");
        iip->qty = 0;
      }
      else
      { Int six = inv->curr_last;
        inv->curr_last = ct - 1;
        (void)shop(si, inv, MININT);
        inv->curr_last = six;
      }
    }}
  }
  inv->curr_base = 0;
  return null;
}}	



private Char * copy_order(inv)
	 Invdescr  inv;
{ if (this_sale.id == 0)
    return "Select an order first";
  this_sale.id = 0;
  this_sale.kind = K_GIN + K_IP;
  put_details_order();
  return null;
}

private const Char * do_edit_oitem(upn, fld, inv, ix, data)
	 Id	  upn;
	 Field	  fld;
	 Invdescr inv;
	 Int	  ix;
	 Char *   data;
{ Invoitem item = &inv->c[ix];
  Quot fld_typ = fld->id & ~(ID_FAC - 1);
  const Char * res = mark_inv(inv, false);
  if (not (item->props & P_ITEM))
    return cndo;

  if (fld_typ == W_ORD or fld_typ == W_MAR)
  { res = vfy_integer(data) ? null : nreq;
    if (res == null)
    { Stockanal sa = &inv->o[ix];
      item->qty = atoi(data);
      if (fld_typ == W_MAR)
      { Int omarg = sa->stock + sa->qty - sa->sale;
      	item->qty = sa->qty + item->qty - omarg;
      }
      sa->qty = item->qty;
    }
  }
  else if (fld_typ == W_SUP or fld_typ == W_KEY)
  { if (toupper(*data) == 'D')
      item->props |= P_OBS;
    else
      res = "D to delete";
  }
  else if (fld_typ == W_PRC)
  { res = disallow(R_CUT) and dflt_kind == K_SALE ? preq('J')           :
            	          not vfy_cash(data)			  ? "price needed"	:
                  	  item->props & P_DISC			  ? "Bonuses are fixed" : null;
    if (res == null)
    { item->price[0] = atoi(data);
      item->price[1] = item->price[0];
    }
  }
  else
    res = cndo;
  if (res == null)
    res = mark_inv(inv, true);
  return res;
}

private const Char * do_edit_order(upn,  fld, order, ds)
	 Id	  upn;
	 Field	  fld;
	 Invdescr order;
	 Char *   ds;
{ const Char * res = null;
  Quot kind = fld->id & ~(ID_FAC - 1);
  switch (kind)
  { case W_TERMS: res = vfy_real(ds) ? null : "eg 0.2";
            		  if (res == null)
            		    def_margin = 1.00000001 + atof(ds) / 100.0;
    when W_STT:
    case W_STP:{ Date date = asc_to_date(ds);
            		 res = date < today ? illd : mark_inv(order, true);
            		 if (res == null)
          		   if (kind == W_STT)
          		     this_sale.tax_date = date;
          		   else
          		     this_sale.terms	= date;
      	       }
    when W_INO:  res = not order->modified ? res  : "Order in Progress";
    case W_ID:	 res = vfy_nninteger(ds)   ? null : nreq;
            		 res = this_custmr.id == 0 or kind == W_INO
      			                        		   ? res  : "not changeable";
            		 if (res == null)
            		 { Id id = atoi(ds);
            		   if	   (kind == W_INO)
            		   { res = load_sale(order,-id);
            		     if (res == null)
            		       this_custmr.id = this_sale.customer;
            		   }
            		   else
            		   { Id ix = dflt_kind & K_GIN ? IX_MANBC : IX_CUST;
            		     Key_t kvi; kvi.integer = id;
            		     rd_lock(upn);
            		   { Supplier sup =
                          			(Supplier)ix_fetchrec(upn,ix,&kvi,&trash,&this_custmr);
            		     if (sup == null)
            		       res = ntfd;
            		     this_sale.customer = this_custmr.id;
            		     rd_unlock(upn);
            		   }}
            		   this_sale_acc = this_custmr.id;
                 { Key_t  kv_t;
            		   kv_t.integer = -this_sale_acc ;
            			 rd_lock(mainupn);
            		 { Lra_t lrb;
            			 Tt_reg_t tt_r;
            		   Tt_reg tt_reg = (Tt_reg)ix_fetchrec(mainupn, IX_TT, &kv_t, &lrb, &tt_r);
            			 rd_unlock(mainupn);
            			 g_cust_TT_id[0] = 0;
            			 if (tt_reg != null)
            			   strpcpy(g_cust_TT_id, tt_reg->tt_id, sizeof(g_cust_TT_id));
            		 }}}
                otherwise	   res = "Not Editable";
  }
  if (res == null)
    put_details_order();
  return res;
}

private void convert_ord_to_dn(upn, sale_ref)
	 Id    upn;
	 Sale  sale_ref;
{ Key_t kv_t;  kv_t.integer = sale_ref->id;
  wr_lock(upn);
{ Lra_t lra;
  Sale sa = (Sale)ix_fetchrec(upn, IX_SALE, &kv_t, &lra, null);
  if (sa != null)
  { sa->kind &= ~K_ORD;
    sa->kind |= K_IP;
    (void)write_rec(upn, CL_SALE, lra, sa);
  }
  sale_ref->kind &= ~K_ORD;
  sale_ref->kind |= K_IP;
  wr_unlock(upn);
}}

					/* order TO supplier */

public const Char * do_order(upn)
	 Id	  upn;
{ Lra_t  lrau;
  Unique un = find_uniques(upn, &lrau);
  Tabled stbl1 = tbl1;
  Tabled tbl2 = mk_form(SPform,g_rows - 11);
  if (tbl2 == null)
    return iner;

  dflt_kind = K_SALE + K_GIN;
  g_ordering = true;
 
  init_sale(&inv);

  tbl1 = tbl2;
  wopen(tbl2,SPover);
  put_details_order();

  clr_alert();
{ Field fld = goto_fld(W_INO);
  Int ix = inv.curr_base-1;
  Cc contcc = SUCCESS;
  const Char * error = null;

  while (contcc == SUCCESS)
  { while (ix - inv.curr_base < SL-1 and ix < inv.curr_last)
    { ++ix;
      put_oitem(ix - inv.curr_base, &inv.c[ix], &inv.o[ix]);
    }

    if (error != null)
    { if (error[0] != '$')
        alert(error);
      else
        salert(&error[1]);
    }
    error = "HOME to quit";
    redisplay = false;

  { Cc cc = get_any_data(&ds[0], &fld);
    Char *d = skipspaces(&ds[0]);
    vfy_lock(upn);

    if      (cc == PAGE_UP or cc == PAGE_DOWN)
    { Short base = inv.curr_base + (cc == PAGE_DOWN ? SL - 1 : 1 - SL);
      if (base + SL - 1 > inv.curr_last)
      	base = inv.curr_last - SL + 2;
      if (base < 0)
      	base = 0;
      if (inv.curr_base != base)
      { inv.curr_base = base;
      	ix = base - 1;
      	wclrgrp();
      }
      error = null;
    }
    else if (cc == HALTED)
      if (fld->props & FLD_FST)
      { if (inv.curr_base > 0)
        { wclrgrp();
          ix = --inv.curr_base;
        } 
      }
      else
      { wscroll(1);
      	inv.curr_base += 1;
      	error = null;
      }
    else if (fld->id == W_CMD)
    { Int newix = inv.curr_base-1;
      Cc cm = sel_cmd(d);
      if      (cm == CMD_DOT)
      { if (inv.modified)
      	  error = dsfst;
      	else      
      	  contcc = HALTED;
      }
      else if (cm == CMD_ORDER or cm == CMD_SAVE or cm == CMD_ACCEPT)
      { Set props = cm == CMD_ORDER  ? K_ORD	  :
            		    cm == CMD_ACCEPT ? K_DED+K_IP : K_IP;
      	error = (props & K_DED)&&(this_sale.kind & K_ORD)? "Unorder it first":
			      	            (this_sale.kind & K_DED) ? "Already accepted":
                                                     save_sale(&inv, true, props);
      	put_details_order();
      	if (error == null)
      	{ if (g_tt_num_ui > 0)
          { save_tt_inv();
            g_tt_num_ui = 0;
          }
      	  if (props != K_IP)
      	    error = do_prt_order(upn, &inv, props);
      	  else
      	  { init_sale(&inv);
      	    wreopen(tbl2);
      	  }
      	  goto_fld(W_INO);
      	}
      	newix = ix;
      }
      else if (cm == CMD_PRINT)
      	error = do_prt_order(upn, &inv);
      else if (cm == CMD_UNORDER)
      { if (not (this_sale.kind & K_ORD))
      	  error = "Not yet ordered";
      	else
      	{ wr_lock(upn);
      	  convert_ord_to_dn(upn, &this_sale);
      	  deduct_stock(&inv, 0, -1);
      	  wr_unlock(upn);
      	  error = "Order cancelled, Any Delivery Note must now be ACCEPTed";
      	  put_details_order();
      	}
      	goto_fld(W_CMD);
      }
      else if (cm == CMD_DROP)
      { if (inv.modified and this_sale.id != 0)
      	  (void)sale_lock(false, this_sale.id);
      	init_sale(&inv); error = null;
      	goto_fld(W_CMD);
      }
      else if (cm == CMD_STOCK)
      { do_stock();
      	wreopen(tbl2);
      	goto_fld(W_ADD);
      	error = null;
      }
      else if (cm == CMD_FIND or cm == CMD_REDO or cm == CMD_COPY)
      { error = cm == CMD_FIND ? find_i_item(&inv, d)  :
    		cm == CMD_REDO ? redo_order(upn, &inv) :
		                		 copy_order(&inv);
      	if (error == null)
      	{ Int pos = 0;
      	  if (inv.curr_base >= 2)
      	  { pos = 2; inv.curr_base -= pos; }
      	  wclrgrp();
      	  goto_fld(W_KEY + pos);
      	  newix = inv.curr_base - 1;
      	}
      }
      else
      { error = *d == 0 ? null :
		        "Order,Unorder,Accept,Save,Copy,Redo,Drop,Stock,Find,Print";
      	newix = ix;
      }
      ix = newix;
    }
    else if (cc == SUCCESS)
    { if      (in_range(fld->id, W_INO, W_STP))
      { error = do_edit_order(upn, fld, &inv, ds);
      	if (fld->id == W_INO and error == null)
      	{ ix = inv.curr_base - 1;
      	  goto_fld(W_ADD);
      	}
      }
      else if (fld->id == W_ADD)
      { if      (this_sale.kind & K_DED)
      	  error = rdoy;
        else if (*d == 0 and inv.curr_last - inv.curr_base > SL - 1)
      	{ redisplay = true;
      	  wclrgrp();
      	  error = null;
      	  inv.curr_base += SL - 1;
      	  if (inv.curr_base >= inv.curr_last)
      	    inv.curr_base = inv.curr_last - 1;
      	}
      	else
      	{ if (this_sale.terms - this_sale.tax_date <= 0)
      	    error = "Invalid dates";
      	  else
	          error = d[0] == '?' ? do_q_stock(upn, &inv, &d[1])
                        				: do_add_item(upn, &inv, d);
      	  if (error != null)
      	  { alert(error);  error = null;
      	    while (hold() != A_CR)
      	      ttbeep();
      	    clr_alert();
      	  }
      	  else
      	  { Int scroll_dist = inv.curr_last - inv.curr_base - SL + 1;
      	    if (scroll_dist > 0)
      	    { if (scroll_dist < 3)
            		scroll_dist = 3;
      	      wscroll(scroll_dist);
      	      inv.curr_base += scroll_dist;
      	    }	
      	    ix = ix >= inv.curr_last-1 ? inv.curr_last-1 : inv.curr_base-1;
      	  }
      	}
      	if (redisplay)
      	  ix = inv.curr_base - 1;
      }
      else if (in_range(fld->id, W_KEY, W_MAR+SL))
      { if      (this_sale.kind & K_DED)
      	  error = rdoy;
      	else
      	{ Int ix_ = inv.curr_base + (fld->id & (ID_FAC - 1));
      	  if (ix_ <= inv.curr_last)
      	  { error = do_edit_oitem(upn, fld, &inv, ix_, ds);
      	    put_oitem(ix_ - inv.curr_base, &inv.c[ix_], &inv.o[ix_]);
      	  }
      	}
      }
    }
  }}
  end_interpret();
  freem(tbl2);
  tbl1 = stbl1;
  g_ordering = false;
  fini_sale(&inv);
  return null;
}}


					/* List items of sale */

static Cc do_inv_items(upn,supps,nsupps,stt_,fin_,seq)
	 Id	  upn;
	 Id       supps[];
	 Int      nsupps;
   Date     stt_;
   Date     fin_;
   Id       seq;
{ Date stt = stt_ - 1;
  Lra_t  lra;
  Key_t kv_t; kv_t.integer = 0;
{ 
  Sale_t sale_r;
#if 1
  Cc cc;
  Int lo;
  Byte * strm;
  kv_t.integer = 0;
  strm = b_new_stream(upn, IX_SALE, &kv_t);
  while ((cc = next_of_ix (strm, &kv_t, &lra)) == OK)
  {
    Byte * s = read_rec(upn, CL_SALE, lra, &sale_r);
    if (sale_r.date >= stt_) break;
  }
  lo = cc != OK ? 0 : sale_r.id;
#else
  Cc cc = b_last(upn, IX_SALE, &kv_t, &lra);
  Int lo = 0;
  Int hi = kv_t.integer;
  Int back = 1;

  sale_r.date = 0;

  while (hi - lo > 1)
  { Int mid = (lo+hi) / 2 - back + 1;
    kv_t.integer = mid;
    Byte * strm = b_new_stream(upn, IX_SALE, &kv_t);
  { Cc cc = next_of_ix (strm, &kv_t, &lra);
    Byte * s = read_rec(upn, CL_SALE, lra, &sale_r);
    if (s == NULL)
    { i_log(mid, "Lost sale %d", kv_t.integer);
      sale_r.id = mid;
      sale_r.date = stt;
    }

    ix_fini_stream(strm);

    if      (sale_r.date == stt)
    { lo = mid;
      hi = mid;
    }
    else if (sale_r.date > stt)
    { if (hi != sale_r.id - 1)
        back = 1;
      else
        back = back * 2;
      
      hi = sale_r.id - 1;
    }
    else
    { lo = sale_r.id + 1;
      back = 1;
    }
  }}
#endif

  kv_t.integer = lo;
{ Char seqfn[80];
  sprintf(seqfn,"PJSSale%d",seq);
{ FILE * op = fopen(seqfn, "w");
  Byte * strm = b_new_stream(upn, IX_SALE, &kv_t);
  Cc cc;
  while ((cc = next_of_ix (strm, &kv_t, &lra)) == 0 && op != NULL)
  { Char datestr[13];
    Byte * s = read_rec(upn, CL_SALE, lra, &sale_r);
    if (s == NULL) break;
    if (sale_r.date < stt_) continue;
    if (sale_r.date > fin_) break;

//  fprintf(op,"Id %d Date %d Total %d\n", sale_r.id,sale_r.date,sale_r.total);
  { Char * ds = date_to_asc(datestr,sale_r.date,2);
    const Char * err = ld_sale(sale_r.id);
    if (err != NULL) continue;
    
  { Postaddr address;
    Char * t;
    Char * addrp[10];
    int ix,six;
    memcpy(address,this_custmr.postaddr,LPADDR+1);
    address[LPADDR] = 0;
    memset(addrp,0,16);
    t = address;
    addrp[0] = t;
    for (ix = 0; ++ix < 4 && (t = strchr(t,',')) != NULL; )
    { *t = 0;
      t = t + 1;
      addrp[ix] = t;
    }
   
    for ( ix = -1; ++ix <= inv.curr_last; )
    { Char linestr[30];
      Invoitem  it = &inv.c[ix];
      for (six = nsupps; --six >= 0 && it->sup != supps[six]; )
        ;
      if (six < 0) continue;

      bc_to_asc(supps[six], it->line, &linestr[0]);
      
      fprintf(op,"SUGRO,%d,,%s,%d,%s,%s,%s,%s,%s,,,\n",
              sale_r.customer,linestr, it->qty, ds,
              addrp[0],addrp[1],addrp[2],this_custmr.postcode);
    }
  }}}

  if (op != NULL)
    fclose(op);

  return 0;
}}}}




const Char * do_report_sales(char * filenm, int from_date, int days)

{ FILE * ip = fopen(filenm,"r");
  if (ip == NULL)
    return "Cannot find report";
  else
	{	const int MAX_SUP = 100;
    Id supps[MAX_SUP+1];
    Int six = 0;
    Id seq = 0;
    Id supp_id = 1;
    Int end_date = 1;

		char buff[133];    
    Char * ln;
    while ((ln = fgets(buff,133,ip)) != 0)
    { supp_id = atol(ln);
      if (supp_id == 0) break;
      if (six >= MAX_SUP) continue;
      supps[++six] = supp_id;
    }
    supps[six+1] = 0;
    if (six == 0) ln = NULL;
    
    if (ln != NULL)
    { 
      if (from_date != 0)
      { end_date = from_date;
      }
      else
        while ((ln = fgets(buff,133,ip)) != 0)
        { Int sl = strlen(buff)-1;
          if (sl < 2) continue;
          end_date = asc_to_date(ln);
          while (--sl >= 0 && in_range(buff[sl],'0','9'))
            ;
          seq = atol(buff+sl);
        }
      fclose(ip);

    { Date new_date = asc_to_date("") - 1;
      if (new_date <= end_date)
        return "Nothing to report";

      if (days != 0)
        new_date = end_date + days;

    { Cc cc = do_inv_items(mainupn,supps,six,end_date+1,new_date,seq+1);
      if (cc == OK && from_date == 0)
      { FILE * op = fopen(filenm,"a+");
        if (op != NULL)
        { Char * b = date_to_asc(buff,new_date,0);
          fprintf(op,"%s %d\n", b,seq+1);
          fclose(op);
        }
      }
    }}}
  }

  return NULL;
}

Char * make_dummy_sales(int ct)

{ Lra_t lra;
  Cc cc = OK;
  this_sale.id = 0;
  this_sale.kind = K_IP;
  this_sale.customer = 23;
  this_sale.match_grp = 1;
  this_sale.cat = 0;
  this_sale.agent = 2;
  this_sale.date = asc_to_date("");
  this_sale.total = 123;
  this_sale.alt_terms = 0;
  this_sale.balance = 99;
  this_sale.terms = 0;
  this_sale.tax_date = this_sale.date;
  this_sale.vat0_1pc = 0;
  this_sale.disct = 0;
  this_sale.chq_no = 0;
  this_sale.vattotal = 0;
  this_sale.id = last_id(mainupn, IX_SALE) + 1;

  while (--ct >= 0)
  { 
//  this_sale.id = last_id(mainupn, IX_SALE) + 1;
    this_sale.id += 1;
    if (ct == 499)
      gdb(0);
    wr_lock(mainupn);
//  cc = i_Sale_t(mainupn, &this_sale, &lra);
    cc = new_init_rec(mainupn, CL_SALE, (char*)&this_sale, &lra);
    lra = 0;
  { Key_t kv_t;  kv_t.integer = 81746;
  { Taking bkt = (Taking)ix_fetchrec(mainupn, IX_TAKE, &kv_t, &lra, &this_take);
    wr_unlock(mainupn);
      
    if (cc != OK)
      break;
    if (bkt == null)
      break;
      
    { time_t tm; 
      int trash = time(&tm);
      char * ctstr = ctime(&tm);
    }  
  }}}
  if (ct >= 0)
    i_log(cc,"Dummy sale failed");

  return cc == OK ? NULL : "failed";
}
