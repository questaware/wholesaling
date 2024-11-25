#include <stdio.h>
#include <string.h>
#include <ctype.h>
/*#include <signal.h>*/
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "version.h"
/*
#include "../env/h/env.h"
*/
#include "../h/defs.h"
#include "../h/errs.h"
#include "../h/prepargs.h"
#include "../form/h/form.h"
#include "../form/h/verify.h"
#include "../db/cache.h"
#include "../db/b_tree.h"
#include "../db/recs.h"
#include "../env/tcap.h"
#include "domains.h"
#include "schema.h"
#include "rights.h"
#include "generic.h"

extern Int lk_base;

#define private 

#define ADD_SESS 0

#define DR_CC 12

Char dbg_file_name[40] = "ilog";

extern Char * getenv();


#define W_TTY	(4 * ID_FAC)
#define W_SEM	(5 * ID_FAC)

#define W_MDB	(4 * ID_FAC)
#define W_IDB	(5 * ID_FAC)

#define W_ADD	(7 * ID_FAC)
#define W_STT	(8 * ID_FAC)
#define W_STP	(9 * ID_FAC)

#define W_AGNT	(10 * ID_FAC)
#define W_CSTMR (11 * ID_FAC)
#define W_ISTAK (12 * ID_FAC)
#define W_RFND	(13 * ID_FAC)
#define W_AFILE (14 * ID_FAC)

#define SL 15

static const Over_t SI1over [] =
  {{0,1,1,0,">"},
   {1,3,1,0, "  OSGOOD SMITH"},
   {1,3,1,14, spaces(54)},
   {1,SL+5,1,0, ssvce},
   {1,SL+5,1,23, hline(45)},
   {-1,0,0,0,null}
  };

static const Over_t SI2over [] =
  {{0,1,1,0,">"},
   {1,3,1,0, "  ADMINISTRATION"},
   {1,3,1,16, spaces(52)},
   {1,SL+5,1,0, ssvce},
   {1,SL+5,1,23, hline(45)},
   {-1,0,0,0,null}
  };

static const Over_t SI3over [] =
  {{0,1,1,0,">"},
   {1,3,1,0, "  GOODS-INWARDS"},
   {1,3,1,15, spaces(53)},
   {1,SL+5,1,0, ssvce},
   {1,SL+5,1,23, hline(45)},
   {-1,0,0,0,null}
  };

static const Fieldrep_t SI1form [] =
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,   60,  1, ""},
   {W_CMD,   FLD_WR,	   1, 1,    20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD,1,30,    24,  1, ""},
   {W_SEM,   FLD_RD,	   1,55,    20,  1, "who"},
   {W_TTY,   FLD_RD,	   2,55,    20,  1, "tty"},
   {W_SEL,   FLD_WR+FLD_S+FLD_BOL,
			   4,10,    30, SL, "Actions"},
   {W_ADD,   FLD_WR,    SL+6,0,     68,  1, "input"},
   {0,0,0,0,0,1,null}
  };

#define SL2 12

const Fieldrep_t SIselform [] =
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20, 60,  1, ""},
   {W_CMD,   FLD_WR,	 1, 1, 30,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,33, 30,  1, ""},

   {W_SEL,   FLD_WR+FLD_BOL+FLD_S,
			    4, 10,  32,   SL2, "Actions"},
   {0,0,0,0,0,1,null}
  };

static const Over_t TI2over [] =
{{0,1,1,0,">"},
 {1,3,1,0,  spaces(40)},
 {0,4,1,1, "Main database"},
 {0,5,1,1, "Invoices database"},
 {0,9,1,1, "use: tar xv8"},

 {1,15,1,0, "----- Historic Data "},
 {1,15,1,20, hline(20)},
 {-1,0,0,0,null}
};

static const Fieldrep_t TI2form [] =
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,      60,  1, ""},
   {W_CMD,   FLD_WR,	    1, 1,      20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,30,      30,  1, ""},
   {W_MDB,   FLD_WR,	    4,20,      20,  1, "Filename"},
   {W_IDB,   FLD_WR,	    5,20,      20,  1, "Filename"},
   {0,     0,      0, 0,  0,  1, null}
  };

