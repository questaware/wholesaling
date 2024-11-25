#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "version.h"

#if MUSER 
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#endif

#include  "../h/errs.h"
#include  "../h/defs.h"
#include 	"memdef.h"
#include 	"page.h"
#include 	"recs.h"
#include 	"cache.h"
#include  "../h/prepargs.h"

#include  "../../osdb/domains.h"

 /* SINGLE_USER: No locking, no shared memory */

#include        "rpc_defs.h"

extern void tcapclose();

#if LINUX
#define Unsemun (union semun)
#else
#define Unsemun
#endif

#define private 

#define SAFE_REQS   4

#define DO_FORCE 1
#define DO_DEST  2

#define PRIME1   119
#define KEEPSIZE   64
#define HASH_SIZE  512   		/* must be a power of two */
#define TTY_MAP_SZ sizeof(Int)

#define CACHE_LRU_CHUNK   40		/* number of entries to consult */

#define wrpg "wr rogue pg %d t %d"
#define csl "cache sem lock"
#define csul "cache sem unlock"
#define twp   "TRYING TO WRITE %d"

#ifndef PATH_LEN				/* MODULE ctrack */
# define PATH_LEN  100
  typedef char Path_t[PATH_LEN+1];
#endif

typedef struct 			        /* stored in shared memory */
{ Path_t   db_path;
  Set      refs;		/* set of logical pids */
  Int      size[2];		/* in blocks */
#define sizec  size[0]
#define sizeix size[1]
} Db_t;
 				        /* stored in process memory */
static Int this_pix;
static Set this_log_pid;               
static Set not_me_mask;

#define MAX_LDN 4
static struct { Char     chan;		/* data Fildes */
                Char     wrchan;	/* boolean */
	 	Char     ref_ct;	/* number of openers */
	 	Char     my_rd_ct;	/* number of read locks */
       	      }  chans2[MAX_LDN*2];	/* chans[MAX_LDN] : origin */

#define chans(ix) chans2[MAX_LDN+(ix)]

/* The cache maps: 1) Database id
		   2) Page number.
			to            ABSENT or < amended: Bool, memory: Ptr >

This is implemented as a hash function indexing an array of the following:
*/

#define BUSY_MASK ((1 << MAXCLIENTS) - 1)
#define Q_AMDD    (13 << MAXCLIENTS)
			  /* Q_AMDD => true, also orders writes */
#define amended(eny) (((eny)->busy_ct & (0xf << MAXCLIENTS)) == Q_AMDD)

typedef struct Block_s
{ Byte c [BLOCK_SIZE];
} Block_t;

#define MAXSTRMS 12
					/* offsets in shared segment */
typedef unsigned short Cache_det_r;

#define CACHE_SIZE ((MAXCLIENTS * KEEPSIZE) + 44) /* (4 * KEEPSIZE) */

typedef struct
{ Cache_det_t lru_l_ref;
  Cache_det_r hash[HASH_SIZE];
  Cache_det_t cache[CACHE_SIZE];
  Char        safe_area[8];
  Int         user_ct;
  Db_t        db_tbl[MAX_LDN];
  Proc_t      pc_tbl[MAXCLIENTS];
  Page_nr     strm_tbl[2*MAXSTRMS*MAX_LDN];
  Int         memlocks[MAX_LDN+1];
} Shm_area_t;

Shm_area_t * shm_p;

static Char * mem;                      /* shared memory page */
#define shnull mem

static Cache_det_r * hash;
static Cache_det_t * cache;
static Block_t * block_tbl;

static Db_t *        db_tbl = null;/* table of databases */
static Proc_t *      pc_tbl;       /* table of processes */
/*static Id *        tty_tbl;      ** table of ttys in use(variable obsolete)*/
static Page_nr *     strm_tbl;
       Int *         memlocks;	   /* to check locks *//* used by rawlock.c */

static Cache_det    keep[KEEPSIZE];	/* pages with busy_ct set */
static Vint         keephd;		/* cycles through keep[]	  */
static Vint         keepsz;		/* cycles through keep[]	  */
static Cache_det_t  null_keep;

int permitted_pages = KEEPSIZE; 
int hwm_pages = 0; 

int max_kept_ct;

static Bool delaywrite = false;
static Int reqs_to_term = 0;

public Int cache_err = 0;

static Bool close_term;		/* close down terminal when terminating */

/******************************************************************/

/* Note: The hash table lists through (Cache_det)->overflow cannot
         contain entries with (Cache_det)->page_upn == 0 so that
         their terminator is ->overflow == shnull or ->page_upn == 0
         The lru_list through ->lru_forwards, lru_backards can 
         contain entries with (Cache_det)->page_upn == 0 so that
         their terminator is ->overfow == shnull, the same as 
         ->overfow == lru_list_ref.

/******************************************************************/

/************ Beginning of locks Section **********************************/
					/* MODULE lock */
#define RW_ME 0600
#define CT_RDRS 16

#ifndef LK_BASE
# define LK_BASE   0xdd18  // was 0xdb19, then 0xdc19
#endif

Int lk_base = LK_BASE;

#define lo_bit(v) (((v)^((v)+1))&((v)+1))

										    /* shared memory on lk_base-1 */
												/* used only by rawlocks.c */
Id cachesem = -1;
struct sembuf cache_lk[2], cache_unlk[1];


public Id get_cache_lock(Bool create)

{ if (cachesem > 0)
    return cachesem ;
{ fast Id id = semget((long)lk_base+1, (MAX_LDN+1)*2, RW_ME);
       Bool first = id < 0 /*and errno == ENOENT*/;
  if (first and not create)
    return -1;
  if (first)
    id = semget((long)lk_base+1, (MAX_LDN+1)*2, IPC_CREAT | RW_ME);

  if (id < 0)
  { i_log(-1, "E semget %d\n", errno);
    return -1;
  }
  else
  { if (first)
    { Int i;
      for (i = (MAX_LDN+1)*2; --i >= 0; )
      { Int iv = CT_RDRS;
				if      (i == 0)
				  iv = 1;		/* the cache_lock */
				else if (i & 1)
				  iv = MAXSTRMS;	/* the streamer lock */
      { Cc cc = semctl(id, i, (long)SETVAL,    Unsemun(int)iv);
				cc = (iv - semctl(id, i, (long)GETVAL, Unsemun(int)0L));
				if (cc != 0)
				  i_log(-3, "E init C sem %d, err %d", iv - cc, errno);
      }}
      			/* The streamer lock happens to have value CT_RDRS */
    }
    cachesem = id;
  }

/*cache_lk[0].sem_num = 0;invariant */
  cache_lk[0].sem_op  = -1;
  cache_lk[0].sem_flg = SEM_UNDO;
  cache_lk[1].sem_op  = -1;
  cache_lk[1].sem_flg = SEM_UNDO;

/*cache_unlk[0].sem_num = 0;invariant */
  cache_unlk[0].sem_op  = 1;
  cache_unlk[0].sem_flg = SEM_UNDO;

  return cachesem;
}}


