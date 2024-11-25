#include  <stdio.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <signal.h>
#include  <fcntl.h>
#include  <sys/ipc.h>
#include  <sys/msg.h>
#include  <sys/errno.h>
#include   "version.h"
#if XENIX
#include  <sys/ascii.h>
#endif
#include   "../h/defs.h"
#include   "../h/prepargs.h"
#include   "h/trail.h"

extern int errno;

#if S_LINUX
#define O_SYNCW O_SYNC
#endif


#define REST_CT  20    /* number of messages to salvage before dying */

static Bool eof;                /* attempt to read past eof */

static Char * ioptr;            /* first character in buffer */
static Char * ioend;            /* last character in buffer */

static Msgbuf_t buf;

Char dbg_file_name[50] = "auditlog";

const Char nulls[1024];

static Id msgid;
static Int gbl_ct;


static const Char coatf[] = "Couldnt open A T file %s";
static const Char atwf[] = "Audit Trail Write Failed";

/* Format of Audit trail:
    WASTE_AREA
    Au_header_t		-- Tag value is TRAIL_TAG
    RECORDS		-- Tag value not TRAIL_TAG
    Au_header_t		-- Tag value is TRAIL_TAG
*/


private Int c_sum(blk, sz)
    Char * blk;
    Int    sz;
{ fast Char * p = &blk[sz];
  fast Int res = 17;
  
  while (--p >= blk)
    res += *p * 23;
    
  return res != TRAIL_TAG ? res : TRAIL_TAG + 1;
}

private void finitrail(n)
	 Int n;
{ gbl_ct = REST_CT;
  i_log(33, "Killed %d", n);
}


private void give_up(str)
	 Char * str;
{ if (msgid > 0)
    (void)msgctl(msgid, IPC_RMID, 0);
  sysabort(str);
}



private Int reseek(fildes, auh_ref, start)
	 Int       fildes;
	 Au_header auh_ref;
	 Int       start;
{ Bool wrapup = false;

  i_log(start, "Reseeking audit file (legal)");
  
  while (true)
  { Int sz;
    Char * buff = (Char *)auh_ref;
    Int cc = lseek(fildes, WASTE_AREA + AT_REC_SZ + start, 0);
    if (cc < 0)
    { i_log(cc, "seekreseek");
      return -1;
    }
    
    if (wrapup)
    { i_log(sz, "Size is %x", start);
      auh_ref->size = start;
      return start;
    }

    for (sz = 0; 
	 sz < AT_REC_SZ and (cc = read(fildes, &buff[sz], AT_REC_SZ - sz)) > 0;
	 sz += cc)
      ;
    
    if      (sz < 4 or cc < 0)
    { i_log(cc, "seeksz");
      return -1;
    }
    else if (auh_ref->tag != TRAIL_TAG)
      start += AT_REC_SZ;
    else
      wrapup = true;
  }
}

#define new_file(flag) (flag != MININT)


private Int open_write_at(auh_ref, flag, destfn)
	 Au_header_t   * auh_ref;
	 Int   	         flag;         /* != MININT => new file */
	 Char *          destfn;
{ Int fildes = open(destfn, O_CREAT + O_RDWR + O_SYNCW, 0644);
  if (fildes < 0)
    i_log(0, coatf, destfn);
  else 
  { Au_header_t  hdr_1;
    Int cc = lseek(fildes, WASTE_AREA, 0);
    if (cc < 0)
      i_log(cc, "seekwrite");
    cc = read(fildes, (Char *)&hdr_1, AT_REC_SZ);

    if      (new_file(flag) or hdr_1.tag == FORMAT_TAG or cc <= 0)
    { cc = lseek(fildes, WASTE_AREA, 0);
      if (cc < 0)
	i_log(cc, "seekwrite");
      hdr_1.tag  = TRAIL_TAG;
      hdr_1.size = 0;
      hdr_1.time = time(null);
      write(fildes, (Char *)&hdr_1, AT_REC_SZ);  /* header */
      write(fildes, (Char *)&hdr_1, AT_REC_SZ);  /* tailer */
    }
    else if (not new_file(flag) and 
	      (hdr_1.tag != TRAIL_TAG or cc != AT_REC_SZ))
    { i_log(cc, "File %s Not an audit file %d", destfn, hdr_1.tag);
      close(fildes);
      fildes = -1;
    } 
    
    if (fildes >= 0)
    { cc = lseek(fildes, WASTE_AREA + AT_REC_SZ + hdr_1.size, 0);
      if (cc < 0)
	i_log(cc, "seekapp %s", destfn);
      cc = read(fildes, (Char*)auh_ref, AT_REC_SZ);
      if (cc == AT_REC_SZ and
  	              auh_ref->tag == hdr_1.tag and auh_ref->size == hdr_1.size)
	lseek(fildes, WASTE_AREA + AT_REC_SZ + hdr_1.size, 0);
      else
      { Int sz = reseek(fildes, auh_ref, hdr_1.size);
	if (sz < 0)
	{ close(fildes);
	  fildes = -1;
	}
      }
    }
  }
  return fildes;
}	 

