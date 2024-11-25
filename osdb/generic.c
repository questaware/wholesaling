#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "version.h"
#if XENIX
 #include <sys/ascii.h>
#endif
#include "../h/defs.h"
#include "../h/errs.h"
#include "../env/tcap.h"
#include "../form/h/form.h"
#include "../form/h/verify.h"
#include "../db/cache.h"
#include "../db/page.h"
#include "../db/b_tree.h"
#include "../db/recs.h"
#include "../db/memdef.h"
#include "domains.h"
#include "schema.h"
#include "generic.h"
#include "rights.h"
#include "trail.h"

#define MAX_ACCT (int)(sizeof(biglistbuff) / sizeof(Accitem_t))

#define PAYMATCHLIM 10000

Id  mainupn;
Id  invupn = -1;        /* Invoice database */

Bool upn_ro = false;

Id this_pid;

Acc_con_t   ths;
Date	    this_first_due;   /* first unpaid bill */
Cash	    this_a_diff;      /* amount by which ths.acct.balance is too big */
Int	    this_unpaiddate;  /* first unpaid entry, -1 => not in effect */
Lra_t	    this_acclra;
Customer_t  this_custmr;
Agent_t     that_agent;
Char	    this_sale_terms[60];
Taking_t    this_take;

Int  last_match_grp;		/* last tag used to match acc entries */

Accdescr_t g_acc;

#if ASMSTR == 0

CONST Char bigline88[] =
"----------------------------------------------------------------------------------------";
CONST Char bigspaces60[61] =
"                                                            ";
CONST Char ymba[] = "You must be admin";
CONST Char sodi[] = "Somebody else deleted it!?";
CONST Char soii[] = "Somebody else inserted it!?";
CONST Char seyl[] = "Somebody else editting, your changes lost";
CONST Char ssee[] = "Sorry: Someone else editting it";
CONST Char cdel[] = "Customer Deleted";
CONST Char ntfd[] = "Not Found";
CONST Char ngfd[] = "Nothing Found";
CONST Char ngth[] = "Nothing There";
CONST Char arth[] = "Already Present";
CONST Char alrp[] = "Already Paid";
CONST Char comd[] = "command";
CONST Char nrsc[] = "no reverse scroll";
CONST Char iner[] = "Internal Error";
CONST Char illk[] = "illegal key";
CONST Char illc[] = "Illegal choice";
CONST Char illd[] = "Illegal Date";
CONST Char rdoy[] = "Read only";
CONST Char dtex[] = ". to exit";
CONST Char akey[] = "Any key";
CONST Char ndon[] = "Not Done";
CONST Char pstc[] = "Press / to confirm otherwise .";
CONST Char sc2d[] = "space, CR twice, or .";
CONST Char crsd[] = "CR, SP, or .";
CONST Char noes[] = "\nNumber of Entries %d\n";
CONST Char amip[] = "Amount in pence";
CONST Char teln[] = "Telephone Number";
CONST Char psta[] = "Postal Address";
CONST Char stdt[] = "Start Date";
CONST Char ssvce[] = "------- Select service ";
CONST Char ltcr[] = "lost the customer";
CONST Char mbyo[] = "Must be at least a year old";
CONST Char ahnz[] = "Account has non zero balance";
CONST Char zrtd [] = "zero rated";
CONST Char amnt[] = "Amount";
CONST Char cndo[] = "Cannot do";
CONST Char aldo[] = "Already Done";
CONST Char invf[] = "Invalid Format";

CONST Char pcde [] = "product code";
CONST Char prty [] = "properties";
CONST Char pck	[] = "pack";
CONST Char retl [] = "retail";
CONST Char dcrn [] = "description";
CONST Char pric [] = "price";
CONST Char splr [] = "supplier";
CONST Char stck [] = "stock";
CONST Char danl [] = "days of analysis";
CONST Char nreq [] = "Number required";
CONST Char nong [] = "Number or Nothing";
CONST Char msgpa[] = "Print Abandoned";
CONST Char dupmsg[] = "DUPLICATE";
CONST Char ill	[] = "*?*?*";
CONST Char illco [] = "illegal code, Type CR";
CONST Char dsfst [] = "Drop or Save First";
CONST Char amtot [] = "Money Total";

#endif


Char ds[133], ds1[133];

Char  /*fmt[500], */msg[500];
Char  hhead[900]/* , str[40] */;

Byte biglistbuff[200000];

Int trash;

	/* terms format:
	   P_AT => fixed8 props16 Deltd1 Bill1 Fill1 1 Edi1 Credband3
	   else =>        props24 Deltd1 Bill1 Fill1 0 Edi1 Credband3
	   
	   Creband3 : 0 cash, 1- 7
	*/
			/* input format: A-W*(n1)Z{n2|@n3}
			   A-W*:  offer props
			   n1:    offer id
			   Z:	  EDI
			   n2:	  credit band
			   n3:	  billing account
			 */
public Int asc_to_terms(str)
	 Char * str;
{      Int res = 0;
       Int fixedpart = 0;
       Char pstate = 0;
  fast Char * s;

  for (s = str; *s != 0; ++s)
  { Char ch = toupper(*s);
    if      (in_range(ch, 'A', 'W'))     /* ASCII */
      res |= (1 << (ch - '@'+8));
    else if (ch == 'Z')
      res |= M_ELEC;
    else if (ch == '(' or ch == ')')
      pstate = ch;
    else if (ch == '@')
      res |= M_BILL;
    else if (in_range(ch, '0', '9'))
    { if (pstate != '(')
        break;
      fixedpart = fixedpart * 10 + ch - '0';
    }
  }
  return (!pstate      ? res : (res & 0xffffff) + (fixedpart << 24) + P_AT) +
         (res & M_BILL ? 0   : (atoi(s) & 7));
}


public Char * terms_to_asc(terms, s)
	 Int    terms;
	 Char  *s;
{ Set set = (unsigned)terms >> 8;
  Char str[70]; 
  if (terms == -1)
    strcpy(&s[0], "Bill");
  else
  { rgt_to_asc(set, &str[0]);
    if (terms & M_ELEC)
      strcat(&str[0], "Z");
    sprintf(&s[0], terms & M_BILL ? "%s %s" : "%s %s %d",
                   set & 7        ? "" : "CASH", str, terms & 7);
    if	    (terms & P_DLTD)
      strcat(&s[0], " clsd");
    else if ((set & to_rgt('@')) == 0 and (set & to_rgt('O')))
      strcat(&s[0], " held");
    else if (terms & M_BILL)
      strcat(&s[0], " Bill");
  }
  return s;
}



public Int get_fopt(const Char * str,
                  	Field   fld)
{ return (fld->id & ~ID_MSK) == W_SEL and *str == 0 ? fld->id & ID_MSK :
      	                  vfy_integer(str)			    ? atoi(str)	       : 100;
}

public Id first_neg_id(upn, ix_ix)
      Id    upn;
      Id    ix_ix;
{ Lra_t lra;
  Key_t  kvi;  kvi.integer = 0;
  (void)ix_first(upn, ix_ix, &kvi, &lra);
  return kvi.integer > 0 ? 0 : kvi.integer;
}



public Id last_id(upn, ix_ix)
      Id    upn;
      Id    ix_ix;
{ Lra_t lra;
  Key_t  kvi;  kvi.integer = 0;
 (void)ix_last(upn, ix_ix, &kvi, &lra);
  return kvi.integer <= 0 ? 0 : kvi.integer;
}


public void free_acc(acc)
	Accdescr   acc;
{ if ((Byte*)(acc->c) != &biglistbuff[0] and acc->c != null)
    free(acc->c);
}


public Accitem extend_acc(acc)
	Accdescr   acc;
{ Accitem  iip= &acc->c[acc->curr_last];
  acc->curr_last += 1;    				/* more output */
  if (acc->curr_last < acc->arr_sz)
    return iip+1;
{ Int offs = iip - acc->c;
  Int size = acc->arr_sz;
  size = size > 100 ? size + 100 : (size + 1) * 2;
{ Accitem tbl = (Accitem)malloc((size + 1) * sizeof(Accitem_t));
  if (tbl == null)
    return null;
  memcpy(&tbl[0], acc->c, acc->arr_sz * sizeof(Accitem_t));
  free_acc(acc);
  acc->c  = tbl;
  acc->arr_sz = size;
  return &acc->c[offs+1];
}}}



public void init_acc(acc, account_ref)
      Accdescr	acc;
      Account	account_ref;
{ acc->curr_base = 0;
  acc->curr_last = -1;
  acc->bal	 = 0;
  acc->odue	 = 0;
  free_acc(acc);
  acc->c      = (Accitem)&biglistbuff[0];
  acc->arr_sz = sizeof(biglistbuff)/sizeof(Accitem_t) - 1;
  *account_ref = null_sct(Account_t);
  ths.a = 123456;
  ths.z = 987654;
}


Int get_rand()

{ static Int seed = 1234567;
 
  seed = ((seed * 1234567) + time(null)) % 999999;
  return seed;
}



public Customer get_cust(upn, ix, cid)
	Id  upn;
	Id  ix;   /* IX_SUP or IX_CUST */
	Id  cid;
{ Key_t    kvi;
  if (cid == 999999)
  { Lra_t lra;
    Key_t key; key.integer = 0;
  { Cc cc = b_last(mainupn, IX_CUST, &key, &lra);
    if (cc != OK)
      i_log(0, "getlast");
    cid = (get_rand() & 0x7fff) % (key.integer + 1);
  }}
  kvi.integer = cid;
  return (Customer)ix_fetchrec(upn, ix, &kvi, null, &this_custmr);
}

