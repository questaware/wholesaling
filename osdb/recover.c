#include  <stdio.h>
#include  <ctype.h>
#include  <errno.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <fcntl.h>
#include  <sys/ipc.h>
#include  <sys/msg.h>
#include "version.h"
#include "../h/defs.h"
#include "../h/errs.h"
#include "../h/prepargs.h"
#include "../form/h/form.h"
#include "../form/h/verify.h"
#include "../db/cache.h"
#include "../db/page.h"
#include "../db/b_tree.h"
#include "../db/recs.h"
#include "domains.h"
#include "schema.h"
#include "rights.h"
#include "generic.h"
#include "trail.h"

extern Char * getenv();
extern Cc softfini();

#define private 

const Char nulls[1024];

Char dbg_file_name[50];
Id invupn;

#define W_TTY   (4 * ID_FAC)
#define W_SEM   (5 * ID_FAC)

#define W_BU   (6 * ID_FAC)

#define W_ADD   (7 * ID_FAC)
#define W_STT   (8 * ID_FAC) /* contiguity */
#define W_STP   (9 * ID_FAC)

#define W_AGNT  (10 * ID_FAC)
#define W_CSTMR (11 * ID_FAC)

#define P_ED 1
#define P_PT 2
#define P_DL 4
#define P_VW 8

static Over_t SI1over [] = 
  {{0,1,1,0,">"},
   {1,3,1,0, " DATA CONTROL          "},
   {1,3,1,23, spaces(42)},
   {1,19,1,0, "------- Select service "},
   {1,19,1,23, hline(42)},
   {-1,0,0,0,null}
  };

static Over_t SI6over [] = 
{{0,1,1,0,">"},
 {1,3,1,0, spaces(52)},
 {0,5,1,0, " Audit File"},
 {0,8,1,0, " Agent"},
 {0,9,1,0, " Customer"},
 {1,15,1,0, "--------- Audit Trail -"},
 {1,15,1,18, hline(30)},
 {-1,0,0,0,null}
};

static Fieldrep_t SI6form [] = 
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,      60,  1, ""},
   {W_CMD,   FLD_WR,        1, 1,      20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,30,      30,  1, ""},
   {W_BU,    FLD_WR,        5,18,LPATH,1, "backup device"},
   {W_AGNT,  FLD_WR,        8,20,      20,  1, "Agent"},
   {W_CSTMR, FLD_WR,        9,20,      20,  1, "Customer No"},   
   {0,     0,      0, 0,  0,  1, null}
  };

static Over_t SI7over [] = 
{{0,1,1,0,">"},
 {1,3,1,0, spaces(52)},
 {0,5,1,0, " Before Date:"},
 {1,9,1,0, ""},
 {1,9,1,34, hline(14)},
 {0,11,1,3, ""},
 {-1,0,0,0,null}
};

#define SI7HDR 3
#define SI7DCN 5

static Fieldrep_t SI7form [] = 
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,   60,1, ""},
   {W_CMD,   FLD_WR,        1, 1,   20,1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,30,   30,1, ""},
   {W_STP,   FLD_WR,        5,18,LDATE,1, "date"},
   {0,     0,      0, 0,  0,  1, null}
  };

#define SL2 13

Fieldrep_t SIselform [] = 
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,      60, 1, ""},
   {W_CMD,   FLD_WR,        1, 1,      30, 1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,33,      30, 1, ""},
   {W_SEM,   FLD_RD,        1,53,      20, 1, "who"},
   {W_TTY,   FLD_RD,        2,53,      20, 1, "tty"},

   {W_SEL,   FLD_WR+FLD_BOL+FLD_S,4, 10, 40, SL2,"Actions"},
   {0,       0,      0, 0,  0,  1, null}
  };

       Char maindb[40];
       Char invdb[40];

Sale_t this_sale;		/* to resolve the symbol */

       Tabled seltbl;
static Tabled tbl1, tbl2;

static Char * op1text[] = 
 { " 0 -- Recovery",
   " 1 -- Edit File",
   " 2 -- Trim Customers",
   " 3 -- Trim Invoices",
   " 4 -- Remove Unreferenced Stock",
   " 5 -- Trim Accounts",
   " 6 -- Delete Banks and Takings",
   " 7 -- Remove Unreferenced Transactions",
   " 8 -- Reindex Database",
   " 9 -- Reload Account",
   "10 -- Find Invoices w/o Accs",
   "11 -- Find free stock",
   "12 -- Find Invoices w/o Banks",
   null
 };


int save_extinv() {};
Char * do_accounts() {};

public Char * reason_invo()

{ return null;
}


public void init_top()
	
{ tbl1 = mk_form(SIselform,0);
  tbl2 = mk_form(SI6form,0);
}

					/* synthesises this_take */
					
private void ensure_tak(tid, agid, date)
	 Id    tid, agid;
	 Date  date;
{ Key_t kvi;   kvi.integer = tid;
{ Lra_t lra;
  Taking take = (Taking)ix_fetchrec(mainupn, IX_TAKE, &kvi, &lra, &this_take);
  if (take != null)
  { if (take->agid != agid and agid != ADMIN_AGT)
      i_log(tid, "Take Agent is %d, sb %d", take->agid, agid);
    return;
  }
  this_take = null_sct(Taking_t);
  this_take.next    = 0;
  this_take.id	    = tid;
  this_take.tdate   = date;
  this_take.agid    = agid;
  this_take.version = 1;
  lra = 0;
{ Cc cc = i_Taking_t(mainupn, &this_take, &lra);
  if (cc != OK)
    i_log(cc , "Tak %d consistency", tid);
  else
    prt_fmt("New takings (%d)\n", tid);
}}}


undo_invo()

{ 
}


private raddb(cid)

{ 
}