public void locks_del()

{ Short i;
  Id semid = semget(lk_base+1, 1L, RW_ME);
  if (semid >= 0)
    semctl(semid, 0L, (long)IPC_RMID, Unsemun(int)0L);
} 


private void clr_lock(upn)
	Id  upn;
{ Vint ix;
  for (ix = 2; --ix >= 0; )
  { Cc cc = semctl(cachesem, upn * 2 + ix, (long)SETVAL, Unsemun(int)CT_RDRS);
    cc = semctl(cachesem, upn * 2 + ix, (long)GETVAL, Unsemun(int)0L);
    if (cc != CT_RDRS)
      i_log(-3, "E init c sem %d, err %d", cc, errno);
  }
}


public void shm_del()

{ Id shmid = shmget(lk_base-1, 1L, RW_ME);
  if (shmid >= 0)
  { Cc cc = shmctl(shmid, (long)IPC_RMID, 0L);
    if (cc != SUCCESS)
      i_log(errno, "shm delete");
  }
}

Bool cache_lock_ct = 0;

public Cc cache_lock()

{ cache_lock_ct = 1;
  
  if (semop(cachesem, &cache_lk[0], 1L) < 0)
    if (semop(cachesem, &cache_lk[0], 1L) < 0)
    { 
      i_log(errno, csl);
      cache_lock_ct = 0;

      return -1;
    }

  int ct = __sync_fetch_and_add(&memlocks[0],1);
  if (ct < 0)
    i_log(- ct, "CacheML");

  return OK;
}



public void cache_unlock()

{ cache_lock_ct = 0;

  int ct = __sync_fetch_and_sub(&memlocks[0],1);
  if (ct <= 0)
    i_log(- ct, "CacheML");

  if (semop(cachesem, &cache_unlk[0], 1L) >= 0)
    return;
  if (semop(cachesem, &cache_unlk[0], 1L) >= 0)
    return;
  i_log(errno, csul);
  cache_lock_ct = 1;
}

#define if_lock(upn, stt, len, ct) \
    cache_lk[stt].sem_op = ct; \
    cache_lk[stt].sem_num = 2 * upn; \
    if (semop(cachesem, &cache_lk[stt], len) >= 0)

public Short ct_rdrs(upn) Sid upn;
 
{ return (upn == 0 ? 1 : CT_RDRS) - 
          semctl(cachesem, upn*2, (long)GETVAL,Unsemun(int)0L);
}


public Int ct_wrrs(upn) Sid upn;

{ return ct_rdrs(upn) >= MAXCLIENTS;
}


static Bool vfy_wr = true;


public void flip_vfywr()

{ vfy_wr = not vfy_wr;
}


public void vfy_wrlock(upn)
    Sid  upn;
{ Id ct = chans(upn < 0 ? -upn : upn).my_rd_ct;
  if (ct < CT_RDRS and vfy_wr)
    i_log(ct, "*NWLP");
}


public void vfy_lock(upn)
    Sid  upn;
{ if (chans(upn).my_rd_ct != 0) i_log(upn, "lockval %d", chans(upn).my_rd_ct); 
}


public void vfy_islocked(upn, min)
    Sid  upn;
    Int  min;
{ if (min > 1)
    min = CT_RDRS;

  if (chans(upn).my_rd_ct < min)
		i_log(upn, "*Db not locked %d", min);
}

static void unkeep_all()

{ Int sz = CACHE_SIZE;
  Cache_det ptr;
  for (ptr = cache - 1; --sz >= 0;)
    if (((++ptr)->busy_ct & this_log_pid) != 0)
      ptr->busy_ct &= ~this_log_pid;

  pc_tbl[this_pix].num_held = 0;

  keepsz = 0;
  keephd = KEEPSIZE-1;                /* or any other value */
}


public Cc rd_lock(upn)
     Sid  upn;
{
  fast Vint try_;

  if (chans(upn).my_rd_ct == 0)
  { 
    for (try_ = 5; --try_ > 0; )
    {
      if_lock(upn, 1, 1L, -1)
        break;
      if (errno == EINTR)
        continue;
      i_log(errno, "sem rdr start");
    }
    if (try_ <= 0)
      return ANY_ERROR;
    __sync_fetch_and_add(&memlocks[upn],1);

    if (pc_tbl[this_pix].num_held != 0)
      unkeep_all(upn);
  }
  chans(upn).my_rd_ct += 1;
  return OK;
}


public void rd_unlock(upn)
      Sid upn;
{ chans(upn).my_rd_ct -= 1;
  if (chans(upn).my_rd_ct == 0)
  { if (pc_tbl[this_pix].num_held != 0)
      unkeep_all(upn);

    Vint try_ = memlocks[upn];
    if (try_ <= 0)
      i_log(upn*1000 - try_, "CacheML");

    for (try_ = 10; --try_ ; )
    { if_lock(upn, 1, 1L, 1)
      { __sync_fetch_and_sub(&memlocks[upn],1);
        return;
      }
      i_log(errno, "sem reader");
    }
  }
  if (chans(upn).my_rd_ct < 0)
    i_log(56, "Locks off");
}

public Cc wr_lock(upn)
     Sid upn;
{ if (reqs_to_term != 0)
    sleep(20);
  chans(upn).my_rd_ct += CT_RDRS;
{ fast Int mrc = chans(upn).my_rd_ct;
  if (mrc >= 2 * CT_RDRS)
    return OK;
  
  permitted_pages = KEEPSIZE * MAXCLIENTS - 1;

  if (mrc > CT_RDRS)
    i_log(mrc, "Deadlock potential");
   
  for (mrc = 5; --mrc >= 0; )
  { if_lock(upn, 1, 1L, -CT_RDRS)
    { __sync_fetch_and_add(&memlocks[upn],CT_RDRS);
      return OK;
    }
    if (errno == EINTR)
      continue;
    if (reqs_to_term > 0)
      cache_term();		/* no return under these conditions */
    i_log(errno, "sem writer start"); 
  }

  chans(upn).my_rd_ct -= CT_RDRS;
  return ANY_ERROR;
}}


public void wr_lock_window(upn)
	Sid  upn;
{ wr_unlock(upn); wr_lock(upn);
}

#if 0
X				/* get a write lock if you can */
X				/* write locks must not be on */
X
Xpublic Bool wr_lock_try(upn, times)
X  Sid upn;
X  Short times;
X{ Vint rdrs = chans(upn).my_rd_ct;
X  Vint flag = IPC_NOWAIT;
X
X  if (rdrs == 0)		/* there is no deadlock potential */
X  { flag = 0;
X    times = 10;
X  }
X
X  if (rdrs < CT_RDRS and chans(upn).wrchan)
X  { cache_lk[1].sem_op = rdrs - CT_RDRS;
X    cache_lk[1].sem_flg = flag;
X   
X    for (; --times >= 0; )
X    { if_lock(upn, 1, 1L)
X        break;
X      if (times > 0 and flag == IPC_NOWAIT)
X        sleep(2);
X    }
X    cache_lk[1].sem_flg = 0;
X    if (times >= 0)
X    { chans(upn).my_rd_ct += CT_RDRS;
X      return true;
X    }
X  }
X  return false;
X}
X
#endif