public Cash get_cacc(cid_ref, terms_ref, ix_)
	 Id    * cid_ref;
	 Quot  * terms_ref;
	 Id   ix_;
{      Id ix = ix_;
       Bool silent = false;
       Cash bal = 0;
       
       Bool got_terms = false;
  fast Id cid = *cid_ref;

  if (ix < 0)
  { silent = true;
    ix = -ix;
  }

  while (cid != 0)    /* and not break */
  { // do_db(-1,-1); 
    Key_t kvi; kvi.integer = cid;
  { Customer_t cu_t;
    Customer cu_ = (Customer)ix_fetchrec(mainupn, ix, &kvi,null,&cu_t);
    if      (cu_ == null)
    { if (not silent)
        i_log(cid, ltcr);
      break;
    }
    if (not got_terms)
    { got_terms = true;
      *terms_ref = cu_t.terms;
    }
    if (bill_alt(cu_->terms))
    { cid = cu_->creditlim;
      *cid_ref = cid;
      continue;
    }
  { Account acct__ = (Account)ix_fetchrec(mainupn, IX_CACC, &kvi, null, null);
    if (acct__ != null)
      bal = acct__->balance;
    break;
  }}}

  return bal;
}

Short get_idue(Accitem iip)

{ if ((iip->props & (K_BFWD+K_PAID+K_ANTI)) != 0 or (iip->props & K_SALE) == 0)
   return iip->idate;
{ Int ivl = iip->grace_ix == 0 ? 0 : uniques.grace[iip->grace_ix - 1];
  return ivl == 31 ? next_month(iip->idate) + 1
		   : ivl + iip->idate;
}}



public Date this_cut_date;	/* inherited only by recalc_acc, load_acc */

public void recalc_acc(acc)
	 Accdescr  acc;
{ fast Accitem iip = &acc->c[-1];
       Accitem iiz = &acc->c[acc->curr_last];
       Cash bal = 0;
  acc->odue = 0;
  acc->pdc_bal = 0;
  this_first_due = 0;
  if (iip < iiz and (iip[1].props & K_BFWD))
  { bal += (++iip)->val;
    iip->balance = bal;		/* just tidy  up */
    if ((iip->props & K_PAID) == 0)
      acc->odue = bal;
  }

  while (++iip <= iiz)
  { fast Quot props = iip->props;
    Cash sval = props & K_ANTI ? -(iip->val) : iip->val;
    if (this_cut_date > 0 and iip->idate > this_cut_date)
      continue;
    if	    (props & K_PAYMENT)
    { if (not (props & (K_PAID + K_ANTI)))
      	acc->odue -= sval;
      if (iip->props & K_PD)
      	acc->pdc_bal -= sval;
      else
      	bal -= sval;
    }
    else if (props & K_SALE)
    { if (not (props & (K_PAID + K_CASH)))
      { if (props & K_ANTI)
      	  acc->odue += sval;
      	else
      	  if (get_idue(iip) <= today)
      	  { iip->props |= K_DUE;
      	    acc->odue += sval;
      	  }
      }
      if (not (iip->props & K_CASH))
      	bal += sval;
    }
#if 0
    if (iip->balance != bal)
      i_log(iip->id, "rd %d %d %d", iip->val, bal, iip->balance);
    else
      i_log(iip->id, "ra %d %d",    iip->val, bal);
#endif
    iip->balance = bal;
    if (bal <= 0)
      this_first_due = 0;
    if (bal < acc->odue)
      acc->odue = bal;
    if (this_first_due == 0 and acc->odue > 0)
    { this_first_due = iip->idate;
    /*i_log(acc->id, "Acc FD %d", this_first_due);*/
    }
  }

  if (acc->odue < 0)
    acc->odue = 0;
  if (acc->odue > acc->bal)	/* plaster over the cracks !! */
    acc->odue = acc->bal;
  if (acc->pdc_bal < 0)
    acc->pdc_bal = 0;
  if (this_cut_date != 0)
    acc->bal = bal;

  this_a_diff = acc->bal - bal;
}



Cash init_curr(Accdescr  acc)

{ acc->curr_bal = 0;
  acc->curr_ix = -1;
  if (acc->c[0].props & K_BFWD)
  { acc->curr_bal = acc->c[0].val;
    acc->curr_ix = 0;
  }
  return acc->curr_bal;
}



public Cash get_bal(acc, ix)
	 Accdescr  acc;
	 Int       ix;
{
#if 0
  fast Accitem iip = &acc->c[acc->curr_ix];
       Int ct = ix - acc->curr_ix;
       Int updn;  /* 1: down */

/*i_log(acc->id, "GB %d %d", ix, ct);*/

  if (ct >= 0)
  { if (ct > acc->curr_last - acc->curr_ix)
      ct = acc->curr_last - acc->curr_ix;
    updn = 1;
  }
  else
  { if (acc->curr_ix + ct < 0) /* => ix < 0 */
      ct = -acc->curr_ix;

    ct = -ct;
    ++iip;
    updn = -1;
  }

{ Cash bal = acc->curr_bal;
 /*if (bal != acc->c[acc->curr_ix].balance)
    i_log(acc->id, "Start point is wrong %d %d %d", bal, acc->curr_ix, 
                          acc->c[acc->curr_ix].balance);*/
  if (ct * 2 > acc->curr_last)
  { if (updn > 0)
    { bal = acc->bal;
      iip = &acc->c[acc->curr_last+1];
      ct = acc->curr_last - ix;
    }
    else
    { bal = init_curr(acc);
      iip = &acc->c[acc->curr_ix];
      ct = ix - acc->curr_ix;
      /*i_log(acc->id, "DoWn %x %d %d", bal, ct, ix);*/
    }
    updn = -updn;
  }

  for (; --ct >= 0; )
  { iip += updn;
  { fast Quot props = iip->props;
         Cash sval = props & K_INTSUG ? 0 	    :
         	     props & K_ANTI   ? -(iip->val) : iip->val;
  /*i_log(acc->id, "Eny %x %d %d", props, bal, sval);*/
    if (updn < 0)
      sval = -sval;
    if	    ((props & (K_PAYMENT+K_PD)) == K_PAYMENT)
      bal -= sval;
    else if ((props & (K_SALE+K_CASH)) == K_SALE)
    	bal += sval;
  /*if (bal != iip[updn > 0 ? 0 : -1].balance)
      i_log(acc->curr_ix, "point is wrong %x %d %d %d", 
                     props, iip - &acc->c[0], bal, iip[updn > 0 ? 0 : -1].balance);*/
  }}

  acc->curr_bal = bal;
  /* note: &acc->c[ix] != iip */
 		/* this diagnostic is an attempt to eliminate .balance */
 		/* since it appears we cannot yet! */ 
  if (bal != acc->c[ix].balance)
    i_log(acc->curr_ix, "End point %d is wrong %d %d %d %d %d", acc->id,
                     updn, ix, acc->curr_last, bal, acc->c[ix].balance);
  acc->curr_ix = ix;

}
#endif
  return acc->c[ix].balance;
}

private Int load_acc_meny;		/* matching entry */

#if 0

private Set find_bal(acc, samt)
	 Accdescr   acc;
	 Cash	    samt;
{ fast Accitem it;
  for (it = &acc->c[acc->curr_last + 1]; --it >= acc->c;)
    if (it->balance == samt)
    { load_acc_meny = it->id;
      if (it->props & K_BFWD)
      	load_acc_meny = -1;
      if (it->props & K_PAYMENT)
      	load_acc_meny = -load_acc_meny;
      return -1;
    }

  return 0;
}

#endif


public Date last_act_date(acc)
	Accdescr   acc;
{ fast Accitem iiz = &acc->c[0];
  fast Accitem iip = &acc->c[acc->curr_last+1];
  fast Date res = 0;

  while (--iip >= iiz)
    if (iip->idate > res)
      res = iip->idate;

  return res;
}

#define AB (ACCBUCKET-ACCHDR)
						/* write locked */
Cc do_adj_intbals(Id cid, Id pid, Cash val)

{ Bool go = false;
  Int wr_ct = 0;
  Lra_t acclra = 0;
  Key_t kv_t;  kv_t.integer = cid;
{ Set q_eflag = 0;
  Account_t ac_t;
  Account acct = (Account)ix_fetchrec(mainupn, IX_CACC, &kv_t, &acclra, &ac_t);
  if (acct == null)
    return NOT_FOUND;
  for (; acclra != 0; )
  { // do_db(-1,-1); 
    Account ac = (Account)read_rec(mainupn, CL_ACC, acclra, &ac_t);
    Id * ip__ = &ac->xactn[q_eflag != 0 ? -ACCHDR-1 : -1];
    if (ac == null)
    { i_log(cid, "Acc Chain int");
      break;
    }
    if (q_eflag == 0)
    { /*i_log(cid, "Adj bal %d %d", ac_t.balance, val);*/
      ac_t.balance += val;
    }
    if (go)
      ac_t.bfwd += val;

    while (++ip__ <= &ac->xactn[AB-1] and *ip__ != 0)
    { Id id = *ip__;
      if (id < 0)
        if (((-id) & ~M_EPAID) == pid)
        { /*i_log(*ip__, "Got Sigg");*/
          go = true;
        }
    }

    if (go || q_eflag == 0)
      (void)write_rec(mainupn, CL_ACC, acclra, &ac_t);

    q_eflag = Q_EFLAG;
  { Lra_t nlra = ac->next & (Q_EFLAG-1);
    if (nlra == acclra)
      nlra = 0;    /* extra protection */

    acclra = nlra;
    if ((++wr_ct & 16) == 0)
      force_db(mainupn);
  }}

  if (!go)
    i_log(cid, "Lost intsug %d", pid);
  return OK;
}}