private const Char * at_apply(forreal, all)
	 Bool    forreal, all;
{  		  /* Entries can be repeated in the audit trail
      		     the later ones changing the meaning of the earlier ones */
  Char fmt[150], str[150];
  Char * chead, * lines;
  Char * row_fmt_s;
  if (this_agent.id != 1 and forreal)
    return ymba;
  if (forreal)
  { salert("To perform recovery, Type Y");
    if (toupper(hold()) != 'Y')
      return aban;
  }
 chead =
 " Type       Id  Customer Take    Date       Vat     Total  Cheque-No  Balance\n"
 "------------------------------------------------------------------------------\n";
 row_fmt_s = 
 "%1l    %7r    %6d    %5d   %5d       %9r     %8v       %9v       %9d       %9v\n";
  lines =   "----------------------------------          ------------";
  sprintf(&fmt[0], "%s\n%s\n%s\n\n", lines, " Backup (%r) %46!%r", lines);
  fmt_data(&msg[0], fmt, "Journal entries: * => lost", todaystr);
{ Prt chan = select_prt(0);
  if (chan == null)
    return aban;
{ Char * err = prt_one_report(chan, msg, " RECOVERY ITEMS\n\n\n\n\n\n\n\n\n\n\n");
  if (err != null)
    return err;
}}
  prt_set_cols(chead, row_fmt_s);
  prt_head();

{ Int ic = noo_incidents();
  Int  badct = 0;
  Int ct = 0;

  Lra_t lra; 
  Unique un = find_uniques(mainupn, &lra);

  Short last_bu = un->backupct;
  Short next_bu = last_bu;
  Bool  newer = all;
  Accdescr_t acc;
  Char * ioptr;
  Sorp  sorp;
  Cc cc;
  Fildes f = open_read_at(this_at);
  if (f < 0)
    return "Not an Audit File";

  if (bad_ptr(un))
    return "Corruption in uniques";

  init_recover();

  while ((cc = rd_nxt_rec(f, (Char**)&sorp)) != NOT_FOUND)
  { Quot kind = sorp->sale.kind;

    if      (cc != OK)
    { badct += 1;
      continue;
    }
    else if (kind == 0)
    { next_bu = sorp->sale.id;
      newer = next_bu >= last_bu;
      continue;
    }
    else if (not newer)
      continue;

    this_agent.id = 1;			/* might have been modified !! */
    wr_lock(mainupn);
    if (kind & (K_SALE + K_PAYMENT))
    { Lra_t bulra;
      Cash bal = 0;
      Key_t kvi;
      Id cid = 0;
      Char * errmsg;
      Char * status = " ";
      this_take.id = sorp->sale.agent;
      kvi.integer = sorp->sale.id;
    { Squot ix = kind & K_PAYMENT ? IX_PMT  :
      	         kind & K_SALE    ? IX_SALE : IX_EXP;
      Sorp_t act_t;
      Sorp busorp = (Sorp)ix_fetchrec(mainupn, ix, &kvi, &bulra, &act_t);
      
      kvi.integer = sorp->sale.customer;
      while (true)
      { Customer ccust = kvi.integer == 0 
     		          ? null : (Customer)ix_srchrec(mainupn, IX_CUST, &kvi, &lra);
      	if (ccust != null)
      	{ if (bill_alt(ccust->terms == -1))
      	  { kvi.integer = ccust->creditlim;
      	    continue;
      	  }
      	  cid = kvi.integer;
      	}
      	else
      	{ prt_text("Customer no longer exists for following entry\n");
      	  status = "*";
      	}
      	break;
      }

      if      (kind & K_SALE)
      { Sale sa = (Sale)sorp;
      	if (forreal)
      	{ Quot takkind = kind & ~K_TBP;

      	  if (busorp != null)
      	  { Taking_t stake;
      	    Tak teny;
      	    Lra_t tlra = find_tak_t(&stake, &teny, sa->agent, kind, sa->id);
      	    takkind = tlra <= 0 ? kind : (teny->kind & ~ K_TBP);
      	    if      (cid != 0 and (sa->kind & K_DEAD))
      	    { prt_text("Sale Deleted\n");
      	      if (busorp->kind & K_DEAD)
      	        prt_text("No change\n");
      	      else
      	      { busorp->kind |= K_DEAD;
      	        if (not (busorp->kind & K_CASH))
      	          (void)sorp_to_account(Q_UPDATE, cid, IX_SALE, sa->id, 
      	  	 		    sa->kind & K_ANTI ? sa->total : -sa->total);
      	      }
      	    }
      	    else if (cid != 0 and (sa->kind & K_CASH))
      	    { if (busorp->sale.kind & K_CASH)
      	        prt_text("No change\n");
      	      else
      	      { prt_text("Cash applied\n");
      	        takkind |= K_CASH;
      	        sa->kind |= K_PAID;
      	        sorp_to_account(Q_UPDATE, cid, IX_SALE, sa->id,
      	  	 		    sa->kind & K_ANTI ? -sa->total : sa->total);
      	      }
      	    }
      	    if (busorp->sale.agent == sa->agent)
      	    { teny->kind = takkind;
      	      if (tlra != 0)
            		(void)write_rec(mainupn, CL_TAKE, tlra, &stake);
      	    }
      	    else
      	    { Quot etakkind;
              Id tid = busorp->sale.agent;
              if (tid < 0x8000)
                tid += 0x10000;
      	    { Cc cc = item_from_tak(mainupn, tid,
                                    busorp->sale.kind, 
                                    busorp->sale.id, &etakkind);
      	      if (cc != OK)
      	        prt_fmt("Existing Sale %d not on taking (%d)", sa->id, cc);
      	    }}
            write_rec(mainupn, CL_SALE, bulra, sorp);
      	  }
      	  else
      	  { Cc cc = i_Sale_t(mainupn, sa, &bulra);
      	    if (cc != SUCCESS)
      	      prt_fmt("Rec: i_Sale_t failed(%d)", cc);
      	    else 
      	    { if (cid != 0)
      	      { sorp_to_account(0, cid, IX_SALE, sa->id,
                	  			      kind & K_CASH ? 0          :
      	               		      kind & K_ANTI ? -sa->total : sa->total);
            		if (kind & K_CASH)
      	        { prt_text("Cash to be applied\n");
      	          takkind &= ~K_TBP;
      	          takkind |= K_CASH+K_PAID;
      	        }
      	      }
      	    }
      	  }
      	  if (busorp == null or busorp->sale.agent != sa->agent)
      	  { ensure_tak(sa->agent, ADMIN_AGT, sa->date);
      	  { Cc cc = item_to_tak(sa->agent, Q_SUBMITV, takkind, sa->id);
      	    if (cc != OK)
      	      prt_fmt("Failed to Write %d to S Takings %d (%d)", 
      	      				sa->id, sa->agent, cc);
      	  }}
      	}
      { Char d[15];  date_to_asc(&d[0], sa->date, 0);
       	prt_row(0, status,
       	        kind & K_ANTI ? kind & K_CASH ? "CREFUND" : "RETURN" :
                kind & K_CASH ? "CSALE"   : "BOOK",
                                sa->id, 
                sorp->sale.customer, sa->agent, d,
                (sa->vattotal*sa->vat0_1pc+500)/1000, sa->total, sa->chq_no, 
                sa->balance);
       	++ct;
      }}
      else if (kind & K_PAYMENT)
      { Paymt pt = (Paymt)sorp;
      	if (forreal)
      	{ Tak te = null;
      	  Quot takkind = kind & ~K_TBP;
      	  Bool done = false;
      	  Cc cc;
      	  if (busorp == null)
      	    cc = i_Paymt_t(mainupn, pt, &bulra);
      	  else
      	  { Taking_t stake;
      	    Tak teny;
      	    Lra_t tlra = find_tak_t(&stake, &teny, pt->agent, kind, pt->id);
      	    if (tlra > 0)
      	    { done = (teny->kind & K_TBP) == 0;
      	      teny->kind &= ~K_TBP;
      	      (void)write_rec(mainupn, CL_TAKE, tlra, &stake);
      	    }
      	    cc = write_rec(mainupn, CL_PMT, bulra, pt);
      	  }
      	  if (cc != OK)
      	    prt_fmt("Rec: i_Paymt_t failed(%d)\n", cc);
      	  else 
      	  { if (te == null)
      	    { ensure_tak(pt->agent, ADMIN_AGT, pt->date);
      	      cc = item_to_tak(pt->agent, Q_SUBMITV-1, takkind, pt->id);
      	      if (cc != OK)
      	        prt_text("Takings Entry cannot be created\n");
      	    }
      	    if (done)
      	      prt_text("No Change\n");
      	    else
      	      do_commit_rev(mainupn, cid, bulra, pt);
      	  }
      	  if (cc != OK)
      	    status = "*";
      	}
      { Char d[15];  date_to_asc(&d[0], pt->date, 0);
      	prt_row(0, status, 
    	          (kind & K_ANTI  ? (kind & K_UNB   ? "BANK-CH" : "RTN-CHQ") :
	              (kind & K_UNB   ? "DISCT" : 
    	                            pt->chq_no < 0 ? "CASH"  : "CHEQUE")),
        	      pt->id, sorp->sale.customer,
          	    pt->agent, d, 0, pt->total, pt->chq_no, pt->balance);
      	++ct;
      }}

#if 0
      if (forreal and kvi.integer != 0 and not (kind & K_MOVD))
      { Account ac = (Account)ix_srchrec(mainupn, IX_CACC, &kvi, &lra);
      	if (ac != null and ac->balance != sorp->sale.balance)
      	  prt_fmt("Error --> Balance is %v\n", ths.acct.balance);
      }
#endif
    }}
    else if (kind & K_TAKE)
    { if (forreal)
      { Key_t kvi;   kvi.integer = sorp->expense.id;
      { Lra_t lra;
      	Taking ta = (Taking)ix_fetchrec(mainupn,IX_TAKE, &kvi,&lra, &this_take);
      	if (ta == null)
      	  prt_fmt("Taking %d missing", kvi.integer);
      	else
      	{ memcpy(&this_take.u5000, &sorp->expense.vat, 
        			   (Int)&this_take.eny[0] - (Int)&this_take.u5000);
      	  (void)write_rec(mainupn, CL_TAKE, lra, &this_take);
      #if 0
      	  this_take.bankid = sorp->expense.agent;
      	  this_take.date   = sorp->expense.date;
      	  this_bank.id = this_take.bankid;
      	  kvi.integer = this_bank.id;
      	  tak_to_bank(mainupn, Q_SUBMITV, &this_take);
      #endif
      	}
      }}
      sprintf(&ds[0],"You must submit Taking %d to Bank %d\n",
			sorp->expense.id, sorp->expense.agent);
      prt_text(ds);
    }
    else if (kind & K_EXP)
    { Expense exp = (Expense)sorp;
      if (forreal)
      { Lra_t  lra;
      	Key_t kvi; kvi.integer = exp->id;
      { Expense buexp = (Expense)ix_fetchrec(mainupn,IX_EXP, &kvi, &lra, null);
      	Cc cc = buexp != null ? write_rec(mainupn, CL_EXP, lra, buexp)
                  			      : i_Expense_t(mainupn, exp, &lra);
      	if (cc == OK)
      	{ ensure_tak(exp->agent, ADMIN_AGT, exp->date);
      	  cc = item_to_tak(exp->agent, Q_SUBMITV-1, K_EXP, exp->id);
      	}
      	if (cc != OK)
      	  i_log(exp->agent, "Failed to Write Exp Takings");
      }}
    { Char d[15];  date_to_asc(&d[0], exp->date, 0);
      prt_row(0, " ", "EXP", exp->id, 0, this_take.id, d, 0, exp->total, 0, 0);
    }}
    wr_unlock(mainupn);
  }
  this_agent.id = 1;
  prt_need(4);
  prt_fmt("\nNumber of Entries %d\n\n", ct);
  prt_fini(NULL);
  close(f);
  fini_recover();
  if (forreal and next_bu > last_bu)
  { wr_lock(mainupn);
    un = find_uniques(mainupn, &lra);
    if (un != null)
    { un->backupct = next_bu;
      (void)write_rec(mainupn, CL_UNIQ, lra, un);
    }
    wr_unlock(mainupn);
  }
  if (badct != 0)
    i_log(0, "In AT file %d bad records", badct); 
  ic -= noo_incidents();
  sprintf(&str[0], "Number of abnormal incidents %d", -ic);
  return ic == 0 ? null : str;
}}

private Char * do_recover()

{ Date stt = 0;
  Date stp = today;
  Id agnt = 0;
  Id custmr = 0;
  Char * error = null;
  Lra_t lra;
  Key_t kvac;

  strpcpy(&this_at[0], &uniques.at_path[0], sizeof(Path));

  wopen(tbl2, SI6over);
  goto_fld(W_BU);  put_data(this_at);

  while (true)
  { if (error != null)
      alert(error);
    error = null;

  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);

    if (fld->id == W_CMD)
    { error = "{ Do | Try } { all }";
      if      (*strmatch("DO", ds) == 0
	    or *strmatch("TRY", ds) == 0)
      { char * allstr = skipspaces(&ds[3]);
	error = at_apply(toupper(*ds) == 'D', toupper(*allstr) == 'A');
      }
      else if (*strmatch(".", ds) == 0)
	break;        
      else if (*ds == 0)
	error = null;
    }
#if 0
    else if (fld->id == W_AGNT)
    { kvac.string = skipspaces(ds);
      rd_lock(mainupn);
    { Agent ag = (Agent)ix_srchrec(mainupn, IX_AGNTNM, &kvac, &lra);
      if (ag == null)
	error = "Not found";
      else
	agnt = ag->id;
      rd_unlock(mainupn);
    }}
    else if (fld->id == W_CUSTMR)
    { kvac.integer = atoi(ds);
    { Customer cu = (Customer)ix_srchrec(mainupn, IX_CUST, &kvac, &lra);
      if (cu == null)
	error = "Not found";
      else
	custmr = kvac.integer;
    }}
#endif
    else if (fld->id == W_BU)
    { strpcpy(&this_at[0], ds, LPATH + 1);
      error = null;
    }
  }}
  return null;
}

