#include  <stdio.h>
#include  <ctype.h>
#include  <errno.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <fcntl.h>
#include  <sys/ipc.h>
#include  <signal.h>
#include "version.h"
#include "../h/defs.h"
#include "../h/errs.h"
#include "domains.h"
#include "schema.h"
#include "rights.h"
#include "../db/b_tree.h"
#include "generic.h"
#include "trail.h"

#define private 

static Id msgid = -1;

static Bool recovering;

static Bool eof;                /* attempt to read past eof */
static Msgbuf_t m;

static Char * ioptr;            /* first char in m */
static Char * ioend;            /* last char in m */

static Bool replied;		/* trail replied */



private void jreply()

{ replied = true;
}


private void at_basic __((Int)) forward;

Set j_props;

public void init_journal(props)
	Set props;
{ Bool force = (props & M_FORCE) != 0;
  
  j_props = props;
  
  if (msgid != -1 or (j_props & M_DUMMY))
    return;

  if (sizeof(Sorp_t) + sizeof(Int) > AT_REC_SZ)
    i_log(-2, "AT_REC_SZ");

  msgid = msgget(MSGKEY, 0777);
  if (msgid < 0)
  { msgid = msgget(MSGKEY, IPC_CREAT | 0777);
    if (msgid < 0)
      sysabort("Can't create msg q.");
    force = true;
  }
  if (not force)
  { m.mtype = getpid();
    m.mtext[0] = 'R';      /* signal me with SIGUSR1 */
    replied = false;
    signal(SIGUSR1, jreply);
    at_basic(SPECIAL_MSGLEN);
    sleep(5);
    force = not replied;
  }
  if (force)
  { Lra_t lra;
    Unique un = find_uniques(mainupn, &lra);
    if (un != null)
    { strpcpy(&this_at[0], &un->at_path[0], sizeof(Path));

      sprintf(&m.mtext[0], "./bin/trail %s %s_at 2>/dev/null &", this_at, maindb);
      i_log(6, ".\n\rRestarting trail");
      i_log(6, ".%s", m.mtext);
      do_osys(m.mtext);
    }
  }
}

public Int c_sum(blk, sz)
    Char * blk;
    Int    sz;
{ fast Char * p = &blk[sz];
  Int res = 17;

  while (--p >= blk)
    res += *p * 23;

  return res != TRAIL_TAG ? res : TRAIL_TAG + 1;
}



public Cc at_drop()               /* SUCCESS, IN_USE, NOT_FOUND */

{ return this_at[0] == 0 ? NOT_FOUND : unlink(this_at);
}

public Cc new_at_xactns()

{ Id at_fd = myopen(this_at, O_CREAT, 0644);
  if (at_fd < 0)
    return CORRUPT_PF;

  close(at_fd);
  return OK;
}


public void init_xactn_strm()

{ eof = false;
  ioptr = &m.mtext[BLOCK_SIZE] - AT_REC_SZ;
  ioend = &m.mtext[BLOCK_SIZE-1];
}



private void at_basic(len)
	Int  len;
{ Short ct;

  if (j_props & M_DUMMY)
    return;

  for (ct = 3; ct >= 0; --ct)
  { Cc cc = msgsnd(msgid, &m, len, IPC_NOWAIT);
    if (cc != -1)
      break;
    if (errno == EAGAIN and ct > 1)
    { salert("Queuing");
      sleep(5);
    }
    else
    { i_log(errno, "Restarting Audit Process");
      msgid = -1;
      init_journal(true);
      sleep(3);
    }
  }
  if (ct < 0)
    i_log(55, "A Trail prob.");
}



public void at_xactn(sorp)
	Sorp    sorp;	       /* breaks rule about ptrs into cache as params */
{ if (not recovering)
  { m.mtype = Q_DATA;
    memcpy(&m.mtext[0], sorp, (long)AT_REC_SZ - sizeof(Int));
    at_basic((long)AT_REC_SZ - sizeof(Int));
  }
}



public void at_chg(str)
	Char * str;
{ m.mtype = Q_CHG_DSK;
  strpcpy(&m.mtext[0], str, BLOCK_SIZE-1);
  at_basic(strlen(str)+1);
}



public void at_push(str)
	Char * str;
{ m.mtype = Q_PSH_DSK;
  strpcpy(&m.mtext[0], str, BLOCK_SIZE-1);

  at_basic(strlen(str)+1);
}