Cc load_acc_(upn, cid, acc, acct_ref, terms, cmd, skip)
	 Id	        upn, cid;
	 Accdescr   acc;
	 Account_t *acct_ref;
	 Set	      terms;			/* cmd & Q_MATCH => balance */
	 Set	      cmd;
	 Int        skip; 			/* leading entries to skip */
{ Set recent = cmd & Q_RECENT;
  Lra_t acclra = 0;
  Key_t kv_t;  kv_t.integer = cid;
  init_acc(acc, acct_ref);
  rd_lock(upn);
{ Account_t ac_t;
  Account acct = (Account)ix_fetchrec(upn, IX_CACC, &kv_t, &acclra, acct_ref);
  if (acct == null)
  { rd_unlock(upn); return NOT_FOUND; }

  if (acct->id == 0)
  { acct = (Account)ix_fetchrec(upn, IX_CACC, &kv_t, &acclra, acct_ref);
    i_log(cid, "*Zero Acc Tag, now %d for lra %x", 
                 acct == null ? 999999 : acct->id, acclra);
    i_log(0, "A %d Z %d", ths.a, ths.z);
  }
 
  this_acclra = acclra;
  this_unpaiddate = acct->props & K_PAID ? 0 : -1;
  last_match_grp = -1;
  acc->id = cid;
  acc->bal = acct->balance;

{      Cc rescc = OK;
  fast Accitem iip = &acc->c[-1];
       Cash bal = acct->bfwd;
       Int bfwd_err = 0;
       Int bfwd_diff = 0;
       Int bkt_ct = 0;
       Set q_eflag = 0;
       Date last_month = 0;
       Cash last_sale = 0;
       Cash last_pmt = 0;
       skip += ACCHDR;

  for (; acclra != 0; )
  { // do_db(-1,-1);
    Account ac = (Account)read_rec(mainupn, CL_ACC, acclra, &ac_t);
    Id * ip__ = &ac->xactn[q_eflag != 0 ? -ACCHDR-1 : -1];

    if (ac == null)
    { i_log(cid, "Acc Chain");
      break;
    }
/*  if (acclra == acct->lastfive && (cmd & Q_RECENT) == 0 && bal != acct->bfwd)
    { i_log(cid, "l5 wrong %d %d", bal, acct->bfwd);
    }*/
    
 /* i_log(cid, "Ldib %d %d", ac_t.id, ac_t.balance); */

    if (ac->id != cid)
    { Byte blk[BLOCK_SIZE];
      i_log(cid, "Lacc id wrong %d page %x lra %x", 
                        ac->id, s_page(acclra), acclra);
      (void)read_db(mainupn, s_page(acclra), &blk[0]);
      dump_blk(blk, 16);
    { Byte * p = page_ref(mainupn, s_page(acclra));
      dump_blk(p, 16);
    }}

    if (cmd & Q_RECENT)
    { cmd &= ~Q_RECENT;
      if (acct->lastfive != 0)
      { acclra = acct->lastfive;
      	acct = (Account)read_rec(upn, CL_ACC, acclra, &ac_t);
      	if (acct == null)
      	{ i_log(cid, "L5 Bad");
      	  break;
      	}
      	else
      	{ bal = acct->bfwd;
      	  this_unpaiddate = ((Accountext)acct)->props & K_PAID ? 0 : -1;
      	  q_eflag = Q_EFLAG;
          continue;
      	}
      }
    }
/*
    i_log(cid, "Apage %x", acclra);
*/
    if ((ac->next & Q_EFLAG) != q_eflag)
      i_log(cid, "Q_EFLAG wrong %d %x", bkt_ct, ac->next);
        bkt_ct += 1;

    if (ac->bfwd != bal)
    { if (bfwd_err == 0)
      { bfwd_err = bkt_ct;
        bfwd_diff = ac_t.bfwd - bal;
      }
      ac_t.bfwd = bal;
      if (cmd & Q_UPDATE)
      	(void)write_rec(upn, CL_ACC, acclra, &ac_t);
    }

    skip -= ACCBUCKET;

    if (iip == &acc->c[-1] and bal != 0 and skip <= 0)
    { iip = extend_acc(acc);
      iip->props      = K_BFWD | 
            ((not q_eflag ? acct->props : ((Accountext)acct)->props) & K_PAID);
      iip->idate      = acct->bfwd_date;
      iip->grace_ix   = 0;
      iip->match_grp  = 0;
      iip->id         = cid;
      iip->val        = bal;
      iip->vat        = 0;
      iip->chq_no     = -1;
    }

    q_eflag = Q_EFLAG;

    if (skip <= 0)
    while (++ip__ <= &ac->xactn[AB-1] and *ip__ != 0)
    { Quot kix = IX_SALE;
      Id id = *ip__;
      if (id < 0)
      { id = -id;
        kix = IX_PMT;
      }
      if ((id & M_EPAID) and (cmd & Q_UNPAID))
      	continue;

      kv_t.integer = id & ~M_EPAID;
      if (cid < PDCHQACC and kix == IX_SALE)
        kv_t.integer = - kv_t.integer;
    { Lra_t lr;
      Paymt pmt = (Paymt)ix_srchrec(upn, kix, &kv_t, &lr);
      if (pmt == null or (pmt->kind & K_DEAD))     /* => it was cancelled */
      { if (pmt == null)
      	  i_log(cid, "Lost Entry %d %d", kix, kv_t.integer);
        continue;
      }

      if ((pmt->kind & (K_INTSUG+K_INTCHG)) && pmt->total == 0)
        continue;
    /*if (ths.acct.id == 0)
        i_log(cid, "Corrpt 1");*/
      iip = extend_acc(acc);
      *iip = null_sct(Accitem_t);
      if (ths.acct.id == 0)
        i_log(cid, "Corrpt 2");
      iip->id	     = kv_t.integer;
      iip->val       = pmt->total;
      iip->match_grp = pmt->match_grp;
      iip->props     = pmt->kind & (-1 ^ (kix == IX_SALE ? K_PD : 0));
	
      if (last_match_grp < 0 or 
          (unsigned)(iip->match_grp - last_match_grp - 1) < 32 or
          (unsigned)(iip->match_grp - last_match_grp + 100 - 1) < 32)
      { last_match_grp = iip->match_grp;
      /*i_log(last_match_grp, "is %d", iip->val);*/
      }
      if      (pmt->kind & K_PAYMENT)
      { iip->chq_no    = pmt->chq_no;
      	iip->idate  = pmt->date;
      	if (pmt->kind & K_INTSUG)
      	{ iip->vat = iip->val;
      	  iip->val = 0;
      	  iip->grace_ix = pmt->cat;
      	} 
      }
      else if (pmt->kind & K_SALE)
      { Sale is__ = (Sale)pmt;
        iip->chq_no   = pmt->chq_no;
        iip->grace_ix = is__->terms & 7;
      	iip->idate    = is__->tax_date;
      	iip->vat      = (is__->vattotal * is__->vat0_1pc + 500)/ 1000;
      }
      else
        i_log(pmt->id, "Kerr %d %d", pmt->kind, cid);
   /*
      if ((id & M_EPAID) or today - iip->idate > PAYMATCHLIM)
      	iip->props |= K_PAID; */
      if (this_unpaiddate == 0 and (iip->props & K_PAID) == 0)
      	this_unpaiddate = iip->idate;
#if 0
      if (id < M_EPAID and (iip->props & K_PAID))	/* it's paid */
      { flip_vfywr();
      { Account tbkt = (Account)rec_mark/**/_ref(upn, CL_ACC, acclra);
        Int ix = ip - &ac->xactn[0];
      	tbkt->xactn[ix] ^= M_EPAID;
      	flip_vfywr();
      }}				/* not write locked, ok, it's atomic */
#endif
    { Cash sval = iip->props & K_INTSUG ? 0           :
    		  iip->props & K_ANTI   ? -(iip->val) : iip->val;/*chk bfwd's*/
      if      ((iip->props & (K_PAYMENT+K_PD)) == K_PAYMENT)
      	bal -= sval;
      else if ((iip->props & (K_SALE+K_CASH)) == K_SALE)
      	bal += sval;
      iip->balance = bal;

    /*i_log(bal, iip->props & K_SALE ? "S %d %d" : "P %d %d", iip->id, iip->val);*/
        
      if (cmd & Q_MATCH)
      { if      (bal == terms and not (iip->props & K_CASH))
      	  ;
      	else if (id & M_EPAID)
      	{ --iip;			/* throw it away */
      	  acc->curr_last -= 1;
      	  continue;
      	}
      }
      if (cmd & Q_HGRAM)
      { int lm = this_month(iip->idate);
        if (lm != last_month)
        { last_sale = 0;
          last_pmt = 0;
        }
        if      (pmt->kind & K_PAYMENT)
          last_pmt += iip->val;
        else if (pmt->kind & K_SALE)
          last_sale += iip->val;

        if (last_month == lm)
        { acc->curr_last -= 1;    /* more output */
          --iip;
          iip->val = last_sale;
          iip->vat = last_pmt;    /* steal vat field */
        }
        last_month = lm;
      }
    }}}
    if (acc->curr_last > MAX_ACCT)
    { i_log(cid, "A too big");
      rescc = HALTED;
      break;
    }
  { Lra_t nlra = ac->next & (Q_EFLAG-1);
    if (nlra == acclra)
      nlra = 0;    /* extra protection */

    acclra = nlra;
  }}

  rd_unlock(upn);

  if (recent)		/* acc->bfwd can not be relied on! */ 
  { bal = acc->bal;

    for (; iip >= &acc->c[0]; --iip)
    { Cash sval = iip->props & K_INTSUG ? 0           :
    		  iip->props & K_ANTI   ? -(iip->val) : iip->val;/*chk bfwd's*/
      iip->balance = bal; 
      if      ((iip->props & (K_PAYMENT+K_PD)) == K_PAYMENT)
      	bal += sval;
      else if ((iip->props & (K_SALE+K_CASH)) == K_SALE)
      	bal -= sval;
      else if (iip->props & K_BFWD)
        iip->val = iip->balance;
    }
  }

  recalc_acc(acc);
  if (rescc == OK and (cmd & (Q_UNPAID+Q_MATCH)) == 0)
  { if (this_a_diff != 0 and (~cmd & Q_HGRAM))
      i_log(cid, "Acc diff %ld", this_a_diff);
    if ((cmd & Q_UPDATE) == 0 and bfwd_err != 0)
      i_log(cid, "Acc bfwd err %d %d", bfwd_err, bfwd_diff);
  }
  if (last_match_grp < 0)
    last_match_grp = 0;

  return rescc;
}}}



Cc load_acc(upn, cid, acc, acct_ref, terms, cmd)
	 Id	    upn, cid;
	 Accdescr   acc;
	 Account_t *acct_ref;
	 Set	    terms;
	 Set	    cmd;
{ Int  skip = 0; 			/* leading entries to skip */
  Cc rescc;
  while ((rescc = load_acc_(upn, cid, acc, acct_ref, terms, cmd, skip)) == HALTED)
  { skip += (MAX_ACCT * 2) / 3;
  }

  return rescc;
}