private Char * get_stop(stp_ref, title, dscn)
	Date   * stp_ref;
	Char *   title;
	Char *   dscn;
{ Lra_t lra; 
  Tabled tbl = mk_form(SI7form,0);

  SI7over[SI7HDR].c = title;
  SI7over[SI7DCN].c = dscn;
  wopen(tbl, SI7over);

  (void)find_uniques(mainupn, &lra);
{ Date maxstp = today - uniques.grace[6];
  Char * error = null;
  Date stp = maxstp;

  goto_fld(W_STP); put_data(date_to_asc(&ds[0], stp, 0));
  
  while (true)
  { if (error != null)
      alert(error);
    error = null;

  { Field fld;
    Cc cc = get_any_data( &ds[0], &fld);
    Quot fldtyp = fld->id;
    if      (cc == HALTED)
      ;
    else if (fldtyp == W_CMD)
    { if      (*strmatch(".", ds) == 0)
      { error = aban;
	break;
      }
      else if (*strmatch("GO", ds) == 0)
        break;
      else
        salert("go");
    }
    else 
    { Date date = asc_to_date(ds);
      if      (date <= 0)
        error = illd;
      else if (date > maxstp)
      { sprintf(&ds1[0], "Must be before %s", date_to_asc(&ds[0], maxstp, 0));
        error = &ds1[0];
      }
      else
        stp = date;
      put_data(date_to_asc(&ds[0], stp, 0));
    }
  }}
  freem(tbl);
  if (error == null)
    *stp_ref = stp;
  return error;
}}

private const Char * do_trim_cmrs()

{ Date stp;
  Char * res = get_stop(&stp, "-------- Delete Old Customers ----", 
			      "delete all details on closed accounts");
  if (res != null)
    return res;
  if (not am_admin)
    return ymba;
  ipfld();
  wr_lock(mainupn);
{ Int ct = 0;
  Lra_t lra;
  Customer_t cu_t;
  Customer cu;
  Char * strm = ix_new_stream(mainupn, IX_CUST, null);  
  while ((cu = (Customer)ix_next_rec(strm, &lra, &cu_t)) != null)
  { if (cu->terms != -1 and (cu->terms & P_DLTD))
    { Cc cc = load_acc(mainupn, cu->id, &acc, &ths.acct, 0, 0);
      if (cc == OK)
      { Date lad = last_act_date(&acc);
      	if (lad != 0 and lad <= stp)
      	  continue;
      }
    { Key_t kvi; kvi.integer = cu->id;
    { Cc cc = d_Customer_t(mainupn, cu, lra);
      if (cc == OK)
      	cc = del_acc(mainupn, cu->id);
      if (cc != OK)
      	++ct;
    }}}
  }
  ix_fini_stream(strm);
  wr_unlock(mainupn);
  sprintf(ds, "Deleted %d Customers", ct);
  return ds;
}}

private const Char * do_do_trim()

{ Date stp;
  Char * err = get_stop(&stp, "------- Delete Invoice Bodies -------", 
			      "Keeps invoice totals though");
  if (err != null)
    return err;
  if (not am_admin)
    return ymba;
  ipfld();
{ Lra_t lra;
  Char * strm = ix_new_stream(mainupn, IX_SALE, null);  
  Sale sa;
  Int e_ct = 0;
  Int ct = 0;
  Int sct = 0;

  wr_lock(mainupn);
  wr_lock(invupn);
  while ((sa = (Sale)ix_next_rec(strm, &lra, null)) != null)
  { if (sa->tax_date <= stp)
    { Key_t kvi; kvi.integer = sa->id; 
    { Cc cc = ix_delete(invupn, IX_INV, &kvi, &lra);
      if      (cc != SUCCESS)
	;
      else if (del_rec(invupn, CL_INV, lra) != SUCCESS)
	++e_ct;
      else
        ++ct;
    }}
    
    if ((++sct & 0x3f) == 0)
    { wr_lock_window(mainupn);
      wr_lock_window(invupn);
      sprintf(ds, (sct & 0x7f) == 0 ? "%d DONE" : "%d Done", ct);
      salert(ds);
    }
  }
  ix_fini_stream(strm);
  wr_unlock(invupn);
  wr_unlock(mainupn);

  if (e_ct != 0)
  { i_log(e_ct, "Errors removing invoices (ct=%d)", e_ct);
    return "Database Errors";
  }
  return null;
}}

private void scan_tak(upn)
	Id    upn;
{ Int ct = 0;
  Taking_t takbuf;
  Lra_t  lra;
  wr_lock(upn);
{ Char * strm = init_rec_strm(upn, CL_TAKE);
    
  while (next_rec(strm, &lra, &takbuf) != null)
  { Taking bkt = (Taking)read_rec(upn, CL_TAKE, lra, &takbuf);
    Tak ip = takbuf.next & Q_EFLAG ? &takbuf.eny[-1-TAKHDR]
			  	   : &takbuf.eny[-1];
    if (bkt == null)
      break;

    while (++ip <= &takbuf.eny[TAKBUCKET-1])
    { Key_t kv_t; kv_t.integer = ip->id;
      if (ip->id == 0)
	 break;
      if (ip->kind == 0)
	continue;
    { Quot ix = ip->kind &  K_SALE    ? IX_SALE : 
		ip->kind & K_PAYMENT ? IX_PMT  :
		ip->kind & K_EXP     ? IX_EXP  : IX_PMT; /* say */
      Lra_t  lra_;
      Sorp_t sorp_t;
      Sorp sorp = (Sorp)ix_fetchrec(upn, ix, &kv_t, &lra_, &sorp_t);
      if (sorp == null)
	i_log(0, "scan_tak err");
      else 
      { sorp_t.kind &= ~K_NOTK;
	(void)write_rec(upn, ddict[ix].cl_ix, lra_, &sorp_t);
      }
    }}
    wr_lock_window(upn);
  }

  fini_stream(strm);
  wr_unlock(upn);
}}

#define MINSPAGE 10
Char dspmsg[20];

private Char * do_del_sp()

{ Lra_t lra;
  Sale_t sp;
  Paymt_t pp;
  Expense_t ep;
  Cc ccmax = OK;
  Int ict = 0;
  Int ct = 0;
  Char * strm = init_rec_strm(mainupn, CL_SALE);

  ipfld();
  wr_lock(mainupn);
  while (next_rec(strm, &lra, &sp) != null)
  { if (today - sp.date > MINSPAGE)
    { sp.kind |= K_NOAC + K_NOTK;
      (void)write_rec(mainupn, CL_SALE, lra, &sp);
    }
  }
  wr_lock_window(mainupn);

  fini_stream(strm);
  strm = init_rec_strm(mainupn, CL_PMT);

  while (next_rec(strm, &lra, &pp) != null)
  { if (today - pp.date > MINSPAGE)
    { pp.kind |= K_NOAC + K_NOTK;
      (void)write_rec(mainupn, CL_PMT, lra, &pp);
    }
  }
  wr_lock_window(mainupn);
  fini_stream(strm);
  strm = init_rec_strm(mainupn, CL_EXP);

  while (next_rec(strm, &lra, &ep) != null)
  { ep.kind |= K_NOTK;
    (void)write_rec(mainupn, CL_EXP, lra, &ep);
  }
  wr_lock_window(mainupn);
  fini_stream(strm);
  scan_tak(mainupn);
  scan_acc(mainupn, sa_action);
  strm = init_rec_strm(mainupn, CL_SALE);

  while (next_rec(strm, &trash, &sp) != null)
  { if (today - sp.date > MINSPAGE
     and (sp.kind & (K_NOAC+K_NOTK))==(K_NOAC+K_NOTK))
    { Cc cc = del_sale(mainupn, sp.id); 
      if (cc != OK)
      { if ((unsigned)cc > ccmax)
	  ccmax = cc;
	ict += 1;
      }
    }
  }

  wr_lock_window(mainupn);
  fini_stream(strm);
  if (ccmax != OK)
    i_log(ccmax, "Sales undeleted");
  ccmax = OK;

  strm = init_rec_strm(mainupn, CL_PMT);

  while (next_rec(strm, &trash, &pp) != null)
  { if (today - pp.date > MINSPAGE
     and (pp.kind & (K_NOAC+K_NOTK))==(K_NOAC+K_NOTK))
    { Cc cc = del_pmt(mainupn, pp.id);
      if (cc != OK)
      { if ((unsigned)cc > ccmax)
	  ccmax = cc;
	ict += 1;
      }
    }
  }

  wr_lock_window(mainupn);
  fini_stream(strm);
  if (ccmax != OK)
    i_log(ccmax, "Payments undeleted");
  ccmax = OK;
  strm = init_rec_strm(mainupn, CL_EXP);

  while (next_rec(strm, &trash, &ep) == SUCCESS)
  { if (today - ep.date > MINSPAGE
     and (ep.kind & (K_NOTK))==(K_NOTK))
    { Cc cc = del_exp(mainupn, ep.id);
      if (cc != OK)
      { if ((unsigned)cc > ccmax)
	  ccmax = cc;
	ict += 1;
      }
    }
  }

  wr_lock_window(mainupn);
  fini_stream(strm);
  wr_unlock(mainupn);
  if (ccmax != OK)
    i_log(ccmax, "Expenses undeleted");
  if (ict != 0)
    sprintf(&dspmsg[0], "%d errors", ict);  
  return ict == 0 ? null : &dspmsg[0];
}