public void flip_writedelay()

{ delaywrite = not delaywrite;
}


public void wr_unlock(upn)
    Sid upn;
{ if (upn < 0)
    i_log(upn, "wr_l neg");

  if (chans(upn).my_rd_ct <= CT_RDRS)
  { if (pc_tbl[this_pix].num_held != 0)
      unkeep_all(upn);
    Int ctupn = 0;
    if (chans(upn).my_rd_ct < 0)
    { i_log(57, "Locks off");
      chans(upn).my_rd_ct = 0;
    }
    else
    { permitted_pages = KEEPSIZE;

      int ct = __sync_fetch_and_sub(&memlocks[upn], CT_RDRS);
      if (ct < 0)
        i_log(upn*1000 - ct, "CachewuML");

      if (not delaywrite)
        ctupn = force_db(upn);

      if_lock(upn, 1, 1L, chans(upn).my_rd_ct);
      else
        i_log(errno, "sem writer end"); 
    }
    Int ct = count_busys(null);
    if (ct > 0)
      printf("Ct %d Ctu %d\n",ct,ctupn);
  }
  chans(upn).my_rd_ct -= CT_RDRS;
  if (reqs_to_term > 0)
    cache_term();
}


				/* drop lock to read lock */
public void wr_droplock(upn)
    Sid upn;
{ Int mrc = chans(upn).my_rd_ct;
  if (mrc < 2 * CT_RDRS)
  { if (not delaywrite)
      force_db(upn);
    if_lock(upn, 1, 1L, CT_RDRS-1)
      mrc = __sync_fetch_and_sub(&memlocks[upn], CT_RDRS);
    else
      i_log(errno, "sem writer end"); 
  }
  chans(upn).my_rd_ct -= CT_RDRS - 1;
}

/************ End of locks Section ****************************************/

Bool page_in_strms(Sid upn, Page_nr pg)

{ Page_nr * st = &strm_tbl[MAXSTRMS * upn - MAXSTRMS];
  fast Vint ix;
  
  for (ix = MAXSTRMS; --ix >= 0; )
  { if (__sync_fetch_and_add(&st[ix],0) == pg)           /* must be atomic */
    { i_log(pg, "Page S Protected");
      return true;
    }
  }
                      
  return false;
}



void page_from_strms(Sid upn, Page_nr pg)

{ Page_nr * st = &strm_tbl[MAXSTRMS * upn - MAXSTRMS];
  Vint ix;

  if (pg == 0)
    return;

  cache_lock();

  for (ix = MAXSTRMS; --ix >= 0; )
    if (st[ix] == pg)
    { if ((st[MAX_LDN * MAXSTRMS + ix] -= 1) == 0)
      { st[ix] = 0;
        break;
      }
    }

  cache_unlock();
}



Cc page_to_strms(Sid upn, Page_nr pg)

{ Page_nr * st = &strm_tbl[MAXSTRMS * upn - MAXSTRMS];
  fast Vint ix;
       Vint fix = -1;

  if (pg == 0)
    return OK;
  cache_lock();

  for (ix = MAXSTRMS; --ix >= 0; )
  { if (st[ix] == pg)
    { st[MAX_LDN * MAXSTRMS + ix] += 1;
      cache_unlock();
      return OK;
    }
    if (st[ix] == 0 and fix < 0)
      fix = ix;
  }

  if (fix >= 0)
  { st[MAX_LDN * MAXSTRMS + fix] = 1;
    st[fix] = pg;
    cache_unlock();
    return OK;
  }

  cache_unlock();
  return HALTED;
}

				      /* MODULE pid */
#define M_DELENY 2
#define M_TTYTBL 1

private Sid  fnd_pid_slot(wh, pid)
         Int  wh;
	 Id   pid;
{ fast Sid ix = MAXCLIENTS;
  fast Proc_t * p;
  for (p =/*(wh & M_TTYTBL) ? &tty_tbl[MAXCLIENTS-1] : */&pc_tbl[MAXCLIENTS-1];
       --ix >= 0 and p->id != 0;
       --p)
    if (pid == p->id)
      break;

  return ix;
}

#define del_pid(wh,id) add_pid((wh) | M_DELENY, (id))

private Cc add_pid(wh, id)
         Char wh;
         Int  id;
{ Sid ix = fnd_pid_slot(wh,id);
  if (ix < 0)
  { if (wh & M_DELENY)
      return -1;

    i_log(id, "Out of Slots");
    return ix;
  }
  
  if (wh & M_DELENY)
    id = 0;

  pc_tbl[ix].id = id;
  pc_tbl[ix].num_held = 0;
{	Char * tty = ttyname(0);
	if (tty != NULL)
	{ const char * pfx = "/dev/";
		while (*tty == *pfx && *pfx != 0)
		{ ++tty; ++ pfx;
		}
		strpcpy(pc_tbl[ix].tty, tty, sizeof(pc_tbl[ix].tty));
	}  
  this_pix = ix;
  this_log_pid = (1 << ix);
  not_me_mask = ~this_log_pid & BUSY_MASK;
  return OK;
}}

     /* Operations on Databases */     /* MODULE ctrack */

private Db_t * recover_upn()

{}


private Db_t * fnd_db (db)
      Path_t    db;			/* chche_locked */
{ fast Db_t *p;
  if (db[0] == '.' and db[1] == '/')
    db += 2;
  for (p = &db_tbl[0]; p < &db_tbl[MAX_LDN]; ++p)
    if (strcmp(&p->db_path[0], db) == 0 and p->refs != 0)
      return p;

  return null;
}



//public Char * chan_dbname (t, upn)
//	Char *  t;
//	Sid     upn;
//{ strpcpy(&t[0], db_tbl[upn-1].db_path, sizeof(Path_t));
//  return t;
//}



public Fildes chan_fileno(upn)    
	Sid upn;			/* at least read locked */
{
#define chan_fileno(upn) chans(upn).chan
  return chan_fileno(upn);
}


public Int chan_filesize(upn)   	/* in Blocks of BLOCK_SIZE */
	Sid upn;			/* write locked */
{ 
#define chan_filesize(u) (u >0 ? db_tbl[u-1].sizec : db_tbl[-(u)-1].sizeix)
  return chan_filesize(upn);
}

Int myopen(filename, md, mask)
	Char * filename;
	int    md, mask;
{ return open(filename, md, mask);
}

Byte * page_ref(Sid,Page_nr) forward;
Byte * _page_ref(Sid,Page_nr) forward;


