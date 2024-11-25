#ifndef UCKEY    
#include "../../db/b_tree.h"
#endif

#define P_ED 1
#define P_PT 2
#define P_DL 4
#define P_VW 8

#define PDCHQACC    -1
#define BANKACC     -2
#define DISCACC     -3
#define MEACC       -4
#define LSTRESACC  -10

#define Q_UNPAID   1
#define Q_CHK      2
#define Q_RECENT   4
#define Q_UPDATE   8
#define Q_MATCH   16
#define Q_PUP     32
#define Q_HGRAM   64
#define Q_CREATE 128

#define Q_CLOSEV  (0x3fff)
#define Q_SUBMITV (0x7fff)

#define P_LITFST  1  /* display the input first */
#define P_NOSPEC  2  /* No special customers */
#define P_SUPPLR  4  /* Suppliers */
#define P_ACCONLY 8  /* Account holders only */

#define Double float
#define dbl(f) ((Double)(f))
#define rnd(f) ((int)(f+0.5))

typedef struct
{ Set    props;
  Id     id;
  Cash   vat;			/* or suggested interest */
  Cash   val;		
  Byte   grace_ix;
  Byte   match_grp; 
  short  idate;
  Int    chq_no;		/* or from_date for suggested interest */
  Cash   balance;
} Accitem_t, *Accitem;

typedef struct
{ Id	   id;
  Int	   arr_sz;
  Int	   curr_last;
  Int	   curr_base;
  Cash	   bfwd;		/* Added 3/1/00 */
  Cash	   bal;
  Int	   curr_ix;	/* set up, then used by get_bal(). Added 29/1/00 */
  Cash	   curr_bal;	/* set up, then used by get_bal(). Added 29/1/00 */
  Cash	   pdc_bal;
  Cash	   odue;
  Accitem  c;
} Accdescr_t, *Accdescr;

typedef struct
{ Set     props;    /* when entered */
  Id      id;
  Char    name[LSNAME];	/* short by one byte to save space */
  short   cid;
  Cash    vat;		
  Cash    val;
  Int     chq_no;
} Takitem_t, *Takitem;

typedef struct
{ Set     props;    /* Not used */
  Id      id ;
  Surname name;
  Cash    val;
} Bagsitem_t, *Bagsitem;


typedef struct
{ Id       id;		/* not always up to date */
  Bool     rw;
  Int      rev_sz;
  Int      rev_last;
  Int      rev_base;
  Takitem  rev_c;
  Cash     tot_money;
  Int      nchq;					 /* CONTIGUOUS STARTS */
  Cash     tot_chqs;	    /* excludes transactions with the PDCHKACC */
  Cash     tot_pdchqs;
  Cash     tot_matchqs;
  Cash     noo_sales;
  Cash     tot_sales;
  Cash     tot_vat;
  Cash     noo_rets;
  Cash     tot_rets;
  Cash     tot_vatret;
  Cash     noo_csales;
  Cash     tot_csales;
  Cash     tot_cvat;
  Cash     noo_crets;
  Cash     tot_crets;
  Cash     tot_cvatret;
  Cash     tot_expense;
  Cash     tot_expvat;
  Cash     tot_disc;
  Cash     tot_chg;
  Cash     tot_swap;
  Cash     tot_journal;
  Cash     tot_revenue;     /* ignores dates on cheques *//* CONTIGUOUS ENDS */
  Bool     modified;	    /* isomorphism required to here in the following */
  Bool     sal_mode;        /* displaying sales */
  Int      sal_sz;
  Int      sal_last;
  Int      sal_base;
  Takitem  sal_c;
} Takdescr_t, *Takdescr;

typedef struct
{ Id       id;
  Bool     rw;
  Int      rev_sz;
  Int      curr_last;
  Int      curr_base;
  Takitem  rev_c;
  Cash     tot_money;
  Int      nchq;
  Cash     tot_chqs;	    /* excludes transactions with the PDCHKACC */
  Cash     tot_pdchqs;
  Cash     tot_matchqs;
  Cash     noo_sales;
  Cash     tot_sales;
  Cash     tot_vat;
  Cash     noo_rets;
  Cash     tot_rets;
  Cash     tot_vatret;
  Cash     noo_csales;
  Cash     tot_csales;
  Cash     tot_cvat;
  Cash     noo_crets;
  Cash     tot_crets;
  Cash     tot_cvatret;
  Cash     tot_expense;
  Cash     tot_expvat;
  Cash     tot_disc;
  Cash     tot_chg;
  Cash     tot_swap;
  Cash     tot_journal;
  Cash     tot_revenue;
  Bool     modified;
} Bankdescr_t, *Bankdescr;