public const Char * audit_acc(upn, cid, acc, atfn)
	 Id	    upn, cid;
	 Accdescr   acc;
	 Char *     atfn;
{      Cc cc;
       Int errct = 0;
  fast Accitem iip = &acc->c[0] - 1;
       Sorp eny;
       Fildes f = open_read_at(atfn);
       if (f < 0)
	 return "Not an audit file";

  rd_lock(upn);

  while ((cc = rd_nxt_rec(f, (Char**)&eny)) != NOT_FOUND)
  { // do_db(-1,-1);
    if	    (cc != OK)
      errct += 1;
    else if ((eny->kind & (K_SALE + K_PAYMENT)) and
	      eny->sale.customer == cid)
    { 
      iip = extend_acc(acc);	/* more output */

      *iip = null_sct(Accitem_t);
      iip->id	  = eny->sale.id;
      iip->props  = eny->kind;
      iip->val	  = eny->sale.total;
      iip->idate  = eny->sale.date;
      ths.acct.balance = eny->sale.balance;
      iip->balance = ths.acct.balance;
      acc->bal =     ths.acct.balance;
      if (iip->props & K_PAYMENT)
      { iip->chq_no = eny->paymt.chq_no;
        if (not (iip->props & K_PAID))
	  acc->odue -= iip->val;
      }
      else			/* K_SALE */	
      { Sale is__ = (Sale)eny;
        iip->chq_no = is__->chq_no;
				iip->idate  = is__->tax_date;
				iip->vat  = (is__->vattotal * is__->vat0_1pc + 500)/ 1000;
				if ((is__->kind & K_SALE) and not(is__->kind & (K_PAID + K_ANTI)))
				{ if (get_idue(iip) <= today)
				  { iip->props |= K_DUE;
				    acc->odue += iip->val;
				  }
				}
				if (iip->props & K_DEAD)		/* cancelled invoice */
				  iip->props ^= K_ANTI+K_DEAD;
      }
    }
  }

  if (acc->odue < 0)
    acc->odue = 0;
  if (acc->odue > acc->bal)	/* plaster over the cracks !! */
    acc->odue = acc->bal;
  acc->id = cid;
  rd_unlock(upn);
  return null;
}

#define keyarea hhead

public Cc get_sname(str, kv_ref)
	 Char *      str;
	 Key_t * kv_ref;
{ kv_ref->string = &keyarea[0];
{ Char * s = get_b_sname(str, kv_ref->string);
  if	  (s < str)
  { alert("invalid");
    return INVALID;
  }
  else
    return s == str ? NOT_FOUND : OK;
}}

			  /* synthesises this_take.id, .version, .date */

public void get_taking(ag_ref, agid, level)
	 Agent_t  * ag_ref;
	 			/* level > 0 => write locked */
	 Id         agid;
	 Int      level; /* 0:this_take.id may be 0, 1:disallow, 2:make new*/
{ Lra_t aglra;
  Key_t kvi; kvi.integer = agid;
{ Agent ag = (Agent)ix_fetchrec(mainupn, IX_AGNT, &kvi, &aglra, ag_ref);
  this_take = null_sct(Taking_t);
  this_take.id = ag == null ? 0 : ag->tak_id;

  if      (level == 0)
  { if (this_take.id != 0)
    { Lra_t lra2;
      kvi.integer = this_take.id;
    { Taking ta = (Taking)ix_fetchrec(mainupn,IX_TAKE, &kvi, &lra2, &this_take);
      if (ta == null)
				i_log(agid, "*tak_id wrong %d, ag %x", kvi.integer, ag);
    }}
  }
  else if (this_take.id == 0 or level > 1)
  { this_take = null_sct(Taking_t);
    this_take.id      = last_id(mainupn, IX_TAKE) + 1;
    this_take.version = 1;
    this_take.tdate   = today;
   /*i_log(this_take.id, "Prepared take (lev %d)", level);*/
  }

  if (ag != null and ag->tak_id != this_take.id)
  { ag->tak_id = this_take.id;
    (void)write_rec(mainupn, CL_AGNT, aglra, ag);
  }
}}

  /* inherits	 this_take.id */
  /* synthesises this_take.id, different if input.this_take.id == 0 */
					/* next procedures have similar code */
public Cc item_to_tak(agid, limit, kind, id)
				      /* write locked */
	 Id   agid;
	 Int  limit;/* Q_CLOSEV => only open takings, MAXINT => make existing */
	 Kind kind;
	 Id   id;
{ Agent_t ag_t;
  Key_t kv_t;
  Bool first = this_take.id == 0;
  if (first)
    get_taking(&ag_t, agid, 1);

  kv_t.integer = this_take.id;
{ Lra_t flra = 0;
  Taking_t ta_t;
  fast Taking ta = (Taking)ix_fetchrec(mainupn, IX_TAKE, &kv_t, &flra, &ta_t);
  Lra_t lastlra = flra;

  if (ta == null)
  { if (not first)
    { i_log(this_take.id, "Cannot find take");
      return NOT_FOUND;
    }
  }
  else
  { if (ta_t.version > limit)
      return HALTED;
    ta_t.version += 1;
    (void)write_rec(mainupn, CL_TAKE, flra, &ta_t);
/*  if (ta_t.version != this_take.version and not first)
      i_log(this_take.id, "TTV %d %d", ta_t.version, this_take.version);
*/
    this_take.version = ta_t.version;

    for (; ta != null and s_offs(ta_t.next) != 0;
		   		 ta = (Taking)read_rec(mainupn, CL_TAKE, lastlra, &ta_t))
	  { // do_db(-1,-1); 
  	  if (ta_t.id != kv_t.integer)
    	{ i_log(ta_t.id, "Mixed Takes"); break; 
    	}
	    lastlra = ta_t.next & (Q_EFLAG-1);
    }

		if (ta == null)
		{ i_log(75, "HeCk");
			return CORRUPT_PF;
 	  }

	{ fast Tak ip = lastlra == flra ? &ta_t.eny[-1] : &ta_t.eny[-TAKHDR-1];
  	Tak ip_ = ip;
		Tak iz = &ta_t.eny[TAKBUCKET-1];
    
   	while (++ip <= iz and ip->id != 0)
     	;
    while (--ip > ip_ and ip->kind == 0)	/* recover dead entries */
 	    ;

    if (++ip <= iz)
 	  { ip->kind = kind;
   	  ip->id   = id;
     	if (++ip <= iz)
				ip->id = 0;
      (void)write_rec(mainupn, CL_TAKE, lastlra, &ta_t);
 	    return SUCCESS;
   	}
 	}}

{ Cc cc = OK;
  Taking_t take;  take = null_sct(Taking_t);
  take.id = this_take.id;
  if (flra == 0)
  { take.tdate	     = today;
    take.agid	     = agid;
    take.version     = this_take.version;
    take.eny[0].kind = kind;
    take.eny[0].id   = id;
    cc = i_Taking_t(mainupn, &take, &flra);
  /*i_log(take.id, "Created taking");*/
  }
  else
  { Lra_t lra;
    take.next			  = Q_EFLAG;
    ((Takeext)&take)->eny[0].kind = kind;
    ((Takeext)&take)->eny[0].id   = id;
    cc = new_init_rec(mainupn, CL_TAKE, (char*)&take, &lra);
    ta_t.next = (ta_t.next & Q_EFLAG) + lra;
    (void)write_rec(mainupn, CL_TAKE, lastlra, &ta_t);
  }
  return cc;	
}}}

					/* inherits this_agent.id */
					/* synthesises this_take.id */
public Cc xactn_to_taking(sorp)
				/* write locked */
	 Sorp sorp;
{ Cc cc;
  Quot kind = sorp->kind;
  Id  id = sorp->sale.id;

  while (true)
  { // do_db(-1,-1); 
    cc = item_to_tak(this_agent.id, Q_CLOSEV-1, kind, id);
    if (cc != HALTED)			/* HALTED => taking is closed */
      break;
    get_taking(&this_agent, this_agent.id, 2);
  }
  return cc;
}

public Cc item_from_tak(upn, tid, kind_, id, kind_ref)
	 Id	upn;	/* write locked */
	 Kind	kind_;
	 Id	tid, id;
	 Kind  *kind_ref;
{ Kind kind = kind_ & K_TPROPS;
  Lra_t lra = 0;
  Key_t kv_t; kv_t.integer = tid;
{ Taking_t ta_t;
  Taking ta = (Taking)ix_fetchrec(upn, IX_TAKE, &kv_t, &lra, &ta_t);
  Lra_t hdlra = lra;
  Lra_t nxtlra, prevlra = 0;

  if (ta == null)
  { if (tid != 0)
  		i_log(tid, "Taking not there");
    return NOT_FOUND;
  }
  if (ta_t.version >= Q_SUBMITV)
    return HALTED;

  for (; lra != 0; lra = nxtlra)
  { // do_db(-1,-1); 
    ta = (Taking)read_rec(upn, CL_TAKE, lra, &ta_t);
    if (ta == null)
      break;
    nxtlra = ta_t.next & (Q_EFLAG-1);
  { Tak ip = prevlra == 0 ? &ta_t.eny[-1] : &ta_t.eny[-1-TAKHDR];
    Tak iz = &ta_t.eny[TAKBUCKET-1];
    Tak ix_ = null;
    Bool all_void = prevlra != 0;

    while (++ip <= iz and ip->id != 0)
      if      ((ip->kind & K_TPROPS) == kind and ip->id == id)
      { if (ip->kind & K_DEAD)
        { i_log(id, "Inv is dead");
          return NOT_FOUND;
        }
        ix_ = ip;
				*kind_ref = ix_->kind;
				ix_->kind = 0;
				(void)write_rec(upn, CL_TAKE, lra, &ta_t);
				if (not all_void)
				  break;
      }
      else if (ip->kind != 0)
				all_void = false;

    if (ix_ != null)
    { if (all_void and prevlra != 0)
      { ta = (Taking)read_rec(upn, CL_TAKE, prevlra, &ta_t);
				if (ta != 0)
				{ ta_t.next = (ta_t.next & Q_EFLAG) + nxtlra;
				  (void)write_rec(upn, CL_TAKE, prevlra, &ta_t);
				 /*i_log(tid, "del_rec(Take)");*/
				  del_rec(upn, CL_TAKE, lra);
				}
      }
      ta = (Taking)read_rec(upn, CL_TAKE, hdlra, &ta_t);
      ta_t.version += 1;
      if (tid == this_take.id)
				this_take.version = ta_t.version;
			if (ta != NULL)
	      (void)write_rec(upn, CL_TAKE, hdlra, &ta_t);
      return OK;
    }
    prevlra = lra;
  }}

//i_log(tid, "Invoice %d not in tak", id);
  return NOT_FOUND;
}}

