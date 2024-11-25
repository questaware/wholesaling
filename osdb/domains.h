#define KEY(ix) 
			  /* these definitions tie together 
			     lengths of windows fields, keys, and record fields 
			     corresponding window fields in the main code
			     corresponding typdefs are in domains.h
			     corresponding key definitions are in initdb.c
			    
			     It is preprocessed, so comments could cause problems
			     and the order of structures is important
			  */
#define LSNAME	  30
#define LFNAMES   16
#define LPADDR	  79
#define LPCODE	   8
#define LDSCN	  30
#define LSTKKEY   14
#define LTEL	  12
#define LPATH	  30
#define LSITEID    6
#define LCARDNO   22
#define LTTID     28

typedef Char Surname[LSNAME+1];
typedef Char Surname_[LSNAME];

typedef Char Forenames[LFNAMES+1];
typedef Char Forenames_[LFNAMES];

typedef Char Postaddr[LPADDR+1];
typedef Char Postaddr_[LPADDR];

typedef Char Postcode[LPCODE+1];
typedef Char Postcode_[LPCODE];

typedef Char Tel[LTEL+1];
typedef Char Tel_[LTEL];

typedef Char Descn[LDSCN+1];
typedef Char Descn_[LDSCN];

typedef Char Stkkey[LSTKKEY+1];
typedef Char Stkkey_[LSTKKEY];

typedef Char Path[LPATH+1];
typedef Char Path_[LPATH];

typedef Char Siteid[LSITEID+1];
typedef Char Siteid_[LSITEID];

typedef Char Cardno[LCARDNO+1];
typedef Char Cardno_[LCARDNO];

typedef Char TT_id[LTTID];

typedef unsigned short UShort;

#ifndef _LINUX_TYPES_H
typedef unsigned short ushort;
#endif

#define B_SR	256  /* base for offer terms */
#define LOG_B_SR 8

			/* Properties for Customer.props */
#define P_HIDEINT 1

			/* Properties for Invoice items (non persistent) */
#define P_LINK    8
			/* Properties for Stockitem */
#define P_VAT0	  1
#define P_VAT	  2
#define P_NA	  4
#define P_SUBS	  8
#define P_HAS2D  0x40
                        /* Properties for Invoice */
#define P_DESCN  0x10
#define P_ITEM	 0x20
#define P_EXTN	 0x40
#define P_TT     0x80

#define P_OBS	 0x80

#define P_SR	 0x100
#define P_DISC	 (1 << (LOG_B_SR+('B'-'@')))
#define P_OOS	 (1 << (LOG_B_SR+('O'-'@')))

			/* Properties for Transactions */
#define K_SALE	  1
#define K_PAYMENT 4
#define K_EXP	    8
#define K_GIN	 0x10

#define K_HGRAM  0x20

#define K_ANTI	     2
#define K_DED     0x20
#define K_DEAD	  0x40
#define K_HASPD   0x80
#define K_CCHQ	 0x100
#define K_PD	   0x200  /* on Paymt only */
#define K_TT     0x200  /* on Sale only */
#define K_PAID	 0x400
#define K_CASH	 0x800
#define K_ALIEN  0x1000
#define K_IP	   0x2000
#define K_UNB	   0x4000	    
#define K_VAT	   0x4000 /* on Expense only */
#define K_TBP	   0x8000
			/* these properties cannot be stored in some structs */
#define K_LOCK  0x10000

#define K_ORD   0x40000
#define K_ASGD  0x40000
#define K_NOAC  0x80000
#define K_NOTK  0x100000

#define K_NOT	  0x200000
#define K_BFWD  0x200000
#define K_DUE	  0x400000

#define K_TAKE	0x800000    /* used only in audit trail */
#define K_MOVD	0x1000000

#define K_INTSUG 0x2000000
#define K_INTCHG 0x4000000

/* K_SALE	     Types 
   K_PAYMENT 
   K_EXP     
   K_GIN    
	    Properties
   K_DEAD     Deleted
   K_HASPD    Postdated cheque chain attached (flag not audit trailed)
   K_ANTI     Anti sale, anti payment, etc.
   K_CCHQ     Cheque not cash, for sales the payment may not yet be definate
   K_PD       Postdated 
   K_PAID     Thought to have been dealt with, e.g.paid (heuristic only)
   K_CASH     Cash sale or cash return only 
   K_ALIEN    Stock statistics stored on another entry
   K_IP       In progress 
   K_UNB      An unbalanced transaction, such as discount 
   K_LOCK     In use
   K_DED      (To be) Deducted from stock
   K_ORD      (To be) Added to stock
   K_ASGD     On Payment, balance holds a suggested match
   K_NOAC     No Account refers to this Sale or Payment
   K_NOTK     No Taking refers to this Sale or Payment
   K_TBP      This Tak_t is to be posted to accounts (Revenue only)

   K_BFWD     Balance brought forward (not stored in DB)
   K_NOT      General negater (not stored in DB)
   K_DUE      Overdue 

   K_TAKE     A Takings in the audit trail
   K_MOVD     The transaction moved takings
   
*/ 
typedef Int  Dba_t;
typedef Int *Dba;
				      /* Start of database entries */