public void at_pop(oldstr, newstr)
	Char * oldstr, * newstr;
{ m.mtype = Q_POP_DSK;
  strpcpy(&m.mtext[0], oldstr, sizeof(Path));
  strcat( strcat(&m.mtext[0], " "), newstr);
  at_basic(strlen(m.mtext)+1);
}



public void at_take(take)
	Taking  take;
{ Sorp atake = (Sorp)&m.mtext[0];
  atake->expense.kind   = K_TAKE;
  atake->expense.id     = take->id;
  atake->expense.ref_no = take->agid;
  atake->expense.agent  = take->bankid;
  atake->expense.date   = take->tdate;
  atake->expense.total  = 0;
  memcpy(&atake->expense.vat, &take->u5000, sizeof(Descn)+3 * sizeof(Int));
  at_xactn(atake);				/* approx. */
}	



public void fini_at_xactns()

{
}


public void init_recover()

{ recovering = true;
}



public void fini_recover()

{ recovering = false;
}

			/* this page of code copied in trail.c */

private Int rd_block(f)
       Fildes f;
{ Int ct = (Int)&m.mtext[BLOCK_SIZE-1] - (Int)ioend;
  for (; ct > 0; --ct)
  { Cc cc = read(f, &ioend[1], 1);
    if (cc <= 0)
      if (cc == 0)
      { eof = true;
      	break;
      }
      else
      { i_log(-1, "Read Error in Backup File");
    	  return -1;
      }
    ioend += 1;
  }

  return ioend - ioptr + 1;
}


public Char * rd_next(f)
      Fildes f;
{ ioptr += AT_REC_SZ;
{ Int sz = ioend - ioptr + 1;
  if (sz < AT_REC_SZ)
  { if (sz > 0)
      memcpy(&m.mtext[0], ioptr, sz);
    ioend -= (Int)ioptr - (Int)&m.mtext[0];
    ioptr = &m.mtext[0];
    if (not eof)
      sz = rd_block(f);
    if (sz < 2 * sizeof(Int))
      return null;
  }
  return *(Int*)ioptr == TRAIL_TAG ? null : ioptr;
}}

public Cc rd_nxt_rec(f, rec_ref)
      Fildes f;
      Char ** rec_ref;
{ ioptr = rd_next(f);
  if (ioptr == null)
    return NOT_FOUND;
{ Int cs = *(Int*)ioptr;
  Char * rec = (Char *)&ioptr[sizeof(Int)];
  Int sz = AT_REC_SZ - sizeof(Int);

  if (cs == TRAIL_TAG)
    return NOT_FOUND;

  if (cs != c_sum(rec, sz))
    return INVALID;

  *rec_ref = rec;
  return OK;
}}



public Fildes open_read_at(srcfn)
	 Char *   srcfn;
{ Int fildes = open(srcfn, O_RDONLY, 0644);
  if (fildes >= 0)
  { Au_header_t  auh;
    lseek(fildes, WASTE_AREA, 0);
  { Cc cc = read(fildes, (Char *)&auh, sizeof(auh));

    if (cc == sizeof(auh) and auh.tag == TRAIL_TAG)
      init_xactn_strm();
    else
    { close(fildes);
      fildes = -1;
    }
  }}
  return fildes;
}