public Cc update_bfwd(acclra, samt)
	 Lra_t  acclra;
	 Cash   samt;
{ Account_t acct;
  Int ct = 0;

  for (; acclra != 0; acclra = acct.next & (Q_EFLAG-1))
  { // do_db(-1,-1); 
    Account ac = (Account)read_rec(mainupn, CL_ACC, acclra, &acct);
    if (ac == null)
      break;
    if (ct > 0)
    { acct.bfwd += samt;
      (void)write_rec(mainupn, CL_ACC, acclra, &acct);
    }
    ++ct;
  }

  return OK;
}

public Cc sorp_to_account(cmd, cid, ix, id, samt) /* set ths.acct.balance */
				      /* write locked */
	Quot cmd;			/* Q_UPDATE: adj existing Q_CHK:check */
	Id   cid, ix, id;		/* ix >= 0 */
	Cash samt;
{ Id ie = ix == IX_PMT ? -id : id;
  Id ieny = cid < PDCHQACC ? -ie : ie;
  Cash obalance = 0;
  Key_t kv_t; kv_t.integer = cid;
  if (cid == 0)
  { i_log(id, "sorp_to_acc 0");
    return CORRUPT_DB;
  }
  
  if (id == 0 and (cmd & Q_CREATE) == 0)
  { i_log(cid, "*Zero going onto account");
  }
  
{ Lra_t acclra = 0;
  Lra_t prevlra = 0;
  Lra_t flra = 0;
  Lra_t lastfive = 0;
  Int bkt_ct = 0;

  Cc cc = OK;
  Account_t n_acct; 
  Account_t ac_t;
  Account acct = (Account)ix_fetchrec(mainupn, IX_CACC, &kv_t, &acclra, &ac_t);
  if (acct != null and (cmd & Q_CHK) == 0)
  { if (ac_t.id != cid)
    { i_log(cid, "Acc id tag wrong %d page %x lra %x", 
                      ac_t.id, s_page(acclra), acclra);
      ac_t.id = cid;
    { Byte blk[BLOCK_SIZE];
      (void)read_db(mainupn, s_page(acclra), &blk);
      dump_blk(blk, 16);
    { Byte * p = page_ref(mainupn, s_page(acclra));
      dump_blk(p, 16);
    }}}

    flra = acclra;
    obalance = ac_t.balance;
    lastfive = ac_t.lastfive;
    if (ac_t.id == ths.acct.id)
    { if (ths.acct.version != ac_t.version and id != 0)
        i_log(cid, "TAV %ld %ld", ac_t.version, ths.acct.version);

      ths.acct.version = ac_t.version + 1;;
    }
    if (id == 0)
    { i_log(cid, "*Create Acc already exists");
      return ENY_EXISTS;
    }
    ac_t.version += 1;
    ac_t.balance += samt;
    cc = write_rec(mainupn, CL_ACC, acclra, &ac_t);
    if (cc != OK)
      i_log(cc, "WR BFWD");
    if ((cmd & (Q_RECENT+Q_UPDATE)) == Q_RECENT and lastfive != 0)
      acclra = lastfive;
  }
  if (acct != null and (cmd & Q_CHK))
    ths.acct.bfwd_date = acct->bfwd_date;
  ths.acct.balance = obalance + samt;
  if ((cmd & Q_CHK) and acclra == 0)
    return ENOTTHERE;

  if (id != 0)
    for (; acclra != 0 and acclra != prevlra; acclra = ac_t.next & (Q_EFLAG-1))
    { // do_db(-1,-1); 
      Account ac = (Account)read_rec(mainupn, CL_ACC, acclra, &ac_t);
      Id * ip__ = &ac->xactn[prevlra == 0 ? -1 : -ACCHDR-1];
      Id * iz__ = &ac->xactn[AB-1];

      if (ac == null)
				return CORRUPT_DB;

      if (ac_t.id != cid)
      { i_log(cid, "DAcc id tag wrong %d page %x lra %x", 
                               ac_t.id, s_page(acclra), acclra);
        ac_t.id = cid;
      }

      prevlra = acclra;

      if (acclra == lastfive)
				bkt_ct = 0;
      bkt_ct += 1;

      while (++ip__ <= iz__ and *ip__ != 0)
				if (((*ip__ ^ ieny) & 0xbfffffff) == 0)
				  return cmd & Q_CHK          ? OK         :
								 not (cmd & Q_UPDATE) ? ENY_EXISTS
	         											      : update_bfwd(acclra, samt);
      if (ip__ <= iz__)
      { if (ac_t.next & (Q_EFLAG-1))
        { i_log(cid, "Acc Z midstream using %d", ieny);
					  return CORRUPT_DB;
        }
        if (cmd & (Q_UPDATE+Q_CHK))
				  return NOT_FOUND;
				*ip__ = ieny;
				if (++ip__ <= iz__)
				  *ip__ = 0;
				(void)write_rec(mainupn, CL_ACC, acclra, &ac_t);
				break;
      }
    }

  if (cmd & Q_CHK)
    return NOT_FOUND;

  if (acclra == 0)
  { if (cmd & (Q_UPDATE+Q_CHK))
      return NOT_FOUND;
    n_acct = null_sct(Account_t);
    n_acct.id = cid;
    if (prevlra == 0)
    { n_acct.version  = 1;		
      n_acct.props    = cid >= 0 ? 0 : K_GIN;
      n_acct.balance  = samt;
      n_acct.xactn[0] = ieny;
      cc = i_Account_t(mainupn, &n_acct, &prevlra);
      ths.acct.version = 1;
    }
    else
    { n_acct.next = Q_EFLAG;
      ((Accountext)&n_acct)->bfwd     = ths.acct.balance - samt;
      ((Accountext)&n_acct)->xactn[0] = ieny;
      cc = new_init_rec(mainupn, CL_ACC, (char*)&n_acct, &acclra);
      ac_t.next = (ac_t.next & Q_EFLAG) + acclra;
      (void)write_rec(mainupn, CL_ACC, prevlra, &ac_t);
    }
  }
  
  if (bkt_ct > RACCTSZ)
  { Account acct_ = (Account)read_rec(mainupn, CL_ACC, flra, &ac_t);
    if (acct_ != null)
      acct_ = (Account)read_rec(mainupn, CL_ACC, lastfive==0 ? flra : lastfive,
 								 &n_acct);
    if (acct_ != null)
    { if (ac_t.id != cid)
      { i_log(cid, "2Acc id tag wrong %d", ac_t.id);
        ac_t.id = cid;
      }
      ac_t.lastfive = acct_->next & (Q_EFLAG-1);
      if (ac_t.lastfive != 0)
	(void)write_rec(mainupn, CL_ACC, flra, &ac_t);
    }
  }
  return cc;
}}

public Bool pay_acceny(cid, kind, id, match_grp)   /* mark acc entry paid,u/p */
	 			        /* write locked */
	 Id    cid;
	 Set   kind;			/* kind & K_MOVD => all up to id PAID */
	 Id    id;			  /* kind & K_BFWD => bfwd         */
	 Int   match_grp;
{ Quot typ = kind & K_PAYMENT ? IX_PMT : IX_SALE;
  Id ieny = (kind & K_PAYMENT ? -id     : id) ^
  			    (kind & K_PAID    ? M_EPAID : 0);
  Key_t kv_t; kv_t.integer = cid;
{ Lra_t acclra = 0;

  Bool all = (kind & K_MOVD) != 0;
  Bool stop = false;
  Lra_t prevlra = 0;

  Cc cc = OK;
  Account_t ac_t;
  Account acct = (Account)ix_fetchrec(mainupn, IX_CACC, &kv_t, &acclra, &ac_t);
  if (acct != null)
  { if (false and (kind & (K_BFWD+K_MOVD+K_NOT)) == 0)
    { prevlra = acclra;		/* not correct but not null so OK */
      acclra = ac_t.lastfive;
    }
  }

	if (all)
		i_log(cid, "Imp Error 413");

  for (; acclra != 0 and acclra != prevlra; acclra = ac_t.next & (Q_EFLAG-1))
  { // do_db(-1,-1); 
    Account ac = (Account)read_rec(mainupn, CL_ACC, acclra, &ac_t);
    Id * ip__ = &ac->xactn[prevlra == 0 ? -1 : -ACCHDR-1];
    Id * iz__ = &ac->xactn[AB-1];
    Bool chgd = false;

    if (ac == null)
      return false;

    if (prevlra == 0 and (kind & K_BFWD))
    { ac_t.props &= ~K_PAID;
      ac_t.props |= (kind & K_PAID);
      (void)write_rec(mainupn, CL_ACC, acclra, &ac_t);
      if (not all)
				return true;
    }

    prevlra = acclra;

    while (++ip__ <= iz__ and *ip__ != 0)
    { // do_db(-1,-1); 
      Bool fnd = ((*ip__ ^ ieny) & 0xbfffffff) == 0;
      if (all or fnd)
      { Quot ix = *ip__ < 0 ? IX_PMT : IX_SALE;
	Int ne = not all ? ieny :
	             (*ip__ & ~M_EPAID) | (~(*ip__ >> 1) & M_EPAID); /* paid */
	Key_t kvi; kvi.integer = *ip__ < 0 ? -*ip__ : *ip__;
	       	   kvi.integer &= 0x3fffffff;
      { Lra_t  lra;
	Sorp_t ssa_t;
	Sale sp_ = (Sale)ix_fetchrec(mainupn, ix, &kvi, &lra, &ssa_t);
	if (sp_ != null)
	{ sp_->kind &= ~K_PAID;
	  sp_->kind |= (kind & K_PAID);
	  sp_->match_grp = match_grp;
	  write_rec(mainupn, ddict[ix].cl_ix, lra, sp_);
	}
	if (*ip__ != ne)
	{ *ip__ = ne;
	  chgd = true;
	}
	if (fnd)
	{ stop = true;
	  break;
	}
      }}
    }
    if (chgd)
    { if (ac_t.id != cid)
      { i_log(cid, "PAcc id tag wrong %d", ac_t.id);
        ac_t.id = cid;
      }
      (void)write_rec(mainupn, CL_ACC, acclra, &ac_t);
    }
    if (stop)
      return true;
  }

  if (kind & K_NOT)
  { i_log(cid, "Accpaid err %lx", ieny);
    return false;
  }
  else
    return pay_acceny(cid, kind | K_NOT, id, match_grp);/* not just last five */
}}