static const Over_t SI6over [] =
{{0,1,1,0,">"},
 {1,3,1,0,  spaces(40)},
 {0,4,1,1, stdt},
 {0,5,1,1, "End Date"},
 {0,8,1,1, "Agent"},
 {0,9,1,1, "Customer"},
 {0,10,1,1,"All Takings"},
 {0,11,1,1,"Refunds Only"},
 {0,13,1,1,"Audit File"},
 {1,15,1,0, "----- Audit Trail "},
 {1,15,1,18, hline(22)},
 {-1,0,0,0,null}
};

static const Fieldrep_t SI6form [] =
  {{W_MSG+1, FLD_RD+FLD_MSG,0,20,      60,  1, ""},
   {W_CMD,   FLD_WR,	    1, 1,      20,  1, comd},
   {W_MSG,   FLD_RD+FLD_FD, 1,30,      30,  1, ""},
   {W_STT,   FLD_WR,	    4,20,      20,  1, stdt},
   {W_STP,   FLD_WR,	    5,20,      20,  1, "End Date"},
   {W_AGNT,  FLD_WR,	    8,20,      20,  1, "Agent"},
   {W_CSTMR, FLD_WR,	    9,20,      20,  1, "Customer No"},
   {W_ISTAK, FLD_WR,	   10,20,      20,  1, "All Takings"},
   {W_RFND,  FLD_WR,	   11,20,      10,  1, "Refunds"},
   {W_AFILE, FLD_WR,	   13,20,      40,  1, "Audit File"},
   {0,0,0,0,0,1,null}
  };

       const Char nulls[1024];

       Char * this_prog;

       Char maindb[40];
       Char invdb[40];

       Tabled seltbl;
static Tabled tbl1;      /* The table above */

       Sale_t     this_sale;
       Acc_con_t  ths;

       Takdescr_t tak;
       Bankdescr_t bak;
       Bagsdescr_t bags;

       Int isbrk;	/* initial sbrk(0) */
       
       Int e_id_opt;
       Int j_opt;
       Int b_opt;
       Char* r_opt;
       Char* r_opt_1;
       Char* r_opt_2;

static const Char * op1text[] =
 { " 0 -- Commit",
   " 1 -- Customers",
   " 2 -- Stock",
   " 3 -- Invoices",
   " 4 -- Returns",
   " 5 -- Payments",
   " 6 -- Accounts",
   " 7 -- Rounds",
   " 8 -- Takings",
   " 9 -- Till",
   "10 -- Sales",
   "11 -- Interest",
   "12 -- Debts",
   "13 -- Admin",
   "14 -- Goods-In",
   null
 };

static const Char * op2text[] =
 { " 0 -- Constants",
   " 1 -- Backup",
   " 2 -- Restore",
   " 3 -- Printers",
   " 4 -- Unix",
   " 5 -- Journal",
   " 6 -- Browse File",
   " 7 -- Edit File",
   " 8 -- List File",
   " 9 -- Delete File",
   "10 -- Users",
   "11 -- Data Control",
   "12 -- Show Barcode",
#if ADD_SESS
   "13 -- Add session",
#endif
   null
 };

static const Char * op3text[] =
 { " 0 -- Suppliers",
   " 1 -- Order / Accept Goods",
   " 2 -- Accept Invoice",
   " 3 -- Accept Delivery Note",
   " 4 -- Supplier Accounts",
   null
 };

private Char * do_gin()

{ const Char * error = null;
  Bool leave = false;
  Bool refresh = true;

  while (not leave)
  { if (refresh)
    { refresh = false;
      wopen(tbl1, SI3over);
      put_choices(op3text);
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
    { refresh = true;
      switch (get_fopt(ds, fld))
      { when 0: error = do_partys(IX_SUP);
      	when 1: error = do_order(mainupn);
      	when 2: error = do_invoices(K_GIN + K_DUE);
      	when 3: error = do_invoices(K_GIN+K_TT);
      	when 4: error = do_accounts(K_GIN);
      	otherwise  refresh = false;
      }
    }
  }}

  return null;
}

private const Char * do_journal()