public Sid chan_link (mode, filename)/*-1 => error, IN_USE => in use*/
      Shortset  mode;
      Char *    filename;
{ Set size_diffs = 0;
  Vint iter;
  
  permitted_pages = KEEPSIZE;
  
  cache_lock();
{ Db_t * p = fnd_db(filename);   /* ptr to entry corresponding to db */
  if (p != null)
  { if (mode & (F_EXCL + F_NEW))
    { cache_unlock();
      return IN_USE;
    }
  }
  else
  { fast Int ix = -1;
    while (++ix < MAX_LDN and db_tbl[ix].refs != 0)
      ;

    if (ix >= MAX_LDN)
    { i_log(0,"too many databases: %d\n", ix + 1);
      return IN_USE;
    }

    p = &db_tbl[ix];
    p->size[0] = 0;
    p->size[1] = 0;
    if (filename[0] == '.' and filename[1] == '/')
      filename += 2;
    strcpy(&p->db_path[0], filename);
  }
  p->refs |= this_log_pid;

{ Sid res = p - db_tbl + 1;
			/* all uses this process must be of the same kind */
  if (chans(res).ref_ct > 0)
  { chans(res).ref_ct += 1;
    i_log(res, ".Local users %d", chans(res).ref_ct);
    cache_unlock();
  }
  else
  {
#if MUSER
    Shortset rwmd = O_RDWR+(mode & F_SYNC ? O_SYNCW : 0);
#else 
    Shortset rwmd = O_RDWR;
#endif
    Char dbn[132];
    Char * fnmx_ = strcat(strpcpy(&dbn[0],filename,PATH_LEN),"_");
    Int fd[2];
    Shortset md =  mode & F_EXISTS ? 0 : 
                	 mode & F_NEW ? O_EXCL+O_CREAT : O_CREAT;
    md += mode & F_WRITE ? rwmd : O_RDONLY;
    md += mode & F_APPEND ? O_APPEND : 0;
    fd[1] = -1;
    fd[0] = myopen(filename, md, 0644);
    if (fd[0] < 0)
    { p->refs &= ~this_log_pid;
      if (p->refs == 0)
      	clr_lock(res);    
      res = -1;
    }
    else
    { if (mode & F_IXOPT)
      { Shortset imd = (mode & F_NEWIX ? O_EXCL+O_CREAT : O_CREAT) |
	               (mode & F_WRIX  ? rwmd   : O_RDONLY);
        fd[1] = myopen(fnmx_, imd, 0644);
      }
      chans(res).chan     = fd[0];
      chans(-res).chan    = fd[1];
      chans(res).wrchan   = (mode & F_WRITE) != 0;
      chans(-res).wrchan  = (mode & F_WRIX) != 0 and fd[1] >= 0;
      chans(res).ref_ct   = 1;
      chans(res).my_rd_ct = 0;

      for (iter = -1; ++iter <= 1; )
      	if      (mode & (iter == 0  ? F_NEW : 
	                       fd[1] < 0  ? -1    : F_NEWIX))
      	  p->size[iter] = 1;
      	else if (p->size[iter] == 0)		/* must be atomic */
        { struct stat stat;
      	  p->size[iter] = fstat(fd[iter], &stat) != 0 
			                  		? -1 : (stat.st_size >> LOG_BLOCK_SIZE);
      	}
    }
    if (res >= 0)
    { for (iter = -1; ++iter <= 1; )
    	if (not (mode & (iter == 0  ? F_NEW : 
		                	 fd[1] < 0  ? -1    : F_NEWIX)))
      { Vint ipn = iter == 0 ? res : -res;
    	  Byte * blk = _page_ref(ipn, ROOT_PAGE);
	      if	(*blk != M_INDEX_SEG)
    	  { res = INVALID;
	        break;
    	  }
	      else
    	  { Int diff = p->size[iter] != r_word(blk, MI_FILE_SZ);
	        if (diff != 0)
    	    { i_log(res, "Db %d W size %d %d", 
		        	    iter, p->size[iter], r_word(blk, MI_FILE_SZ));
            if (in_range(diff, 0, 10))
	            size_diffs |= (1L << iter);
    	      else
	          { res = -1;
          		close(chans(ipn).chan); 		
          		chans(ipn).chan = -1;
          		chans(ipn).wrchan = false;
    	      }
	        }
    	  }
    	}
    }

    cache_unlock();

    if (size_diffs != 0 and res >= 0)
    { for (iter = -1; ++iter <= 1; )
      { Vint ipn = iter == 0 ? res : -res;
        if (size_diffs & (1L << iter) and chans(ipn).wrchan)
        { wr_lock(res);
			    /* correct root */
	      { Byte * blk = page_mark_ref(iter == 0 ? res : -res, ROOT_PAGE);
          w_word(blk, MI_FILE_SZ, p->size[iter]);
          if (r_word(blk, MI_FIRST_UNUSED) > p->size[iter])
            w_word(blk, MI_FIRST_UNUSED, p->size[iter]);
          wr_unlock(res);
       	}}
      }
    }
  }

  return res;
}}}

public Cc chan_unlink (upn)
    Sid     upn;		/* > 0 */
{ Bool clock = /* reqs_to_term <= SAFE_REQS */ true;
  fast Db_t *p = &db_tbl[upn-1];
  if (upn < 0)
    return NOT_FOUND;

  if (clock and not cache_lock_ct)
    cache_lock();
  chans(upn).ref_ct -= 1;
  if (chans(upn).ref_ct != 0)
    i_log(upn, ".Local Fusers %d", chans(upn).ref_ct);
  else
  { Int cat = p->refs == 0 ? DO_DEST :
              delaywrite   ? DO_FORCE + DO_DEST : DO_FORCE;
    if (chans(upn).my_rd_ct >= CT_RDRS or delaywrite)
      do_db(upn, cat);

    p->refs &= ~this_log_pid;
    if (cat == DO_DEST)
    { i_log(0, ".Last chan_unlink");
      clr_lock(upn);
    }

    file_close((int)(chans(upn).chan));
    if (chans(-upn).chan != 0)
      file_close((int)(chans(-upn).chan));
  }
  if (cache_lock_ct)
    cache_unlock();
  return OK;
}


public void set_db_size(upn,sz)
    Sid   upn;
    Int   sz;
{ if (upn > 0)
    db_tbl[upn-1].sizec = sz;
  else
    db_tbl[-upn-1].sizeix = sz;
}

				       /* MODULE cache */
#define calc_hash(upg) \
 (((upg * PRIME1) >> 8) & (HASH_SIZE-1))



#define rem_from_lru_list(eny) \
{ cache[eny->lru_backwards].lru_forwards = eny->lru_forwards; \
  cache[eny->lru_forwards].lru_backwards = eny->lru_backwards; \
 } 

#define append_to_lru_list(eny) \
{ eny->lru_forwards = 0;        \
  eny->lru_backwards = cache[0].lru_backwards;        \
  cache[eny->lru_backwards].lru_forwards = eny - cache;\
  cache[0].lru_backwards = eny - cache;                \
}

#define to_end_lru_list(eny) \
{ rem_from_lru_list(eny); \
  append_to_lru_list(eny);\
}

#if 0

Id vvv;

private void vfy_gdb()

{ vvv = 1;
}



private void vfy_cache()