public Cc create_account(upn, cid)
	   Id  upn;
	   Id  cid;
{ Lra_t acclra = 0;
  Key_t kv_t;  kv_t.integer = cid;
{ Account_t ac_t;
  Account acct = (Account)ix_fetchrec(upn, IX_CACC, &kv_t, &acclra, &ac_t);
  if (acct != null)
  { i_log(cid, "Account reappeared");
    return OK;
  }
  i_log(cid, "Creating account");
  return sorp_to_account(Q_CREATE, cid, 0, 0, 0);
}}


				/* Put Entry on the current Agent's Takings */
public Cc eny_to_accs(eny_)
	 Sorp	  eny_;		/* not in the cache */
{ Sale eny = (Sale)eny_;

  if (this_take.id != INTEREST_AGT)
    xactn_to_taking((Sorp)eny);
  eny->agent = this_take.id;

{ Lra_t  lra;
  Quot ix = eny->kind & K_PAYMENT ? IX_PMT : IX_SALE;
  Cc cc = ix == IX_PMT ? i_Paymt_t(mainupn, (Paymt)eny, &lra)
		       : i_Sale_t(mainupn, (Sale)eny, &lra);

  if (cc == OK and eny->customer != 0)
  { Int sign = (eny->kind & K_INTSUG  ? 0  : eny->kind & K_PAYMENT ? -1 : 1) *
	       (eny->kind & K_ANTI    ? -1 : 1);
    cc = sorp_to_account(0, eny->customer, ix, eny->id, sign * eny->total);
  }

  return cc;
}}

#if 0

public Cc clr_notk(upn, tid, kind_, id)
	 Id	upn;
	 Kind	kind_;
	 Id	tid, id;
{ Kind kind = kind_ & K_TPROPS;
  Lra_t lra = 0;
  Key_t kv_t; kv_t.integer = tid;
  wr_lock(upn);
{ Taking ta__ = (Taking)ix_srchrec(upn, IX_TAKE, &kv_t, &lra);

  for (; lra != 0; lra = ta__->next & (Q_EFLAG-1))
  { ta__ = (Taking)rec_ref(upn, lra);
    if (ta__ == null)
      break;
  { Tak ip__ = &ta__->eny[-1];
    Tak iz__ = &ta__->eny[TAKBUCKET-1];  /* ASLEEP */

    while (++ip__ <= iz__ and ip__->id != 0)
    { if ((ip__->kind & K_TPROPS) == kind and ip__->id == id)
      { ta__ = (Taking)rec_mark_/**/ref(upn, CL_TAKE, lra);
	ip__->kind &= ~K_TBP;
	wr_unlock(upn);
	return SUCCESS;
      }
    }
  }}

  wr_unlock(upn);
  return NOT_FOUND;
}}

#endif

				/* also sets this_bank.id */

public Cc tak_to_bank(upn, limit, take)
	 Id	 upn;	/* write locked */
	 Int	 limit;
	 Taking  take;
{ Id   tid = take->id;
  Lra_t flra;
  Key_t kv_t;	 kv_t.integer = this_bank.id != 0 ? this_bank.id
						  : last_id(upn, IX_BANK);
{ Bank ba = (Bank)ix_fetchrec(upn, IX_BANK, &kv_t, &flra, &this_bank);
  Lra_t lastlra = flra;
  Bool first = ba == null or this_bank.version > limit;
  if (first)
  { memset(&this_bank, 0, sizeof(this_bank));
    this_bank.id = last_id(upn, IX_BANK) + 1;
  }
  add_cash(&this_bank, take);

{ Cc cc = OK;
  if (first)
  { flra = 0;
    this_bank.date    = today;
    this_bank.version = 1;
    this_bank.eny[0]  = tid;
    cc = i_Bank_t(mainupn, &this_bank, &flra);
  }
  else
	{ Bank_t ba_t;
    this_bank.version += 1;
    (void)write_rec(upn, CL_BANK, flra, &this_bank);
    
    ba_t = this_bank;

    for (; true; ba = (Bank)read_rec(upn, CL_BANK, lastlra, &ba_t))
    { // do_db(-1,-1); 
      Id * iz = &ba_t.eny[BANKBUCKET-1];
		  Id * ip = &ba_t.eny[-1];

      while (++ip <= iz and *ip != 0)
				if (*ip == tid)
				  return HALTED;

      if (s_offs(ba_t.next) == 0)
      { if (ip > iz)
				  break;
				*ip = tid;
				if (++ip <= iz)
				  *ip = 0;
				(void)write_rec(upn, CL_BANK, lastlra, &ba_t);
				return OK;
      }
	
      lastlra = ba_t.next & (Q_EFLAG-1);
    }
  { Lra_t lra = 0;
    Bank_t bank = null_sct(Bank_t);
    bank.id     = this_bank.id;
    bank.next	= Q_EFLAG;
    bank.eny[0] = tid;
    cc = new_init_rec(mainupn, CL_BANK, (char*)&bank, &lra);
    ba_t.next = (ba_t.next & Q_EFLAG) + lra;
    (void)write_rec(mainupn, CL_BANK, lastlra, &ba_t);
  }}
  return cc;	
}}}

public Int next_match_grp()

{ 
  Int res = (last_match_grp + 1) % 100;
 /*i_log(last_match_grp, "LMG %ld", res);*/
  return res;
}

					/* also uses load_acc_meny */
public Set commit_match(wh, acc, set_, matix_, v)
	 Quot	  wh;			/* write locked */
	 Accdescr acc;
	 Set	  set_;
	 Int      matix_;
	 Cash     v;
{ fast Set set = set_ & 0x7fffffff;
       Bool tail = (set_ & 0x80000000) != 0;
       Int szm1 = acc->curr_last;
  fast Accitem iip = &acc->c[not tail ? 0 : szm1];
       Int matix = matix_;

  if (matix >= 0)
    set_ = 0;

  if (wh != Q_CHK)
    last_match_grp = next_match_grp();
/*
  if (set_ == -1)
  { if (wh != Q_CHK)
    { Set props = load_acc_meny == -1 ? K_BFWD+K_PAID		:
  		  load_acc_meny < 0   ? K_PAYMENT+K_MOVD+K_PAID :
					K_SALE+K_MOVD+K_PAID;
      Id id = abs(load_acc_meny);
      if (id == 0)
      	i_log(acc->id, "lam 0");
      else
      	pay_acceny(acc->id, props, id, last_match_grp);
    }
    return 0;
  }
*/
  for (; szm1 >= 0; --szm1)
  { // do_db(-1,-1); 
    Set props = (set & 1) or matix > 0 ? K_PAID : 0;
    --matix;
  
    if (matix_ >= 0 or not (iip->props & (K_PAID + K_CASH + K_IP + K_PD)))
    { 
      if ((props != 0) != ((iip->props & (K_PAID + K_CASH + K_IP + K_PD))!=0))
      { Quot ix = iip->props & (K_SALE + K_GIN) ? IX_SALE :
			  iip->props & K_BFWD		? IX_CACC : IX_PMT;
				if (iip->id == 0 and ix != IX_CACC)
				  i_log(acc->id, "Acceny is 0");
				if (wh == Q_CHK)
				{ Key_t kvi; kvi.integer = iip->id;
				{ Lra_t  lra;
				  Sale_t ss_t;
				  fast Sale sp_ = (Sale)ix_fetchrec(mainupn, ix, &kvi, &lra, &ss_t);
				  if (sp_ == null)
				    i_log(35, "Lost an account entry");
				  else
				  { Cash t = ix == IX_CACC ? ((Account)sp_)->bfwd : sp_->total;
				    if (ix == IX_PMT)
				      t = -t;
				    if (iip->props & K_ANTI)
				      t = -t;
				    v -= t;
				    if (ix == IX_CACC ? ((Account)sp_)->props & K_PAID
	    									      : sp_->kind & K_PAID)
				      return 0;
	  			}
				}}
				else
				{ Set props_ = (set & 1) or matix >= 0 ? K_PAID : 0;
				  pay_acceny(acc->id, (iip->props &~(K_MOVD+K_PAID)) | props_, iip->id,
								     last_match_grp);
          if ((iip->props & K_PAID) != props_)
				  { iip->props &= ~K_PAID;
				    iip->props |= props_;
				    iip->match_grp = 0;
				    if (props_ & K_PAID)
				      iip->match_grp = last_match_grp;
				  }
				}
      }
      set = set >> 1;
    }
    if (tail)
      --iip;
    else
      ++iip;
  }
  return v != 0 ? 0 : set_;
}