{ Date stt = 0;
  Date stp = today;
  Id agnt = 0;
  Id custmr = 0;
  Bool istak = false;
  Bool rfnds = false;
  const Char * error = null;
  Lra_t lra;
  Key_t kvac;
  Char sthis_at[100];
  Tabled tbl2 = mk_form(SI6form,0);
  wopen(tbl2, SI6over);
  fld_put_data(W_AFILE, this_at);
  goto_fld(W_STT);
  sthis_at[0] = 0;

  while (true)
  { if (error != null)
      alert(error);
    error = null;

  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);
    Int fld_id = fld->id;

    if      (cc != OK)
      error = dtex;
    else if (in_range(fld->id, W_STT, W_STP))
    { Date date = asc_to_date(ds);
      error = date > 0 ? null : illd;
      if (error == null)
      	if (fld_id == W_STT)
      	  stt = date;
      	else
      	  stp = date;
    }
    else if (fld_id == W_RFND)
    { Char * ds_ = skipspaces(ds);
      rfnds = *ds_ != 0 and toupper(*ds_) != 'N';
    }
    else if (fld_id == W_CMD)
    { error = "List";
      if      (*strmatch("LIST", ds) == 0)
      { ipfld();
	      error = at_prt_sorp(K_SALE+K_PAYMENT+K_EXP+(istak ? K_TAKE : 0),stt,stp,
				                    agnt, custmr, istak, rfnds);
      }
      else if (*strmatch(".", ds) == 0)
      	break;
      else if (*ds == 0)
      	error = null;
    }
    else if (fld_id == W_AGNT)
    { kvac.string = skipspaces(ds);
      rd_lock(mainupn);
    { Agent ag = (Agent)ix_srchrec(mainupn, IX_AGNTNM, &kvac, &lra);
      if (ag == null)
      	error = ntfd;
      else
      	agnt = ag->id;
      rd_unlock(mainupn);
    }}
    else if (fld_id == W_CSTMR)
    { kvac.integer = atoi(ds);
    { Customer cu = (Customer)ix_srchrec(mainupn, IX_CUST, &kvac, &lra);
      if (cu == null)
      	error = ntfd;
      else
      	custmr = kvac.integer;
    }}
    else if (fld_id == W_ISTAK)
    { Char * ds_ = skipspaces(ds);
      istak = *ds_ != 0 and toupper(*ds_) != 'N';
    }
    else if (fld_id == W_AFILE)
    { if (sthis_at[0] == 0)
      	strpcpy(&sthis_at[0], this_at, sizeof(sthis_at));

      strcpy(&this_at[0], skipspaces(ds));
    }
  }}

  if (sthis_at[0] != 0)
  { strcpy(&this_at[0], sthis_at);
  }

  freem(tbl2);
  return null;
}

private const Char * do_restore()

{ const Char * error = null;
  Tabled tbl = mk_form(TI2form,0);
  wopen(tbl, TI2over);
  form_put(T_DAT+W_MDB, maindb,
           T_DAT+W_IDB, invdb, 0);

  while (true)
  { if (error != null)
      alert(error);
    error = null;

  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);
    Int fld_id = fld->id;

    if      (cc != OK)
      error = dtex;
    else if (fld->id == W_CMD)
    { if (*strmatch(".", ds) == 0)
        break;
    }
    else if (fld->id == W_MDB or fld->id == W_IDB)
    { Id nupn = init_db(ds, F_EXISTS /*+F_CHECK*/);
      if (nupn < 0)
      { alert(nupn == INVALID ? "Not a DB file" : ntfd);
      	put_data(fld->id == W_MDB ? maindb : invdb);
      }
      else
      { if (fld->id == W_MDB)
      	{ chan_unlink(mainupn);
      	  mainupn = nupn;
      	  strpcpy(&maindb[0], ds, sizeof(maindb));
      	}
      	else
        { chan_unlink(invupn);
      	  invupn = nupn;
      	  strpcpy(&invdb[0], ds, sizeof(invdb));
      	}
        upn_ro = true;
      }
    }
  }}

  freem(tbl);
  return null;
}


private const Char * do_show_code(void)

{ 
  salert("Scan Barcode");
  goto_fld(W_ADD);   get_data(&ds[0]);
  
{ static char res[132];
  sprintf(res, "Barcode is %s\n", ds);
  return res;
}}

#if ADD_SESS

private Char * do_add_session()

