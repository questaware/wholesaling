#define to_rgt(ch) ((unsigned int)1 << ((ch) - '@'))

				  /* Properties for Customer.terms */
#define P_AT	 (0x100)	/* P-W are exact match */
#define P_DLTD   (0x80)
#define M_BILL	 (0x40)
#define M_ELEC   (0x8)

#define s_fixed(terms) ((terms >> 24) & 0xff)

#define R_ACCT  0x2      /*  A      Post to Accounts */
#define R_BANK  0x4      /*  B      Do Takings */
#define R_CUST  0x8      /*  C      Edit Customers */
#define R_DISC  0x10     /*  D      Allow discounts */
#define R_VACCT 0x20     /*  E      View Accounts */
#define R_DUP   0x40     /*  F      Make Duplicates */
#define R_RPT   0x80     /*  G      Report Statistics */
#define R_DBILL 0x100    /*  H      Discount Invoices */
#define R_INV   0x200    /*  I      Make Invoices */
#define R_CUT   0x400    /*  J      Change Invoice Prices */
#define R_PRC   0x800    /*  K      Change Stock Prices */
#define R_SCRT  0x1000   /*  L      See Supplier prices */
#define R_ITRST 0x2000   /*  M      Charge/Suggest interest */
#define R_GIN   0x4000   /*  N      Goods Inwards */
#define R_EOP   0x8000   /*  O      Edit Output */
#define R_PRT   0x10000  /*  P      Configure Printers */

#define R_RTN   0x40000  /*  R      Make Credit Notes */
#define R_SALE  0x80000  /*  S      Make Credit Sales */
#define R_TIDY  0x100000 /*  T      Tidy */
#define R_SYS   0x200000 /*  U      Unix */
#define R_STK   0x400000 /*  V      Edit Stock */
#define R_WEAK  0x800000 /*  W      Minimum Rights */

#define R_BODY  0x2000000 /* Y      Delete invoices */


#define disallow(r) (~this_agent.props & r)
#define disallow_r(r) (~real_agent_props & r)