/* owe = .tot_revenue + tot_swap - 
         (.tot_money + .tot_chqs - .tot_pdchqs + .tot_matchqs + .tot_expense) */

typedef struct
{ Id        id;
  Bool      rw;
  Int       arr_sz;
  Int       curr_last;
  Int       curr_base;
  Bagsitem  c;
} Bagsdescr_t, *Bagsdescr;

typedef struct
{ Int       arr_sz;
  Int       curr_last;
  Int       curr_base;
  Prt       cs;
  Lra       clra;
} Prtsdescr_t, *Prtsdescr;


#if S_LINUX == 0
extern double atof();
#endif
#define freem(x) free((char*)x)

extern Int asc_to_terms(Char *);
extern Char * terms_to_asc(Int, Char *);

extern void init_journal(Set);
extern void analyse(Stockitem  si);

extern Int get_rand(void);
extern Char * get_core_code(const Char *, Char *, int *);
extern Cash get_cacc (Id *, Quot *, Id);
extern Customer get_cust (Id, Id, Id);
extern Id get_one_cust(Id, Char *, Set, void(*)(void));
extern Cc get_sname(Char *, Key_t *);
extern char * get_email(Id cid, char * email, char * uses);
extern void get_taking(/*upn, agid, always*/);
extern Cc create_account (Id, Id);
extern Cc eny_to_accs(/*pmt*/);
extern void recalc_acc __((Accdescr));
extern void match_credit(Id,Id);
extern Cash mvbal_acc __((Accdescr, Int, Int, Cash));
extern Id first_neg_id(Id,Id);
extern Date last_act_date __((Accdescr));
extern const Char * audit_acc __((Id, Id, Accdescr, Char *));
extern Accitem extend_acc(/*acc*/);
extern void free_acc __((Accdescr));
extern Cc sorp_to_account();
extern void add_cash(Bank, Taking);
extern Cc xactn_to_taking(Sorp);
extern void at_xactn(Sorp);
extern Cc item_from_tak(Id, Id, Kind, Id, Kind*);
extern void do_commit_rev(Id, Id, Lra_t, Paymt);
extern Cc load_acc(Id, Id, Accdescr, Account_t *, Set, Set);
extern Char * do_run_int(void);
extern Id last_id(Id,Id);

#define NOMATCH  (-2)
#define MATCHSET (-1)

extern Cc do_adj_intbals(Id aid, Id pid, Cash val);

extern Quot  match_amount __((Set *, Accdescr, Cash));
extern Set commit_match __((Quot, Accdescr, Set, Int, Cash));
extern Bool pay_acceny __((Id, Set, Id, Int));
extern Cc sa_action __((Quot, Lra_t));
extern void scan_acc();
extern Lra_t find_tak_t __((Taking, Tak *, Id, Quot, Id));

extern Unique find_uniques(/* upn, lra */);
extern Char * find_that_agent(/* upn, aid */);
#define s_interest_on(cu) ((cu)->interest_01 & 1)
extern double s_interest_rate(Customer cu);

extern Char * reason_invo(/*id, reason, reason_len*/);

extern Bool         do_login(Id,Char*);
extern const Char * do_partys();
extern const Char * do_stock();
extern const Char * do_invoices();
extern const Char * do_invoices();
extern const Char * do_report_sales(char * repnm, int from, int by);
extern const Char * do_sho_paymt();
extern const Char * do_order();
extern const Char * do_accounts();
extern const Char * do_debts();
extern const Char * do_rounds();
extern const Char * do_takings();
extern const Char * do_till();
extern const Char * do_commit();
extern const Char * do_overall();
//extern const Char * do_admin();
extern const Char * do_uniques();
extern const Char * at_prt_sorp();
extern const Char * do_backup();
//extern const Char * do_restore();
// extern const Char * do_journal();
extern const Char * at_apply();
extern const Char * do_printers();
//extern const Char * do_spawn();
//extern Char * do_comments();
extern const Char * do_prt_cmr();
extern const Char * do_reset_stock(Quot);
extern void         do_stkstats(Id, Bool);

extern int gdb(int);

