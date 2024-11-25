#include <stdio.h>
#include <string.h>
#include   <ctype.h>
#include "version.h"
#include "../h/defs.h"
#include "../h/errs.h"
#include "../h/prepargs.h"
#include "../form/h/form.h"
#include "../form/h/verify.h"
#include "../db/cache.h"
#include "../db/recs.h"
#include "../db/b_tree.h"
#include "domains.h"
#include "schema.h"
#include "rights.h"
#include "generic.h"

#define W_KEY  (4 * ID_FAC)
#define W_Z    (5 * ID_FAC)
#define W_CASE (6 * ID_FAC)
#define W_PROP (7 * ID_FAC)
#define W_PACK (8 * ID_FAC)
#define W_TKT  (10 * ID_FAC)
#define W_DSCN (11 * ID_FAC)
#define W_PRC  (12 * ID_FAC)
#define W_SUP  (13 * ID_FAC)
#define W_STK  (14 * ID_FAC)
#define W_OTW  (15 * ID_FAC)
#define W_AVG  (16 * ID_FAC)
#define W_GTH  (17 * ID_FAC)
#define W_AGE  (18 * ID_FAC)
#define W_RED  (19 * ID_FAC)
#define W_WHCH (10 * ID_FAC)
#define W_REF  (21 * ID_FAC)

#define W_ADD  (22 * ID_FAC)
#define W_CST  (23 * ID_FAC)

#define SL (SCRN_LINES-7)
#define LUI 42

static int stk_sl;


static const Over_t SI1over [] =
 {{0,1,1,0,">"},
  {1,4,1,0,
" Code Props   Pack Ticket Description                 Price Source Stock Days "},
  {-1,0,0,0,"Stock"}
 };

static Fieldrep_t SIform [] =               // Update Form
 {{W_MSG+1,FLD_RD+FLD_MSG,0,20,55,  1, ""},
  {W_CMD, FLD_WR,        1, 1, 28,  1, comd},
  {W_MSG, FLD_RD+FLD_FD, 1,30, 40,  1, ""},

  {W_KEY, FLD_WR+FLD_BOL, 5, 0,  5,  -SL, pcde},
  {W_Z,   FLD_WR,	  5, 5,  1,  -SL, zrtd},
  {W_PROP,FLD_WR,	  5, 6,  7,  -SL, prty},
  {W_PACK,FLD_WR,	  5,14,  5,  -SL, "Pack or #*#"},
  {W_TKT, FLD_WR,	  5,20,  4,  -SL, retl},
  {W_DSCN,FLD_WR,	  5,25,LDSCN,-SL, dcrn},
  {W_PRC, FLD_WR,	  5,55,  7,  -SL, pric},
  {W_SUP, FLD_WR,	  5,63,  8,  -SL, splr},
  {W_STK, FLD_WR,	  5,72,  4,  -SL, stck},
  {W_AGE, FLD_WR+FLD_EOL,5,77,3,-SL, danl},
  {W_BAN, FLD_RD,	 -5,0,  80, 1,""},
  {0,0,0,0,0,1,null}
 };

static const Over_t SA1over [] =
 {{0,1,1,0,">"},
  {1,4,1,0,
" Bar-Code      Z Case Props   Pack Ticket Description                     Price "},
  {-1,0,0,0,"Stock"}
 };

static const Fieldrep_t SAform [] =         // Add Form
 {{W_MSG+1,FLD_RD+FLD_MSG,0,20,55,  1, ""},
  {W_CMD, FLD_WR,        1, 1, 28,  1, comd},
  {W_MSG, FLD_RD+FLD_FD, 1,30, 40,  1, ""},

  {W_KEY, FLD_WR+FLD_BOL,5,0,LSTKKEY,-SL, pcde},
  {W_Z,   FLD_WR,	 5,16,  1,   -SL, zrtd},
  {W_CASE,FLD_WR,  5,18,  2,   -SL, "Case#?"},
  {W_PROP,FLD_WR,	 5,23,  8,   -SL, prty},
  {W_PACK,FLD_WR,	 5,32,  3,   -SL, pck},
  {W_TKT, FLD_WR,	 5,36,  4,   -SL, retl},
  {W_DSCN,FLD_WR,	 5,41,LDSCN, -SL, dcrn},
  {W_PRC, FLD_WR,	 5,72,  7,   -SL, pric},
  {W_BAN, FLD_RD,	-5,0,  78, 1,""},
  {0,0,0,0,0,1,null}
 };

static const Over_t SR1over [] =
 {{0,1,1,0,">"},
  {1,5,1,0,   "   Which           Bargain  "},
  {-1,0,0,0,"Bargains"}
 };

static const Fieldrep_t SRform [] =
 {{W_MSG+1,FLD_RD+FLD_MSG,0,20,55,  1, ""},
  {W_CMD, FLD_WR,        1, 1, 20,  1, comd},
  {W_MSG, FLD_RD+FLD_FD, 1,30, 40,  1, ""},

  {W_KEY, FLD_WR+FLD_BOL,3, 0,  5,   1, pcde},
  {W_Z,   FLD_WR,	 3, 6,  1,   1, zrtd},
  {W_PROP,FLD_WR,	 3, 8,  7,   1, prty},
  {W_PACK,FLD_WR,	 3,16,  3,   1, pck},
  {W_TKT, FLD_WR,	 3,20,  4,   1, retl},
  {W_DSCN,FLD_WR,	 3,25,LDSCN, 1, dcrn},
  {W_PRC, FLD_WR,	 3,55,  7,   1, pric},
  {W_SUP, FLD_WR,	 3,62,  8,   1, splr},
  {W_STK, FLD_WR,	 3,71,  4,   1, stck},
  {W_AGE, FLD_WR+FLD_EOL,3,76,  3,   1, danl},
  {W_REF, FLD_WR,        4,48,  6,   1, "SupRef"},
  {W_CST, FLD_WR,	 5,48, 31,   1, "Constraint"},

  {W_WHCH,FLD_WR+FLD_BOL,6, 2, 10, -SL, "groups"},
  {W_RED, FLD_WR+FLD_EOL,6,15,  9, -SL, "bargain"},
  {W_BAN, FLD_RD,	-6,0,  72, 1,""},
  {0,0,0,0,0,1,null}
 };

static const Over_t SC1over [] =
 {{0,1,1,0,">"},
  {0,2,1,7,"Statistics up until"},
  {1,4,1,0,
" Source  Code  Props Case Description                 Price Stk OTW Avge Inc Age"},
  {-1,0,0,0,"Stock Analysis"}
 };

static const Fieldrep_t SCform [] = 		/* Analysis Form */
 {{W_MSG+1,FLD_RD+FLD_MSG,0,20,55,  1, ""},
  {W_CMD, FLD_WR,        1, 1, 28,  1, comd},
  {W_MSG, FLD_RD+FLD_FD, 1,30, 40,  1, ""},
  {W_WHCH,FLD_RD,        2,28, LDATE,1, ""},

  {W_SUP, FLD_WR+FLD_BOL,5, 0,  8,   -SL, splr},
  {W_KEY, FLD_WR,	 5, 9,  5,   -SL, pcde},
  {W_PROP,FLD_WR,	 5,15,  4,   -SL, prty},
  {W_PACK,FLD_WR,	 5,19,  3,   -SL, "case"},
  {W_DSCN,FLD_WR,	 5,23,LDSCN-2,-SL,dcrn},
  {W_PRC, FLD_WR,	 5,50,  7,   -SL, pric},
  {W_STK, FLD_WR,	 5,58,  4,   -SL, "Stock or @"},
  {W_OTW, FLD_WR,	 5,63,  4,   -SL, "on the way"},
  {W_AVG, FLD_WR,	 5,68,  4,   -SL, "sales /month"},
  {W_GTH, FLD_WR,	 5,73,  3,   -SL, "growth /month"},
  {W_AGE, FLD_WR+FLD_EOL,5,77,  3,   -SL, danl},
  {W_BAN, FLD_RD,	-5,0,  72, 1,""},
  {0,0,0,0,0,1,null}
 };

static const Over_t SS1over [] =
 {{0,1,1,0,">"},
  {1,4,1,0,
" >,<  Z Props   Pack Ticket Description                 Price Source Stock Days "},
  {-1,0,0,0,"Stock Search"}
 };

static const Over_t SI2over [] =
 {{0,1,1,0,">"},
  {1,3,1,0,    "  STOCK                                    "},
  {1,13+5,1,0, "------- Select service --------------------"},
  {-1,0,0,0,null}
 };

static const Char * seltext[] =
 {" A           -- Add",
  " U name      -- Update",
  " B name      -- Browse",
  " K name name -- List (brief)",
  " L name name -- List",
  " D name name -- List Offers",
  " S           -- Search",
  " R           -- Report",
  " P name name -- Pricelist",
  " Y name      -- Stock Analysis",
  " I name      -- Import Sugro APL",
  null
 };

extern Tabled seltbl;
static Tabled tbl1;

const Char pian [] = "Pack is # or #*#";

static void init_sif()

{
}



static void scroll_stk(keytbl, n)
	Char    keytbl[SL+1][sizeof(Stkkey)+1];
	Int     n;
{ Char * s = &keytbl[n][0];

  memcpy(&keytbl[0][0], s, &keytbl[SL+1][0] - s);
  for (s = &keytbl[SL - n][0]; s < &keytbl[SL][0]; s += sizeof(Stkkey)+1)
    s[0] = 0;
}


static void scroll_bstk(suptbl, linetbl, n)
	Int     suptbl[SL+1], linetbl[SL+1];
	Int     n;
{ fast Int * s = &suptbl[n];
  memcpy(&suptbl[0], s, (Char*)&suptbl[SL+1] - (Char*)s);
  s = &linetbl[n];
  memcpy(&linetbl[0], s, (Char*)&linetbl[SL+1] - (Char*)s);
}


static void scroll_sr(keytbl, n)
	Int     keytbl[SL+1];
	Int     n;
{ fast Int * s = &keytbl[n];
  memcpy(&keytbl[0], s, (Int)&keytbl[SL+1] - (Int)s);

  for (s = &keytbl[SL - n]; s <= &keytbl[SL -1]; ++s)
    *s = 0;
}

Int    an_n;				/* age of analysis */
Double an_a, an_b;			/* per day rate and increase */
Double an_rate;				/* rate of sale at uniques.lst_anal */

#define anal_day 5
#define s_period 28.0
#define per_period(f) ((int)(f * s_period + 0.5))


/* static */ void analyse(si)                /* uniques.lst_anal is read */
	 Stockitem  si;
{ an_n = 0;
  an_a = 0.0;
  an_b = 0.0;
  an_rate = 0.0;

  if (not (si->props & P_SUBS) and si->sell_stt != 0)
  { Int n = uniques.lst_anal - si->sell_stt;
    if (n > 0)
    { an_b =  n < 2 ? 0.0 : dbl(12 * si->sig_vt - 6 * (n-1) * si->sig_v) /
	                                                 dbl((n-1) * n * (n+1));
      an_a = dbl(si->sig_v) / dbl(n)  -  an_b * dbl(n-1) / 2.0;
      an_rate = an_a + an_b * dbl(n);
      an_n = n;
    }
  }
}


FILE * stkrepfile;

#define best_hist 200
#define topple   1000
#define intprec 10000
 /* topple value is topple / intprec
	         for an interval of a week,
	             a history of 10 weeks
	             this is a 30% deviation from the weekly norm
 */
#define moderator 0.008
	                  /* OOS if 2 days sale are in stock */
#define days_waste 2

static int last_suplr;