private void init_xactn_strm()

{ eof = false;
  ioptr = &buf.mtext[BLOCK_SIZE] - AT_REC_SZ;
  ioend = &buf.mtext[BLOCK_SIZE-1];
}


private Fildes open_read_at(srcfn)
	 Char *   srcfn;
{ Int fildes = open(srcfn, O_RDONLY, 0644);
  if (fildes < 0)
    i_log(0, coatf, srcfn);
  else 
  { Au_header_t  auh;
    Cc cc;
    lseek(fildes, WASTE_AREA, 0);
    cc = read(fildes, (Char *)&auh, AT_REC_SZ);

    if (cc == AT_REC_SZ and auh.tag == TRAIL_TAG)
      init_xactn_strm();
    else
    { i_log(0, "Popped file not an Audit File");
      close(fildes);
      fildes = -1;
    }
  }
  return fildes;
}



private Int rd_block(f)               
       Fildes f;
{ Int ct = (Int)&buf.mtext[BLOCK_SIZE-1] - (Int)ioend;
  for (; ct > 0; --ct)
  { Cc cc = read(f, &ioend[1], 1);
    if (cc <= 0)
      if (cc == 0)
      { eof = true;
	break;
      }
      else
	return i_log(-1, "Read Error in Backup File");
     
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
      memcpy(&buf.mtext[0], ioptr, sz);
    ioend -= (Int)ioptr - (Int)&buf.mtext[0];
    ioptr = &buf.mtext[0];
    if (not eof)
      sz = rd_block(f);
    if (sz < 2 * sizeof(Int))
      return null;
  }
  return *(Int*)ioptr == TRAIL_TAG ? null : ioptr;
}}