public const Char * at_prt_sorp(which, stt, stp, agnt, custmr, istak, rfnds)
	Set   which;
	Date  stt, stp;
	Id    agnt, custmr;
	Bool  istak, rfnds;
{ if (disallow(R_RPT))
    return preq('G');
{ Char * chead =
 " Type       Id  Customer Taking  Date       Vat     Total  Cheque-No  Balance";
  Char work[81+80];
  Char * row_fmt_s =
 "    %7r    %6d     %6d   %5d       %9r     %8v       %9v       %9d       %10v\n";
  Prt chan = select_prt(0);
  if (chan == null)
    return aban;
{ Char area[100];
  Int ct = 0;
  Char * lines =   "----------------------------------          ------------";

  sprintf(&area[0], "%s\n%s\n%s\n\n", lines, " Backup (%r) %46!%r", lines);
  sprintf(&work[0], "%s%s%s",
		not (which & K_SALE)    ? "" : "Sales",
	       (which & (K_SALE+K_PAYMENT)) != (K_SALE+K_PAYMENT) ? "" : " ",
	        not (which & K_PAYMENT) ? "" : "Payments");
  fmt_data(&msg[0], area, work, todaystr);
{ Char * err = prt_one_report(chan, msg, " AUDIT TRAIL\n\n\n\n\n\n\n\n\n\n\n");
  if (err != null)
    return err;
  sprintf(&work[0], "%s\n%s\n", chead, hline(78));
  prt_set_cols(work, row_fmt_s);
  prt_head();

{ Cc cc;
  Sale sa;
  Fildes f = open_read_at(this_at);
  if (f < 0)
    return "Not an Audit File";

  while ((cc = rd_nxt_rec(f, (Char**)&sa)) != NOT_FOUND)
    if (cc != OK)
      prt_text("BAD RECORD\n");
    else
    { Char d[15];
      if (sa->date < stt or sa->date > stp)
				continue;

      date_to_asc(&d[0], sa->date, 0);

      if (sa->kind == 0)
      { sprintf(&area[0], "BACKUP %ld at %s\n", sa->id, d);
				prt_text(area);
				continue;
      }
	
      if (istak and not (sa->kind & K_TAKE))
				continue;

      if (rfnds and (sa->kind & (K_SALE+K_ANTI)) != K_SALE+K_ANTI
			and (sa->kind & (K_SALE+K_ANTI)) != K_SALE+K_DEAD)
				continue;

      if (custmr != 0 and sa->customer != custmr and sa->kind != K_TAKE)
				continue;
      if (agnt != 0 && ((int)sa->agent & 0xffff) != agnt)
				continue;
	
      ++ct;
      if      ((which & K_TAKE) and (sa->kind &  K_TAKE))
      { memcpy(&this_take.u5000,&((Expense)sa)->ref_no,
       						sizeof(Descn) + 3*sizeof(Int));
      { Cash mtot = this_take.u5000 + this_take.u2000 + this_take.u1000 +
	            this_take.u500  +                   this_take.u100  +
	          this_take.u50   + this_take.u20 + this_take.u5 + this_take.u1;
				prt_row(0, "TAKING", sa->id, sa->customer, ((int)sa->agent & 0xffff), d, 
								0, 0, sa->chq_no,mtot);
      }}
      if      ((which & K_PAYMENT) and (sa->kind & K_PAYMENT))
      { Paymt pt = (Paymt)sa;
				prt_row(0, pt->kind & K_UNB  ? "DISCT" :
	    			    pt->chq_no < 0    ? "CASH"  : "CHEQUE",
        				pt->id, pt->customer, ((int)pt->agent & 0xffff), d,0,pt->total, 
                pt->chq_no, pt->balance);
      }
      else if ((which & K_SALE) and (sa->kind & K_SALE))
      { Char typestr_[8];
				Char * s = (sa->kind & K_ANTI) ? sa->kind & K_CASH ? "CREFUND":"RETURN" :
	       		       (sa->kind & K_CASH) ? "CSALE"  :"INV";
				if (sa->kind & K_MOVD)
				{ Char * t = &typestr_[0];
				  for (; *s != 0; ++s)
				    *t++ = tolower(*s);
				  *t = 0;
				  s = typestr_;
				}
				prt_row(0, s, sa->id, sa->customer, ((int)sa->agent & 0xffff), d,
						   (sa->vattotal*sa->vat0_1pc+500)/1000, sa->total, sa->chq_no,
                sa->balance);
/*              sa->disc0_1pc * 10); */
      }
      else if ((which & K_GIN) and (sa->kind & K_GIN))
				prt_row(0, "GOODS-IN",
							  sa->id, sa->customer, ((int)sa->agent & 0xffff), d,
							  (sa->vattotal*sa->vat0_1pc + 500)/1000, sa->total, sa->chq_no,
                sa->disct);
      else if ((which & K_EXP) and (sa->kind & K_EXP))
				prt_row(0, "Expense",
							  sa->id, 0, ((int)sa->agent & 0xffff), d,0, sa->total, sa->chq_no, 0);
      else
				--ct;
    }

  prt_need(4);
  fmt_data(&work[0], noes, ct);
  prt_text(work);
  prt_fini(NULL);
  close(f);
  return null;
}}}}}