{ fast Cache_det     ptr;     /* 1) Scan cdr's from 0 to current in use     */
  for (ptr = &cache[CACHE_SIZE - 1]; ptr >= cache; ptr -= 1)
  { Byte * p = &block_tbl[ptr->memory].c[0];
    if ((p[0] == 29 or p[0] == 33) and p[8] == 0)
    { vfy_gdb();
      i_log(ptr->page_upn, "*Vapped %s %d", p[0] == 29 ? "s" : "p", *(Int*)&p[12]);
    }
  }
}

#endif


#if 0
X
Xprivate  Cache_det_r  get_lru()
X
X{ Vint lru_r = *lru_robin;		/* lru_robin has poor atomicity */
X  Vint clamp = CACHE_LRU_CHUNK;         /* but it's OK                  */
X  Vint bestix = 0;
X  Vint best = 0;
X 
X  while (--clamp >= 0)
X  { if (++lru_r >= CACHE_SIZE)
X      lru_r = 0;
X    
X  { Int la = *last_access - cache[lru_r].last_access;
X    if (la < 0)
X      la += 0xffffL;
X
X    if (la > best)
X    { best = la;
X      bestix = lru_r;
X    }
X  }}
X  
X  *lru_robin = lru_r;
X
X  return bestix;
X}
X
#endif
#if 1

Int count_busys(Int * all_ct_ref)

{      Int ct = 0;
       Int ct1 = 0;
       Int all_ct = 0;
  fast Cache_det ptr;

  for (ptr = &cache[CACHE_SIZE - 1]; ptr >= cache; ptr -= 1)
  { if (ptr->busy_ct == 0)
      continue;
    ++all_ct;
    if (ptr->busy_ct & this_log_pid)
      ++ct;
    if ((ptr->page_upn >> 16) == 1)
      ++ct1;
  }

  if (ct > max_kept_ct)
    max_kept_ct = ct;

  if (ct > hwm_pages)
    hwm_pages = ct;

  if (ct > permitted_pages + 2)
    i_log(ct, "Too busy");

  if (all_ct_ref  != null)
    *all_ct_ref = all_ct;

  if (ct != pc_tbl[this_pix].num_held)
  { printf("Ct %d Ct1 %d Nh %d  ", ct, ct1, pc_tbl[this_pix].num_held);
    pc_tbl[this_pix].num_held = ct;
  }
 
  return ct;
}
#endif


#if 1

#define block_cs(a,b) (a[-1] == 69)

#else

				/* always set CS */
Bool block_cs(res, wh)
	Byte *  res;
	Quot    wh;	/* 0=>skip, 1=>check lo, CS, 2=>check lo */
{ Int tot = 0;
  Vint ix;
  Int old_cs = res[BLOCK_SIZE-2];
  res[BLOCK_SIZE-2] = 0;
  
  for (ix = BLOCK_SIZE; (ix -= 4) >= 0; )
    tot += *(Int*)&res[ix];
    
  tot += (tot >> 8) + (tot >> 16) + (tot >> 24);
    
  if (wh)
  { if (res[-1]==69 and ((wh & 1)==0 or ((tot - old_cs) & 0xff) == 0))
    { res[BLOCK_SIZE-2] = tot;
      return true;
    }

    i_log(res[0], "Corr cache %x %x s.b. %x", tot, res[-1], res[BLOCK_SIZE]);
  }
  
  res[BLOCK_SIZE-2] = tot;
  return false;
}

#endif


public void fini_cache()

{ int clc = cache_lock_ct;
  if (not clc)
    cache_lock();

{ Int users = shm_p->user_ct - 1;
  i_log(getpid(), ".%d %s user logs out", users+1, 
 				users == 0 ? "st" :
  			users == 1 ? "nd" : 
  			users == 2 ? "rd" : "th");
  shm_p->user_ct -= 1;
  if (del_pid(0, getpid()) != OK)
    i_log(6, "Unh?");

  if (not clc)
    cache_unlock();
  
  if (users == 0)
  { 
    locks_del();
    shm_del();
  }
  
  cachesem = -1;
}}

private Cc cache_null_proc()

{ 
}

public Short init_cache(closeterm) /* -7: cache_lock 
                                     -ve: too many
                                     +ve: No of users in bottom nibble */
        Bool closeterm;
{ fast Cache_det c;
  fast Ptr    h;
#define SZ (sizeof(Shm_area_t) + CACHE_SIZE * (BLOCK_SIZE + 2) + 2)
  Int sz = (SZ + 3) & ~3;

  if (shm_p != null)
    return shm_p->user_ct;

/*i_log(sz, "Size is %x", sz);*/
 
  close_term = closeterm;
  cachesem = get_cache_lock(true);

  signal(SIGALRM, cache_null_proc);
  
  if (semop(cachesem, &cache_lk[0], 1L) < 0)
    return -7;

{ Id shmid = shmget(lk_base-1, sz+8, RW_ME);
  Bool first = shmid < 0;

  if (first and errno == ENOENT)
    shmid = shmget(lk_base-1, sz+8, IPC_CREAT | RW_ME);

  if (shmid < 0)
    i_log(errno, "shmget failed\n");

  mem = shmat(shmid, null, 0);
  if (mem == (char *)-1)
  { i_log(errno, "shmat failed\n");
    return -2;
  }
  shm_p = (Shm_area_t *)mem;
  hash = &shm_p->hash[0];
  cache = &shm_p->cache[0];
  db_tbl = &shm_p->db_tbl[0];
  pc_tbl = &shm_p->pc_tbl[0];
  strm_tbl = &shm_p->strm_tbl[0];
  memlocks = &shm_p->memlocks[0];

  Int m = (sizeof(Shm_area_t) + 2 + 2 + 3)& ~3; /* 2 for extra safety space!!*/
  block_tbl = (Block_t*)&mem[m];
  if (first)
	{				     /* shnull shares lru_list_ref */
    for (h = (Ptr)&mem[(sz+3) & 0xfffffc]; h > (Ptr)&mem[0]; )/* zero memory */
      *--h = 0;                
                                     /* The descriptors point permanently to */
				     /* the memory blocks for safety reasons */
    strcpy(shm_p->safe_area, "Correct");
    cache[0].lru_forwards  = 0;		 /* make an empty list */
    cache[0].lru_backwards = 0;
    mem[m-1] = 69;

    int ix = -1;

    for (c = cache; c < &cache[CACHE_SIZE]; c += 1)
    { /* c->amended = false; */
      /* c->busy_ct = 0;     */
      c->memory  = ++ix;
      m += BLOCK_SIZE;
      mem[m-1] = 69;
      append_to_lru_list(c);
    }
    if (m > sz)
      i_log(55, "SM OFLO");
  }
/*sop_t.sem_flg = 0; */

{ fast Cache_det *p;
  for (p = &keep[KEEPSIZE]; --p >= &keep[0]; )
    *p = &null_keep;

{ Int cc = add_pid(0, getpid());
  if (cc != OK)
  { (void)semop(cachesem, &cache_unlk[0], 1L);
    return cc;
  }
  /*i_log(shm_p->user_ct,"Init User %d", getpid());*/
  shm_p->user_ct += 1;
{ Int  ct = shm_p->user_ct;
  (void)semop(cachesem, &cache_unlk[0], 1L);
  return ct;
}}}}}