Char * do_get_stkitem(Id stkid)

{ Char * strm = stkid != 0 ? null : init_rec_strm(mainupn, CL_SI);
  Char msg[40];
  Lra_t  lra;
  Stockitem_t si_t;
  Int treesz = 0;
  Int errct = 0;
  
  while (stkid != 0 ? true : next_rec(strm, &lra, &si_t) != null)
  { Lra_t lran;
    Key_t kvi;  
    kvi.integer = si_t.id;
    if (stkid != 0)
      kvi.integer = stkid;
    
  { Cc cc = ix_search(mainupn, IX_STK, &kvi, &lran);
    if (cc != SUCCESS or stkid == 0 and lra != lran)
    { i_log(cc, "Not in index (%d)", kvi.integer);
      errct += 1;
    }
    ++treesz;
    if (stkid != 0)
      break;
  }}
  
  i_log(errct, "Lost entries out of %d", treesz);
  return null;
}

#if 0

private Cc chk_tree(ct_ref, upn, ix)
	 Int   * ct_ref;
	 Id      upn;		/* at least read locked */
	 Id	 ix;
{ Lra_t lra;
  Char * strm = ix_new_stream(upn, ix, null);
  Cc ccr = OK;
  Int ct = 0;
  Cc cc;
  Int lastkey = MININT;
  Key_t kvi;
  while ((cc = next_of_ix(strm, &kvi, &lra)) == OK)
  { if (kvi.integer <= lastkey)
    { ccr = INVALID;
      break;
    }
    ++ct;
    lastkey = kvi.integer;
  }
  ix_fini_stream(strm);
  *ct_ref = ct;
  return ccr;
}

#endif

Id esr;

private Date earliest_stkref()

{ Date early = MAXSINT;
  Char * ustrm = init_rec_strm(mainupn, CL_SALE);
  Sale_t  sa;
  Lra_t   lra;
  Int ct = 0;
  
  rd_lock(mainupn);
  rd_lock(invupn);
  while (next_rec(ustrm, &lra, &sa) != null)
  { if (sa.date > 0 and sa.date < early)
    { Date date = sa.date;      
      Key_t kvi;  kvi.integer = sa.id;
    { Cc cc = ix_search(invupn, IX_INV, &kvi, &lra);
      if (cc == SUCCESS)
      { early = date;
	esr = sa.id;
      }
    }}
    if ((++ct & 0xff) == 0)
    { rd_unlock(invupn); rd_unlock(mainupn); rd_lock(mainupn); rd_lock(invupn);
    }
  }

  rd_unlock(invupn);
  rd_unlock(mainupn);
  fini_stream(ustrm);
  return early;
}

	                                    /* delete obsolete stock items */

private Int do_garbage(first_date, tot_ref, del_ref)
	 Date  first_date;
	 Int  *tot_ref, *del_ref;
{ Char msg[40];
  Int treesz = 0;
  Int errct = 0;
  *tot_ref = 0;
  *del_ref = 0;

  ipfld();

  wr_lock(mainupn);
{ Char * strm = ix_new_stream(mainupn, IX_STKLN, null);
  Lra_t lra;
  Stockitem_t si_t;
  Stockitem si;
  while ((si = (Stockitem)ix_next_rec(strm, &lra, null)) != null)
  { if (si->props & P_OBS)
    { i_log(si->id, "Obs s.n.b");
      si->props &= ~P_OBS;
      (void)write_rec(mainupn, CL_SI, lra, si);
    }
  }
  ix_fini_stream(strm);

  strm = init_rec_strm(mainupn, CL_SI);

  while (next_rec(strm, &lra, &si_t) != null)
  { ++*tot_ref;
    sprintf(&msg[0], "Records Examined: %d", *tot_ref);
    salert(msg);
    if ((si_t.props & P_OBS) and si_t.sale_date < first_date)
    { Int otreesz = treesz;
#if 0
      Cc chk = chk_tree(&treesz, mainupn, IX_STK);
      i_log(si_t.id, "Ct %d at %x", treesz, lra);
      if (chk != OK or otreesz != 0 and otreesz != treesz)
      { i_log(chk, "Discrepancy %d", otreesz - treesz);
	dumptree(mainupn, IX_STK);
	break;
      }
#endif
    { Lra_t lran;
      Key_t kvi;  kvi.integer = si_t.id;
    { Cc cc = ix_delete(mainupn, IX_STK, &kvi, &lran);
      if (cc != SUCCESS or lra != lran)
      { i_log(cc, "Error Cleaning Stock (%d)", kvi.integer);
	errct += 1;
      }
      treesz -= 1;
#if 1
      cc = del_rec(mainupn, CL_SI, lra);
      if (cc != SUCCESS)
      { i_log(cc, "Extra Stock Index Entry(%d)", kvi.integer);
	errct += 1;
      }
#endif
      ++*del_ref; 
    { /* Int lv = last_id(mainupn, IX_STK);
      i_log(lv, "Del Stock (%d)", kvi.integer) */;
    }}}}
    if ((*tot_ref & 0x3f) == 0) 
      wr_lock_window(mainupn);
  }
  fini_stream(strm);

{ Stockred_t sr_t;
  Int ct = 0;

  strm = init_rec_strm(mainupn, CL_SR);
  
  while (next_rec(strm, &lra, &sr_t) != null)
  { Lra_t lras;
    Stockitem_t si_t;
    Key_t kvi;  kvi.integer = sr_t.id;
  { Stockitem si = (Stockitem)b_fetchrec(mainupn, IX_STK, &kvi, &lras, &si_t);
    if (si == null or (si->props & P_OBS))
    { Cc cc = ix_delete(mainupn, IX_SR, &kvi, &lra);
      if (cc != OK)
      { i_log(cc, "Error Cleaning Reductions (%d)", kvi.integer);
	errct += 1;
      }
#if 1
      else
      { cc = del_rec(mainupn, CL_SR, lra);
	if (cc != OK)
	{ i_log(cc, "Extra Reduction Index Entry(%d)", kvi.integer);
	  errct += 1;
	}
      }
#endif
      ct += 1;
    }
  }}
  
  fini_stream(strm);
  wr_unlock(mainupn);
  return errct;
}}}

private const Char * do_clean_stk()

{ if (this_agent.id != 1)
    return ymba;
  
  while (true)
  { salert("Delete (U)nreferenced/(A)ll Stock? (U/A)");
    get_data(&ds[0]);
    ds[0] = toupper(ds[0]);
    if (ds[0] == 'U' or ds[0] == 'A')
      break;
  }

  ipfld();

{ Date early = ds[0] == 'A' ? 0 : earliest_stkref();
  Int tot, del;
  
  sprintf(&ds[0], "Clean up to %s, Type CR (Yes), n (No) or give date",
             early == MAXSINT ? "today" : date_to_asc(&ds1[0], early, 0));
  salert(ds);
  get_data(&ds[0]);

  if (ds[0] != 0)
    early = asc_to_date(ds);
  if (toupper(ds[0]) == 'N' or early < 0)
      return aban;

{ Int errct = do_garbage(early, &tot, &del);
  sprintf(ds, "%d out of %d Deleted", del, tot);
  alert(ds);
  if (errct != 0)
  { hold();
    sprintf(ds, "%d Errors Encountered", errct);
    return &ds[0];
  }

  return null;
}}}

private const Char * do_trim_accs()

{ Date stp;
  Char * res = get_stop(&stp, "---------- Trim Accounts ------------", 
			      "Remove entries before date");
  if (res != null)
    return res;
  if (not am_admin)
    return ymba;

  init_acc(&acc, &ths.acct);
  ipfld();
  wr_lock(mainupn);
{ Int ac_ct = 0;
  Int cct = 0;
  Int ct = 0;
  Lra_t lra;
  Customer_t cu_t;
  Customer cu;
  Char * strm = ix_new_stream(mainupn, IX_CUST, null);  
  while ((cu = (Customer)ix_next_rec(strm, &lra, &cu_t)) != null)
  { i_log(0, "Doing %d %s", cu_t.id, cu_t.surname);
  { Cc cc = load_acc(mainupn, cu_t.id, &acc, &ths.acct, cu_t.terms, 0);
    if (cc == OK)
    { ct += do_compact_acc(cu->id, &acc, stp);
      if ((++cct & 0x1f) == 0)
        wr_lock_window(mainupn);
    }
  }}
  ix_fini_stream(strm);
  wr_unlock(mainupn);
  sprintf(&ds[0], "Removed %d Entries", ct);
  return ds;
}}



private const Char * do_trim_acc()

{ int cid;
  Date stp;
  Char * res = get_stop(&stp, "---------- Trim Account ------------", 
			      "Remove entries before date");
  if (res != null)
    return res;
//if (not am_admin)
//  return ymba;

  while (true)
  { salert("Give Account no");
    get_data(&ds[0]);
    if (ds[0] == 0)
      return null;
    cid = atol(ds);
    if (cid != 0)
      break;
  }

  init_acc(&acc, &ths.acct);
  ipfld();
  wr_lock(mainupn);
{ Int ac_ct = 0;
  Int cct = 0;
  Int ct = 0;
  Lra_t lra;
  Customer_t cu_t;
  Customer cu;
  Key_t kvi;  kvi.integer = cid;
  cu = (Customer)b_fetchrec(mainupn, IX_CUST, &kvi, &lra, &cu_t);
  if (cu != NULL)
  { i_log(0, "Doing %d %s", cu_t.id, cu_t.surname);
  { Cc cc = load_acc(mainupn, cu_t.id, &acc, &ths.acct, cu_t.terms, 0);
    if (cc == OK)
    { ct += do_compact_acc(cu->id, &acc, stp);
    }
  }}
  wr_unlock(mainupn);
  sprintf(&ds[0], "Removed %d Entries", ct);
  return ds;
}}

