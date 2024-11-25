#include        <fcntl.h>
#include        <stdio.h>
#include	"version.h"
#include	"../../h/defs.h"
#include	"../../h/errs.h"
#include	"../../db/cache.h"
#include	"../../db/page.h"
#include	"../../db/recs.h"
#include	"../../db/b_tree.h"
#include	"../domains.h" 
#include	"../schema.h" 



public Cc init_ix(upn)
	Id   upn;
{ wr_lock(upn);
{ Cc cc = init_roottbl(-upn);
  if (cc != OK)
    i_log(cc,"ix init");
  else
  { Int ix;
    for (ix = IX_stt; ix <= IX_end; ++ix)
    { /* ddict[ix].size_ix = (ddict[ix].size_ix + 1) & -2; */
      cc = add_index(upn,ix, 
			ddict[ix].kind_ix,ddict[ix].size_ix,ddict[ix].cl_ix);
      if (cc != OK)
      { i_log(cc, "init_ix: add_index %d", cc);
        break;
      }
    }
  }
  wr_unlock(upn);
  return cc;
}}

public Cc init_db(dbname, props)
	Char * dbname;
	Set    props;
{ Int ix;				/* default values */
  for (ix = IX_end+1; --ix >= IX_stt; )
  { ddict[ix].kind_ix = INTEGER;
    ddict[ix].size_ix = sizeof(Id);
  }

  ddict[CL_UNIQ].size_cl = sizeof(Unique_t);
  ddict[CL_PRT ].size_cl = sizeof(Prt_t);
  ddict[CL_SI  ].size_cl = sizeof(Stockitem_t);
  ddict[CL_SUP ].size_cl = sizeof(Supplier_t);
  ddict[CL_CUST].size_cl = sizeof(Customer_t);
  ddict[CL_SR].size_cl   = sizeof(Stockred_t);
  ddict[CL_AGNT].size_cl = sizeof(Agent_t);
  ddict[CL_INV ].size_cl = sizeof(Invoice_t);
  ddict[CL_SALE].size_cl = sizeof(Sale_t);
  ddict[CL_PMT ].size_cl = sizeof(Paymt_t);
  ddict[CL_ACC ].size_cl = sizeof(Account_t);
  ddict[CL_EXP ].size_cl = sizeof(Expense_t);
  ddict[CL_ROUND].size_cl= sizeof(Round_t);
  ddict[CL_TAKE].size_cl = sizeof(Taking_t);
  ddict[CL_BANK].size_cl = sizeof(Bank_t);
  ddict[CL_TRANCHE].size_cl = sizeof(Tranche_t);
  ddict[CL_TT_REG].size_cl = sizeof(Tt_reg_t);
   
  ddict[IX_SUPDCN].size_ix = sizeof(Surname);
  ddict[IX_CUSTSN].size_ix = sizeof(Surname);
  ddict[IX_STKBC ].size_ix = sizeof(Stkkey);
  ddict[IX_AGNTNM].size_ix = sizeof(Surname);
  
  ddict[IX_SUPDCN].kind_ix = NU_FSTR+UCKEY;
  ddict[IX_CUSTSN].kind_ix = NU_FSTR+UCKEY;
  ddict[IX_STKLN ].kind_ix = NU_INT;
  ddict[IX_STKBC ].kind_ix = FXD_STR;
  ddict[IX_SR    ].kind_ix = NU_INT;
  ddict[IX_AGNTNM].kind_ix = FXD_STR+UCKEY;
  ddict[IX_BACC  ].kind_ix = NU_INT;

  ddict[IX_PRT   ].cl_ix = CL_PRT;
  ddict[IX_SUP   ].cl_ix = CL_SUP;
  ddict[IX_SUPDCN].cl_ix = CL_SUP;
  ddict[IX_CUST  ].cl_ix = CL_CUST;
  ddict[IX_CUSTSN].cl_ix = CL_CUST;
  ddict[IX_STK   ].cl_ix = CL_SI;
  ddict[IX_STKLN ].cl_ix = CL_SI;
  ddict[IX_STKBC ].cl_ix = CL_SI;
  ddict[IX_SR    ].cl_ix = CL_SR;
  ddict[IX_AGNT  ].cl_ix = CL_AGNT;
  ddict[IX_AGNTNM].cl_ix = CL_AGNT;
  ddict[IX_INV   ].cl_ix = CL_INV;
  ddict[IX_SALE  ].cl_ix = CL_SALE;
  ddict[IX_PMT   ].cl_ix = CL_PMT;
  ddict[IX_EXP   ].cl_ix = CL_EXP;
  ddict[IX_CACC  ].cl_ix = CL_ACC;
  ddict[IX_ROUND ].cl_ix = CL_ROUND;
  ddict[IX_TAKE  ].cl_ix = CL_TAKE;
  ddict[IX_BANK  ].cl_ix = CL_BANK;
  ddict[IX_TCHE  ].cl_ix = CL_TRANCHE;
  ddict[IX_BACC  ].cl_ix = CL_CUST;
  ddict[IX_MANBC ].cl_ix = CL_SUP;
  ddict[IX_TT].cl_ix = CL_TT_REG;

{ Int cl;
  for (cl = CL_stt; cl <= CL_end; ++cl)
  { Vint sz = align(ddict[cl].size_cl, RP_ALIGN);
    Vint osoffs = 0;
    Vint soffs = RP_FREEVEC + RP_ALIGN;

    while (osoffs != soffs)
    { osoffs = soffs;
      ddict[cl].cl_vecsz = (unsigned)(BLOCK_SIZE - 2 - soffs) / sz;
      soffs = RP_FREEVEC * 8;
      if (ddict[cl].cl_vecsz > 8)
        soffs += ddict[cl].cl_vecsz-8;
      soffs = align(soffs,RP_ALIGN * 8) >> 3;
    }

    ddict[cl].cl_soffs = soffs;
    if (props & F_VERBOSE)
      fprintf(stderr, "CL %d  SZ %d  VS %d  SOFFS %d\n", 
			   cl, sz, ddict[cl].cl_vecsz, soffs);
/**/
  }

{ Id upn = chan_link(props, dbname);
  if (upn < 0)
    return upn;

{ Cc cc = OK;

  if (props & (F_NEW+F_NEWIX))
  { wr_lock(upn);
    if (props & F_NEW)
    { cc = init_roottbl(upn);
      if (cc != OK)
        i_log(cc,"root init");
      else 
        for (cl = CL_stt; cl <= CL_end; ++cl)
        { Vint sz = align(ddict[cl].size_cl, RP_ALIGN);
	  cc = add_class(upn, cl, sz);
	  if (cc != OK)
	  { i_log(cc, "init_db: add_class %d", cc);
	    break;
	  }
	}
    }
    if (cc == OK and (props & F_NEWIX))
      cc = init_ix(upn);
    wr_unlock(upn);
  }

  return cc != OK        		         ? cc      : 
	 (props & F_CHECK) and 
	  vfy_roottbl(upn, props & F_IXOPT) != 0 ? INVALID : upn;
}}}}