public Byte * give_shm(Int * sz_ref)

{ *sz_ref = ((SZ + 3) & ~3) + 8;
  return mem;
}


public int get_cache_users(Proc_t * * pc_tbl_ref)

{
#define SZ (sizeof(Shm_area_t) + CACHE_SIZE * (BLOCK_SIZE + 2) + 2)
  Int sz = (SZ + 3) & ~3;
  Id shmid = shmget(lk_base-1, sz+8, RW_ME);
  Bool first = shmid < 0;

  if (first and errno == ENOENT)
    shmid = shmget(lk_base-1, sz+8, IPC_CREAT | RW_ME);

  if (shmid < 0)
    i_log(errno, "shmget failed\n");

  mem = shmat(shmid, null, 0);
  if (mem == (char *)-1)
  { i_log(errno, "shmat failed\n");
    return -2;
  }
  shm_p = (Shm_area_t *)mem;
  hash = &shm_p->hash[0];
  cache = &shm_p->cache[0];
  db_tbl = &shm_p->db_tbl[0];
  pc_tbl = &shm_p->pc_tbl[0];
  
  *pc_tbl_ref = pc_tbl;

  return shm_p->user_ct;
}


public Int * tty_map()

{ return mem == null ? null : (Int*)&mem[((SZ + 3) & ~3) - TTY_MAP_SZ]; 
}

void dump_blk(Byte * p, int len32m)

{ Int rix;
  Char lbuf[132];
  for (rix = -1; ++rix < len32m; )
  { sprintf(&lbuf[0], ".%04x %04x %04x %04x %04x %04x %04x %04x"
                      " %04x %04x %04x %04x %04x %04x %04x %04x",
      *(unsigned short*)&p[0],
      *(unsigned short*)&p[2],
      *(unsigned short*)&p[4],
      *(unsigned short*)&p[6],
      *(unsigned short*)&p[8],
      *(unsigned short*)&p[10],
      *(unsigned short*)&p[12],
      *(unsigned short*)&p[14],
      *(unsigned short*)&p[16],
      *(unsigned short*)&p[18],
      *(unsigned short*)&p[20],
      *(unsigned short*)&p[22],
      *(unsigned short*)&p[24],
      *(unsigned short*)&p[26],
      *(unsigned short*)&p[28],
      *(unsigned short*)&p[30]);
    i_log(rix, lbuf);
    p += 32;
  }
}



Bool read_db_silent = 0;


private void vfy_acc(Byte * p, Id upn, Page_nr page)

{ Set probs = 0; 
  Int ix;
#if 1
  ix = p[1008] & 255;
  if (ix != 0xfe)
  { i_log(ix, "*Tappimg Acc %x", page);
    probs = 1;
  }
#endif
  for (ix = 908; (ix -= 100) > 0; )
  { if (not rec_is_free(p, ix))
    { if (*(Id*)&p[ix] == 0)
      { extern Acc_con_t ths;
	extern char msg[500];
        i_log(ix, "*Zappimg Acc %x, %x %d", page, p, ths.acct.id);
        i_log(ix, "this_acct: %x %x %x", ths.a, ths.acct.id, ths.z);
        probs |= 2;
      }
    }
  }
  if (probs != 0)
  { dump_blk(p, 16);
  { Byte blk[BLOCK_SIZE];
    read_db_silent = 1;
    (void)read_db(upn, page, &blk[0]);
    read_db_silent = 0;
    dump_blk(blk, 16);
  }}
}


private Bool write_db(upn, page, p)
	 Id       upn;			/* write locked */
	 Page_nr  page;
	 Byte *   p;
{ if (upn == 0)				/* a dummy entry */
    return true;
    
  block_cs(p, 2);
#if 0
  if ((p[0] == 29 or p[0] == 33) and p[8] == 0)
    i_log(page, "*Zapped %s %d", p[0] == 29 ? "s" : "p", *(Int*)&p[12]);
#endif

  if (p[0] == 37)
    vfy_acc(p, upn, page);
  
  if      (page >= chan_filesize(upn))
    i_log(upn, wrpg, page, p[SEG_TYPE]);
  else if (chans(upn).wrchan != 0)
  { Int ct = 4;
    while (--ct >= 0 and not write_page(chan_fileno(upn), page, p))
      ;
    if (ct >= 0)
      return true;
    i_log(upn, twp, page);
  }
  return false;
}



public Bool read_db(upn, page, p)
	 Sid      upn;			/* read locked */
	 Page_nr  page;
	 Byte *   p;
{ Char * msg;
  Int clamp = 4;
  unsigned long need = BLOCK_SIZE;
				/* once only */
  while (true)
  { Int sz;
    Int fdesc = chan_fileno(upn);
    if (not ((sz = lseek(fdesc, (page-1)*BLOCK_SIZE + need, 0)) >= 0 and
             (sz = read(fdesc,p, need) == need)))
    { if (fdesc < 0 or page >= chan_filesize(upn))
      { cache_err = 1;
        msg = "ARP %d %d";
        break;
      }
      else
      { sleep(2);
        msg = "ARPage %d %d";
        i_log(upn, msg, page, chan_filesize(upn));
        need - sz;
        if (--clamp <= 0)
          break;
        continue;
      }
    }
    
    block_cs(p, 0);
    if (p[10] == 0 and p[0] == 33 and *(Int*)&p[14] != 0)
      i_log(page, "*Rapped pmt");
  { Set probs = 0; 
    Int ix;

    if (p[0] == 37)
    for (ix = 908; (ix -= 100) > 0; ) /* was 406 BLOCK_SIZE 512 */
    { if (not rec_is_free(p, ix))
      { if (*(Id*)&p[ix] == 0 and not read_db_silent)
        { i_log(ix, "*Rapping Acc %x", page);
          probs |= 2;
        }
      }
    }
    if (probs != 0)
      dump_blk(p, 16);
    return true;
  }}

  i_log(upn, msg, page, chan_filesize(upn));
  wr_unlock(upn);				/* probably not write_locked! */
  exit(2);
  return false;
}