/* static */ void add_stkstats(si)
	Stockitem si;
{ analyse(si);
{ Char * tag = " ";
  Int tw_mid_r = (uniques.lst_anal + today) - 2 * si->sell_stt;
  Double midval = an_a + an_b * dbl(tw_mid_r) / 2.0;
  Date ivl = today - uniques.lst_anal;
  Int predict = rnd(midval * ivl);
  Int dev = si->week_sale - predict;
  Int cumdev   = (dev > 0) != (si->cumdev > 0) ? 0 : si->cumdev + dev;
  Int an_n_ = an_n;
  Int act_hist = an_n == 0 ? 0 : today - si->sell_stt;
  Int weight  = act_hist == 0         ? 1         :
	        act_hist <= best_hist ? si->sig_v :
	                               (si->sig_v * best_hist) / act_hist;
  Set oos = act_hist > 14 and  si->stock	    < an_rate * days_waste
  		  	  and (si->week_sale+1) * 3 < predict	    ? P_OOS : 0;
  if (ivl == 0)		/* a kludge */
    ivl = 1;
#if 0
  myprintf("MV %f IVL %d W %d ", midval, ivl, weight);
  myprintf("PDICT %d TMR %d VT %d",
			predict, tw_mid_r, (si->week_sale * tw_mid_r)>>1);
#endif
  si->props = (si->props & ~P_OOS) + oos;

  if      (oos != 0)
  {
#if SA
    i_log(predict, "OOS");
#endif
    si->week_sale = predict;
  }
  else if (weight <= 0)
  {
#if 0
    myprintf("\nWeight %d\n", weight);
#endif
    si->sell_stt = today - ivl;
    si->sig_v /= act_hist / ivl;
    si->sig_vt = (today - si->sell_stt) * si->sig_v;
  }
  else if (an_n > 0)
  { Bool adjust = false;
    Int abscumdev = cumdev > 0 ? cumdev : -cumdev;

    if (act_hist > 8 * ivl and
        abscumdev * intprec > weight * topple and
        predict > 2)
    { Int n = act_hist * (1000 - 2000 * abscumdev / weight) / 1000;
      if (n != act_hist)
      { if (n < 2)
          n = 2;
        adjust = an_n != n;
      	an_n = n;
      }
    }
    if (adjust)
    { Int n = an_n;
      Int old_a = an_a;
#if SA
      i_log(weight, "Adjusting");
#endif
      an_a += (an_n_ - an_n) * an_b;
      /*myprintf("\nNew Average %f\n", an_a);*/
/*    an_a *= (1 + cumdev / si->sig_v); */
      an_b = an_b / 2;

    { Double sv = dbl(n) * (an_a + dbl(n-1) * an_b / 2.0);
      si->sig_v  = rnd(sv);
      si->sig_vt = (dbl(n*(n+1))*an_b + 6*si->sig_v) * (n-1) / 12;

      cumdev = cumdev * n / (uniques.lst_anal - si->sell_stt);
      si->sell_stt = uniques.lst_anal - n;
      tw_mid_r = (uniques.lst_anal + today) - 2 * si->sell_stt;
      analyse(si);
      tag = not in_range(old_a - an_a, -2, 2) ? "D" : "S";
    }}
    si->cumdev =  cumdev == 0 ? dev / 3 : cumdev;
  }
  si->sig_v  += si->week_sale;
  si->sig_vt += (si->week_sale * tw_mid_r) >> 1;
  if (si->sig_vt < 0)
    si->sig_vt = (today - si->sell_stt) * si->sig_v;
    
  if (stkrepfile != null)
  { Char buf[22];
    Char * ls = &buf[0];
    Char * fmt = "%s%13s %4d %5d %6d %9d\n";
    
    bc_to_asc(si->supplier, si->line, &buf[0]);
    if (last_suplr == si->supplier)
    { ls = &buf[7];
      fmt = "%s\t%6s %4d %5d %6d %9d\n";
    }
    fprintf(stkrepfile, fmt, tag, ls,
  			       si->stock, si->week_sale, si->sig_v, si->sig_vt);
    last_suplr = si->supplier;
  }
  si->week_sale = 0;
}}

public void do_stkstats(upn, force)
	 Id    upn;
	 Bool force;
{ wr_lock(upn);
{ Lra_t  lrau;
  Unique un = find_uniques(upn, &lrau);
  Date last_ad = uniques.lst_anal;

  if (last_ad < today or force)
  { Char * strm = init_rec_strm(upn, CL_SI);
    Lra_t  lra;
    Stockitem_t si;
    Int wrct = 0;

    uniques.lst_anal = today;
    if (not upn_ro)
      (void)write_rec(upn, CL_UNIQ, lrau, &uniques);

    sprintf(&si.descn[0], "stock/%d", last_file("stock")+1);
    stkrepfile = fopen(si.descn, "w");
    if (stkrepfile != null)
    { fprintf(stkrepfile,
	   "%s period = %d days\n  Product Code  Stock  WeekSale Sigv  Sigvt\n",
	   todaystr, today - last_ad);

      while (next_rec(strm, &lra, &si) != null)
      { if (not (si.props & (P_OBS+P_SUBS)))
        { /*printf("(%d)", si.id);*/printf(".");
         /*fprintf(stkrepfile, "(%d)", si.id);*/
      	  add_stkstats(&si);
          if (not upn_ro)
	          (void)write_rec(upn, CL_SI, lra, (Char*)&si);
        }
        if ((++wrct & 0x7f) == 0)
        { /*printf("\n");*/
          fprintf(stkrepfile, "\n");
          fflush(stkrepfile);
          wr_lock_window(upn);
        }
      }
      fclose(stkrepfile);
    }
    stkrepfile = null;
    fini_stream(strm);
  }
  wr_unlock(upn);
}}

public const Char * do_reset_stock(wh)
	 Quot   wh;
{ /*if (this_agent.id != 1)*/
    return ymba;
  { sprintf(&ds[0], "Really Reset %s Y/N", wh == 0 ? "all sales statistics"
						   : "stocks");
    alert(ds);
    if ((hold() & 0xdf) != 'Y')
      return aban;
  }
  ipfld();
{ Lra_t  lra;
  Unique un = find_uniques(mainupn, &lra);
  Date last_ad = uniques.lst_anal;
  Char * strm = init_rec_strm(mainupn, CL_SI);
  Stockitem_t si;

  wr_lock(mainupn);
  while (next_rec(strm, &lra, &si) != null)
  { // do_db(-1,-1);
    if      (wh == 2)
    { si.props &= ~P_SUBS;
      si.stk_alias = 0;
    }
    else if (wh == 1)
    { si.stock = 0;
      si.order = 0;
    }
    else
    { si.cumdev   = 0;
      si.sig_v    = 0;
      if ((si.props & P_SUBS) == 0)
        si.sig_vt   = 0;
      si.sell_stt = last_ad;
    }
    (void)write_rec(mainupn, CL_SI, lra, &si);
/*  wr_lock_window(mainupn);  */
  }
  wr_unlock(mainupn);

  fini_stream(strm);
  return &ndon[4];
}}

static void put_si(si_)
	Stockitem si_;
{ fast Stockitem si = si_;
       Int spos = save_pos();
       Field fld = goto_fld(ID_MSK);
       Char linestr[35];  bc_to_asc(0, si->line, &linestr[0]);

  if (not (fld->props & FLD_BOL))
    left_fld(FLD_BOL, &fld);
  put_data( linestr);
  linestr[0] = 0;
  if (si->props & 0xffffff00)
    rgt_to_asc((si->props >> 8) & -2, &linestr[0]);
  if (si->props & P_AT)
    strcat(&linestr[0], "@");
{ char packstr[10];
  if (s_case(si) <= 1)
    sprintf(packstr,"%d", si->pack);
  else
    sprintf(packstr,"%d*%d",s_case(si), si->pack);

  form_put(T_DAT+W_Z+ID_MSK,    si->props & P_VAT ? "" : ".",
           T_DAT+W_PROP+ID_MSK, linestr,
           T_DAT+W_PACK+ID_MSK, packstr,
           T_INT+W_TKT+ID_MSK,  si->ticket,
           T_DAT+W_DSCN+ID_MSK, si->descn,
           T_CSH+W_PRC+ID_MSK,  si->price, 0);
  
  if (goto_fld(W_SUP+ID_MSK) != null)
  {			    put_integer(si->supplier);
    form_put(T_INT+W_STK+ID_MSK, si->stock,
             T_DAT+W_AGE+ID_MSK, "", 0);
    if (si->sale_date != 0 and today-si->sale_date > 7)
      put_integer(today-si->sale_date);
  }

  restore_pos( spos);
}}



static void put_red( wh, pr)
	Set      wh;
	Cash     pr;
{ Char str[50];
  Int spos = save_pos();
  Field fld = goto_fld(ID_MSK);

  if (not (fld->props & FLD_BOL))
    left_fld(FLD_BOL, &fld);
  rgt_to_asc(wh, str);     put_data( str);
  form_put(T_CSH+W_RED+ID_MSK, pr, 0);
  restore_pos( spos);
}

static void put_sa(si_)
	Stockitem si_;
{ fast Stockitem si = si_;
       Int spos = save_pos();
       Field fld = goto_fld(ID_MSK);
       Char linestr[22];    bc_to_asc(0, si->line, &linestr[0]);

  if (not (fld->props & FLD_BOL))
    left_fld(FLD_BOL, &fld);
  form_put(T_OINT+W_SUP+ID_MSK, si->supplier,
           T_INT+W_TKT+ID_MSK,  si->ticket,
           T_DAT+W_KEY+ID_MSK,  linestr, 0);
  linestr[0] = 0;
  if (si->props & 0xffffff00)
    rgt_to_asc(si->props >> 8, &linestr[0]);
  form_put(T_DAT+W_PROP+ID_MSK, linestr,
           T_INT+W_PACK+ID_MSK, s_case(si),
           T_DAT+W_DSCN+ID_MSK, si->descn,
           T_CSH+W_PRC+ID_MSK,  si->price, 0);
  if (si->props & P_SUBS)
  { goto_fld(W_STK+ID_MSK);
  { Key_t kvi; kvi.integer = si->stk_alias;
    rd_lock(mainupn);
  { Lra_t lra;
    Stockitem sii = (Stockitem)ix_srchrec(mainupn, IX_STK, &kvi, &lra);
    rd_unlock(mainupn);
    if (sii != null)
    { 			   put_integer(sii->stock);
      bc_to_asc(sii->supplier, sii->line, &linestr[0]);
      goto_fld(W_OTW+ID_MSK); wclreol(); put_free_data(linestr);
    }
  }}}
  else
  { analyse(si);
    form_put(T_INT+W_STK+ID_MSK, si->stock,
             T_INT+W_OTW+ID_MSK, si->order,
             T_INT+W_AVG+ID_MSK, per_period(an_rate),
             T_INT+W_GTH+ID_MSK, per_period(an_b*s_period),
             T_INT+W_AGE+ID_MSK, an_n, 0);
  }

  restore_pos( spos);
}

static void rename_offers(upn, oid, nid)
	  Id     upn;               /* write locked */
	  Id     oid, nid;
{ Key_t okvi;  okvi.integer = oid;
{ Key_t nkvi;  nkvi.integer = nid;
{ Lra_t  lra;
  Cc cc;
  while ((cc = ix_search(upn, IX_SR, &okvi, &lra)) == SUCCESS)
  { // do_db(-1,-1);
    Stockred_t sr_t;
    Stockred  sr = (Stockred)read_rec(upn, CL_SR, lra, &sr_t);
    if (sr != null)
    { sr->id = nid;
      (void)write_rec(upn, CL_SR, lra, sr);
    }
    cc = ix_delete(upn, IX_SR, &okvi, &lra);
    if (cc != SUCCESS)
      i_log(cc, iner);
    else
      ix_insert(upn, IX_SR, &nkvi, &lra);
  }
}}}