#if 0

public void scan_all(upn)
        Id    upn;
{ Char buff[BLOCK_SIZE];
  Char sbuf[BLOCK_SIZE];
  Account accbuf = (Account)&buff[0];
  Lra_t  lra;
  Key_t key_t;
  Int ix;
  
  for (ix = IX_stt; ix <= IX_end; ++ix)
  { Quot cl = ddict[ix].cl_ix;
    Char * strm = ix_new_stream(upn, ix, null);
    Int errct = 0;

    rd_lock(upn);
    key_t.string = &sbuf[0];

    while (next_of_ix(strm, &key_t, &lra) == OK)
    { Cc cc = write_rec(upn, cl, lra, null);
      if (cc != OK)
        ++errct;
      if (cl == CL_ACC or cl == CL_TAKE or cl == CL_BANK)
      { Bool fst = true;
        for (; s_page(lra) != 0; lra = accbuf->next & (Q_EFLAG-1))
        { Account acct = (Account)read_rec(upn, cl, lra, accbuf);
          Cc cc = write_rec(upn, cl, lra, null);
          if (cc != OK)
          { if (not fst)
              ++errct;
            break;
          }
          fst = false;
        }
      }
    }
    if (errct > 0)
      i_log(ix, "Overlaid records, type %d %d", cl, errct);
    ix_fini_stream(strm);
    rd_unlock(upn);
  }
}

#endif
