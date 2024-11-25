			/* MODULES lock, pid, ctrack, cache */
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include        "version.h"
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include        "../h/errs.h"
#include        "../h/defs.h"
#include 	"memdef.h"
#include 	"page.h"
#include 	"recs.h"
#include 	"cache.h"
#include        "../h/prepargs.h"


extern  Id cachesem;
extern  struct sembuf cache_lk[2], cache_unlk[1];

extern  Char *        memlocks;	   /* to check locks */



#define if_lock(upn, stt, len, ct) \
    cache_lk[stt].sem_op = ct; \
    cache_lk[stt].sem_num = 2 * upn; \
    if (semop(cachesem, &cache_lk[stt], len) >= 0)


				    /* for x = rd, wr, strm                   */
            			    /*  all x_lock, x_unlock calls must match */
public Int raw_lock(upn, n)
      Sid upn;
      Int n;
{ Vint try_;
  if (n < 0)
    memlocks[upn] += n;			/* in theory this is unsafe */
  cache_lk[1].sem_flg = 0;

  for (try_ = 10; --try_ ; )
  { if_lock(upn, 1, 1L, -n)
      break;
    i_log(errno, "sem reader unlk");
  }
  if (n > 0)
    memlocks[upn] += n;			/* in theory this is unsafe */
  cache_lk[1].sem_flg = SEM_UNDO;
{ Int ct = ct_rdrs(upn);
  if (ct < 0)
    i_log(errno, "sem count %d", ct);
  return ct;
}}



public Int raw_unlock(upn, n)
      Sid upn;
      Int n;
{ return raw_lock(upn, -n);
}