static void rename_aliases(upn, lra, nid)
	 Id      upn;        /* Locks on */
 	 Lra_t   lra;		/* the entry to come out */
 	 Id      nid;		/* the entry to go in */
{ Stockitem_t s_t;
  Stockitem  si = (Stockitem)read_rec(upn, CL_SI, lra, &s_t);
  if (si != null and si->stk_alias != 0)
  { Id  sid = 0;			/* the sibling of the new master */
    Id	oid = si->id;
    Set is_subs = si->props & P_SUBS;
    Key_t kvi; kvi.integer = si->stk_alias;
  { Lra_t lra_;
    Stockitem partner = (Stockitem)ix_fetchrec(upn, IX_STK, &kvi, &lra_, &s_t);
    if (partner != null)
      if (is_subs == 0)
      { partner->props &= ~P_SUBS;
        if (partner->sig_vt = partner->id)
          partner->stk_alias = 0;
        write_rec(upn, CL_SI, lra_, partner);
        sid = partner->sig_vt;
        lra = lra_;
      }
      else
      { if (partner->stk_alias == oid)
        { partner->stk_alias = nid == oid ? 0 : nid;
          write_rec(upn, CL_SI, lra_, partner);
        }
      }
  { Lra_t sttlra = lra;
    Int clamp = 1000;
    Cc cc = HALTED;
    do
    { // do_db(-1,-1);
      Stockitem  si_i = (Stockitem)read_rec(upn, CL_SI, lra, &s_t);
      if (si_i == null)
      	break;
      kvi.integer = s_t.sig_vt;
      if (s_t.stk_alias == oid)
      	s_t.stk_alias = nid;
      if (s_t.sig_vt == oid)
      	s_t.sig_vt = nid;
      if (sid != 0 and s_t.sig_vt == nid)
      	s_t.sig_vt = sid;
      write_rec(upn, CL_SI, lra, &s_t);
      cc = ix_search(upn, IX_STK, &kvi, &lra);
    } while (cc == OK and lra != 0 and lra != sttlra and --clamp >= 0);

    if (cc != OK or clamp < 0)
      i_log(cc, "Bad stock aliasing %d", kvi.integer);
  }}}
}

static Int do_del_allreds(upn, kv)
	 Id      upn;        /* Locks on */
 	 Key     kv;
{ Lra_t  lra;
  Int ct = 0;
  Cc cc;
  while ((cc = ix_search(upn, IX_SR, kv, &lra)) == SUCCESS)
  { ix_delete(upn, IX_SR, kv, &lra);
    ++ct;
  }
  return ct;
}

static Cc do_del_stk(upn, si, lra_ref)
	 Id         upn;          /* Locks on */
	 Stockitem  si;
 	 Lra        lra_ref;
{ Char * res = null;
  Stkkey bc;
  Key_t  kvi;  kvi.integer = si->line;

{ Cc cc = ix_delete(upn, IX_STKLN, &kvi, lra_ref);
  if (cc == OK)
  { Stockitem_t s_t;
    Stockitem si = (Stockitem)read_rec(upn, CL_SI, *lra_ref, &s_t);
    if (si == null)
      cc = CORRUPT_PF;
    else
    { Set props = s_t.props & P_SUBS;
      Id nid = props != 0 ? s_t.sig_vt : s_t.stk_alias;
      Key_t kvi_;   kvi_.integer = s_t.id;
      kvi.string = bc_to_asc(s_t.supplier, s_t.line, &bc[0]);
      cc |= ix_delete(upn, IX_STKBC, &kvi, lra_ref);
    { Int ct = do_del_allreds(upn, &kvi_);
      rename_aliases(upn, *lra_ref, nid);
      read_rec(upn, CL_SI, *lra_ref, &s_t);
      s_t.props |= P_OBS;
      write_rec(upn, CL_SI, *lra_ref, &s_t);
#if 1
      if (props == 0)
      { Lra_t lra;
        kvi.integer = nid;
      { Stockitem_t si_i;
        Stockitem si = (Stockitem)ix_fetchrec(upn, IX_STK, &kvi, &lra, &si_i);
        if (si != null)
        { si_i.stock     = s_t.stock;
          si_i.order     = s_t.order;
          si_i.sig_v     = s_t.sig_v;
          si_i.sig_vt    = s_t.sig_vt;
          si_i.sale_date = s_t.sale_date;
          si_i.sell_stt  = s_t.sell_stt;
          si_i.cumdev    = s_t.cumdev;
          si_i.week_sale = s_t.week_sale;
          write_rec(upn, CL_SI, lra, &si_i);
        }
      }}
#endif
    }}
  }
  if (cc != SUCCESS)
    i_log(cc, "Stk Index NOK");

  return cc;
}}



static Cc to_price(ds, vatprops, pack, ticket, price_ref)
      Char *   ds;
      Set      vatprops;
      Int      pack, ticket, *price_ref;
{ fast Char * s = &ds[-1];

  while (*++s != 0)
    if (*s == '%')
    { float pr = ((float)(pack * ticket))*(1.0 - atof(ds) / 100.00);
      if (vatprops & P_VAT)
	pr = (pr * 1000.0) / (1000 + uniques.vatrate);
      *price_ref = (int)(pr + 0.5);
      return SUCCESS;
    }

  return to_integer(ds, price_ref);
}

static const Char * do_edit_props(Stockitem s, Char * data)

{ Set n = ((asc_to_rgt(data, true) >> 1) << 9) + (s->props & 0x1ff);
  const Char * res = n < OK		              ? illc  			     :
            	      (n ^ s->props) & P_DISC ? "stock cannot become a bonus" :
                              				        null;
  if (res == null)
    s->props = n;
  return res;
}



static void do_reds(Id, Stockitem, Bool) forward;


static const Char * do_edit_stk(upn, fld, keytbl, data, redraw_ref)
	 Id	   upn;
	 Field	   fld;
	 Char	   keytbl[SL+1][sizeof(Stkkey)+1];
	 Char *    data;
	 Bool	  *redraw_ref;
{ Int row = s_frow(fld);
  Id  fldtyp = s_fid(fld);
  Bool rdlk = fldtyp == W_KEY and data[0] == 0;
  *redraw_ref = false;
  if (keytbl[row][0] == 0)
    return ngth;

  if (rdlk)
    rd_lock(upn);
  else
    wr_lock(upn);

{ Key_t  kv_t;	kv_t.string = &keytbl[row][0];
{ Stockitem_t s;
  Lra_t silra;
  Stockitem si = (Stockitem)ix_fetchrec(upn, IX_STKBC, &kv_t, &silra, &s);
  const Char * error = si != null ? null : sodi;

  if (error == null)
    if (fldtyp == W_KEY)
      if      (toupper(*data) == 'D' and data[1] == 0)
      { if (kv_t.string[0] == '@') 
        { error = "Protected";
      	  put_si( si);
        }
        else
        { (void)do_del_stk(upn, si, &silra);
      	  keytbl[row][0] = 0;
      	  put_data("*");
        }
      }
      else if (data[0] == 0)
      	*redraw_ref = true;
      else
      { error = "d to delete";
	      put_si( si);
      }
    else
    { Bool inplace = false;

      switch (fldtyp)
      { when W_Z:
          	{ s.props &= ~P_VAT | (*data == 0 or *data == ' ' ? P_VAT :
			                   toupper(*data) == 'Z'	 ? 0	 :-1);
          	  error = s.props != -1 ? null : "VAT code wrong";
          	}
      	when W_PROP:
      	    { error = do_edit_props(&s, data);
          	  if (error == null)
          	    inplace = true;
          	}
      	when W_PACK:
      	    { char * star = strchr(data,'*');
      	      if (star != NULL)
      	      { *star = 0;
            	  error = vfy_integer(data) ? null : pian;
            	  s_case(&s) = atoi(data);
            	  data = star + 1;
            	}
      	        
            	error = vfy_integer(data) ? null : pian;
            	s.pack = atoi(data);
            }
      	when W_TKT:
          	{ error = vfy_cash(data) ? null : "unit price needed";
          	  s.ticket = atoi(data);
          	}
      	when W_DSCN:            
        	  strpcpy(&s.descn[0], data, sizeof(Descn));      
      	when W_PRC:
          	{ Cash oprice = s.price;
          	  inplace = true;
          	  error = to_price(data, s.props, s.pack, s.ticket, &s.price)
                  			 == SUCCESS ? null : "price or percent needed";
          	  *redraw_ref = (s.props & P_SR) != 0 and s.price != oprice;
            }
      	when W_STK:
          	{ Char * d_ = *data != '+' and *data != '-' ? data : &data[1];
          	  Cash ct = atoi(d_);
          	  inplace = true;
          	  if (not vfy_cash(d_))
          	    error = "stock level";
          	  else
          	    s.stock = d_==data ? ct : (s.stock + (*data=='+' ? ct : -ct));
          	}
      }
      if      (error == null)
      	if (inplace)
      	  write_rec(upn, CL_SI, silra, (Char *)&s);
      	else
        { Stockitem_t si_t;
          Stockitem  si = (Stockitem)read_rec(upn, CL_SI, silra, &si_t);
      	  if (si != null)
      	  { si->props |= P_OBS;
            (void)write_rec(upn, CL_SI, silra, si);
          }

      	{ Id oid = s.id;
      	  Id nid = last_id(upn, IX_STK) + 1;
      	  Lra_t nlra;

      	  s.id = nid;
      	{ Cc cc = new_init_rec(upn, CL_SI, (Char*)&s, &nlra);
      	  if (cc == SUCCESS)
      	  { Key_t kvi;	
      	    kvi.integer = s.line;
      	    cc |= ix_delete(upn, IX_STKLN, &kvi, &silra);
      	    cc |= ix_insert(upn, IX_STKLN, &kvi, &nlra);
      	    kvi.integer = s.id;
      	    cc |= ix_insert(upn, IX_STK, &kvi, &nlra);
      	    cc |= ix_replace(upn, IX_STKBC, &kv_t, &nlra);
      	  }
      	  if (cc == OK)
      	  { rename_aliases(upn, silra, nid);
      	    rename_offers(upn, oid, nid);
      	  }
      	  else
      	  { i_log(cc, "Corrupt Stock indeces");
      	    error = iner;
      	  }
      	}}}
      put_si( &s);
    }
  if (rdlk)
    rd_unlock(upn);
  else
    wr_unlock(upn);
  if (error == null and *redraw_ref)
    do_reds(upn, si, false);
  return error;
}}}

static const Char * do_substitute(si, lra)
	Stockitem si;
	Lra_t     lra;
{ Int spos = save_pos();
  goto_fld(W_STK+ID_MSK); put_free_data(spaces(21));
  goto_fld(W_CMD);
  if (si->stk_alias != 0)
  { Char ch = salerth("Press CR to unlink item");
    restore_pos(spos);
    if (ch != A_CR)
      return aban;
    wr_lock(mainupn);
  { Id nid = si->props & P_SUBS ? si->sig_vt : si->stk_alias;
    rename_aliases(mainupn, lra, nid);
    wr_unlock(mainupn);
  }}
  else
  { salert("Item to link to?");
    get_data(ds);
    restore_pos(spos);
    wr_lock(mainupn);
  { Lra_t lral;
    Key_t kvi; kvi.string = skipspaces(ds);
  { Stockitem_t l_t;
    Stockitem loop = (Stockitem)ix_fetchrec(mainupn,IX_STKBC,&kvi, &lral, &l_t);
    Id master = l_t.id;
    if (loop == null)
    { wr_unlock(mainupn);  return ntfd;
    }
    if (loop->id == si->id)
    { wr_unlock(mainupn);  return cndo;
    }
    if      (loop->props & P_SUBS)
      master = l_t.stk_alias;
    else if (l_t.stk_alias == 0)
    { l_t.stk_alias = si->id;
      loop = null;
    }
    else
    { kvi.integer = l_t.stk_alias;
      loop = (Stockitem)ix_fetchrec(mainupn, IX_STK, &kvi, &lral, &l_t);
      if (loop == null)
      { wr_unlock(mainupn);  return iner; }
    }
    si = (Stockitem)read_rec(mainupn, CL_SI, lra, si);
    if (si == null)
    { wr_unlock(mainupn);  return sodi; }
    si->props    |= P_SUBS;
    si->sig_vt    = loop == null ? si->id : loop->id;
    si->stk_alias = master;
    l_t.stock += si->stock;
    write_rec(mainupn, CL_SI, lra, si);
    write_rec(mainupn, CL_SI, lral, &l_t);
    if (loop != null)
      rename_aliases(mainupn, lral, si->id);

    wr_unlock(mainupn);
  }}}
  return null;
}