public const Char * do_del_banks()

{ Date stp;
  Char * res = get_stop(&stp, "---------- Remove Bank Takings -------",
			      "Remove daily sales/revenue details");
  if (res != null)
    return res;
  if (not am_admin)
    return ymba;

  ipfld();
  wr_lock(mainupn);

{ Int bag_ct = 0;
  Lra_t lra;
  Key_t kvi;  kvi.integer = 1;
{ Char * strm = ix_new_stream(mainupn, IX_BANK, &kvi);  
  Bank bk;
  while ((bk = (Bank)ix_next_rec(strm, &lra, null)) != null)
  { if (bk->date < stp and bk->id != 0)
    { Lra_t nlra;
      for (; lra != 0; lra = nlra)
      { bk = (Bank_t *)rec_ref(mainupn, lra);
        nlra = bk == null ? 0 : (bk->next & (Q_EFLAG-1));
      { Int ix = -1;
        while (++ix <= BANKBUCKET-1 and bk->eny[ix] != 0)
        { i_log(9, "Deleting Takings %d", bk->eny[ix]);
         (void)del_tak(mainupn, bk->eny[ix]);
        }
      }}
    { Cc cc = del_bank(mainupn, bk->id);
      if (cc != OK)
	i_log(cc, "del_bank failed");
      bag_ct += 1;
    }}
    wr_lock_window(mainupn);
  }
    
  wr_unlock(mainupn);
  sprintf(ds, "Dltd: Bags %d", bag_ct);
  alert(ds);	hold();
  return null;
}}}

private const Char * do_reindex()

{ salert("Database to reindex?");
  get_data(&ds[0]);

{ Id id = chan_link(F_EXCL, ds);
  Char * res = id >= 0      ? null     : 
	       id == -1     ? ntfd     :
	       id == IN_USE ? "In use" : iner;
  if (res == null)
  { chan_unlink(id);
    ipfld();
  { Char cmdline[80]; sprintf(&cmdline[0], "./cvtdb -r %s", ds);
  { Cc cc = do_osys(cmdline);
    if (cc != OK)
    { sprintf(&ds[0], "Return Code was %d", cc);
      res = &ds[0];
    }
  }}}

  return res;
}}


					/* -1: not in the taking */
					/*  0: bank not found */
private Lra_t find_bak_t(bank_ref, eny_ref, bid, id)
	 				/* write locked */
	 Bank    bank_ref;
	 Id *  * eny_ref;
	 Id	 bid;
	 Id	 id;
{ Lra_t lra = 0;
  Key_t kv_t;  kv_t.integer = bid;
{ Bank bkt_ = (Bank)ix_fetchrec(mainupn, IX_BANK, &kv_t, &lra, bank_ref);
  if (bkt_ == null)
    return 0;

  for (; lra != 0; lra = bkt_->next & (Q_EFLAG-1))
  { bkt_ = (Bank)read_rec(mainupn, CL_BANK, lra, bank_ref);
    if (bkt_ == null)
      break;
  { Id * ip = &bkt_->eny[-1];

    while (++ip <= &bkt_->eny[BANKBUCKET-1] and *ip != 0)
      if (*ip == id)
      { *eny_ref = ip;
	return lra;
      }
  }}

  return -1;
}}



private const Char * do_find_free(Bool v_opt, Char wh)

{ Date stt_date = 0;
  Date stp_date = 0;

  while (true)
  { salert("Give Start date");
    get_data(&ds[0]);
    if (ds[0] == 0)
      return null;
    stt_date = asc_to_date(ds);
    if (stt_date > 0)
      break;
  }

  while (true)
  { salert("Give Final date");
    get_data(&ds[0]);
    if (ds[0] == 0)
      return null;
    stp_date = asc_to_date(ds);
    if (stp_date > 0)
      break;
  }

{ Char fmt[150], str[150], datestr[13];
  Char * chead, * lines, *dtstr;
  Char * row_fmt_s;
 chead =
 " Take          Cid     Id      Date      Vat     Value   Customer\n"
 "---------------------------------------------------------------------\n";
 row_fmt_s = 
 "     %8d  %4l   %5d     %7d       %9r      %8v       %9v %r\n";
  { Prt chan = select_prt(0);
    if (chan == null)
      return aban;
  { Char * err = prt_one_report(chan, "Lost Xactns\n", " LOST XACTNS\n\n\n");
    if (err != null)
      return err;
  }}
  prt_set_cols(chead, row_fmt_s);
  prt_head();

  ipfld();

{ Set dont_want = wh == 'a' ? K_DEAD+K_CASH : K_DEAD;
  Lra_t lra;
  Char * strm = ix_new_stream(mainupn, IX_SALE, null);
  Cc ccr = OK;
  Cc cc;
  Key_t kvi;
  Sale sa;
  Sale_t sa_t;
  Paymt pmt;
  Paymt_t pmt_t;

  Int ct = 0;
  Int lost = 0;
  Int noacct = 0;
  Id last_id = 0;
  Date last_date = 0;
  Cash total = 0;
  Cash totalvat = 0;

  prt_text("SALES\n");

  while ((sa = (Sale)ix_next_rec(strm, &lra, &sa_t)) != null)
  { Key_t  kv_t;  
    Cc cc;
    Bool nobk = false;
    Set terms;
    Id cid = sa->customer;
    
    if (sa->date < stt_date or sa->date > stp_date)
      continue;
    if (sa->kind & dont_want)
      continue;

    if (v_opt)
    { if (last_id != 0 and last_id+1 != sa->id)
        prt_fmt("Gap at %d (%d)\n", last_id, sa->id - last_id - 1);
      last_id = sa->id;
    }

    rd_lock(mainupn);
    if      (wh == 'a')
    { Cash bal = get_cacc(&cid, &terms, -IX_CUST);
      cc = sorp_to_account(Q_CHK, cid, IX_SALE, sa->id, 0);
    }
    else if (wh != 't')
      cc = ENOTTHERE;
    else
    { Taking_t take;
      Bank_t   bank;
      Tak teny;
      Id * x;
      Lra_t tlra = find_tak_t(&take, &teny, sa->agent, sa->kind, sa->id);
      cc = tlra > 0 ? OK        : 
           tlra < 0 ? NOT_FOUND : ENOTTHERE;
      if (cc == OK)
      { kv_t.integer = take.id;
        (void)ix_fetchrec(mainupn, IX_TAKE, &kv_t, &tlra, &take);
        tlra = find_bak_t(&bank, &x, take.bankid, take.id);
        if (tlra <= 0)
        { nobk = true;
          cc = tlra < 0 ? NOT_FOUND : ENOTTHERE;
        }
      }
    }
    if (cc != OK)
    { kv_t.integer = sa->customer;
    { Lra_t lra;
      Char * msg = wh == '.' ? "--"   :
                   nobk      ? "NOBK" : 
                   wh == 'a' ? "NOAC" : "NOTK";
      Customer_t c_t; strcpy(&c_t.surname, "Unknown");
    { Customer cu = (Customer)ix_fetchrec(mainupn, IX_CUST, &kv_t, &lra, &c_t);
      if (cu == null)
	cu = &c_t;
    { Cash val = sa->kind & K_ANTI ? -sa->total    : sa->total;
      Cash vat = sa->kind & K_ANTI ? -sa->vattotal : sa->vattotal; 
      total += val;
      totalvat += (vat * sa->vat0_1pc + 500) / 1000;

      dtstr = last_date == sa->date ? "" : date_to_asc(&ds1[0], sa->date, 0);
     
      if (cc == ENOTTHERE)
      { ++noacct;
      }
      else if (ths.acct.bfwd_date <= sa->date)
      { ++lost;
      	msg = nobk ? "TOST" : "LOST";
      }
      prt_row(0, sa->agent, msg, cid, sa->id, dtstr, vat, val, cu->surname);
    }}}}
    rd_unlock(mainupn);
    ++ct;
  }
  ix_fini_stream(strm);
  if (wh != '.')
    prt_fmt(wh == 'a' ? "\nNo Acc %d, Lost %d; out of %d\n"
                      : "\nNo (Bank/Take) %d, Lost %d; out of %d\n", noacct, lost, ct);
  prt_fmt(  "Total Vat %v, Total %v\n", totalvat, total);
  
  if (wh == 'a')
  { prt_text("PAYMENTS\n");
    strm = ix_new_stream(mainupn, IX_PMT, null);
    
    ct = 0;
    lost = 0;
    noacct = 0;
    last_id = 0;
    last_date = 0;
    total = 0;
    
    while ((pmt = (Paymt)ix_next_rec(strm, &lra, &pmt_t)) != null)
    { Set terms;
      Id cid = pmt->customer;
      
      if (v_opt)
      { if (last_id != 0 and last_id+1 != pmt->id)
      	  prt_fmt("Gap at %d (%d)\n", last_id, pmt->id - last_id - 1);
      	last_id = pmt->id;
      }
  
      if (pmt->date < stt_date)
      	continue;
      if (pmt->kind & K_DEAD)
      	continue;
      if (cid == 0)
      	continue;
  
      rd_lock(mainupn);
    { Cash bal = get_cacc(&cid, &terms, -IX_CUST);
      Cc cc = sorp_to_account(Q_CHK, cid, IX_PMT, pmt->id, 0);
      if (cc != OK)
      { Key_t  kv_t;  kv_t.integer = pmt->customer;
      { Lra_t lra;
      	Char * msg = "NOAC";
      	Customer_t c_t; strcpy(&c_t.surname, "Unknown");
      { Customer cu = (Customer)ix_fetchrec(mainupn, IX_CUST, &kv_t, &lra, &c_t);
      	if (cu == null)
      	  cu = &c_t;
      { Cash val = pmt->kind & K_ANTI ? -pmt->total : pmt->total; 
      	total += val;
      	dtstr = last_date == pmt->date ? "" : date_to_asc(&ds1[0], pmt->date, 0);
             
      	if (cc == ENOTTHERE)
      	{ ++noacct;
      	}
      	else if (ths.acct.bfwd_date <= pmt->date)
      	{ ++lost;
      	  msg = "LOST";
      	}
      	prt_row(0, pmt->agent, msg, cid, pmt->id, dtstr, 0, val, cu->surname);
      }}}}
      rd_unlock(mainupn);
      ++ct;
    }}
    ix_fini_stream(strm);
    prt_fmt("\nNo Acc %d, Lost %d; out of %d\n", noacct, lost, ct);
    prt_fmt(  "Total  %d\n", total);
  }
  prt_fini(NULL);
  return "Done";
}}}

				/* readopt takings */