typedef struct	 _UNIQ 
{ Tel	vatno;
  Int	vatrate;
  Tel	tel1;
  Tel	tel2;
  Tel	fax;
  Id	grace[8];
  short	interest_b_bp;
  Byte  props;
  Byte	loanlee;
  Id    first_sugg;
  Date	lst_anal;
  Int		backupct;
  Path	bu_path;
  Path	at_path;
  Int		last_dnote;
	Int		spare;
	Int		spare1;
} Unique_t, *Unique;

typedef struct _PRT
{ Char	type;
  short pagelen;
  Int	in_use;
  Path	path;
  Path	file;
  Tel   tcap;
} Prt_t, *Prt;

typedef struct	 _SI 
{ Set	     props;
  Id	     id;		KEY(IX_STK)
  Id	     line;	KEY(IX_STKLN)
  Id	     supplier;	
  Descn    descn;
  short    stock;
  short    order;
  Int	     stk_alias;
  short    pack;
  short    ticket;
  Cash	   price;
  short    sale_date;
  short    sell_stt;
  short    cumdev;
  ushort   sig_v;
  Int	     sig_vt;	     /* also used to loop together aliased stock */
  short    week_sale;
  short    profit;
  short    altref;
  Shortset constraint;
} Stockitem_t, *Stockitem;

#define s_case(si) (si)->descn[sizeof(Descn)]

		/* terms format:
		   P_AT => fixed8 : props16 : Deltd1 : Fill3 Edi1 Credband3
		   else =>          props24 : Deltd1 : Fill3 Edi1 Credband3
		*/

typedef struct	 _SUP
{ Id	      id; 	KEY(IX_SUP)
  Id	      bank_acc;
  Id	      barcode;	KEY(IX_MANBC)
  Quot	    terms;
  Surname   surname;	KEY(IX_SUPDCN)
  Forenames forenames;
  Postaddr  postaddr;
  Postcode  postcode;
  Tel	      tel;
} Supplier_t, *Supplier;

#define bill_alt(set) ((set) & M_BILL)
#define old_bill_alt(set) ((set) == -1)

typedef struct	 _CUST
{ Id	      id; 	KEY(IX_CUST)
  Id	      bank_acc;	KEY(IX_BACC)
  Cash	    creditlim;	  /* or alternative account */
  Quot	    terms;
  Surname   surname;	KEY(IX_CUSTSN)
  Forenames forenames;
  Postaddr  postaddr;
  Postcode  postcode;
  Tel	      tel;
  Short     interest_01;	/* bottom bit 1 => on; interest p.a. * 20 */
  Byte      props;
  Byte	    loanlee;
  Id 	      bank_sort_code;
  char      card_type;		/* 'S' switch debit, 's' switch credit, etc */
  Cardno    card_no;
  Short     card_from;		/* yymm */
  Short     card_to;
} Customer_t, *Customer;


typedef struct 
{ Shortset props;
  Short	   qty;
  Id	     id;
  Cash	   price;
/*Short    alt_p_diff;*/
} Item_t, *Item;
  
typedef Char Dbucket [sizeof(Item_t) - sizeof(Shortset)];

typedef struct
{ Shortset  props;
  Dbucket   descn;
} Descitem_t, *Descitem;

typedef union
{ Item_t     i;
  Descitem_t d;
} Item_u_t, *Item_u;

#define INVBUCKET 13

typedef struct	 _INV 
{ Id	     id;		KEY(IX_INV)
  Dba_t    next;
  Int	     version;
  Item_u_t items[INVBUCKET];
} Invoice_t, *Invoice;

typedef struct	 _SALE	
{ Kind	 kind;
  Id	   id;	       KEY(IX_SALE)
  short  customer;
  Sbyte  match_grp;
  Sbyte  cat;
  UShort agent;
  short  date;
  Cash	 total;
  Int	   alt_terms;
  Cash	 balance;   /* only definitive in the audit trail */
  Quot	 terms;     /* or next delivery date date on kind == K_GIN */
  short  tax_date;
  short  vat0_1pc;
  short	 disct;
  short  chq_no;
  Cash	 vattotal;
} Sale_t, *Sale;