static const Char * do_edit_ana(upn, fld, keytbl, data)
	 Id	   upn;
	 Field	   fld;
	 Char	   keytbl[SL+1][sizeof(Stkkey)+1];
	 Char *    data;
{ Id  fldtyp = s_fid(fld);
  Int row = s_frow(fld);
  if (keytbl[row][0] == 0)
    return ngth;
  if (upn_ro)
    return rdoy;
  wr_lock(upn);

{ Lra_t silra;
  Key_t  kv_t;	kv_t.string = &keytbl[row][0];
{      Stockitem_t s_t;
  fast Stockitem si = (Stockitem)ix_fetchrec(upn,IX_STKBC, &kv_t, &silra, &s_t);
  const Char * error = si != null ? null : sodi;

  if (error == null)
  { if      (fldtyp == W_SUP)
    { if (toupper(data[0]) != 'D' or data[1] != 0)
        error = "only (D)elete";
      else if (kv_t.string[0] == '@') 
        error = "Protected";
      else
      { (void)do_del_stk(upn, si, &silra);
      	keytbl[row][0] = 0;
      	put_data("*");
      	si = null;
      }
    }
    else if (fldtyp == W_PROP)
    { error = do_edit_props(si, data);
    }
    else if (fldtyp == W_PACK)
    { error = vfy_integer(data) ? null : pian;
      s_case(si) = atoi(data);
    }
    else if (in_range(fldtyp,W_STK,W_AGE))
    { Char * d_ = *data != '+' and *data != '-' ? data : &data[1];
      Cash ct = *data != '-' ? atoi(d_) : -atoi(d_);
      if      (*data == '@')
      	error = do_substitute(si, silra);
      else if (not vfy_integer(d_))
      	error = "number or +/- number";
      else
      { if	(fldtyp == W_STK)
      	  si->stock = d_==data ? ct : (si->stock + ct);
      	else if (fldtyp == W_OTW)
      	  si->order = d_==data ? ct : (si->order + (*data == '+' ? ct : -ct));
      	else
      	{ if (fldtyp != W_AGE and uniques.lst_anal - si->sell_stt < s_period)
      	    si->sell_stt = uniques.lst_anal - s_period + 1;
      	  analyse(si);
      	{ Int n = an_n;
      	  if	  (fldtyp == W_AVG)
      	    an_rate = d_ == data ?	     dbl(ct) / dbl(s_period)
			                        	 : an_rate + dbl(ct) / dbl(s_period);
      	  else if (fldtyp == W_GTH)
      	    an_b = dbl(ct) / dbl(s_period * s_period);
      	  else
      	  { if (d_==data)
      	      si->sell_stt = uniques.lst_anal - ct;
      	    else
      	      si->sell_stt -= ct;
      	    n = uniques.lst_anal - si->sell_stt;
      	    an_n = n;
      	  }
      	  an_a = an_rate - an_b * dbl(n);
      	  si->cumdev = 0;
      	  if (an_n <= 0)
      	  { si->sig_v = 0;
      	    si->sig_vt = 0;
      	  }
      	  else
      	  { si->sig_v  = rnd(dbl(n) * (an_a + dbl(n-1) * an_b / 2.0));
      	    an_a = dbl(si->sig_v) / dbl(n)  -  an_b * dbl(n-1) / 2.0;
      	    si->sig_vt = rnd(dbl(n * (n-1)) * (an_a/2.0 + dbl(2*n-1) * an_b/6.0));
      	    analyse(si);
      	  }
      	}}
      }
    }
    else
      error = cndo;
    if (si != null)
    { if (error == null)
        write_rec(upn, CL_SI, silra, si);
      put_sa(si);
    }
  }
  wr_unlock(upn);
  return error;
}}}

static const Char * do_upd_stk(upn,	ds)
	 Id	upn;
	 Char * ds;
{ Char keytbl[SL+1][sizeof(Stkkey)+1];	
  Int i;
  for (i = SL; i >= 0; --i)
    keytbl[i][0] = 0;

  if (disallow(R_STK))
    return preq('V');
  if (upn_ro)
    return rdoy;

{ Id  sup, line;
  Char * ccstr = get_bc(ds, &sup, &line);
  if (ccstr < ds)
    return illk;

{ Int lim;
  Int cont;

  for (cont = 1; cont >= 1;)
  { Char keystr[LSTKKEY+10];
    Key_t  kv_t; kv_t.string = bc_to_asc(sup, line, &keystr[0]);
  { Field fld;
    Lra_t lra;
    Bool fst = true;
    const Char * error = null;

    Char * strm = ix_new_stream(upn, IX_STKBC, ccstr == ds ? null : &kv_t);

    wreopen(tbl1);
    salert("CR in key field for offers");
    goto_fld(W_KEY);
    lim = tbl1->tabledepth;

    for (cont = 2; cont >= 2;)
    { rd_lock(upn);
      while (lim > 0 and next_of_ix(strm, &kv_t, &lra) == SUCCESS)
      { // do_db(-1,-1);
        Stockitem si = (Stockitem)rec_ref(upn, lra);
      	if (not bad_ptr(si))
      	{ strpcpy(&keytbl[stk_sl-lim][0], kv_t.string, sizeof(Stkkey));
      	  put_si(si);
      	  if (--lim > 0)
      	    down_fld(0, &fld);
        } 
      }
      rd_unlock(upn);

      if (fst)
      { (void)goto_fld(W_KEY);
 	      fst = false;
      }

      for (cont = 3; cont >= 3; )
      { if (error != null)
       	  alert(error);
       	error = dtex;

      { Cc cc = get_any_data( &ds[0], &fld);
       	if	(cc == HALTED)
       	{ if (not (fld->props & FLD_LST))
       	    error = nrsc;
       	  else
       	  { wscroll(1);
       	    scroll_stk(keytbl,1);
       	    ++lim;
       	    error = null;
       	    break;
       	  }	
       	}
       	else if (cc == PAGE_DOWN && lim == 0)
       	{ error = null;
       	  wscroll(stk_sl-2);
       	  scroll_stk(keytbl,stk_sl-2);
       	  (void)goto_fld(W_KEY+2);
       	  lim += stk_sl-2;
       	  break;
       	}
       	if (fld->id == W_CMD)
       	{ Cc cm = sel_cmd(ds);
       	  if	  (cm == CMD_DOT)
       	    cont = 0;
       	  else if (cm == CMD_FIND)
       	  { ccstr = get_bc(&ds[5], &sup, &line);
       	    if (ccstr < &ds[5])
       	      error = illk;
       	    else
       	    { if (ccstr == &ds[5])
             		ccstr = ds;
       	      cont = 1;
       	    }
       	  }
       	}
       	else if (cc == SUCCESS and in_range(fld->id, W_KEY, W_STK+stk_sl))
       	{ Bool redraw;
       	  error = do_edit_stk(upn, fld, keytbl, ds, &redraw);
       	  if (redraw)
       	  { cont = 1;
       	    (void)get_bc(keytbl[0], &sup, &line);
       	    ccstr = &ds[1];
       	  }
       	}
      }}
      end_interpret();
    }
    ix_fini_stream(strm);
  }}

  return lim != stk_sl ? null : ngfd;
}}}

static const Char * do_upd_ana(upn,	ds)
	 Id	upn;
	 Char * ds;
{ Lra_t  lrau;
  Unique un = find_uniques(upn, &lrau);
  Char keytbl[SL+1][sizeof(Stkkey)+1];	
  Char upto[LDATE+1];
  Int i;
  for (i = SL; i >= 0; --i)
    keytbl[i][0] = 0;
  date_to_asc(&upto[0], un->lst_anal, 0);

  if (disallow(R_STK))
    return preq('V');
  if (upn_ro)
    return rdoy;

{ Id  sup, line;
  Char * ccstr = get_bc(ds, &sup, &line);
  if (ccstr < ds)
    return illk;

{ Int lim;
  Int cont;
  Tabled tbl4 = mk_form(SCform,stk_sl); // g_rows - 7
  if (tbl4 == null)
    return iner;

  for (cont = 1; cont >= 1;)
  { Char keystr[LSTKKEY+10];
    Key_t  kv_t; kv_t.string = bc_to_asc(sup, line, &keystr[0]);

  { Field fld;
    Lra_t lra;
    Bool fst = true;
    const Char * error = null;

    Char * strm = ix_new_stream(upn, IX_STKBC, ccstr == ds ? null : &kv_t);

    wopen(tbl4, SC1over);
    fld_put_data(W_WHCH, upto);
    goto_fld(W_SUP);
    lim = tbl4->tabledepth;

    for (cont = 2; cont >= 2;)
    { rd_lock(upn);
      while (lim > 0 and next_of_ix(strm, &kv_t, &lra) == SUCCESS)
      { // do_db(-1,-1);
        Stockitem si = (Stockitem)read_rec(upn, CL_SI, lra, null);
      	if (si != null)
      	{ strpcpy(&keytbl[SL-lim][0], kv_t.string, sizeof(Stkkey));
      	  put_sa(si);
      	  if (--lim > 0)
      	    down_fld(0, &fld);
      } }
      rd_unlock(upn);

      if (fst)
      { (void)goto_fld(W_SUP);
      	fst = false;
      }

      for (cont = 3; cont >= 3; )
      { if (error != null)
      	  alert(error);
      	error = dtex;

      { Cc cc = get_any_data(&ds[0], &fld);

      	if	(cc == HALTED)
      	{ if (not (fld->props & FLD_LST))
      	    error = nrsc;
      	  else
      	  { wscroll(1);
      	    scroll_stk(keytbl,1);
      	    ++lim;
      	    error = null;
      	    break;
      	  }	
      	}
      	else if (cc == PAGE_DOWN)
      	{ error = null;
      	  wscroll(stk_sl-2);
      	  scroll_stk(keytbl,stk_sl-2);
      	  (void)goto_fld(W_SUP+2);
      	  lim += SL-2;
      	  break;
      	}
      	if (fld->id == W_CMD)
      	{ if (*strmatch(".", ds) == 0)
      	    cont = 0;
      	}
      	else if (cc == SUCCESS and in_range(fld->id, W_KEY, W_AGE+SL))
      	  error = do_edit_ana(upn, fld, keytbl, ds);
      }}
      end_interpret();
    }
    ix_fini_stream(strm);
  }}

  freem(tbl4);
  return lim != SL ? null : ngfd;
}}}