#define MAX_ADSZ 20

public void load_prtrs(Prt_t prtarea[MAX_ADSZ], Lra_t lraarea[MAX_ADSZ]);

extern Char * prt_one_report();
extern Char * prt_open_printer();
extern Int sel_cmd(Char *);
extern Prt select_prt __((Char));
extern Prt select_io __((Char *, Char));

extern const Char * do_access_files __((Set, Id));
extern Char * do_reloadacc();

extern Char * preq(Char v);

extern Char * make_dummy_sales(int);

extern Id  mainupn;
extern Id  invupn;
extern Char maindb[];
extern Char invdb[];

extern Bool upn_ro;

extern Accdescr_t g_acc;

extern Id this_pid;

extern Customer_t this_custmr;
extern Agent_t    that_agent;
extern Agent_t    this_agent;
extern Sale_t     this_sale;
extern Char       this_sale_terms[60];
extern Acc_con_t  ths;
extern Cash	  this_a_diff;
extern Int	  this_unpaiddate;
extern Lra_t	  this_acclra;
extern Taking_t   this_take;
extern Bank_t     this_bank;
extern Bool       am_admin;

extern Date this_cut_date;		/* inherited only by load_acc */

extern Int last_match_grp;		/* synthesised by load_acc */

extern Cash this_mindebt;
extern Date this_first_debt;
extern Cash this_min_excess;
extern Int  this_min_pcex;

extern Date this_first_due;

extern Unique_t uniques;

extern Date     today;
extern Char     todaystr[LDATE+1];
extern Id       this_pid;
extern Char     this_tfile[30];

extern Char *   this_ttype;
extern Path     this_tty;
extern Prt_t    this_prt;
extern Path     this_at;

extern Prtsdescr_t adscr;

extern const Char bigline88[];
#define hline(n) ((char*)&bigline88[88-n])

extern const Char ymba[],
  sodi[],
  soii[],
  seyl[],
  ssee[],
  cdel[],
  ngfd[],
  ntfd[],
  ngth[],
  arth[],
  alrp[],
  comd[],
  nrsc[],
  iner[],
  illc[],
  illd[],
  illk[],
  rdoy[],
  dtex[],
  akey[],
  ndon[],
  pstc[],
  sc2d[],
  crsd[],
  noes[],
  amip[],
  teln[],
  psta[],
  stdt[],
  ssvce[],
  ltcr[],
  ahnz[],
  mbyo[],
  amnt[],
  cndo[],
  invf[],
  
  pcde [],
  zrtd [],
  prty [],
  pck  [],
  retl [],
  dcrn [],
  pric [],
  splr [],
  stck [],
  danl [],
  nreq [],
  nong [],
  msgpa[],
  dupmsg[],
  aldo [],
  ill  [],
  illco[],
  dsfst[],
  amtot[]
;

#define da_te &illd[8]
#define dltd  &cdel[9]
#define aban  &msgpa[6]
#define adss  &psta[7]
#define tcr   &illco[14]
#define wdtot &amtot[6]

extern FILE * dbg_file; 

extern Char ds[133];
extern Char ds1[133];

extern Char msg[500];
extern Char fmt[500];
extern Char hhead[900];

extern Byte biglistbuff[200000];


#define CMD_DOT    0
#define CMD_ACCEPT 1
#define CMD_ADJ 2
#define CMD_ASSIGN 3
#define CMD_AUDIT 4
#define CMD_CASH 5
#define CMD_CHECK 6
#define CMD_CHOP 7
#define CMD_CLOSE 8
#define CMD_COPY 9
#define CMD_CREATE 10
#define CMD_D 11
#define CMD_DELETE 12
#define CMD_DNOTE 13
#define CMD_DROP 14
#define CMD_EDI 15
#define CMD_EXIT 16
#define CMD_FIND 17
#define CMD_FLIP 18
#define CMD_GO 19
#define CMD_I 20
#define CMD_LIST 21
#define CMD_ORDER 22
#define CMD_PRINT 23
#define CMD_REDO 24
#define CMD_SAVE 25
#define CMD_SELECT 26
#define CMD_STOCK 27
#define CMD_SUBMIT 28
#define CMD_UNORDER 29
#define CMD_STATS 30
#define CMD_LEVY 31
#define CMD_SUGGEST 32
#define CMD_HELP 33
#define CMD_UNSUGG 34
#define CMD_INTEREST 35