private Char * do_readopt(doit)
	 Bool doit;
{ Lra_t lra;
  Key_t kvi;  kvi.integer = 1;
  wr_lock(mainupn);
{ Char * strm = ix_new_stream(mainupn, IX_BANK, &kvi);  
  fast Bank bk;

  while ((bk = (Bank)ix_next_rec(strm, &lra, null)) != null)
  { while (s_page(lra) != 0)
    { fast Int ix = -1;
      Bank_t  bank_ext;
      Bank bkt = (Bank)read_rec(mainupn, CL_BANK, lra, &bank_ext);
      if (bkt == null)
      { i_log(51, "Corrupt Readopting Bank");
        break;
      }
      while (++ix <= BANKBUCKET-1 and bkt->eny[ix] != 0)
      { kvi.integer = bkt->eny[ix];
      { Taking_t tk_t;
        Taking tk = (Taking)ix_fetchrec(mainupn, IX_TAKE, &kvi, &lra, &tk_t);
        if (tk == null)
        { i_log(kvi.integer, "Lost Takings");
        }
        else if (tk->bankid == 0)
        { i_log(bk->id, doit ? "Re-adopt %d" : "ZTakId %d", tk->id);
          tk->bankid = bk->id;
          if (doit)
	    (void)write_rec(mainupn, CL_TAKE, lra, tk);
        }
      }}

      lra = bkt->next & (Q_EFLAG-1);
    }
  }

  ix_fini_stream(strm);
  wr_unlock(mainupn);
  return "Done";
}}

Char * do_test()

{ Lra_t lra;
  Key_t kvi;  kvi.integer = 1;
  rd_lock(mainupn);
{ Char * strm = ix_new_stream(mainupn, IX_BANK, &kvi);  
  fast Bank bk;
  while ((bk = (Bank)ix_next_rec(strm, &lra, null)) != null)
  { i_log(bk->id, "Bid");
  }

  ix_fini_stream(strm);
  rd_unlock(mainupn);

  return "Done";
}}

private Char * do_t_scoop() forward;
private Char * do_t_check_taks() forward;

private Char * do_dump_blk()

{ Id tid;
  
  while (true)
  { salert("Give page no");
    get_data(&ds[0]);
    if (ds[0] == 0)
      return null;
    tid = atol(ds);
    if (tid > 0)
      break;
  }
  
{ Byte * p = page_ref(mainupn, tid);
  dump_blk(p, 16);
  return null;
}}



private Cc do_top()

{ Char * error = null;
  Bool leave = false;
  Bool refresh = true;

  while (not leave)
  { if (refresh)
    { refresh = false;
      wopen(tbl1, SI1over);
      form_put(T_DAT+W_SEM, this_agent.name,
               T_DAT+W_TTY, this_tty, 0);
      put_choices(op1text);
    }
  
    if (error != null)
      alert(error);
    error = dtex;

  { Field fld;
    Cc cc = get_any_data( &ds[0], &fld);
    vfy_lock(mainupn);
    
    if (cc == HALTED)
      error = null;
    else
      if (*strmatch(".", ds) == 0)
	leave = salerth("Press CR to end the session") == A_CR;
      else
      { refresh = true;
	switch (get_fopt(ds, fld))
	{ when 0: error = do_recover();
	  when 1: error = do_access_files(P_ED, W_CMD);
	  when 2: error = do_trim_cmrs();
	  when 3: error = do_do_trim();
	  when 4: error = do_clean_stk();
	  when 5: error = do_trim_accs();
	  when 6: error = do_del_banks();
	  when 7: error = do_del_sp();
	  when 8: error = do_reindex();
	  when 9: error = do_reloadacc();
	  when 10:error = do_find_free(false, 'a');
	  when 11:error = do_get_stkitem(0);
	  when 12:error = do_find_free(false, 't');
	  when 13:error = do_find_free(true, 'a');
	  when 14:error = do_readopt(false);
	  when 15:error = do_readopt(true);
	  when 16:error = do_test();
	  when 17:unzap();
	          error = "Unzapped"; 
	  when 18:error = do_find_free(true, '.');
	  when 19:error = do_t_scoop();
	  when 20:error = do_t_check_taks();
	  when 21:error = do_dump_blk();
	  when 22:error = do_trim_acc();
	}
      }
  }}
  
  return OK;
} 




mygdb()
{ printf("GDB");
}



#define MAX_SUBSIDS 300

private Bool in_subsids(subsids, sz, cid)
	 Id *   subsids;
	 Int    sz;
	 Id     cid;
{ for ( ; --sz >= 0; )
   if (subsids[sz] == cid)
     return true;

  return false;
}


private Cc do_rebuild_acc(id, bal, bfwd_date, bfwd_paid)
        Id     id;
	Cash   bal;			/* brought forward */
	Short  bfwd_date;		/* brought forward date */
	Bool   bfwd_paid;
{ Id * subsids = (Id*)malloc(MAX_SUBSIDS * sizeof(Id));
  Int slim = 0;
  Char * strm_c = ix_new_stream(mainupn, IX_CUST, null);
  Lra_t  lra_c;
  Customer_t  c_s;
  Customer  cust;
  
  subsids[0] = id;

  while ((cust = (Customer)ix_next_rec(strm_c, &lra_c, &c_s)) != null)
  { if (bill_alt(cust->terms))
    { Id cid = cust->creditlim;
      if (cid == id)
      { if (++slim >= MAX_SUBSIDS)
	{ i_log(id, "Too many subsid accounts");
	  return OUT_OF_MEMORY;
	}
	subsids[slim] = cust->id;
	i_log(cust->id, "Subsid %d %s\n\r", cust->id, cust->surname);
      }
    }
  }
  ix_fini_stream(strm_c);

  while (true)
  { salert("Subsids to add");
    get_data(&ds[0]);
    if (atoi(ds) == 0)
      break;
    if (++slim >= MAX_SUBSIDS)
    { i_log(id, "Too many subsid accounts");
      return OUT_OF_MEMORY;
    }
    subsids[slim] = atoi(ds);
  }

{ Char * strm_s = ix_new_stream(mainupn, IX_SALE, null);
  Char * strm_p = ix_new_stream(mainupn, IX_PMT, null);
  Lra_t lra_s, lra_p;
  Sale_t  s_s;
  Paymt_t p_s;
  Sale sale = null;
  Paymt pay = null;
  Bool do_s = true;
  Bool do_p = true;
  Int date;
  Id last_sale = 0;

  Int sale_ct = 0;
  Int pay_ct = 0;

  Accdescr ac = &acc;

  Int diag_lim = 3;

  init_acc(ac, &ths.acct);
  acc.id = id;
  ths.acct.id = id;

  if (bal != 0)
  { fast Accitem item = extend_acc(ac, &ac->c[ac->curr_last]);
    item->props   = K_BFWD + (bfwd_paid == 0 ? 0 : K_PAID);
    item->id	  = 0;
    item->idate   = bfwd_date;
    item->val     = 0;
    item->balance = bal;
  }

  while (do_s | do_p)
  { while (do_s and sale == null)
    { sale = (Sale)ix_next_rec(strm_s, &lra_s, &s_s);
      if      (sale == null)
      { i_log(0, "Last sale %d", last_sale);
        do_s = false;
      }
      else 
      { last_sale = sale->id;
      /* printf("S%d ", sale->id / 1000); */
      /*if (sale->id == 182642)
          mygdb();*/
        if      (not in_subsids(subsids, slim+1, sale->customer))
          sale = null;
        else if (sale->date < bfwd_date)
        { printf("early %d %d", sale->id, sale->date);
          sale = null;
        }
        else if (sale->kind & K_DEAD)
        { printf("dead %d %d", sale->id, sale->date);
          sale = null;
        }
        else
          printf("\n\rSale %d C %d\n\r", sale->id, sale->customer);
      }
    }
    while (do_p and pay == null)
    { pay = (Paymt)ix_next_rec(strm_p, &lra_p, &p_s);
      if      (pay == null)
        do_p = false;
      else 
      { /*printf("P");*/
        if (pay->customer != id or pay->date < bfwd_date)
          pay = null;
        else if (pay->kind & K_DEAD)
        { printf("dead %d %d", pay->id, pay->date);
          pay = null;
        }
        else
          printf( "\n\rPmt %d C %d\n\r", pay->id, pay->customer);
      }
    }
    /*printf("+");*/

    date = 0x7fff;
    if (sale != null)
      date = (unsigned)sale->date;
    if (pay != null and (unsigned)pay->date < date)
      date = (unsigned)pay->date;
    
    if      (sale != null and (unsigned)sale->date <= date)
    { Accitem item = extend_acc(ac);
      if ((sale->kind & K_CASH) == 0)
        if (sale->kind & K_ANTI)
          bal -= sale->total;
        else
          bal += sale->total;
      item->props   = sale->kind & (K_SALE+K_CASH+K_ANTI);
      item->id	    = sale->id;
      item->idate   = date;
      item->val     = sale->total;
      item->balance = bal;
      ++sale_ct;
      if (--diag_lim >= 0)
        i_log(bal, "Sale %d %d %ld", sale->id, sale->total, bal);
      sale = null;
    }
    else if (pay != null and (unsigned)pay->date <= date)
    { Accitem item = extend_acc(ac);
      if (pay->kind & K_ANTI)
        bal += pay->total;
      else
        bal -= pay->total;
      item->props   = pay->kind & (K_PAYMENT+K_ANTI);
      item->id	    = pay->id;
      item->idate   = date;
      item->val     = pay->total;
      item->balance = bal;
      ++pay_ct;
      if (--diag_lim >= 0)
        i_log(bal, "Pmt %d %d %ld", pay->id, pay->total, bal);
      pay = null;
    }
  }

  ac->bal = bal;

  ix_fini_stream(strm_s);
  ix_fini_stream(strm_p);

{ Char msg[50];
  i_log(id, "Acc reloaded");
  printf("Sales %d, Payments %d, Bal %d\n", sale_ct, pay_ct, bal);
  /* salert(msg); */

  sleep(20);

  return save_acct(mainupn, ac);
}}}