static const Char * do_add_stk(upn, ds)
	 Id	upn;
	 Char * ds;
{ if (disallow(R_STK))
    return preq('V');
  if (upn_ro)
    return rdoy;

{ Cc cc;
  Tabled tbl3 = mk_form(SAform,stk_sl);
  if (tbl3 == null)
    return iner;

  wopen(tbl3,SA1over);

  while (true)			/* and not break */
  { Field fld = goto_fld(W_KEY+ID_MSK);
		Fieldrep ofr = fld->class;
		Fieldrep_t fr = {W_KEY, FLD_WR+FLD_BOL,5,0,50,-SL, pcde};
		fld->class = &fr;

    while ((cc = get_any_data( &ds[0], &fld)) != SUCCESS)
      ;
		fld->class = ofr;

    if (ds[0] == '.')
      break;

  { Stockitem_t si; si = null_sct(Stockitem_t);
    Char ui[LUI+100]; ui[0] = 0;
    int outer;
    Char * bc = get_core_code(ds, ui, &outer);
  { Char * ccstr = get_bc(&bc[0], &si.supplier, &si.line);

		wopen_row(tbl3);
	  wputstr("\r");
		wputstr(bc);

    if (ccstr <= bc or s_fid(fld) != W_KEY)
    { alert(ccstr < bc ? "invalid code" : dtex);
      continue;
    }
  { Lra_t silra;
    Key_t kv_t;  kv_t.string = bc_to_asc(si.supplier, si.line, &bc[0]);
  { Stockitem_t sii_t;
    Stockitem sii = (Stockitem)ix_fetchrec(upn, IX_STKBC, &kv_t, null, &sii_t);
    if (sii != null)
    { alert(arth);
      if (sii != null)
      	put_si( sii);
      continue;
    }

    si.props = P_VAT;
    while (true)
    { goto_fld(W_Z+ID_MSK);
      get_data(ds);
      if (vfy_zrate(ds))
      	break;
      alert("Z or nothing");
    }

    if (toupper(ds[0]) == 'Z')
      si.props &= ~ P_VAT;

    while (true)
    { Int outer;
      goto_fld(W_CASE+ID_MSK);
      if (get_integer(&outer) == SUCCESS)
      { s_case(&si) = outer;
      	break;
      }
      alert("Number in case or 0");
    }

    goto_fld(W_PROP+ID_MSK);
    get_data(ds);
    si.props = ((asc_to_rgt(ds, true) >> 1) << 9) + (si.props & 0x1ff);

    while (true)
    { Int pack;
      goto_fld(W_PACK+ID_MSK);
      if (get_integer(&pack) == SUCCESS)
      { si.pack = pack;
      	break;
      }

      alert("Number");
    }

    while (true)
    { Int ticket;
      goto_fld(W_TKT+ID_MSK);
      if (get_integer( &ticket) == SUCCESS)
      { si.ticket = ticket;
      	break;
      }
      alert(nreq);
    }

    goto_fld(W_DSCN+ID_MSK);
    (void)get_data( &si.descn[0]);

    while (true)
    { goto_fld(W_PRC+ID_MSK);
      get_data(ds);
      if (to_price(ds, si.props, si.pack, si.ticket, &si.price) == SUCCESS)
      { put_cash( si.price);
      	break;
      }

      alert("Price is a number or e.g. 27.5%");
    }

    wr_lock(upn);
    si.id = last_id(upn, IX_STK) + 1;
    si.sale_date = asc_to_date("");
    cc = i_Stockitem_t(upn, &si, &silra);
    if (cc == DUP_UK_ATTR)
    { Stockitem sii = (Stockitem)ix_srchrec(upn, IX_STKBC, &kv_t, &silra);
      if (sii != null)
      	cc = NOT_FOUND;
    }
    if (cc < SUCCESS)
      i_log(61, "Corr. Stock Area");
    wr_unlock(upn);

    if (fld->props & FLD_LST)
    { wscroll(1);
      up_fld( 0, &fld);
    }

    cc = salerth("Press O for offers");
    if (cc == 'O')
    { Screenframe_t stk;
      push_form(&stk);
      do_reds(upn, &si, false);
      pop_form(&stk);
    }
  }}}}}
  freem(tbl3);
  return cc == SUCCESS   ? null              : 
         cc == NOT_FOUND ? "Others added it" : "Database error";
}}

static const Char * do_epat_stk(upn, fld, si_, data_)
	 Id	   upn;
	 Field	   fld;
	 Stockitem si_;	
	 Char *    data_;
{ Stockitem si = si_;	
  fast  Char * data = data_;
  const Char * error = "not legal";
       Quot f = s_fid(fld);

  if	  (f == W_KEY)
  { error = *data == 0 or *data == '<' or *data == '>' ? null : "> or <";
    si->id = *data == 0   ? 0 :
	     *data == '>' ? 1 :
	     *data == '<' ? -1 : si->id;
  }
  else if (f == W_Z)
  { Set props = *data == 0 or *data == ' ' ? P_VAT :
		toupper(*data) == 'Z'	   ? P_VAT0  : -1;

    error = props != -1 ? null : "VAT code wrong";
    if (error == null)
      si->props = (si->props & 0xffffff00) + props;
  }
  else if (f == W_PROP)
  { error = null;
    si->props = asc_to_rgt(data, true) << 8 + (si->props & 0xff);
  }
  else if (f == W_PACK)
  { error = vfy_integer(data) ? null : pian;
    si->pack = atoi(data);
  }
  else if (f == W_TKT)
  { error = vfy_cash(data) ? null : "unit price needed";
    si->ticket = atoi(data);
  }
  else if (f == W_DSCN)
  { error = null;
    strpcpy(&si->descn[0], data, sizeof(Descn));
  }
  else if (f == W_PRC)
  { error = vfy_cash(data) ? null : "price needed";
    si->price = atoi(data);
  }
  else if (f == W_SUP)
  { error = vfy_integer(data) ? null : "supplier";
    si->supplier = atoi(data);
  }
  else if (f == W_STK)
  { error = vfy_integer(data) ? null : "stock level";
    si->stock = atoi(data);
  }
  else if (f == W_AGE)
  { error = vfy_integer(data) ? null : "days";
    si->sale_date = today - atoi(data);
  }
  return error;
}

static Bool pat_match(pat, s_)
     Char * pat, *s_;
{ fast Char * p = &pat[0];
  fast Char * s = &s_[-1];

  while (*++s != 0)
    if (toupper(*s) == toupper(*p))
    { Char * ss = s;
      while (*++p != 0 and toupper(*++ss) == toupper(*p))
	;
      if (*p == 0)
	return true;
      p = &pat[0];
    }

  return false;
}


static Bool match_stk(pat, si_)
      Stockitem_t  pat[];
      Stockitem    si_;
{ fast Stockitem  si = si_;
       Int lim;
  for (lim = 0; lim < tbl1->tabledepth; ++lim)
  { Int match = -1;
    Stockitem p = &pat[lim];
    Int cmp = p->id;
    Int val;

    if ((p->props & 0xff) != 0)
      match =  (si->props & P_VAT) and (p->props & P_VAT) or
	      (~si->props & P_VAT) and (p->props & P_VAT0);
    if ((p->props & 0xffffff00) != 0)
      match &= (p->props & si->props & 0xffffff00) == (p->props & 0xffffff00);
    val = p->supplier;
    if (val != 0)
      match &= cmp == 0 ? si->supplier == val :
			  si->supplier * cmp > val * cmp;
    val = p->stock;
    if (val != 0)
      match &= cmp == 0 ? si->stock == val:
			  si->stock * cmp > val * cmp;
    val = p->sale_date;
    if (val != 0)
      match &= cmp == 0 ? si->sale_date == val :
			  si->sale_date * cmp < val * cmp;
    val = p->pack;
    if (val != 0)
      match &= cmp == 0 ? si->pack == val :
			  si->pack * cmp > val * cmp;
    val = p->ticket;
    if (val != 0)
      match &= cmp == 0 ? si->ticket == val :
			  si->ticket * cmp > val * cmp;
    val = p->price;
    if (val != 0)
      match &= cmp == 0 ? si->price == val :
			  si->price * cmp > val * cmp;
    if (p->descn[0] != 0)
      match &= pat_match(p->descn, si->descn);

    if	    (match == 1 and (cmp == 0 or lim == SL-1 or p[1].id == 0))
      return true;
    else if (match != 1)
      while (++lim < SL)
	if (pat[lim].id == 0)
	{ --lim;
	  break;
	}
  }

  return false;
}

static Bool prep_pat_stk(upn, pat)
	 Id	     upn;
	 Stockitem_t pat[SL];
{ Key_t  kv_t;
  Lra_t lra;
  Int lim = tbl1->tabledepth;
  Int cont = 1;
  Bool fst = true;
  const Char * error = null;
  while (--lim >= 0)
    pat[lim] = null_sct(Stockitem_t);

  wopen(tbl1, SS1over);
  salert("Separate by space for 'or'");
{ Field fld = goto_fld(W_KEY);

  while (cont > 0)
  { if (in_range(fld->id, W_KEY, W_AGE+SL))
    { Stockitem pi = &pat[s_frow(fld)];
      Int pos = save_pos();
      put_si( pi);
      fld_put_data( W_KEY + s_frow(fld), pi->id == 0 ? "" : pi->id > 0 
                                                     ? " >" : " <");
      right_fld( 0, &fld);
      put_data( pi->props & P_VAT ? "S" : pi->props & P_VAT0 ? "Z" : "");
      restore_pos( pos);
    }

    if (error != null)
      alert(error);
    error = null;

  { Cc cc = get_any_data(&ds[0], &fld);

    if	    (cc == HALTED)
    { error = null;
      if (fld->id != W_CMD)
      	error = "no scrolling";
    }
    else if (cc == SUCCESS)
      if      (in_range(fld->id, W_KEY, W_AGE+SL))
      	error = do_epat_stk(upn, fld, &pat[s_frow(fld)], ds);
      else if (fld->id == W_CMD)
      { cc = sel_cmd(ds);
      	if	(cc == CMD_DOT)
      	  cont = -1;
      	else if (cc == CMD_GO)
      	  cont =  0;
      	else
      	  error = "Go";
      }
  }}
  end_interpret();

  return cont == 0;
}}

static void do_list_srch(upn, pat)
	 Id	     upn;
	 Stockitem_t pat[SL];
{ Lra_t lra;
  Char * strm = ix_new_stream(upn,IX_STKBC, null);
  Int lim = tbl1->tabledepth;
  Field fld = goto_fld(W_KEY);
  Key_t kv_t;  kv_t.string = &ds1[0];

  ipfld();
  rd_lock(upn);
  while (next_of_ix(strm, &kv_t, &lra) == SUCCESS)
  { do_db(-1,-1);
    Char kb_ch;
    Stockitem si = (Stockitem)rec_ref(upn, lra);
    if (bad_ptr(si) || ! match_stk(pat, si))
      continue;
    rd_unlock(upn);
    put_si( si);
    down_fld( 0, &fld);

    if (--lim <= 0)
    { kb_ch = salerth(crsd);
      if      (kb_ch == A_CR)
      { wscroll(1);
      	up_fld( 0, &fld);
      	lim = 1;
      }
      else if (kb_ch == ' ')
      { wscroll(stk_sl-2);
      	goto_fld(W_KEY+1);
      	lim = tbl1->tabledepth-1;
      }
      else
      { rd_lock(upn);
      	break;
      }
    }
    rd_lock(upn);
  }
  rd_unlock(upn);
  ix_fini_stream(strm);

  salerth(lim == tbl1->tabledepth ? ngfd : akey);
}

static const Char * do_list_stk(upn, ds)
	 Id	upn;
	 Char * ds;
{ Id idtbl[SL+1], propstbl[SL+1];
  Id sup, line;
  Key_t  kv_t;
  Char * ccstr = get_bc(ds, &sup, &line);
  if (ccstr < ds)
    return illk;

{ Int lim;
  Int cont;

  for (cont = 1; cont >= 1; )
  { kv_t.string = bc_to_asc(sup, line, &ds[0]);

  { Char * strm = ix_new_stream(upn,IX_STKBC, ccstr == ds ? null : &kv_t);
    Lra_t lra;
    Field fld;

    ccstr = null;
    wopen(tbl1,SI1over);
    goto_fld(W_KEY);
    lim = tbl1->tabledepth;

    rd_lock(upn);
    for (cont = 2; cont == 2 and next_of_ix(strm, &kv_t, &lra) == SUCCESS; )
    { do_db(-1,-1);
      Char kb_ch;
      Stockitem si = (Stockitem)rec_ref(upn, lra);
      if (bad_ptr(si))
      	continue;
      rd_unlock(upn);
      put_si( si);
      idtbl[SL-lim] = si->id;
      propstbl[SL-lim] = si->props;

      if (--lim > 0)
      	down_fld(FLD_BOL, &fld);
      else
      { kb_ch = salerth((propstbl[0] & P_SR) ? "O, CR, SP, or other" : crsd);
      	if	(toupper(kb_ch) == 'O' and (propstbl[0] & P_SR))
      	{ kv_t.integer = idtbl[0];
      	{ Stockitem_t s_t;
      	  Stockitem sii = (Stockitem)ix_fetchrec(upn,IX_STK,&kv_t,&lra,&s_t);
      	  if (sii == null)
      	    salert("Is gone!!");
	        else
      	  { sup = sii->supplier;
	          line = sii->line;
      	    do_reds(upn, sii, not upn_ro);
	          cont = 1;
      	  }
        }}
        else if (kb_ch == A_CR)
      	{ wscroll(1);
      	  scroll_bstk(&idtbl[0], &propstbl[0], 1);
      	  lim = 1;
      	}
      	else if (kb_ch == ' ')
      	{ wscroll(stk_sl-2);
      	  scroll_bstk(&idtbl[0], &propstbl[0], stk_sl-1);
      	  goto_fld(W_KEY+1);
      	  lim = tbl1->tabledepth - 1;
      	}
      	else
	        cont = 0;
      }
      rd_lock(upn);
    }
    rd_unlock(upn);
    ix_fini_stream(strm);
    if (cont == 2)
      break;
  }}

  salerth(lim == tbl1->tabledepth ? ngfd : akey);
  return null;
}}