public Id get_one_cust(upn, ds, props, prx)	 /* similar code follows */
	 Id	upn;
	 Char * ds;
	 Set	props;
	 void   prx();
{ Key_t  kv_t;	
  Cc cc = get_sname(ds, &kv_t);
  if (cc != OK)
    return 0;
  (void)count_busys(null);

{ Id ix = props & P_SUPPLR ? IX_SUPDCN : IX_CUSTSN;
  Char * strm = ix_new_stream(upn, ix, &kv_t);
  Surname param_name; strpcpy(&param_name[0], kv_t.string, sizeof(Surname));

  (void)count_busys(null);
{ Char rd_ch = 0;
  Bool prevfits = false;
  Int suppress = (props & P_LITFST) ? -1 : 0;
  fast Int i;
  Customer_t nxt_cust;
  this_custmr = null_sct(Customer_t);
  strpcpy(&this_custmr.surname[0], ds, sizeof(Surname));

  for (i = 0; ;)
  { // do_db(-1,-1); 
    Lra_t lra;
    Customer cust = (Customer)ix_next_rec(strm, &lra, &nxt_cust);
    (void)count_busys(null);
    if (cust != null)
    { if ((props & P_NOSPEC) and
	        (in_range(nxt_cust.id, LSTRESACC, 0) or
      	   (nxt_cust.terms & P_DLTD) and nxt_cust.terms != -1))
        continue;
      if ((props & P_ACCONLY) and bill_alt(nxt_cust.terms))
        continue;
    }

    terms_to_asc(this_custmr.terms, &this_sale_terms[0]);
    if (bill_alt(this_custmr.terms))
      sprintf(&this_sale_terms[0], "Bill %d", this_custmr.creditlim);
    prx();

  { Bool fits = struccmp(nxt_cust.surname, param_name) == 0;
    if (i == 1 and prevfits and not fits)
    { rd_ch = A_CR;
      break;
    }
    if (i != suppress)
    { rd_ch = salerth(sc2d);
      if (rd_ch == A_CR)
      	rd_ch = hold();
      if (rd_ch != A_SP or cust == null)
      	break;
    }
    prevfits = fits;
    memcpy(&this_custmr, &nxt_cust, sizeof(Customer_t));
    ++i;
  }}

  clr_alert();
  ix_fini_stream(strm);
  (void)count_busys(null);
  vfy_lock(upn);
  if (rd_ch != A_CR)
  { this_custmr = null_sct(Customer_t);
    strpcpy(&this_custmr.surname[0], param_name, sizeof(Surname));
    prx();
  }
  return this_custmr.id;
}}}

public Id get_one_agent(upn, ds, props, prx) /* similar code above */
	 Id	upn;
	 Char * ds;
	 Set	props;
	 void   prx();
{ Key_t  kv_t;	
  Cc cc = get_sname(ds, &kv_t);

  Char * strm = ix_new_stream(upn, IX_AGNTNM, cc != OK ? null : &kv_t);
  Surname param_name; strpcpy(&param_name[0], kv_t.string, sizeof(Surname));

{ Char rd_ch = 0;
  Bool prevfits = false;
  Int suppress = (props & P_LITFST) ? -1 : 0;
  Int i;
  Agent_t nxt_agnt;
  that_agent = null_sct(Agent_t);
  strpcpy(&that_agent.name[0], ds, sizeof(Surname));

  for (i = 0; ; ++i)
  { // do_db(-1,-1); 
    Lra_t lra;
    Agent ag = (Agent)ix_next_rec(strm, &lra, &nxt_agnt);
    if (ag == null)
      break;
  { Bool fits = struccmp(nxt_agnt.name, param_name) == 0;
    prx();
    if (i == 1 and prevfits and not fits)
    { rd_ch = A_CR;
      break;
    }
    if (i != suppress)
    { rd_ch = salerth(sc2d);
      if (rd_ch == A_CR)
      	rd_ch = hold();
      if (rd_ch != A_SP or ag == null)
      	break;
    }
    prevfits = fits;
    memcpy(&that_agent, &nxt_agnt, sizeof(Agent_t));
  }}

  clr_alert();
  ix_fini_stream(strm);
  vfy_lock(upn);
  if (rd_ch == A_SP)
  { that_agent = null_sct(Agent_t);
    strpcpy(&that_agent.name[0], param_name, sizeof(Surname));
    prx();
  }
  return that_agent.id;
}}

public Cc del_pmt(upn, id)
	Id    upn;
	Id    id;
{ Paymt_t paymt;  paymt.id = id;

  wr_lock(upn);
{ Cc cc = paymt.id == last_id(upn, IX_PMT) ? OK : d_Paymt_t(upn,&paymt);
  wr_unlock(upn);
  return cc;
}}



public Cc del_exp(upn, id)
	Id    upn;
	Id    id;
{ Expense_t exp;  exp.id = id;

  wr_lock(upn);
{ Cc cc = exp.id == last_id(upn, IX_PMT) ? OK : d_Expense_t(upn,&exp);
  wr_unlock(upn);
  return cc;
}}



public Cc del_sale(upn, id)
	Id    upn;
	Id    id;
{ Cc cc = OK;
  wr_lock(upn);
  if (last_id(upn, IX_SALE) != id)
  { Sale_t sa_t;  sa_t.id = id;
    cc = d_Sale_t(upn, &sa_t);
  }
  wr_unlock(upn);

  if (cc == OK)
  { Invoice_t inv_t;  inv_t.id = id;
    wr_lock(invupn);
    cc = d_Invoice_t(upn, &inv_t);
    wr_unlock(invupn);
  }
  return cc;
}

public Cc del_acc(upn, id)
	Id    upn;
	Id    id;
{ Account_t acc_t;  acc_t.id = id;
  wr_lock(upn);
{ Cc cc = d_Account_t(upn, &acc_t);
  wr_unlock(upn);
  return cc;
}}


public Cc del_tak(upn, id)
	Id    upn;
	Id    id;
{ Taking_t tak_t;  tak_t.id = id;
  wr_lock(upn);
{ Cc cc = tak_t.id == last_id(upn, IX_TAKE) ? OK : d_Taking_t(upn, &tak_t);
  wr_unlock(upn);
  return cc;
}}



public Cc del_bank(upn, id)
	Id    upn;
	Id    id;
{ Bank_t bank_t;  bank_t.id = id;
  wr_lock(upn);
{ Cc cc = bank_t.id == last_id(upn, IX_BANK) ? OK : d_Bank_t(upn, &bank_t);
  wr_unlock(upn);
  return cc;
}}



public Cc del_round(upn, id)
	Id    upn;
	Id    id;
{ Round_t round;  round.id = id;
  wr_lock(upn);
{ Cc cc = /* round.id == last_id(upn, IX_ROUND) ? OK :*/d_Round_t(upn,&round);
  wr_unlock(upn);
  return cc;
}}

			/* only called from del_sorp() in recover.c */

public Cc sa_action(ix, lra)
	Quot   ix;
	Lra_t  lra;
{ Sorp_t sorp_t;
  Sorp ip = (Sorp)read_rec(mainupn, ddict[ix].cl_ix, lra, &sorp_t);
  sorp_t.kind &= ~K_NOAC;
  if (ip != null)
    (void)write_rec(mainupn, ddict[ix].cl_ix, lra, &sorp_t);

  return cache_err == OK ? OK : CORRUPT_DB;
}


public void scan_acc(upn, action)
	Id     upn;
	void   action();
{ Int ct = 0;
  Account_t accbuff;
  Lra_t  lra;
  wr_lock(upn);
{ Char * strm = init_rec_strm(upn, CL_ACC);

  while (next_rec(strm, &lra, &accbuff) != null)
  { // do_db(-1,-1); 
    Account acct = (Account)read_rec(upn, CL_ACC, lra, &accbuff);
    Id * ip = &acct->xactn[-1];

    if (acct == null)
      break;
    if (acct->next & Q_EFLAG)
      ip = &ip[-ACCHDR];

    while (++ip <= &accbuff.xactn[AB-1] and *ip != 0)
    { Id ix = *ip > 0 ? IX_SALE : IX_PMT;
      Key_t kvii; kvii.integer = *ip > 0 ? *ip : -*ip;
      if (kvii.integer & M_EPAID)
	kvii.integer &= ~M_EPAID;
    { Lra_t lrasp;
      Cc cc = ix_search(upn, ix, &kvii, &lrasp);
      if (cc != OK)
	i_log(0, "lost dlt acc eny");
      else
	(void)action(ix, lrasp);
    }}	
    if ((++ct & 0x3f) == 0)
      wr_lock_window(upn);
  }
  wr_unlock(upn);
}}

#define AB (ACCBUCKET-ACCHDR)