private unzap()

{ Char * strm_p = ix_new_stream(mainupn, IX_PMT, null);
  Lra_t lra_p;
  Paymt_t p_s;
  Paymt pay = null;
  
  wr_lock(mainupn);
    
  while ((pay = (Paymt)ix_next_rec(strm_p, &lra_p, &p_s)) != null)
  { if (pay->kind == 0)
    { pay->kind = 1278;
      (void)write_rec(mainupn, CL_PMT, lra_p, pay);
      i_log(pay->id, "Corrected Pmt");
    }
  }
  
  wr_unlock(mainupn);

  ix_fini_stream(strm_p);
}



private Char * do_reloadacc()

{ Int  cid;
  Int  bfwd = 0;
  Date bfwd_date;

  if (this_agent.id != 1)
    return ymba;

  while (true)
  { salert("Give Account no");
    get_data(&ds[0]);
    if (ds[0] == 0)
      return null;
    cid = atol(ds);
    if (cid != 0)
      break;
  }

  while (true)
  { salert("Give Total bfwd in pence");
    get_data(&ds[0]);
    if (ds[0] == 0)
      return null;
    if (ds[0] == '0' and ds[1] == 0)
      break;
    bfwd = atol(ds);
    if (bfwd != 0)
      break;
  }

  while (true)
  { salert("Give Bfwd date");
    get_data(&ds[0]);
    if (ds[0] == 0)
      return null;
    bfwd_date = asc_to_date(ds);
    if (bfwd_date > 0)
      break;
  }

  while (true)
  { salert("Press G to proceed");
    get_data(&ds[0]);
    if (ds[0] == 0 or
        toupper(ds[0]) != 'G')
      return "Abandoned";
    break;
  }

{ Cc cc = do_rebuild_acc(cid, bfwd, bfwd_date, false);
  return cc == OK ? "Done" : "Failed";
}}

static const Char sttmsg [] = "\n\rPress CR or . then CR";


Char * this_prog;

void main (argc,argv)
	Int argc;
	Char ** argv;
{ Char * srcfn = null;
  Char * io_ix_str;
  Int io_ix = -1;
  this_prog = argv[0];
  this_pid = getpid();

  if (moreargs(argc,argv,&srcfn))
  { setpgrp();
  { FILE * si = freopen(srcfn, "r", stdin);
    if (si == null)
      exit(0);
    if (strncmp(srcfn, "/dev/", 5) ==0)
    { freopen(srcfn, "w", stdout);
      freopen(srcfn, "w", stderr);
    }
    
    (void)moreargs(argc,argv,&this_ttype);
    (void)moreargs(argc,argv,&io_ix_str);
    io_ix = atoi(io_ix_str);
  }}
  
  if (this_ttype == null)
    this_ttype = getenv("TERM");
  if (ARGS('N') == MININT)
    tcapopen(this_ttype);
  setup_sig(null);
  init_top();
  init_form();

{ static Char pw[20] = "UNIX";
  Char * epw = getenv("DBPW");
  if (epw != null)
    strpcpy(&pw[4], epw, 15);

  putc(A_FF, stdout);

  while (true)
  { printf(sttmsg);
    get_b_str(50, false, &maindb[0]);
    putc(A_FF, stdout);
    if (strcmp(maindb, ".") == 0)
      break;

    strcpy(&maindb[0], maindb[0] == '=' ? &maindb[1] : DBFILEM);
    sprintf(&invdb[0], "%si", maindb);
    if (dbg_file_name[0] == 0)
    { sprintf(&dbg_file_name[0], "%slog", maindb);
    { Char ch;
      Int users = init_cache(softfini);
      if (users < 0)
      { printf("Too many users, Any Key\n");    read(0, &ch, 1);
        exit(0);
      }
    }}      
  { Set props = F_SYNC + F_WRITE + F_IXOPT + F_WRIX +
		             (ARGS('I') == MININT ? F_EXISTS : F_NEW+F_NEWIX);
    mainupn = init_db(maindb, props);
    if (mainupn < 0)
    { i_log(mainupn, "database access failure");
      break;
    }

    invupn = init_db(invdb, props);
    if (invupn < 0)
    { invupn = init_db(invdb, (props & ~F_EXISTS) | (F_NEW+F_NEWIX));
      i_log(2, "All invoice bodies lost!!");
      if (invupn < 0)
        i_log(1, "inv db access");
    }
#if 0
    i_log(0, "Start Lock trial");
   { Int ct;
     for (ct = 1000; --ct >= 0; )
     { cache_lock();
       cache_unlock();
     }
   }
    i_log(0, "End Lock trial");
#endif
    
    putc(A_FF, stdout);
    if (do_login(mainupn, ds))    
    { Lra_t lrau; (void)find_uniques(mainupn, &lrau);
    { Cc cc = do_top();
    }}

    chan_unlink(invupn);
    chan_unlink(mainupn);
    fini_log(false);
  }}
  if (dbg_file_name[0] != 0)
    fini_cache();
  if (io_ix >= 0)
  { Int * ttym = tty_map();
    *ttym &= ~(1 << io_ix);
  }
  tcapclose();
  exit(maindb[0] != '.' ? 0 : 9);
}}



				/* similar to item_to_tak() */

public Cc item_opt_to_tak(tid, kind, id)
				      /* write locked */
	 Id   tid;
	 Kind kind;
	 Id   id;
{ Key_t kv_t; kv_t.integer = tid;
{ Lra_t flra = 0;
       Taking_t ta_t;
  fast Taking ta = (Taking)ix_fetchrec(mainupn, IX_TAKE, &kv_t, &flra, &ta_t);
       Lra_t thislra = flra;
       Lra_t nextlra;

  for (; ta != null; ta = (Taking)read_rec(mainupn, CL_TAKE, thislra, &ta_t))
  { nextlra = ta_t.next & (Q_EFLAG-1);
    if (ta_t.id != kv_t.integer)
    { i_log(ta_t.id, "Mixed Takes %d %d %d", kv_t.integer, kind, id);
      return -777;
    }

  { fast Tak ip = thislra == flra ? &ta_t.eny[-1] : &ta_t.eny[-TAKHDR-1];
  	 Tak ip_ = ip;
	 Tak iz = &ta_t.eny[TAKBUCKET-1];
    
    while (++ip <= iz)
      if (((ip->kind ^ kind) & (K_PAYMENT+K_SALE+K_EXP)) == 0 and
          ip->id   == id
         )
        return HALTED;
        
    if (nextlra == 0)
    { /*i_log(thislra, "at end");*/
      for (ip = ip_; ++ip <= iz and ip->id != 0; )
        ;
      while (--ip > ip_ and ip->kind == 0)	    /* recover dead enries */
        ;
  
      if (++ip <= iz)
      { ip->kind = kind;
	ip->id	 = id;
	if (++ip <= iz)
	  ip->id = 0;
        ta_t.id = kv_t.integer;
	(void)write_rec(mainupn, CL_TAKE, thislra, &ta_t);
	return SUCCESS;
      }
      
      break;
    }
    thislra = nextlra;
  }}

{ Cc cc = OK;
  Taking_t take;  take = null_sct(Taking_t);
  take.id = tid;
  if (flra == 0)
  { take.tdate	     = today;
    take.agid	     = ADMIN_AGT;
    take.version     = this_take.version;
    take.eny[0].kind = kind;
    take.eny[0].id   = id;
    cc = i_Taking_t(mainupn, &take, &flra);
    i_log(take.id, "Created taking");
    return cc != OK ? cc : NOT_FOUND;
  }
  else
  { Lra_t lra;
    take.next			  = Q_EFLAG;
    ((Takeext)&take)->eny[0].kind = kind;
    ((Takeext)&take)->eny[0].id   = id;
    cc = new_init_rec(mainupn, CL_TAKE, &take, &lra);
    ta_t.next = (ta_t.next & Q_EFLAG) + lra;
    (void)write_rec(mainupn, CL_TAKE, thislra, &ta_t);
    return cc;	
  }
}}}