static const Char * do_prt_srch(upn, pat, ds)
	 Id          upn;
	 Stockitem_t pat[SL];
	 Char *      ds;
{ if (disallow(R_RPT))
    return preq('G');
{ Prt chan = select_prt(0);
  if (chan == null)
    return aban;

{ Char * strm = ix_new_stream(upn, IX_STKBC, null);
  Lra_t lra;
  Int ct = 0;
  Key_t kv_t;  kv_t.string = &ds[0];

  ipfld();
  sprintf(&msg[0], "Stock Description: %s at %s\n", ds, todaystr);
{ Char * err = prt_one_report(chan, msg, "STOCK\n\n\n");
  if (err != null)
    return err;

  sprintf(&msg[0], "%s\n%s\n%s\n",
 " Supplier    Product  Z  Pack Ticket Description                     Price",
 "                      Id              Stock  Last-Sale  Sales1   Sales2",
	  hline(75));
{ Char * format =
 "      %8d         %6r%1r  %4d   %4v                             %30r    %7v\n"
 "                     %5d                %5d        %10l   %5d      %5d\n\n";
  prt_set_cols(msg, format);

  rd_lock(upn);
  while (next_of_ix(strm, &kv_t, &lra) == SUCCESS)
  { // do_db(-1,-1);
    Stockitem si = (Stockitem)rec_ref(upn, lra);
    if (bad_ptr(si))
      i_log(0, "Corrupt Stock Area");
    if (not match_stk(pat, si))
      continue;
    ct += 1;
    if (ct == 1)
      prt_head();
    bc_to_asc(0, si->line, &msg[0]);
    if      (si->sale_date == today)
      strcpy(&ds[0], "TODAY");
    else if (si->sale_date == 0)
      strcpy(&ds[0], "NOT SOLD");
    else
      date_to_asc(&ds[0], si->sale_date, 0);
    prt_row(0, si->supplier, msg, si->props & P_VAT ? "" : "Z", si->pack,
	          si->ticket, &si->descn[0], si->price, si->id,
    	      si->stock, ds, 0, 0);
    rd_unlock(upn); rd_lock(upn);
  }
  rd_unlock(upn);

  ix_fini_stream(strm);
  prt_fmt(noes, ct);
  prt_fini(NULL);

  if (ct == 0)
    return ngfd;
  return null;
}}}}}

static const Char * do_prt_stk(wh, ds)
	 Char     wh;
	 Char *   ds;
{ Id sup, line;
  Key_t    kv_t;
  Char * d_ = get_bc(ds, &sup, &line);
  Char * secname = d_;
  kv_t.string = bc_to_asc(sup, line, &ds1[0]);
  if (d_ < ds)
    return illk;
  if (disallow(R_RPT))
    return preq('G');

{ Prt chan = select_prt(0);
  if (chan == null)
    return aban;

  sprintf(&msg[0], "Stock from %s to %s at %s\n",
	             d_ == ds      ? "BEGINNING" : kv_t.string,
	             *secname == 0 ? "END"       : secname, todaystr);
{ Char * err = prt_one_report(chan, msg, "STOCK\n\n\n");
  if (err != null)
    return err;

  ipfld();
  strcpy(&msg[0],
   wh != 'L' ?
 " Supplier  Code  Ticket Description                  Stock   On-the-Way\n" :
 " Supplier  Code  Z  Pack Ticket Description                      Price\n"
 "                         Stock    On-the-way   Last-Sale\n");
  sprintf(&msg[strlen(msg)], "%s\n", hline(75));
{ Char * format = wh != 'L' ?
 "     %8r   %5r  %4d                           %30l    %5d    %5d          %10l\n" :
 "     %8r   %5r%1r  %4d   %4v                             %30r      %8v\n"
 "                            %5d       %5d          %10l\n\n";
  prt_set_cols(msg, format);
{ Id last_sup = 0;
  Id sup = MAXINT;
  Id line = *secname == 0 ? MAXINT : asc_to_bcsup(secname, &sup);
  Char * strm = ix_new_stream(mainupn, IX_STKBC, d_ == ds ? null : &kv_t);
  Lra_t lra;
  Int ct = 0;

  rd_lock(mainupn);
  while (next_of_ix(strm, &kv_t, &lra) == SUCCESS)
  { // do_db(-1,-1);
    Stockitem si = (Stockitem)rec_ref(mainupn, lra);
    if (bad_ptr(si))
    { i_log(0, "Corrupt Stock Area"); continue; }
    if (si->supplier > sup or si->supplier == sup and si->line > line)
      break;
    ct += 1;
    if (ct == 1)
      prt_head();
    bc_to_asc(0, si->line, &msg[0]);
    if      (si->sale_date == today)
      strcpy(&ds[0], "TODAY");
    else if (si->sale_date == 0)
      strcpy(&ds[0], "NOT SOLD");
    else
      date_to_asc(&ds[0], si->sale_date, 0);
  { Char suplr[34];
    suplr[0] = 0;
    if (last_sup != si->supplier)
      sprintf(&suplr[0], "%d", si->supplier);
    last_sup = si->supplier;
    
    if      (wh == 'L')
      prt_row(0, suplr, msg, si->props & P_VAT ? "" : "Z", si->pack,
      	      si->ticket, &si->descn[0], si->price, si->stock, si->order, ds);
    else if (not (si->props & P_SUBS))
      prt_row(0, suplr, msg, si->ticket, &si->descn[0], si->stock, si->order, ds);
    else
    { Char withs[32];
      Key_t kvi; kvi.integer = si->stk_alias;
      withs[0] = 0;
    { Lra_t lra;
      Stockitem sii = (Stockitem)ix_srchrec(mainupn, IX_STK, &kvi, &lra);
      if (sii != null)
      	bc_to_asc(sii->supplier, sii->line, &withs[0]);
      prt_fmt("     %8r   %5r  %4d                           %30l at %13r %9l\n",
      	      suplr, msg, si->ticket, &si->descn[0], withs, ds);
    }}
    rd_unlock(mainupn); rd_lock(mainupn);
  }}
  rd_unlock(mainupn);

  ix_fini_stream(strm);
  prt_fmt(noes, ct);
  prt_fini(NULL);

  return ct != 0 ? null : ngfd;
}}}}}

static const Char * do_prt_pl(upn, ds)
	 Id       upn;
	 Char *   ds;
{ Char buff[80], sbuff[17];
  Id sup, line;
  Key_t    kv_t;
  Char * d_ = get_bc(ds, &sup, &line);
  Char * secname = *d_ == 0 ? d_ : skipspaces(&d_[1]);
  kv_t.string = bc_to_asc(sup, line, &ds1[0]);
  if (d_ < ds)
    return illk;
  if (disallow(R_RPT))
    return preq('G');

{ Char tfname[22];
  Char * fn = strcat(strcpy(&tfname[0], "/tmp/pl"), &this_tty[8]);
  FILE * out = fopen(fn,"w");

  Prt chan = select_prt(0);
  if (chan == null)
    return aban;

{ Char * err = prt_one_report(chan, "", "");
  if (err != null)
    return err;

  sprintf(&msg[0], "Price List from %s to %s at %s\n\n",
	             d_ == ds      ? "BEGINNING" : kv_t.string,
	             *secname == 0 ? "END"       : secname, todaystr);
  ipfld();

{ Char * format = "   %6r  %4d   %5v                           %30l    %7v\n";
  Id sup = MAXINT;
  Id line = *secname == 0 ? MAXINT : asc_to_bcsup(secname, &sup);
  Id lastsup = 0;
  Char * strm = ix_new_stream(upn, IX_STKBC, d_ == ds ? null : &kv_t);
  Lra_t lra;
  Int ct = 0;

  rd_lock(upn);
  while (next_of_ix(strm, &kv_t, &lra) == SUCCESS)
  { // do_db(-1,-1);
    Stockitem si = (Stockitem)rec_ref(upn, lra);
    if (si->supplier > sup or si->supplier == sup and si->line > line)
      break;
    if (si->props & (1 << (8+'P'-'@')))
      continue;
    ct += 1;
    if (lastsup != si->supplier)
    { Lra_t lras;
      Key_t kvs;  kvs.integer = -si->supplier;
    { Supplier spr = (Supplier)ix_srchrec(upn, IX_MANBC, &kvs, &lras);
      if (spr == null)
      	fprintf(out, "\n");
      else
      { Int i, l = strlen(spr->surname);
      	buff[0] = '\n';
      	for (i = 1; i <= l; ++i)
      	{ buff[i] = spr->surname[i-1]; buff[l+i+1] = '-';
      	}
      	buff[i] = '\n'; buff[l+i+1] = '\n'; buff[l+i+2] = 0;
      	fprintf(out, buff);
      }

      si = (Stockitem)rec_ref(upn, lra);
      lastsup = si->supplier;
    }}

    bc_to_asc(0, si->line, &sbuff[0]);
    fmt_data(buff, format, sbuff, si->pack, si->ticket, si->descn, si->price);
    fprintf(out, buff);
    rd_unlock(upn); rd_lock(upn);
  }
  rd_unlock(upn);
  ix_fini_stream(strm);
  fclose(out);
  sprintf(buff, "./plsort %s > %sA", fn, fn);
  do_osys(buff);
  sprintf(buff, "mv %sA %s", fn, fn);
  do_osys(buff);

  out = fopen(fn, "r");
  if (ct != 0)
    prt_head();
  prt_set_cols("", "");
  prt_text(msg);

  while ((format = fgets(&msg[0], 132, out)) != null)
    prt_text(format);

  prt_fmt(noes, ct);
  prt_fini(NULL);

  fclose(out);
  do_osys(strcat(strcpy(buff, "rm "), fn));

  salert("Done");
  return ct != 0 ? null : ngfd;
}}}}