{ if (disallow(R_PRT))
    return "Permit P required";

  salert("Which tty?");
  goto_fld(W_ADD);   get_data(&ds[0]);
{ Char tty[20]; sprintf(&tty[0], &"/dev/tty%s"[ds[0] == '/' ? 8 : 0], ds);

{ Prt_t prtarea[MAX_ADSZ];
  Lra_t lraarea[MAX_ADSZ];
  load_prtrs(prtarea, lraarea);

  (void)select_io(ds, 0);
  if (this_prt.path[0] == 0)
    return ntfd;

{ extern Byte prt_io_ix;      /* from agent.c */
  Int * ttym = tty_map();
  
  if (true)
/*if ((*ttym & (1 << prt_io_ix)) == 0)*/
  { sprintf(&ds[0], "%s %s %s %d&", this_prog, tty, this_prt.tcap, prt_io_ix);
    if (do_osys(ds) != OK)
      return "Failed";
    i_log(prt_io_ix, "+ttym %x", *ttym);
    *ttym |= (1 << prt_io_ix);
    return null;
  }

  i_log(prt_io_ix, "!ttym %x", *ttym);
  return "In use";
}}}}

#endif

private const Char * do_admin()

{ const Char * error = null;
  Bool leave = false;
  Bool refresh = true;

  while (not leave)
  { if (refresh)
    { refresh = false;
      wopen(tbl1, SI2over);
      form_put(T_DAT+W_SEM, this_agent.name,
               T_DAT+W_TTY, this_tty, 0);
      
      put_choices(op2text);
    }

    if (error != null)
      alert(error);
    error = null;

  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);
    vfy_lock(mainupn);

    if (cc == HALTED)
      ;
    else if (*strmatch(".", ds) == 0)
      leave = true;
#if 0
    else if (*strmatch("showdi", ds) == 0)
    { sprintf(&ds1[0], "csh %s", ds);
      error = do_osys(ds1) == OK ? null : "Failed";
      refresh = true;
    }
#endif
    else
    { refresh = true;
      switch (get_fopt(ds, fld))
      { when 0: error = do_uniques();
      	when 1: error = do_backup();
            		salerth(error);
            		error = null;
        when 2: error = do_restore();
      	when 3: error = do_printers();
      	when 4: error = disallow(R_SYS) ? "Permit U required" :
          			do_osys(NULL) != OK 		? "Failed"		   			: null;
      	when 5: error = do_journal();
      	when 6: error = do_access_files(P_VW, W_ADD);
      	when 7: error = do_access_files(P_ED, W_ADD);
      	when 8: error = do_access_files(P_PT, W_ADD);
      	when 9: error = do_access_files(P_DL, W_ADD);
      	when 10:error = do_partys(IX_AGNT);
      	when 11:return this_prog;
      	when 12:error = do_show_code();
#if ADD_SESS
      	when 13:error = do_add_session();
#endif
    /*  when 12:  raw_unlock(mainupn, 8); */
      	when 14:  if (this_agent.id == 1)
            		  { alert("Perform Stock Analysis, Y/N?");
            		    get_data(&ds[0]);
            		    if (toupper(*ds) == 'Y')
            		      do_stkstats(mainupn, true);
            		  }
      	when 15:  do_reset_stock(0);
#if 0
      	when 16:{ extern Int an_n;
            		  extern Double an_a, an_b;
            		  extern Double an_rate;
            		  Date stoday = today;
            		  Stockitem_t si_t;
            		  extern FILE * stkrepfile;
		
            		  if (stkrepfile != null)
            		    fclose(stkrepfile);
            		  stkrepfile = fopen("/tmp/pjsfig", "w");

            		  uniques.lst_anal = today;
            		  get_data(&ds[0]);
            		  sscanf(ds, "%hd %d %f %f",
          				&si_t.cumdev, &an_n, &an_a, &an_b);
            		  si_t.props    = 0;
            		  si_t.cumdev   = 0;
            		  si_t.stock    = 0;
            		  si_t.sell_stt = today - an_n;
            		  si_t.sig_v    = rnd(dbl(an_n) *
                        					(an_a + dbl(an_n-1) * an_b / 2.0));
            		  si_t.sig_vt   = rnd(dbl(an_n * (an_n-1)) *
            			            		(an_a/2.0 + dbl(2*an_n-1) * an_b/6.0));
            		  while (true)
            		  { analyse(&si_t);
            		    myprintf("\n\n\r%hd %hd %ld N %d A %f B %f -- %f\n\r",
            				si_t.cumdev, si_t.sig_v, si_t.sig_vt,
            				an_n, an_a, an_b, an_rate);
            		    get_data(&ds[0]);
            		    sscanf(ds, "%hd", &si_t.week_sale);
            		    if (si_t.week_sale == -1)
            		      break;
            		    tteeop();
            		  { Date dayson = 7;
            		    today += dayson;
            		    add_stkstats(&si_t);
            		    uniques.lst_anal = today;
            		  }}
            		  today = stoday;
            		}
       	when 17:  alert("Acc for BFWD upd");
            		  get_data(&ds[0]);
              	  ths.acct.id = atoi(ds);
            		  if (this_agent.id == 1)
            		  { wr_lock(mainupn);           
            		  { Cc cc = load_acc(mainupn, ths.acct.id, &acc, &ths.acct,
                    		  						0, Q_UPDATE);
            		    if (cc != OK)
            		      error = ntfd;
            		    wr_unlock(mainupn);
            		  }}
#endif
#if ACCTXT
      	when 18:{ Lra_t lra;
            		  Char * strm = ix_new_stream(mainupn, IX_CACC, null);
            		  Account ac;
      	          (void)ta_init_rdwr(1);
              	  wr_lock(mainupn);
            		  while ((ac=(Account)ix_next_rec(strm, &lra,&ths.acct)) != null)
            		  { Cc cc = load_acc(mainupn, ths.acct.id, &acc, &ths.acct,
                        		  				0, Q_UPDATE);
            		    printf("\n\rAcc %d", ths.acct.id);
#if 0
            		    Cc cc = write_text_acc(ac->id);
            		    if (cc != OK)
            		      i_log(34, "WTA %d", ac->id);
#endif
            		  }
            		  wr_unlock(mainupn);
            		  ix_fini_stream(strm);
            		}
      	when 19:
#if 0
            		 if (this_agent.id == 1)
            		 { alert("Really Read Accs Y/N");
            		   get_data(&ds[0]);            
            		   if (toupper(*ds) == 'Y')           
            		   { Cc cc;
            		     (void)ta_init_rdwr(0);
            		     wr_lock(mainupn);            
            		     while ((cc = read_text_acc()) == OK)
            		     { Lra_t lra = 0;           
            		       Cc ccc = i_Account_t(mainupn, &ths.acct, &lra);
            		       Cc ccs = save_acct(mainupn, &acc);
            		     }
            		     wr_unlock(mainupn);
            		   }
            		 }

      	when 20: { Int now = raw_unlock(mainupn, 1);
            		   printf("\n\rNow %d\n\r", now); hold();
            		 }
#endif
#endif
#if TAKTXT
      	when 21:{ Lra_t lra;
            		  Char * strm = ix_new_stream(mainupn, IX_TAKE, null);
#if 1
            		  wr_lock(mainupn);
#else
      	          (void)tt_init_rdwr(1);
            		  rd_lock(mainupn);
#endif
            		  while (ix_next_rec(strm,&lra,&this_take) != null)
            		  { Cc cc = write_text_tak(this_take.id);
              	    if (cc != OK)
            		      i_log(34, "WTT %d", this_take.id);
            		  }
#if 1
            		  wr_unlock(mainupn);
#else
            		  rd_unlock(mainupn);
#endif
            		  ix_fini_stream(strm);
            		}
      	when 22:{ Lra_t lra;
            		  Char * strm = ix_new_stream(mainupn, IX_BANK, null);
            		  wr_lock(mainupn);

            		  while (ix_next_rec(strm,&lra,&this_bank) != null)
            		    ;

            		  wr_unlock(mainupn);
            		  ix_fini_stream(strm);
            		}
      	when 23:
            		 if (this_agent.id == 1)
            		 { alert("Really read Taks Y/N");
            		   get_data(&ds[0]);
            		   if (toupper(*ds) == 'Y')
            		   { Cc cc;
            		     (void)tt_init_rdwr(0);
            		     wr_lock(mainupn);
            		     while ((cc = read_text_tak()) == OK)
            		       ;
            		     wr_unlock(mainupn);
            		   }
            		 }
#endif
        when 24: { Char repnm[65];
                   alert("Name of report");
            		   get_data(&repnm[0]);

                   alert("Type R to report Sales");
            		   get_data(&ds[0]);
            		   if (ds[0] != 'R')
                     error = "Abandoned";
                   else
                     error = do_report_sales(repnm,0,0);
                 }
      	otherwise  error = dtex;
            		   refresh = false;
      }
    }
  }}

  return null;
}