private Bool decache_page(eny_)
      Cache_det eny_;			/* write or cache locked */
{ Cache_det eny = eny_;

  if (amended(eny))
  { Char upn = eny->page_upn >> 16;
    if (not chans(upn).wrchan)
      return false;
/*                        		    // write locks on: others can't act
    if (chans(upn).my_rd_ct < CT_RDRS and   // Well, not always true!
        chans(-upn).my_rd_ct < CT_RDRS and not delaywrite) 
      return false;
*/
    Byte * p = &block_tbl[eny->memory].c[0];
    if (p[SEG_TYPE] & REC_SEG)
      vfy_wifr(eny->page_upn & 0xffff, p, eny->wifr);
    
    if (not write_db(upn, eny->page_upn & 0xffff, p))
      return false;
  }
//printf("-- hash %d %d %x\n",eny->page_upn >> 16, eny->page_upn & 0xffff, eny);

{ Cache_det_r * h = &hash[ calc_hash(eny->page_upn) ];

  if (h[0] == eny - cache)
    h[0] = eny->overflow;	              /* must be atomic */
  else
  { Cache_det prev_ptr = null;
    Cache_det e = &cache[h[0]];

    while (1 == 1)
    { if (e == eny)
      { prev_ptr->overflow = eny->overflow;  /* must be atomic */
        break;
      }
      if (e->overflow == 0)
        break;
      prev_ptr = e;
      e = &cache[e->overflow];
    }
  }

  eny->busy_ct &= BUSY_MASK & ~this_log_pid ;            /* clear amended */
  eny->page_upn = 0;
  return true;
}}

static Cache_det find_my_lru()

{ Cache_det ptr = cache;

  while (true)                      // take ones own last used      
  { ptr = &cache[ptr->lru_forwards];
    if (ptr == cache)
      break;
    if (ptr->busy_ct & this_log_pid)
      break;
  }
  if (ptr != cache)                   // always true
  { pc_tbl[this_pix].num_held -= 1;
    ptr->busy_ct &= ~this_log_pid;
  }

  return ptr;
}

static Cache_det add_page(upn, page_nr)
    Sid           upn;				/* cache locked */
    Page_nr       page_nr;
{ Cache_det ptr = cache;
  Int err_ct = 3;
  if (pc_tbl[this_pix].num_held - permitted_pages >= 0)
    ptr = find_my_lru();

  if (ptr == cache)
  while (true)
  { ptr = &cache[ptr->lru_forwards];
    if (ptr == cache)
    { ptr = &cache[ptr->lru_forwards];
      i_log(0, ": cache_choked");
      cache_unlock();
      if (--err_ct < 0)
      { chan_unlink(upn);
        exit;
      }
      sleep(10);
      cache_lock();
    }
    else if (SINGLE_USER or (ptr->busy_ct & BUSY_MASK) == 0)
      break;
  }

  if (ptr->page_upn != 0)
    (void)decache_page(ptr);

  ptr->page_upn = 0x10000000;		/* must not be found */
  ptr->busy_ct = this_log_pid;
  pc_tbl[this_pix].num_held += 1;

 /*ptr->amended = false;*/

  cache_unlock();
  if (not read_db(upn, page_nr, &block_tbl[ptr->memory].c[0]))
    upn = 0;

  cache_lock();
{ Int page_upn = (upn << 16) + page_nr;
  ptr->page_upn = page_upn;

{ Cache_det_r *h = &hash[ calc_hash(ptr->page_upn) ];
  Cache_det e = &cache[h[0]];
  Cache_det prev_ptr = null;

  if (h[0] != 0)
  while (1 == 1)
  { if (e->page_upn == page_upn)        /* arrived meantime */
    { ptr->busy_ct = 0;			/* free up ptr */
      if ((e->busy_ct & this_log_pid) == 0)
        e->busy_ct |= this_log_pid;

      ptr = e;
      h = null;
      printf("In hash %d %d %x\n", page_upn >> 16, page_upn & 0xffff, e); 
      break;
    }
    if (e->overflow == 0)
      break;
    prev_ptr = e;
    e = &cache[e->overflow];
  }

  if (h != null)
  { ptr->overflow = h[0];                 /* add to hash */
    h[0] = ptr - cache;
//  printf("++ hash %d %d %x\n", page_upn >> 16, page_upn & 0xffff, ptr); 
  }

  to_end_lru_list(ptr)
  return ptr;  
}}}



public Cc vfy_page(upn, page_nr)
	Sid     upn;
	Page_nr page_nr;
{ return page_nr < chan_filesize(upn) ? OK : NOT_FOUND;
}

public Cache_det p_r_eny;			/* auxiliiary result */

private Byte * _page_ref(upn, page_nr)
    Sid        upn;			/* must be cache_locked */
    Page_nr    page_nr;
{ Int page_upn = (upn << 16) + page_nr;
  Cache_det_r *h = &hash[ calc_hash(page_upn) ];
  Cache_det eny = &cache[*h];
  if (*h != 0)
  while (1 == 1)
  { if (eny->page_upn == page_upn)
    { to_end_lru_list(eny)
//    printf("IN hash %d %d %x\n", page_upn >> 16, page_upn & 0xffff, eny); 
      if ((eny->busy_ct & this_log_pid) == 0)
      { eny->busy_ct |= this_log_pid;
        pc_tbl[this_pix].num_held += 1;
        if (pc_tbl[this_pix].num_held - permitted_pages >= 0)
        { Cache_det ptr = find_my_lru();
          if (ptr != cache)
            (void)decache_page(ptr);
        }
        (void)count_busys(null);
      }
      break;
    }
    if (eny->overflow == 0)
      break;
    eny = &cache[eny->overflow];
  }

  cache_err = 0;
{ int sz,ix;

  if (eny->page_upn == page_upn && eny != cache)
  { ix = keephd;
    for (sz = keepsz; --sz >= 0 && keep[ix] != eny;)
      ix = ++ix & (KEEPSIZE-1);
  }
  else
  { eny = add_page(upn, page_nr);
    (void)count_busys(null);
    ix = -1;
    sz = -1;
  }

  if (sz < 0)
  { keepsz = ++keepsz;
    if (keepsz > KEEPSIZE)
      keepsz = KEEPSIZE;
    else
      keephd = --keephd & (KEEPSIZE - 1);
    ix = keephd;
  }

  keep[ix] = eny;
  
  if (chans(upn).my_rd_ct < CT_RDRS)
  { int xs_hld = pc_tbl[this_pix].num_held - permitted_pages;
    if (xs_hld > 0)
    { Cache_det ptr = cache;
      Int mask = Q_AMDD | this_log_pid;
      while (true)
      { ptr = &cache[ptr->lru_forwards];
        if (ptr == cache)
          break;
        if ((ptr->busy_ct & mask) == 0)
          continue;
        ptr->busy_ct &= ~this_log_pid;
        if (--xs_hld <= 0)
          break;
      }
      pc_tbl[this_pix].num_held = xs_hld + permitted_pages;
  
      if (ptr == cache)
      { i_log(xs_hld, "Protection lost");
      }
      else
      { int mine = 0;
        while (true)
        { ptr = &cache[ptr->lru_forwards];
          if (ptr == cache)
            break;
          if ((ptr->busy_ct & this_log_pid) == 0)
            continue;
          ++mine;
        }
        if (mine >= permitted_pages)
          i_log(mine,"Have too many");
      }
    }
  }

  p_r_eny = eny;
#if 0
  if (not delaywrite)
  { Int ct = count_busys(null);
  }
#endif

  if (eny->memory == 0)
    i_log(eny,"mem is 0");

  return &block_tbl[eny->memory].c[0];
}}