static const Char * do_prt_offers(upn, ds)
	 Id       upn;
	 Char *   ds;
{ Char prc_str[80], rgt_str[34];
  Id sup, line;
  Key_t    kv_t;
  Char * d_ = get_bc(ds, &sup, &line);
  Char * secname = *d_ == 0 ? d_ : skipspaces(&d_[1]);
  kv_t.string = bc_to_asc(sup, line, &ds1[0]);
  if (d_ < ds)
    return illk;
  if (disallow(R_RPT))
    return preq('G');

{ Prt chan = select_prt(0);
  if (chan == null)
    return aban;

  sprintf(&msg[0], "Bargains from %s to %s at %s\n",
	             d_ == ds      ? "BEGINNING" : kv_t.string,
	             *secname == 0 ? "END"       : secname, todaystr);
{ Char * err = prt_one_report(chan, msg, "OFFERS\n\n\n");
  if (err != null)
    return err;

  ipfld();
  sprintf(&msg[0], "%s\n%s\n%s\n",
 " Supplier  Product   Z Pack Ticket Description                     Price",
 "  Prices ",
	  hline(75));
{ Char * format = 
 "      %7d        %6r%1r  %4d   %4v                             %30r    %7v\n"
 " %r\n\n";
  prt_set_cols(msg, format);

{ Id sup = MAXINT;
  Id line = *secname == 0 ? MAXINT : asc_to_bcsup(secname, &sup);
  Char * strm = ix_new_stream(upn, IX_STKBC, d_ == ds ? null : &kv_t);
  Lra_t lra;
  Int ct = 0;

  rd_lock(upn);
  while (next_of_ix(strm, &kv_t, &lra) == SUCCESS)
  { // do_db(-1,-1);
    Stockitem_t si_;
    Stockitem si = (Stockitem)rec_ref(upn, lra);
    if (bad_ptr(si))
    { i_log(0, "Corrupt Stock Area"); continue;
    }
    if (si->supplier > sup or si->supplier == sup and si->line > line)
      break;
    if (not (si->props & P_SR))
      continue;
    memcpy(&si_, si, sizeof(Stockitem_t));
  { Lra_t lrai;
    Key_t kvi;  kvi.integer = si_.id;
    prc_str[0] = 0;
  { Char * strn = ix_new_stream(upn, IX_SR, &kvi);
    Int strix = 0;
    while (next_of_ix(strn, &kvi, &lrai) == SUCCESS and kvi.integer == si_.id)
    { Stockred sr = (Stockred)rec_ref(upn, lrai);
      if (bad_ptr(sr))
      	continue;
      rgt_to_asc(sr->which, &rgt_str[0]);
      if (strix + strlen(rgt_str) > 65)
      	break;
      fmt_data(&prc_str[strix], "%r:    %7v  ", rgt_str, sr->price);
      strix = strlen(prc_str);
    }
    ix_fini_stream(strn);

    ct += 1;
    if (ct == 1)
      prt_head();
    rd_unlock(upn);
    if (strix == 0)
    { wr_lock(upn);
    { Stockitem_t si_t;
      Stockitem si = (Stockitem)ix_fetchrec(upn, IX_STKBC, &kv_t, &lrai, &si_t);
      if (si != null)
      { si->props &= ~P_SR;
        write_rec(upn, CL_SI, lrai, si);
      }
      wr_unlock(upn);
    }}
    else
    { bc_to_asc(0, si_.line, &msg[0]);
      prt_row(0, si_.supplier, msg, si_.props & P_VAT ? "" : "Z", si_.pack,
    	        si_.ticket, &si_.descn[0], si_.price, prc_str);
    }
    rd_lock(upn);
  }}}
  rd_unlock(upn);

  ix_fini_stream(strm);
  prt_fmt(noes, ct);
  prt_fini(NULL);

  return ct != 0 ? null : ngfd;
}}}}}

static const Char * do_del_red(upn, kv, lra)
	 Id      upn;        /* Locks on */
 	 Key     kv;
 	 Lra     lra;
{ Char * res = null;

  Cc cc = ix_delete(upn, IX_SR, kv, lra);
  if (cc == SUCCESS)
    cc = del_rec(upn, CL_SR, *lra);
  return cc == SUCCESS ? null : sodi;
}



static Stockred find_sr(id, which, price, lra_ref, sr)
					/* locks on */
	 Id       id;
	 Set      which;
	 Cash     price;
	 Lra      lra_ref;
	 Stockred sr;
{      Key_t  kvi;  kvi.integer = id;
{      Char * strm = ix_new_stream(mainupn, IX_SR, &kvi);
  fast Stockred sr_ = null;

  while ((sr_ = (Stockred)ix_next_rec(strm, lra_ref, sr)) != null
	 and sr_->id == id)
    if (sr_->which == which and (price == 0 or sr_->price == price))
      break;

  ix_fini_stream(strm);
  return sr_;
}}

static const Char * do_edit_red(upn, fld, si, whichtbl, pricetbl, data)
	 Id	   upn;
	 Field	   fld;
	 Stockitem si;
	 Set	   whichtbl[SL+1];
	 Cash	   pricetbl[SL+1];
	 Char *    data;
{ const Char * error = dtex;
  Int row = s_frow(fld);
  Key_t  kvi;  kvi.integer = si->id;

  wr_lock(upn);

  if (whichtbl[row] == MININT)
    if (s_fid(fld) != W_WHCH)
      error = ngth;
    else
    { Stockred_t sr_t;
      sr_t.id	 = kvi.integer;
      sr_t.which = asc_to_rgt(data, true);

      right_fld( 0, &fld);
      get_data(&data[0]);
      error = to_price(data, si->props, si->pack, si->ticket, &sr_t.price)
			           == SUCCESS ? null : "price or percent needed";
      if (error == null)
      { Lra_t lra;
      	Cc cc = i_Stockred_t(upn, &sr_t, &lra);
      	error = cc == SUCCESS ? null : "Corrupt Stock Offer Area";
      }
    }
  else
  { Lra_t  lra;
    Stockred_t sr_t;
    Stockred sr = find_sr(si->id, whichtbl[row], pricetbl[row], &lra, &sr_t);
    if (sr == null)
    { wr_unlock(upn); return sodi; }

    if (s_fid(fld) == W_WHCH)
    { if      ((toupper(*data) == 'D' or *data == '0') and data[1] == 0)
      { error = do_del_red(upn, &kvi, &lra);
      	whichtbl[row] = MININT;
      	direct_data(fld, "DELETED");
      }
      else			   /* ASCII */
      { Int cc = asc_to_rgt(data, true);
      	if (cc < OK)
      	  error = "letters required";
      	else
      	{ sr->which = cc;
      	  whichtbl[row] = sr->which;
      	  (void)write_rec(upn, CL_SR, lra, sr);
      	}
      	put_red(sr->which, sr->price);
      }
    }
    else /* if (s_fid(fld) == W_RED) */
    { error = vfy_integer(data) ? null : "price is a number";
      if (error == null)
      { sr->price = atoi(data);
      	pricetbl[row] = sr->price;
      	(void)write_rec(upn, CL_SR, lra, sr);
      }
      put_cash(sr->price);
    }
  }
  wr_unlock(upn);
  return error;
}

#if 0

static void put_reds(upn,  id)
	 Id      upn;
	 Id      id;
{ Lra_t lra;
  Key_t  kvi; kvi.integer = id;
{ Int i;
  for (i = SL-1; i >= 0; --i)
    whichtbl[i] = MININT;
{ Char * strm = ix_new_stream(upn, IX_SR, &kvi);
  Int lim = tbl?->tabledepth;
  Field fld = goto_fld(W_WHCH);
  rd_lock(upn);
  while (lim > 0
	      and next_of_ix(strm, &kvi, &lra) == SUCCESS and kvi.integer == id)
  { // do_db(-1,-1);
    Stockred sr = (Stockred)rec_ref(upn, lra);
    if (not bad_ptr(sr))
    { whichtbl[SL-lim] = sr->which;
      pricetbl[SL-lim] = sr->price;
      put_red( whichtbl[SL-lim], pricetbl[SL-lim]);
      if (--lim > 0)
      	down_fld( 0, &fld);
  } }
  rd_unlock(upn);
}}}

#endif

static void do_reds(upn,  si, ro)
	 Id        upn;
	 Stockitem si;  /* not in db cache */
	 Bool      ro;
{ Cash whichtbl[SL], pricetbl[SL];
  Lra_t lra;
  Key_t  kvi;
  Int cont;
  Tabled tbl2 = mk_form(SRform,stk_sl);// g_rows - 8
  if (tbl2 == null)
   return;

  for (cont = 1; cont >= 1; )
  { Int i;
    Field fld;
    kvi.integer = si->id;

    wopen(tbl2,SR1over);
    goto_fld(W_KEY);
    put_si( si);
    goto_fld(W_REF);
    put_ointeger(si->profit * (256*256L) + si->altref);
    goto_fld(W_CST);
    put_data(rgt_to_asc(si->constraint, &ds[0]));

    for (i = stk_sl-1; i >= 0; --i)
      whichtbl[i] = MININT;

  { Char * strm = ix_new_stream(upn, IX_SR, &kvi);
    Int lim = tbl2->tabledepth;
    Bool fst = true;
    const Char * error = null;

    /*i_log(4, "FFinding offers for %d", kvi.integer);*/

    goto_fld(W_WHCH);
  { Int spos = save_pos();

    for (cont = 2; cont >= 2; )
    { rd_lock(upn);
      while (lim > 0
        	&& next_of_ix(strm, &kvi, &lra) == SUCCESS and kvi.integer == si->id)
      { // do_db(-1,-1);
        Stockred sr = (Stockred)rec_ref(upn, lra);
      	if (not bad_ptr(sr))
      	{ whichtbl[stk_sl-lim] = sr->which;
      	  pricetbl[stk_sl-lim] = sr->price;
        /*i_log(4, "Got %d %x %d", sr->id, sr->which, sr->price);*/
      	  put_red( whichtbl[stk_sl-lim], pricetbl[stk_sl-lim]);
      	  if (--lim > 0)
      	    down_fld( 0, &fld);
      } }
      kvi.integer = si->id;
      rd_unlock(upn);

      if (fst)
      { restore_pos(spos);
        goto_fld(W_RED);
      	fst = false;
      }

      for (cont = 3; cont >= 3; )
      { if (error != null)
      	  alert(error);
      	error = dtex;

      { Cc cc = get_any_data( &ds[0], &fld);

      	if	(cc == HALTED)
      	{ error = null;
      	  if (fld->id != W_CMD)
      	    if (not (fld->props & FLD_LST))
      	      error = nrsc;
      	    else
      	    { wscroll(1);
      	      scroll_sr(whichtbl,1);
      	      scroll_sr(pricetbl,1);
      	      ++lim;
      	      error = null;
      	      break;
	          }	
      	}
      	else if (fld->id == W_CMD)
      	{ if	  (*strmatch(".", ds) == 0)
      	    cont = 0;
      	}
      	else if (fld->id == W_REF or
                 fld->id == W_CST)
        { Int cst = fld->id == W_REF ? vfy_integer(ds)
                                     : asc_to_rgt(ds, false);
          if      (fld->id == W_REF and not cst)
            error = "Number Only";
          else if (fld->id == W_CST and (unsigned)cst > 0xffff)
            error = "only letters A-O";
          else
      	  { Key_t kvs;  kvs.integer = si->id;
      	    wr_lock(upn);
          { Lra_t lra;
      	    Stockitem sii = (Stockitem)ix_fetchrec(upn,IX_STK,&kvs,&lra,si);
            if (sii != null)
            { if (fld->id == W_CST)
                sii->constraint = cst;
              else
              { Int spc = atoi(ds);
                sii->altref = spc;
                sii->profit = spc >> 16;
              }
      	      write_rec(upn, CL_SI, lra, (Char *)sii);
	          }
      	    wr_unlock(upn);
            if (fld->id == W_CST)
      	    { rgt_to_asc(sii->constraint, &ds[0]);
      	      put_data(ds);
      	    }
      	    error = null;
      	  }}
      	}
      	else if (cc == SUCCESS and in_range(fld->id, W_WHCH, W_RED+stk_sl))
      	{ error = ro ? "not in edit mode"
            		     : do_edit_red(upn, fld, si, whichtbl, pricetbl, ds);
      	  if (error == null and whichtbl[s_frow(fld)] == MININT)
      	    cont = 1;
      	}
      }}
      end_interpret();
    }
    ix_fini_stream(strm);
  }}}
{ Bool has_reds = (si->props & P_SR) != 0;
  Stockred_t sr_t;
  Stockred sred = (Stockred)ix_fetchrec(upn, IX_SR, &kvi, null, &sr_t);
  if ((sred != null) != has_reds)
  { wr_lock(upn);
  { Stockitem si = (Stockitem)ix_fetchrec(upn, IX_STK, &kvi, &lra, null);
    if (si != null)
    { if (has_reds)
      	si->props &= ~P_SR;
      else
      	si->props |=  P_SR;
      (void)write_rec(upn, CL_SI, lra, si);
    }
    wr_unlock(upn);
  }}
  freem(tbl2);
}}

#if SUGRO_PL