private Cc do_top()

{ Cc cc = OK;
  fast const Char * error = null;
       Bool leave = false;
       Bool refresh = true;

  while (not leave)
  { if (refresh)
    { refresh = false;
      init_journal(j_opt == 0 ?  false : M_DUMMY);
      wopen(tbl1, SI1over);
      form_put(T_DAT+W_SEM, this_agent.name,
               T_DAT+W_TTY, this_tty, 0);

      put_choices(op1text);
    }

    if (error != null)
      alert(error);
    error = dtex;

  { Field fld;
    Cc cc = get_any_data(&ds[0], &fld);
    vfy_lock(mainupn);

    if (cc == HALTED)
      error = null;
    else
      if (*strmatch(".", ds) == 0)
      	leave = true;
      else
      { refresh = true;
      	switch (get_fopt(ds, fld))
      	{ when 0:   error = do_commit(Q_UNLOCKED);
                    error = null;
      	  when 1:   error = do_partys(IX_CUST);
      	  when 2:   error = do_stock();     
      	  when 3:   error = do_invoices(K_SALE);
      	  when 4:   error = do_invoices(K_SALE + K_ANTI);
      	  when 5:   error = do_sho_paymt();     
      	  when 6:   error = do_accounts(0);
      	  when 7:   error = do_rounds();
      	  when 8:   error = do_takings();
            		    leave = true;
      	  when 9:   error = do_till();
      	  when 10:  error = do_overall();
      	  when 11:  error = do_run_int();
      	  when 12:  error = do_debts();
      	  when 13:  error = do_admin();
            		    if (error == this_prog)
            		      return DR_CC;
      	  when 14:  error = do_gin();
          when 15:  error = do_accounts(K_HGRAM);
          when 20:  error = strcmp("peter",this_agent.name) != 0 ? null 
                                                        : make_dummy_sales(5000);
                  { char buff[132];
                    rd_lock(mainupn);
                    sprintf(buff, "Top sale %d", last_id(mainupn, IX_SALE) );
                    rd_unlock(mainupn);
                    alert(buff);
                  }
#if 0
      	  when 16:{ Int x = IX_stt + salerth("letter?") - 'A';
            		    if (in_range(x, IX_stt, IX_end))
            		      dumptree(mainupn, x);
            		  }
          when 17:  printf("IData %d %d\r\n", sbrk(0) - isbrk, tot_malloc());
          	  	    hold();
#endif
      	  otherwise refresh = true;
      	}
      }
    if (leave)
      leave = salerth("Press CR to end the session") == A_CR;
  }}
  return cc;
}