public Cc save_acct(upn, acc)          /* Only done after compacting */
      Id        upn;                    /* Locks on                   */
      Accdescr  acc;
{ Key_t kvi;  kvi.integer = acc->id;
  wr_lock(upn);
{ Bool got = true;
	Lra_t lra, lra0;
  Account acct = (Account)ix_fetchrec(upn, IX_CACC, &kvi, &lra, &ths.acct);
  if (acct == null)
  { got = false;
    acct = &ths.acct;
    lra = 0;
    memset(&ths.acct, 0, sizeof(ths.acct));
    ths.acct.id = acc->id;
  }
  lra0 = lra;
  
  ths.acct.version += 1;

{      Cc cc = OK;
       Cash   bfwd_bal = 0;
       Date   bfwd_date = 0;
       Lra_t  nlra, flra = 0;
       Lra_t  qext = 0;
       Account_t  bkt_t;
       Int  bkt_ct = 0;
  fast Accitem iip = &acc->c[0];
       Accitem iiz = &acc->c[acc->curr_last];
       Set props = iip->props;

  if (not (props & K_BFWD))
    --iip;
  else
  { bfwd_bal = iip->balance;
    bfwd_date = iip->idate;
    props = 0;			/* always not paid */
  }

  props &= K_PAID;

  if (ths.acct.id != kvi.integer)
  { i_log(kvi.integer, "Racc id tag wrong %d", ths.acct.id);
    ths.acct.id = kvi.integer;
  }

  memcpy(&bkt_t, &ths.acct, sizeof(ths.acct));
  bkt_t.lastfive    = 0;
  bkt_t.version    += 1;
  bkt_t.props       = (props & (K_PAID+ K_BFWD));
  bkt_t.prt_date    = 0;
  bkt_t.unpaid_date = 0;

  while (iip < iiz)
  { // do_db(-1,-1); 
    ++bkt_ct;
    bkt_t.next      = qext;
    bkt_t.bfwd      = bfwd_bal;
    bkt_t.bfwd_date = bfwd_date;
  { Id * ip = &bkt_t.xactn[-1];

    if (not (bkt_t.next & Q_EFLAG))
      bkt_t.props = props;
    else
    { ip = &ip[-ACCHDR];
      ((Accountext)&bkt_t)->props = props;
    }

    memset(&ip[1], 0, (&bkt_t.xactn[ACCBUCKET-ACCHDR-1] - ip) * sizeof(Id));

    while (ip < &bkt_t.xactn[AB-1])
    { if (iip >= iiz)
      	break;
      ++iip;                      /* more input */
      *(++ip) = (iip->props & K_PAYMENT ? -iip->id : abs(iip->id));
      if (iip->props & K_PAID)
      	*ip ^= M_EPAID;
      else
      	props = 0;
      bfwd_date = iip->idate;
    { Cash val = iip->val;
      if ((iip->props & (K_SALE+K_CASH)) == K_SALE+K_CASH)
        val = 0;
      if (iip->props & K_PAYMENT)
        val = -val;
      if (iip->props & K_ANTI)
        val = -val;
      bfwd_bal += val;
      if (bfwd_bal != iip->balance)
        i_log(bfwd_bal - iip->balance, "AccBal");
    /*i_log(iip->id, "Id %d %d", iip->val, iip->balance); */
    }}
    if (ip < &bkt_t.xactn[AB-1])
      *(++ip) = 0;

    if (bkt_t.id != kvi.integer)
    { i_log(kvi.integer, "RAcc id tag wrong %d", bkt_t.id);
      bkt_t.id = kvi.integer;
    }
      
    (void)new_init_rec(upn, CL_ACC, (char*)&bkt_t, &nlra);
    if (flra == 0)
      flra = nlra;
    else
    { Account_t ac_t;
      Account prev = (Account)read_rec(upn, CL_ACC, lra, &ac_t);
      prev->next |= nlra;
      (void)write_rec(upn, CL_ACC, lra, prev);
    }
    lra = nlra;
    qext = Q_EFLAG;
  }} 

  lra = flra;
  cc = lra == 0  ? OK :
       not got   ? ix_insert(upn, IX_CACC, &kvi, &lra)
                 : ix_replace(upn, IX_CACC, &kvi, &lra);
  if (cc != OK)
  { i_log(acc->id, "Recov %s failed (%d)", not got ? "INS" : "RPL", cc);
    wr_unlock(upn);
    return cc;
  }
  
  if (bkt_ct > RACCTSZ+1)
  { Lra_t lastfive = 0;			/* derive a pointer to the last five */
    
    for (lra = flra; s_page(lra) != 0; lra = bkt_t.next & (Q_EFLAG-1))
    { Account acct = (Account)read_rec(upn, CL_ACC, lra, &bkt_t);
      if (acct == null)
      { i_log(kvi.integer, "Acc bad in bkts");
      	break;
      }
      if (--bkt_ct >= RACCTSZ)
      lastfive = lra;
    }
    acct = (Account)read_rec(upn, CL_ACC, flra, &ths.acct);
    if (acct != null)			/* and write it in the first */
    { ths.acct.lastfive = lastfive;
      (void)write_rec(upn, CL_ACC, flra, &ths.acct);
    }
  }

  for (lra = lra0; s_page(lra) != 0; )
  { // do_db(-1,-1); 
    Account acct = (Account)read_rec(upn, CL_ACC, lra, &bkt_t);
    if (acct == null)
    { i_log(5, "Acc %d corr.", ths.acct.id);
      cc = CORRUPT_DB;
      break;
    }
  { Lra_t nlra = bkt_t.next & (Q_EFLAG-1);
    del_rec(upn, CL_ACC, lra);
    lra = nlra;
  }}  
    
  wr_unlock(upn);
  return cc;
}}}

public Cc do_delete_acc(acc)
      Accdescr  acc;
{ Lra_t lra, lra0;
  Key_t kvi;  kvi.integer = acc->id;
  wr_lock(mainupn);
{ Bool got = true;
  Account acct = (Account)ix_fetchrec(mainupn, IX_CACC, &kvi, &lra, &ths.acct);
  if (acct == null)
    return OK;

  while (lra != 0)
  { // do_db(-1,-1);
    Account_t ac_t;
    Account ac = (Account)read_rec(mainupn, CL_ACC, lra, &ac_t);
    Cc cc = del_rec(mainupn, CL_ACC, lra);
    lra = ac->next & (Q_EFLAG-1);
  } 

  Cc cc = b_delete(mainupn, IX_CACC, &kvi, &lra);
    
  wr_unlock(mainupn);
  return cc;
}}

public Int do_compact_acc(id, acc, date)
	Id	 id;
	Accdescr acc;
	Date	 date;
{ fast Accitem iip = &acc->c[0];
       Accitem iiz = &acc->c[acc->curr_last];
       Accitem iipt = iip;

  Bool isbf = iip <= iiz and (iip[0].props & K_BFWD);
  Cash bfwd = not isbf ? 0 : iip[0].val;
  Date bfwd_date = bfwd == 0 ? 0 : iip[0].idate;

  Int ct = 0;

  if (not isbf)
  { --iip; --iipt; }

  while (++iip <= iiz)
    if ((iip->props & K_DEAD) or iip->idate <= date)
    { ++ct;
      if (iip->idate > bfwd_date)
	bfwd_date = iip->idate;
    }
    else
    { ++iipt;
      if (iipt != iip)
	memcpy(iipt, iip, sizeof(Accitem_t));
    }

  if (ct != 0)
  { acc->curr_last -= ct;
    recalc_acc(acc);
    bfwd += this_a_diff;
    iip = &acc->c[0];
    if (bfwd == 0)
    { if (isbf)
      { memmove(iip, &iip[1], acc->curr_last * sizeof(Accitem_t));
        acc->curr_last -= 1;
      }
    }
    else
    { if (not isbf)
      { extend_acc(acc);
        iip = &acc->c[0];
        if (acc->curr_last > 1)
          memmove(&iip[1], &iip[0], (acc->curr_last-1) * sizeof(Accitem_t));
      }

      iip->props   = K_BFWD;
      iip->id	   = 0;
      iip->idate   = bfwd_date;
      iip->vat	   = 0;
      iip->val	   = bfwd;		
      iip->balance = bfwd;
    }
    wr_lock(mainupn);
    recalc_acc(acc);
    if (save_acct(mainupn, acc) != SUCCESS)
      i_log(0, "SS");
    wr_unlock(mainupn);
  }

  return ct;
}

public Char * find_that_agent(upn, aid)
       Id  upn;
       Id  aid;
{ Key_t kvi; kvi.integer = aid;
  rd_lock(upn);
{ Lra_t lra;
  Agent ag = (Agent)ix_fetchrec(upn, IX_AGNT, &kvi, &lra, &that_agent);
  rd_unlock(upn);
  return ag != null ? null : "No such employee";
}}



static Prt_t  alt_prt;

public Prt select_prt(cat)
	 Char	cat;
{ Char fst_ch;
  Char choice[99];
  Prt prt = select_io(this_tty, cat);

  if (prt->path[0] == 0 and prt->file[0] == 0)
  { salerth("You have no listing device");
    return null;
  }

  sprintf(&choice[0],prt->file[0] == 0 ? "Prt %s (P, X)"
				       : "Prt %s File %s (P, F, X)",
		       prt->path, prt->file);
  salert(choice);
  get_data(&choice[0]);
  tteeol();
  clr_alert();
  fst_ch = toupper(choice[0]);

  if	  (fst_ch == 'P')
    ;
  else if (fst_ch == 'F' and prt->file[0] != 0)
  { alt_prt.pagelen = prt->pagelen;
    strpcpy(&alt_prt.path[0], prt->file, sizeof(Path));
    prt = &alt_prt;
  }
  else
    prt = null;
  return prt;
}

public const Char * do_access_files(cmd, addfld)
	Set  cmd;
	Id   addfld;
{ Int len = strlen(this_prt.file);

  if (this_prt.file[len - 1] != '/')
    strpcpy(&ds1[0], this_prt.file, sizeof(Path));
  else
  { Int last = last_file(this_prt.file);
    Int wh = last;
    if (wh <= 0)
      return "No files";

    while (true)
    { sprintf(&ds1[0], "Which file? (1 to %d)", last);
      salert(ds1);
      goto_fld(addfld);
      get_data(ds1);
      if (ds1[0] == '.')
      	return aban;
      if (ds1[0] == 0 and not (cmd & P_DL))
      	break;
      if (vfy_nninteger(ds1))
      { wh = atoi(ds1);
      	break;
      }
    }
    if (wh > last)
      return ntfd;
    sprintf(&ds1[0], "%s%d", this_prt.file, wh);
  }
  if ((cmd & P_ED) and disallow(R_EOP))
    return "You need permit O";
  if      (cmd & (P_VW + P_ED))
  { salerth("CTRL-X CTRL-C to quit, Any Key");
    sprintf(&ds[0], "e %s %s", cmd & P_VW ? "-v -r " : "", ds1);
  }
  else if (cmd & P_PT)
  { Char * err = prt_lpt_file((Char *)&this_prt, ds1);
    if (err != null)
      return err;
    ds[0] = 0;
  }
  else if (cmd & P_DL)
    sprintf(&ds[0], "rm %s", ds1);

  return *ds == 0  or
	 do_osys(ds) == SUCCESS ? null : "Failed";
}


public Char * preq(Char v)

{ static Char pmsg[] = "Permit ? Required";
  pmsg[7] = v;
  return pmsg;
}



static const Char line_cmds[][10] = 
{
".",
"ACCEPT",
"ADJ",
"ASSIGN",
"AUDIT",
"CASH",
"CHECK",
"CHOP",
"CLOSE",
"COPY",
"CREATE",
"D",
"DELETE",
"DNOTE",
"DROP",
"EDI",
"EXIT",
"FIND",
"FLIP",
"GO",
"I",
"LIST",
"ORDER",
"PRINT",
"REDO",
"SAVE",
"SELECT",
"STOCK", 
"SUBMIT",
"UNORDER",
"STATS",
"LEVY",
"SUGG",
"HELP",
"UNSUGG",
"INTEREST",
};


public Int sel_cmd(Char * line)

{ Int ix;
  for (ix = sizeof(line_cmds)/ sizeof(line_cmds[0]); --ix >= 0; )
    if (*strmatch(&line_cmds[ix][0], line) == 0)
      return ix;

  return -1;
}