private Cc do_tak_scoop(Date from_date)

{ wr_lock(mainupn);
{ Char * strm_s = ix_new_stream(mainupn, IX_SALE, null);
  Char * strm_p = ix_new_stream(mainupn, IX_PMT, null);
  Char * strm_e = ix_new_stream(mainupn, IX_EXP, null);
  Lra_t lra_s, lra_p, lra_e;
  Sale_t  s_s;
  Paymt_t p_s;
  Expense_t e_s;
  Sale sale = null;
  Paymt pay = null;
  Expense exp = null;
  Bool do_s = true;
  Bool do_p = true;
  Bool do_e = true;
  Int date = MAXINT;
  Id id, tid;
  Kind kind;
  Char * dscn = "???";

  Int sale_ct = 0;
  Int pay_ct = 0;
  Int exp_ct = 0;

  while (true)
  { while (do_s and sale == null)
    { sale = (Sale)ix_next_rec(strm_s, &lra_s, &s_s);
      if      (sale == null)
      { i_log(0, "Last sale");
        do_s = false;
      }
      else 
      { if      (sale->date < from_date or sale->agent == 0)
        { /*printf("early %d %d", sale->id, sale->date); */
          sale = null;
        }
      }
    }
    while (do_p and pay == null)
    { pay = (Paymt)ix_next_rec(strm_p, &lra_p, &p_s);
      if      (pay == null)
        do_p = false;
      else 
      { if      (pay->date < from_date or pay->agent == 0)
          pay = null;
      }
    }
    while (do_e and exp == null)
    { exp = (Expense)ix_next_rec(strm_e, &lra_e, &e_s);
      if      (exp == null)
        do_e = false;
      else 
      { if      (exp->date < from_date or exp->agent == 0)
          exp = null;
      }
    }

    date == MAXINT;
    if (sale != null)
      date = sale->date;
    
    if (pay != null and pay->date < date)
      date = pay->date; 
    if (exp != null and exp->date < date)
      date = exp->date; 

    if      (sale != null and sale->date <= date)
    { id = sale->id;
      kind = sale->kind;
      tid = sale->agent;
      dscn = "Sale";
      sale = null;
      ++sale_ct;
    }
    else if (pay != null and pay->date <= date)
    { id = pay->id;
      kind = pay->kind;
      tid = pay->agent;
      dscn = "Pmt";
      ++pay_ct;
      pay = null;
    }
    else if (exp != null and exp->date <= date)
    { id = exp->id;
      kind = exp->kind;
      tid = exp->agent;
      dscn = "Exp";
      ++exp_ct;
      exp = null;
    }
    if (not (do_s | do_p | do_e))
      break;
  { Cc cc = item_opt_to_tak(tid, kind, id);
    if      (cc == OK)
      i_log(0, "Restored %s %d to Take %d\n", dscn, id, tid);
    else if (cc == NOT_FOUND)
      i_log(0, "Added %s %d to New Take %d\n", dscn, id, tid);
  }}

  ix_fini_stream(strm_s);
  ix_fini_stream(strm_p);
  ix_fini_stream(strm_e);
  wr_unlock(mainupn);

  i_log(id, "Takings  reloaded");
  printf("Sales %d, Payments %d, Exp %d\n", sale_ct, pay_ct, exp_ct);
  return OK;
}}


private Char * do_t_scoop()

{ Date from_date;
  while (true)
  { salert("Give Start date");
    get_data(&ds[0]);
    if (ds[0] == 0)
      return null;
    from_date = asc_to_date(ds);
    if (from_date > 0)
      break;
  }
  
  while (true)
  { salert("Press G to proceed");
    get_data(&ds[0]);
    if (ds[0] == 0 or
        toupper(ds[0]) != 'G')
      return "Abandoned";
    break;
  }

  return do_tak_scoop(from_date) == OK ? null : "Failed";
}



Cc put_out_tak(Taking outbuf, Lra_t * foutlra_ref, Lra_t * poutlra_ref)

{ outbuf->next = *poutlra_ref == 0 ? 0 : Q_EFLAG;
{ Lra_t lrao;
  Cc cc = new_init_rec(mainupn, CL_TAKE, outbuf, &lrao);
  if (cc != OK)
  { i_log(cc, "Err Creating Taking");
    return cc;
  }
  
  if (*foutlra_ref == 0)
    *foutlra_ref = lrao;

  if (*poutlra_ref != 0)
  { (void)read_rec(mainupn, CL_TAKE, *poutlra_ref, outbuf);
    outbuf->next = (outbuf->next & Q_EFLAG) + lrao;
    (void)write_rec(mainupn, CL_TAKE, *poutlra_ref, outbuf);
  }

  *poutlra_ref = lrao;
  return cc;
}}



private Cc pack_chk_tak(tid)
	 Id	  tid;
{ Taking_t outbuf;
  Tak      op = &outbuf.eny[-1];
  Taking_t inbuf;
  Tak      ip = &inbuf.eny[-1];
  Lra_t lra = 0;
  Lra_t fstlra;
  Lra_t foutlra = 0;
  Lra_t poutlra = 0;
  Key_t kv_t;  kv_t.integer = tid;
{ Taking bkt = (Taking)ix_fetchrec(mainupn, IX_TAKE, &kv_t, &lra, &inbuf);
  if (bkt == null)
    return NOT_FOUND;

  fstlra = lra;
  outbuf = inbuf;

  while (s_page(lra))
  { bkt = (Taking)read_rec(mainupn, CL_TAKE, lra, &inbuf);
    if (bkt == null)
      break;
  { Tak ip = inbuf.next & Q_EFLAG ? &inbuf.eny[-1-TAKHDR]
				  : &inbuf.eny[-1];

    while (++ip <= &inbuf.eny[TAKBUCKET-1] and ip->id != 0)
    { Kind kind = ip->kind;
      if (kind == 0 or (kind & K_DEAD))
        continue;
    { kv_t.integer = ip->id;
    { Quot ix = kind & K_SALE ? IX_SALE : 
		kind & K_EXP  ? IX_EXP	: IX_PMT;
      Lra_t  lra_;
      Sorp_t pay_t;
      Paymt ip_ = (Paymt)ix_fetchrec(mainupn, ix, &kv_t, &lra_, &pay_t);
      if (ip_ == null)
      { i_log(dbcc, "Missing xactn %d", kv_t.integer);
	i_log(tid, "was tid");
	continue;
      }
      else 
      { if (ix == IX_EXP ? ((Expense)ip_)->agent != tid : ip_->agent != tid)
	{ kind |= 0x2000000;
	  i_log(kind & 0xff, "alien t.entry %ld in %ld", kv_t.integer, tid);
	  continue;
	}
      }
    
      ++op;
      op->kind = kind;
      op->id = ip->id;
      if (op >= &outbuf.eny[TAKBKT-TAKHDR-1])
      { put_out_tak(&outbuf, &foutlra, &poutlra);
        op = &((Takeext)&outbuf)->eny[-1];
      }
    }}}
    lra = inbuf.next & (Q_EFLAG-1);
  }}

  if (op != &((Takeext)&outbuf)->eny[-1])
  { ++op;
    memset(op, 0, (char*)&outbuf.eny[TAKBKT-TAKHDR-1] - (Char*)op);
    put_out_tak(&outbuf, &foutlra, &poutlra);
  }

  kv_t.integer = tid;
{ Lra_t nlra;
  Cc cc = ix_replace(mainupn, IX_TAKE, &kv_t, &foutlra);
  if (cc != OK)
  { i_log(cc, "Tak replace failed");
    return cc;
  }

  for (lra = fstlra; s_page(lra) != 0; )
  { bkt = (Taking)read_rec(mainupn, CL_TAKE, lra, &inbuf);
    if (bkt == null)
      break;
    nlra = inbuf.next & (Q_EFLAG-1);
    del_rec(mainupn, CL_TAKE, lra);
    lra = nlra;
  }

  return OK;
}}}



private Char * do_t_check_taks()

{ Id tid;
  Id lid;
  
  while (true)
  { salert("Give Start taking id");
    get_data(&ds[0]);
    if (ds[0] == 0)
      return null;
    tid = atol(ds);
    if (tid > 0)
      break;
  }

  while (true)
  { salert("Give End taking id");
    get_data(&ds[0]);
    if (ds[0] == 0)
      return null;
    lid = atol(ds);
    if (lid > 0)
      break;
  }

  while (true)
  { salert("Press G to proceed");
    get_data(&ds[0]);
    if (ds[0] == 0 or
        toupper(ds[0]) != 'G')
      return "Abandoned";
    break;
  }

  wr_lock(mainupn);
{ Key_t kvi; kvi.integer = tid;
{ Char * strm_s = ix_new_stream(mainupn, IX_TAKE, &kvi);
  Lra_t lra_t;
  Taking_t  t_s;
  Taking take;
  
  while ((take = (Taking)ix_next_rec(strm_s, &lra_t, &t_s)) != null)
  { Cc cc;
    Id ttid = take->id;
    if (ttid > lid)
      break;
    cc = pack_chk_tak(ttid);
    if (cc == OK)
      i_log(ttid, "Taking packed");
    else
      i_log(ttid, "Taking pack failed (%d)", cc);
  }
  ix_fini_stream(strm_s);
  wr_unlock(mainupn);

  return null;
}}}