static const Char sttmsg [] = "\tPress CR or . then CR";

private Cc do_main()

{ Cc cc = OK;

  if (r_opt == NULL && e_id_opt == 0)
  { printf(sttmsg);
    get_b_str(50, false, &maindb[0]);
    if (strcmp(maindb, ".") == 0)
      return -1;
  }

  strcpy(&maindb[0], maindb[0] == '=' ? &maindb[1] : DBFILEM);
  sprintf(&dbg_file_name[0], "%slog", maindb);
  sprintf(&invdb[0], "%si", maindb);

{ Int users = init_cache(true);
  if (users < 0)
  { printf("Too many users, Any Key\n"); hold();
    return ANY_ERROR;
  }
  i_log(users, ".users");

{ Set props = F_SYNC + F_WRITE + F_IXOPT + F_WRIX + F_EXISTS;

  mainupn = init_db(maindb, props);
  if (mainupn < 0)
  { i_log(mainupn, "database access failure");
    return -4;
  }

  invupn = init_db(invdb, props);
  if (invupn < 0)
  { invupn = init_db(invdb, (props & ~F_EXISTS) | (F_NEW+F_NEWIX));
    i_log(2, "All invoice bodies lost!!");
    sleep(1);
  }
  if (invupn < 0)
  { i_log(1, "inv db access");
    return -5;
  }

  if (b_opt != 0)
  { frame_blks(mainupn, CL_ACC);
  }

  if      (r_opt != NULL)
  { Bool is_date = r_opt_2 != NULL && strlen(r_opt_2) > 5;
    Int end_date = r_opt_1 == NULL ? 0 : asc_to_date(r_opt_1);
    Int of_days = r_opt_2 == NULL ? 0 :
                  is_date         ? asc_to_date(r_opt_2) - end_date + 1 :
                                    atoi(r_opt_2);
    if (do_report_sales(r_opt, end_date, of_days) != 0)
      printf("Cannot find file %s\n", r_opt);
  }
  else if (e_id_opt != 0)
    (void)print_eo_ids();
  else
  while (true)
  { putc(A_FF, stdout);
    if (do_login(mainupn, ds))
    { Lra_t lrau; (void)find_uniques(mainupn, &lrau);
      strpcpy(&this_at[0], uniques.at_path, LPATH + 1);
      if (today - uniques.lst_anal >= 7)
      { printf("\012\015Stock analysis to be done, Press CR\012\015");
        	if (hold() == A_CR)
        	  do_stkstats(mainupn, true);
      }
      cc = do_top();
    }
    if (cc != OK)
      break;
    printf(sttmsg);
    if (hold() == '.' and hold() != '.')
      break;
  }

  chan_unlink(invupn);
  chan_unlink(mainupn);
  fini_log(false);
  fini_cache();
  return cc;
}}}