void main (argc,argv)
	Short argc;
	Char ** argv;
{ Char * mvgfn = null;
  Char * fxdfn = null;

  (void)moreargs(argc,argv,&mvgfn);
  (void)moreargs(argc,argv,&fxdfn);

  if (ARGS('X') != MININT or mvgfn == null or fxdfn == null)
  { fprintf(stderr, "useage: trail [-i] safeauditfile fixedauditfile\n");
    exit(0);
  }

  signal(SIGHUP, finitrail);
  signal(SIGINT, finitrail);
  signal(SIGQUIT,finitrail);
  signal(SIGTERM,finitrail);

  setpgrp();
  (void)freopen("/dev/null", "w", stderr);

{ msgid = msgget(MSGKEY, IPC_CREAT | 0644);
  if (msgid == -1)
    give_up("Couldn't create message queue");

  buf.mtype = getpid();
  buf.mtext[0] = 'D';
  
{ Cc cc = msgsnd(msgid, (void*)&buf, SPECIAL_MSGLEN, IPC_NOWAIT);
  if (cc == -1 and errno != EAGAIN)
    give_up("Audit Process Problem");

  sleep(8);  /* let others take the Die message */

{ Au_header_t  mvgauh;
  Au_header_t  fxdauh;
  Int mvgfd = open_write_at(&mvgauh, ARGS('I'), mvgfn);
  Int fxdfd = open_write_at(&fxdauh, ARGS('I'), fxdfn);
  
#define CLEN ((long)AT_REC_SZ - sizeof(Int))

  for (gbl_ct = 10000000; gbl_ct > 0; --gbl_ct)
  { Cc cc = msgrcv(msgid, (void*)&buf, CLEN, 0, 
  		   gbl_ct > REST_CT ? 0 : IPC_NOWAIT);
    if (cc == -1)
      if (errno != EAGAIN and errno != EINTR)
	break;
      else 
      { i_log(33, "Gotcha");
	continue;
      }
    if      (buf.mtype == Q_DATA)
    { if (mvgfd >= 0)
      { buf.mtype = c_sum((Char*)&buf.mtext[0], CLEN);

	lseek(mvgfd, WASTE_AREA + AT_REC_SZ + mvgauh.size, 0);
	cc = write(mvgfd, (char*)&buf.mtype, AT_REC_SZ);
	if (cc < 0)
	{ i_log(errno, atwf);
	  break;
	}
	lseek(fxdfd, WASTE_AREA + AT_REC_SZ + fxdauh.size, 0);
	cc = write(fxdfd, (char*)&buf.mtype, AT_REC_SZ);
	if (cc < 0)
	{ i_log(errno, atwf);
	  break;
	}
	mvgauh.size += AT_REC_SZ;
	mvgauh.time = time(null);
	write(mvgfd, (Char*)&mvgauh, AT_REC_SZ);
	fxdauh.size += AT_REC_SZ;
	fxdauh.time = time(null);
	write(fxdfd, (Char*)&fxdauh, AT_REC_SZ);
	if ((gbl_ct & 15) == 0)
	{ lseek(mvgfd, WASTE_AREA, 0);
	  write(mvgfd, (Char *)&mvgauh, AT_REC_SZ);
	  lseek(fxdfd, WASTE_AREA, 0);
	  write(fxdfd, (Char *)&fxdauh, AT_REC_SZ);
	}
      }
    }
    else if (in_range(buf.mtype, Q_CHG_DSK, Q_POP_DSK))
    { Int ct;
      
      if (mvgfd >= 0)
      { lseek(mvgfd, WASTE_AREA, 0);
	write(mvgfd, (Char *)&mvgauh, AT_REC_SZ);
	close(mvgfd);
      }
      i_log(buf.mtype, "Moving AT to %d %s", mvgfd, buf.mtext);
      sleep(2);

    { Char * tgt = buf.mtext;
      Char * txt2 = strchr(buf.mtext, ' ');
      if (txt2 != null)
      { *txt2++ = 0;
	tgt = txt2;
      }

      for (ct = 12; ct >= 0; --ct)
      { mvgfd = open_write_at(&mvgauh, MININT, tgt);      /* 0 => initialise */
	if (mvgfd >= 0)
	  break;
	sleep(10);
      }
      if (mvgfd < 0)
	i_log(1, "Couldnt find AT %s", txt2);

      if (buf.mtype == Q_POP_DSK)
      { Int efild = open_read_at(buf.mtext);
	if (efild >= 0 and mvgfd >= 0)
	{ lseek(mvgfd, WASTE_AREA + AT_REC_SZ + mvgauh.size, 0);

	  while ((ioptr = rd_next(efild)) != null)
	  { Cc cc = write(mvgfd, (char*)ioptr, AT_REC_SZ);
	    if (cc < 0)
	    { i_log(errno, atwf);
	      break;
	    }
	    mvgauh.size += AT_REC_SZ;
	  }
	  mvgauh.time = time(null);
	  write(mvgfd, (Char*)&mvgauh, AT_REC_SZ);

	  lseek(mvgfd, WASTE_AREA, 0);
	  write(mvgfd, (Char *)&mvgauh, AT_REC_SZ);
	  close(efild);
	}
      }
    }}
    else
    { if      (buf.mtext[0] == 'R')
	kill(buf.mtype, SIGUSR1);
      else if (buf.mtext[0] == 'D')
	if (buf.mtype != getpid())
	  break;
    }
  }

  if (mvgfd >= 0)
  { lseek(mvgfd, WASTE_AREA, 0);
    write(mvgfd, (Char *)&mvgauh, AT_REC_SZ);
  }
  if (fxdfd >= 0)
  { lseek(fxdfd, WASTE_AREA, 0);
    write(fxdfd, (Char *)&fxdauh, AT_REC_SZ);
  }
  if (msgid > 0)
    (void)msgctl(msgid, IPC_RMID, 0);
  exit(0);
}}}}