#define ALTI_INIT_SZ 3000
#define ALTI_INC_SZ   500

typedef struct
{ Id    id;
  Lra_t lra;
} Lra_index_t, * Lra_index;


Int  index_tbl_sz = 0;


static Lra_index bld_alt_ix_cons()
						/* write locked */
{ Int sz = ALTI_INIT_SZ;
  Int act_sz = 0;
  Lra_index tbl = (Lra_index)calloc(sz + 1, sizeof(Lra_index_t));
  if (tbl == null)
    return null;
    					/* also clear Sugro Constraints */
{ Set sugconmask = ~asc_to_rgt("BCGL", false);
  Stockitem_t s_t;
  Stockitem si;
  Lra_t  lra;
  Char * sistrm = new_stream(mainupn, CL_SI);
  if (sistrm == null)
    return null;
  
  while  ((si = (Stockitem)next_rec(sistrm, &lra, &s_t)) != null)
  { Id ar = si->profit * (256*256L) + si->altref;
  
    if (not (si->props & P_OBS) and ar != 0)
    { if (act_sz >= sz)
      { Lra_index ntbl = (Lra_index)calloc(sz + ALTI_INC_SZ + 1, sizeof(Id));
        if (ntbl == null)
          return null;
        memcpy(&ntbl[0], tbl, sz * sizeof(Id));
        sz += ALTI_INC_SZ;
        free(tbl);
        tbl = ntbl;
      }
      tbl[act_sz].id  = ar;
      tbl[act_sz].lra = lra;
      act_sz += 1;
      si->constraint &= sugconmask;
      (void)write_rec(mainupn, CL_SI, lra, si);
    }
  }
  
  fini_stream(sistrm);

  if (tbl != null)
  { tbl[act_sz].id  = MAXINT;
    tbl[act_sz].lra = 0;
    memsort((Byte *) tbl, sizeof(Lra_index_t), act_sz + 1);

    index_tbl_sz = act_sz;
  }
  return tbl;
}}




static Lra_index find_altref(Id altref, Lra_index tbl)

{ fast Vint u = index_tbl_sz - 1;
  fast Vint l = 0;
       Vint m;

  while (u >= l)      
  { m = (u + l) >> 1;
  { Id eid = tbl[m].id;

    if	    (altref < eid)
      u = m - 1;
    else if (altref > eid)
      l = m + 1;
    else
      break;
  }}

  return &index_tbl[u < l ? index_tbl_sz : m];
}



static const Char * do_read_apl(Char * fn)

{ Id    osugno = MAXINT;
  Id    sugno;
  Char  descn[25];
  Char pdescn[7];
  Int   pack;
  Cash  cost;
  Cash  ticket;
  Char  vatcode;
  Date  date;
  Char  apl[24];
  Int   constraint;
  Int   oconstraint = 0;
  Bool  amended;
  Bool  new_;
  Cash  band[3];
  Char * ln;
  Char aplline[132];

  FILE * ip = fopen(fn ,"r");
  if (ip == null)
    return "File Not Found";
  
  if (disallow(R_STK))
    return preq('V');

  ln = fgets(&ds[0], sizeof(ds)-1, ip);
  cost = 0;
  while (*++ln != 0)
    if (*ln == ',')
      ++cost;
  
  fclose(ip);

  if (cost < 10 or atol(ds) == 0)
    return "File is not an apl";

         ip = fopen(fn ,"r");
{ FILE * op = fopen("/tmp/sugcon", "w");
  if (op == null)
    return "Cannot open /tmp/sugcon";

  wr_lock(mainupn);
{ Int ix;
  Lra_index index_tbl = bld_alt_ix_cons();
  if (index_tbl == null)
  { i_log(cc, "out of space baic");
    wr_unlock(mainupn);
    return "Program error";
  }
#if 0
  for (ix = -1; ++ix < index_tbl_sz; )
  { fprintf(op, "%d\n", index_tbl[ix].id);
  }

  wr_unlock(mainupn);
  return "Generated";
#endif

  aplline[sizeof(aplline)-1] = 0;

  while (true)
  { ln = fgets(&aplline[0], sizeof(aplline)-1, ip);
    sugno = MAXINT;
    if (ln != null)
      sugno = strtol(ln, &ln, 10);
    if (sugno != osugno and osugno != MAXINT)
    { Int ix;
      Lra_index lri;
      Int ct = 0;
      for (lri = find_altref(osugno, index_tbl); lri->id == osugno; ++lri)
  				/* update the constraints */
      { Stockitem_t s_t;
        Stockitem si = (Stockitem)read_rec(mainupn, CL_SI, lri->lra, &s_t);
        if (si == null)
        { i_log(osugno, "Lost Sugro prod");
          continue;
        }
        ++ct;
        if (si->pack != pack)
        { fprintf(op, "%s Sugro Pack differs %d s.b. %d\n", 
              bc_to_asc(si->supplier, si->line, &ds1[0]), pack, si->pack);
          continue;
        }
        if (si->ticket != ticket)
        { fprintf(op, "%s Sugro ticket differs %d s.b. %d\n", 
              bc_to_asc(si->supplier, si->line, &ds1[0]), ticket, si->ticket);
          continue;
        }
        
        si->constraint |= oconstraint;
        (void)write_rec(mainupn, CL_SI, lri->lra, si);
  				/* update the CI_SR */
        for (ix = -1; ++ix <= 2; )
        { if (band[ix] == 0)
            continue;
	  
      	  sprintf(&ds[0], "(%d)", ix + 1);  
      	{ Set wh = asc_to_rgt(ds, true);
      	  Lra_t  lra;
      	  Stockred_t sr_t;
      	  Stockred sr = find_sr(si->id, wh, 0, &lra, &sr_t);
      	  if (sr == null)
          { sr_t.id    = si->id;
            sr_t.which = wh;
            sr_t.price = band[ix];
       	  { Cc cc = i_Stockred_t(mainupn, &sr_t, &lra);
      	    if (cc == OK)
      	    { fprintf(op, "Inserted %s (%d) at %d\n",
                    bc_to_asc(si->supplier, si->line, &ds1[0]), ix+1, sr->price);
  	        }
    	    }}
      	  else if (sr->price != band[ix])
      	  { Cash pr = sr->price;
  	        sr->price = band[ix];
            (void)write_rec(mainupn, CL_SR, lra, sr);
            fprintf(op, "Updated %s (%d) from %d to %d\n",
                 bc_to_asc(si->supplier, si->line, &ds1[0]), ix+1, pr, sr->price);
          }
      	}}
      }
      if (ct == 0)
        fprintf(op, "Found no Sugro Product %d\n", osugno);
      oconstraint = 0;
    }
    if (sugno == MAXINT)
      break;
    osugno = sugno;
    
    ln = skipover(ln, ',');
    (void)strpcpydelim(&descn[0], ln, sizeof(descn), ',');
    ln = skipover(ln, ',');
    (void)strpcpydelim(&pdescn[0], ln, sizeof(pdescn), ',');
    ln = skipover(ln, ',');
    pack = strtol(ln, &ln, 10);
    ln = skipover(ln, ',');
    cost = strtocash(ln);
    ln = skipover(ln, ',');
    ticket = strtocash(ln);
    ln = skipspaces(skipover(ln, ','));
    vatcode = *ln;
    ln = skipover(ln, ',');
    band[2] = strtocash(ln);
    ln = skipover(ln, ',');
    date = asc_to_date(ln);
    if (date < 0)
    { i_log(date, "Corrupt apl file"); 
      break;
    }
    ln = skipspaces(skipover(ln, ','));
    apl[1] = 0;
    apl[0] = ln[0];
    if (apl[0] == 'S')
      apl[0] = 'L';
    if (apl[0] == 'T')
      apl[0] = 'B';
    constraint = asc_to_rgt(apl, false);
    if (constraint != -1)
      oconstraint |= constraint;
    ln = skipover(ln, ',');
    amended = ln[0] != 'N' and ln[0] != ' ';
    ln = skipover(ln, ',');
    new_ = ln[0] != 'N' and ln[0] != ' ';
    ln = skipover(ln, ',');
    band[0]= strtocash(ln);
    ln = skipover(ln, ',');
    band[1] = strtocash(ln);
  }
  wr_unlock(mainupn);

  fclose(ip);
  fclose(op);
  free(index_tbl);

  return "Done";
}}}



static const Char * do_read_appl(Char * sfn_)

{ Char * sfn = skipspaces(sfn_);
  if (*sfn == 0)
  { salert("Name of APL file");
    goto_fld(W_CMD);
    sfn = sfn_;
    get_data(&sfn[0]);
  }
  strpcpy(&ds1[0], sfn, sizeof(ds1));
{ Int sl = strlen(ds1);
  while (--sl >= 0 and ds1[sl] == '\n' or ds1[sl] == '\r')
    ds1[sl] = 0;

  return do_read_apl(ds1);
}}

#endif

const const Char * do_stock()

{ stk_sl = g_rows - 7;
  today = asc_to_date("");

  if (disallow(R_WEAK))
    return preq('W');

  if (tbl1 == null)
  { tbl1 = mk_form(SIform,stk_sl);// g_rows - 7
    if (tbl1 == null)
      return iner;
    wsetover(tbl1,SI1over);
  }
{ Field fld;
  const Char * error = null;
  Bool leave = false;
  Bool refresh = true;

  while (not leave)
  { if (refresh)
    { refresh = false;
      wopen(seltbl, SI2over);
      put_choices(&seltext[0]);
    }
    if (error != null)
      alert(error);
    error = dtex;

  { Cc cc = get_any_data(&ds[0], &fld);
    vfy_lock(mainupn);

    if (cc == HALTED)
    { if (*ds == 0)
      	error = null;
    }
    else if (*strmatch(".", ds) == 0)
      leave = true;
    else
    { Char * d_ = &ds[0];
      Char ch = toupper(*d_);
	    ch = s_fid(fld) == W_SEL and ch==0 ? fld->id - W_SEL :
        	 in_range(ch, 'A', 'Z')        ? ch		: -1;
      if (in_range(ch, 'A', 'Z'))
      	++d_;
      refresh = true;
      switch (ch)
      { when 0:
      	case 'A': error = do_add_stk(mainupn, d_);
      	when 1:
      	case 'U': error = do_upd_stk(mainupn,  d_);
      	when 2:          
      	case 'B': error = do_list_stk(mainupn,	d_);
      	when 3:
      	case 4:   ch = 'K'-3+ch;
      	case 'K':
      	case 'L': error = do_prt_stk(ch, d_);
            		  if (error != null)
            		    salerth(error);
            		  error = null;
      	when 5:
      	case 'D': error = do_prt_offers(mainupn, d_);
      	when 6:
      	case 'S':
      	case 7:
      	case 'R':{ Stockitem_t pat[SL];
		               error = null;
            		   if (prep_pat_stk(mainupn, pat))
            		     if (ch == 6 or ch == 'S')
            		       do_list_srch(mainupn, pat);
            		     else
            		     { salert("Short Description");
            		       goto_fld(W_CMD);
            		       get_data(&ds[0]);
		                   error = do_prt_srch(mainupn, pat, ds);
            		     }
              	 }
      	when 8:
      	case 'P': error = do_prt_pl(mainupn, d_);
      	when 9:
      	case 'Y': error = do_upd_ana(mainupn, d_);
#if SUGRO_PL
        when 10:  
        case 'I': error = do_read_appl(&ds[1]);
#endif
      	when 'Z': error = do_reset_stock(1);
      	when 'X': error = do_reset_stock(2);
        otherwise refresh = false;
            		  error = illc;
      }
    }
  }}

  return null;
}}


int myprintf(format, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)
	Char * format;
  int p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13;
{ printf(format, p1, p2, p3, p4, p5, p6, p7, p8,p9,p10,p11,p12,p13);
  if (stkrepfile != null)
    fprintf(stkrepfile,format, p1, p2, p3, p4, p5, p6, p7, p8,p9,p10,p11,p12,p13);
}