void explain()

{ fprintf(stderr, "book [ -j ] [-R] [ db ]\n"
                  "  -j                      -- Dont use trail\n"
                  "  -R file [ date days ]   -- Report Sales\n");
  fprintf(stderr, " Built " __DATE__ "\n");
  fprintf(stderr, " Shared memory at %x\n", lk_base);
  exit(0);
}


void main(argc,argv)
	Int argc;
	Char ** argv;
{ Char * srcfn = null;
  Char * io_ix_str = "-1";
  Int io_ix = -1;
  Int ttt = asc_to_date("");
  int argsleft = argc - 1;
  Char ** argv_ = &argv[1];
  Char * user = getenv("PWD");
  lk_base += strcmp(user,"/home/osdb") == 0  ?  0 :
             strcmp(user,"/home/osdb2") == 0 ? 20 :
             *strmatch("/home/peter",user) == 0 ? 40 : 60;

  for (; argsleft > 0 and argv_[0][0] == '-'; --argsleft)
  { Char * flgs;
    for (flgs = &argv_[0][1]; *flgs != 0; ++flgs)
      if      (*flgs == 'j')
        j_opt = 1;
      else if (*flgs == 'b')
        b_opt = 1;
      else if (*flgs == 'E')
        e_id_opt = 1;
      else if (*flgs == 'R')
      { if (argsleft > 0)
      	{ --argsleft;
      	  if (argsleft <= 0)
      	    explain();      

      	  r_opt = *++argv_;
      	  --argsleft;
      	  --argsleft;
      	  if (argsleft > 0)
      	  { r_opt_1 = *++argv_;
      	    r_opt_2 = *++argv_;
      	  }
      	}
      }
      else
        explain();
    ++argv_;
  }

//lineix = 0;
//linect = 0;
  
{ Char * srcfn = argsleft <= 0 ? null : argv_[0];

  this_prog = argv[0];
  this_pid = getpid();
  isbrk = (Int)sbrk(0);

  if (srcfn != NULL)
  { setpgrp();
  { FILE * si = freopen(srcfn, "r", stdin);
    if (si == null)
      exit(0);
    if (strncmp(srcfn, "/dev/", 5) ==0)
    { freopen(srcfn, "w", stdout);
      freopen(srcfn, "w", stderr);
    }

    this_ttype = null;
    if (argsleft > 1) this_ttype = argv_[1];
    if (argsleft > 2) io_ix_str = argv[2];
    io_ix = atoi(io_ix_str);
  }}

  if (this_ttype == null)
    this_ttype = getenv("TERM");

  setup_sig(null);
  ps_init();
  malloc_init();

  if (r_opt == NULL && e_id_opt == 0)
  {	init_form();
		tbl1 = mk_form(SI1form,0);
		seltbl = mk_form(SIselform,0);
	  putc(A_FF, stdout);
  	tcapopen(this_ttype);
    ttignbrk(0);
  }
{ Cc cc = do_main();

  if (io_ix >= 0)
  { Int * ttym = tty_map();
    if (ttym != null)
    { i_log(io_ix, "-ttym %x", *ttym);
      *ttym &= ~(1 << io_ix);
    }
  }

  form_fini();
  tcapclose();
  if (cc == OK)
  { const Char * error = do_commit();
//  if (error != null)
//    printf("\n%s\n", error);
    sleep(2);
  }

  exit(cc);
}}}