public Byte * page_ref(upn, page_nr)
    Sid       upn;
    Page_nr   page_nr;
{ cache_lock();
{ Byte * res = _page_ref(upn, page_nr);
  cache_unlock();
  return res;
}}


public Byte * page_mark_ref(upn, page_nr)
    Sid       upn;
    Page_nr   page_nr;
{ vfy_wrlock(upn);
{ Byte * p = page_ref(upn, page_nr);
  if (not amended(p_r_eny))
    if (not block_cs(p, 1))
      read_db(upn, page_nr, p);

  p_r_eny->busy_ct |= Q_AMDD;
{ Vint wifr_fld = sf_wifr(p);
  if (in_range(wifr_fld, 1, BLOCK_SIZE-1))
      p_r_eny->wifr = *(Short*)&p[wifr_fld];

  return p;
}}}



public Byte * rec_ref_unchecked(upn, lra)
    Sid       upn;
    Lra_t     lra;
{ return &page_ref(upn, s_page(lra))[s_offs(lra)];
}



int mygddb_ct = 0;

void mygddb(int x)

{ ++mygddb_ct;
}

Int do_db(upn, mode)		
    Sid       upn;			/* cache_locked */
    Set       mode;
{ Int ct = 0;                 /* 1) Scan cdr's from 0 to current in use     */
  //   Vint wix;              /* 2) Identify relevant pages, if amended     */
			      /*  write them out to disc (stack and order?) */
  if (reqs_to_term <= SAFE_REQS && mode > 0)
  for (; ;)
  { // wix = -1;
    int mask = ~this_log_pid & BUSY_MASK;
    int want = this_log_pid + Q_AMDD;
    int sz = CACHE_SIZE;
    Cache_det ptr;
    for (ptr = cache - 1; --sz >= 0;)
    { if (((++ptr)->busy_ct & want) == 0)
        continue;

      if (upn != (ptr->page_upn >> 16))
        continue;

      if      ((mode & DO_DEST) and 
               (SINGLE_USER or (ptr->busy_ct & BUSY_MASK) == this_log_pid))
      { if (decache_page(ptr))
        { ptr->busy_ct = 0;
          ++ct;
        }
      }
      else if (mode & DO_FORCE)
      { 
        if (amended(ptr))
        {
          Byte * p = &block_tbl[ptr->memory].c[0];
          if (p[SEG_TYPE] & REC_SEG)
            vfy_wifr(ptr->page_upn & 0xffff, p, ptr->wifr);
#if 0
      	  writesort[++wix] = ptr - cache;
#else
      	  if (!write_db(upn, ptr->page_upn & 0xffff, p))
	          i_log(0, "Write Failed");
#endif
      	}
      	if (ptr->busy_ct & this_log_pid)
      	  ++ct;
      	ptr->busy_ct &= mask;       /* amended = false, not mine */
      }
#if 0
      if ((mode & DO_FORCE) and wix >= 0)
      { Vint wix_;
        mysrt(&writesort[0], &writesort[wix]);
      
        for (wix_ = wix + 1; --wix_ > 0; )
          if (writesort[wix_] CMP_OP writesort[wix_-1])
          { i_log(0, "Sort Failed");
            break;
          }

        for (++wix; --wix >= 0; )
        { Cache_det ptr = &cache[writesort[wix]];
      	  if (write_db(upn,ptr->page_upn & 0xffff,&block_tbl[ptr->memory].c[0]))
	          ptr->busy_ct &= BUSY_MASK;   /* amended = false; */
      	}
      }
#endif
    }
    if (upn < 0) break;
    upn = -upn;		/* now the index */
  }
  
  pc_tbl[this_pix].num_held -= ct;
  keepsz = 0;

/* if (mode & DO_FORCE)
    SOME MEANS OF EMPTYING THE BUFFER
*/
  return ct;
}

public void dest_db(upn)
    Sid   upn;
{ cache_lock();
  (void)do_db(upn, DO_DEST);
  cache_unlock();
}



public Int force_db(upn)
    Sid   upn;
{ cache_lock();
  Int ct = do_db(upn, DO_FORCE);
  cache_unlock();
  return ct;
}



public void dump_cache()

{      Bool amdd = false;
  fast Cache_det_r * h_ptr = &hash[HASH_SIZE-1];
  fast Cache_det ptr;

  FILE * op = fopen("/tmp/peterdc", "w");
  if (op == null)
    op = stdout;

  for (; h_ptr >= hash; h_ptr -= 1)
  { Bool done = false;
    for (ptr = &cache[*h_ptr]; 
	 ptr != (Cache_det)shnull; ptr = &cache[ptr->overflow])
      if (ptr->page_upn != 0)
      { 
        if (not done)
        { done = true;
          fprintf(op, "Hash (%d)\n", h_ptr - hash);
        }
        fprintf(op, "ix = %d, upn = %d, page = %d, amdd = %d", 
		            ptr-cache,(char)(ptr->page_upn >> 16),ptr->page_upn & 0xffff, 
                amended(ptr));
        fprintf(op, ptr->busy_ct == 0 ? "\n" : " B %x\n", ptr->busy_ct);
        if (amended(ptr))
          amdd = true;
      }
  }

  fprintf(op, not amdd ? "End\n" : "End (Amended)\n");
  fprintf(op, "From cache\n");

  amdd = false;

  for (ptr = cache; ptr < &cache[CACHE_SIZE]; ptr += 1)
  
      if (ptr->page_upn != 0)
      { 
        fprintf(op, "ix = %d, upn = %d, page = %d, amdd = %d", 
          ptr-cache,(char)(ptr->page_upn >> 16),ptr->page_upn & 0xffff, 
                     amended(ptr));
        fprintf(op, ptr->busy_ct == 0 ? "\n" : " B %x\n", ptr->busy_ct);
        if (amended(ptr))
          amdd = true;
      }

  fprintf(op, not amdd ? "End\n" : "End (Amended)\n");
  if (op != stdout)
    fclose(op);
}



public Cc cache_term()

{ Int ix;
  Bool wr_ct = 0;

  if (cachesem >= 0)
  {
    for (ix = MAX_LDN*2; --ix >= 0; )
    { if (chans2[ix].my_rd_ct >= CT_RDRS)
				wr_ct += 1;
    }
		
    reqs_to_term += 1;

    if	    (reqs_to_term > SAFE_REQS && shm_p->user_ct == 1 
    														      && memlocks[0] > 0)
    { i_log(0, "Breaking locks to terminate");
      cache_unlock();
    }
    else if (wr_ct > 0 and shm_p->user_ct > 1)
    { i_log(0, "Write locked terminator");
      return HALTED;
    }

    for (ix = sizeof(db_tbl) / sizeof(db_tbl[0]); --ix >= 0; )
      if (db_tbl[ix].refs & this_log_pid)
				chan_unlink(ix+1);
  
    fini_cache();
  }
  tcapclose();
  if (close_term)
    softfini();

  exit(0);
  return OK;
}