typedef struct	 _PMT  
{ Kind	 kind;
  Id	   id;	       KEY(IX_PMT)
  short  customer;
  Sbyte  match_grp;
  Sbyte  cat;
  UShort agent;
  short  date;
  Cash	 total;
  Int	   chq_no;    /* or from_date in kind & K_INTCHG, K_INTSUG */
  Cash	 balance;   /* only definitive in the audit trail */
} Paymt_t, *Paymt;

#define M_EPAID 0x40000000 

#define ACCHDR     5
#define ACCBUCKET 21

typedef struct	 _ACC
{ Id	     id;		KEY(IX_CACC)
  Dba_t    next;
  Cash	   bfwd;
  short    bfwd_date;
  short    filler;
  Dba_t    lastfive;  			/* these 6 present only in the 1st: */
  Int	     version;
  Set	     props;
  Cash	   balance;
  short    prt_date;
  short    unpaid_date;
  Id	     xactn[ACCBUCKET-ACCHDR];	/* +ve --> Sale, -ve --> Paymt */
  					/* bit 30 != bit 31 => paid */
} Account_t, *Account;

typedef struct
{ Int       a;
  Account_t acct;
  Int       z;
} Acc_con_t;


typedef struct	 _AGNT
{ Id	    id;		KEY(IX_AGNT)
  Surname name; 	KEY(IX_AGNTNM)
  Int	    passwd;
  Set	    props;
  Id	    tak_id;
} Agent_t, *Agent;

typedef struct	 _EXP	
{ Kind	  kind;
  Id	    id;	      KEY(IX_EXP) 
  Squot   cat;
  short   ref_no;
  UShort   agent;
  short   date;
  Cash	  total;
  Cash	  vat;
  Descn   descn;
} Expense_t, *Expense;

typedef /* */ struct /* */
{ Id	     id;
  Dba_t    next;
  Cash	   bfwd;
  short    bfwd_date;
  short    props;
  Id	     xactn[ACCBUCKET];
} Accountext_t, *Accountext;

#define K_TPROPS (K_SALE + K_PAYMENT + K_EXP)

typedef struct
{ Int	  kind;  /* contains K_TPROPS, K_ANTI, K_CASH, K_UNB, K_NOTC only */
  Id	  id;	 /* id == 0 => empty slot, kind == 0 => removed */
} Tak_t, *Tak; 

#define TAKBUCKET 10
#define TAKHDR     4
#define TAKBKT    14

typedef struct	 _TAKE
{ Id	    id;		KEY(IX_TAKE)
  Dba_t   next;
  Date	  tdate;
  Short   agid;
  Int	    version;
  Short   bankid;
  Short   filler1;
  Short	  u5000;
  Short	  u2000;
  Short	  u1000;
  Short	  u500;
  Short	  u200;
  Short	  u100;  
  Short	  u50;
  Short	  u20;
  Short	  u5;
  Short	  u1;
  Tak_t   eny[TAKBKT-TAKHDR];
} Taking_t, *Taking;

typedef /* */ struct /* */
{ Id	    id;
  Dba_t   next;
  Tak_t	  eny[TAKBKT];
} Takeext_t, *Takeext;

#define BANKBUCKET 10

typedef struct	 _BANK
{ Id	    id;		KEY(IX_BANK)
  Dba_t   next;
  Date	  date;
  Short   agid;
  Int	    version;
  Short   filler1;
  Short   filler2;
  Short	  u5000;
  Short	  u2000;
  Short	  u1000;
  Short	  u500;
  Short	  u200;
  Short	  u100;  
  Short	  u50;
  Short	  u20;
  Short	  u5;
  Short	  u1;
  Id	    eny[BANKBUCKET];
} Bank_t, *Bank;

#define RNDBUCKET 20

typedef struct	 _ROUND
{ Kind	  kind;
  Id	    id;		KEY(IX_ROUND)
  Dba_t   next;
  Surname name;
  Id	    customer[RNDBUCKET];
} Round_t, *Round;

typedef struct	 _SR
{ Id	   id;		KEY(IX_SR)
  Set	   which;
  Cash	 price;
} Stockred_t, *Stockred;

typedef struct	 _TRANCHE
{ Id	    id;		KEY(IX_TCHE)
  Dba_t   next;
} Tranche_t, *Tranche;

typedef struct	 _TT_REG
{ Id	     cid; 	KEY(IX_TT)
  TT_id    tt_id;
} Tt_reg_t, *Tt_reg;

typedef struct
{ Cash	  price;		/* what is this structure? */
  short   sell_by;
  short   stock;
} Teny_t, *Teny; 

typedef /* */ union /* */ { Kind      kind;
			    Sale_t    sale;
			    Paymt_t   paymt;
			    Expense_t expense;
			  } Sorp_t, *Sorp;

typedef /* */ union /* */ { Id         id;
			    Agent_t    agent;
			    Customer_t customer;
			    Supplier_t supplier;
			  } Party_t, *Party;

