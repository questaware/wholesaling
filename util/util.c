#include   <stdio.h>
#include   <string.h>
#include   <ctype.h>
#include   <time.h>
#include   <errno.h>
#include   <signal.h>
#include   <sys/stat.h>
#include   <unistd.h>
#include   <sys/ipc.h>
#include   <sys/shm.h>
#include   <sys/sem.h>
#include   "version.h"
#include   "../h/defs.h"

extern long lseek();

extern Char dbg_file_name[];
extern Int lk_base;

public Int trash;

public Int incident_ct = 0;


int gdb(res)
     int res;
{ return trash;
}


public Cc i_log(cc,f,a,b,c, d)
	Int  	cc;
	Char *  f;
	Int     a, b, c, d;
{ if (f[0] == '.')
    ++f;
  else
  { fprintf(stderr,f,a,b,c, d);
    fprintf(stderr,"\n\012\015");
    ++incident_ct;
  }
  if (dbg_file_name[0] != 0)
  { 
    FILE * dbg_file = fopen(dbg_file_name, "a");
    if (dbg_file != null)
    { time_t tm; 
      int trash = time(&tm);
      (void)localtime(&tm);
      char * ctstr = ctime(&tm);
      Char d3 = ctstr[2];
      Int sl = strlen(ctstr);
      ctstr[sl-5] = ctstr[sl-3];
      ctstr[sl-4] = ctstr[sl-2];
      ctstr[sl-3] = 0;
      if ( not (ctstr[2] == 'T' and d3 == 'u' or
                ctstr[2] == 'S' and d3 == 'n'))
        ctstr[2] = ctstr[0] + 'a' - 'A';
      ctstr += 2;

      fprintf(dbg_file, "%s %d \t%ld ", ctstr, getpid() % 100, cc);
      fprintf(dbg_file,f,a,b,c, d);
      fprintf(dbg_file,"\n");
      fclose(dbg_file);
    }
  }
  if (f[0] == '*')
    ps_dump();
  if (*strmatch("Corrupt", f) == 0)
    sleep(900);
	return cc;
}



public void ie_log(f,a,b,c, d, e)
	Char *  f;
	Int     a, b, c, d, e;
{ if (dbg_file_name[0] != 0)
  { FILE * dbg_file = fopen(dbg_file_name, "a");
    if (dbg_file != null)
    { fprintf(dbg_file,f,a,b,c,d,e);
      fprintf(dbg_file,"\n");
      fclose(dbg_file);
    }
  }
}


public void sysabort(s)
	Char * s;
{ i_log(0, "abort: %s\n", s);
  exit(100);
}


public Int noo_incidents()

{ return incident_ct;
}



public void fini_log(wait)
        Bool wait;
{ Char ch;

  if (incident_ct > 0)
  { fprintf(stderr,"\n\rERRORS: %d incidents occurred\n", incident_ct);
    if (wait)
      read(0, &ch, 1);
    incident_ct = 0;
  }
}

public Int read_page(fdesc, page_nr, block_addr)
	Short     fdesc;
	Page_nr   page_nr;
	void      *block_addr;
{ 
  return lseek(fdesc, page_nr*BLOCK_SIZE, 0) < 0 
            ? -1 : read(fdesc, block_addr, (unsigned long)BLOCK_SIZE);
}


public Bool write_page(fdesc, page_nr, block_addr)
	Short   fdesc;
	Page_nr page_nr;
	void    *block_addr;
{ /* 
  fprintf(stderr,"writing page %d, size %d\n", page_nr, block_addr[3]);
  */
  if (lseek(fdesc, page_nr*BLOCK_SIZE, 0) < 0)
    i_log(errno, "lseek failed !!");
  return write(fdesc, block_addr, (unsigned long)BLOCK_SIZE) == BLOCK_SIZE;
}




#define EXTEND_SZ 32

public Int extend_file(fdesc, page_nr)
	Short   fdesc;
	Page_nr page_nr;
{ /* 
  fprintf(stderr,"writing page %d, size %d\n", page_nr, block_addr[3]);
  */
  if (lseek(fdesc, (page_nr+EXTEND_SZ-1)*BLOCK_SIZE, 0) < 0)
  { i_log(errno, "lseek failed !!");
    return 0;
  }
  if (write(fdesc, nulls, (unsigned long)BLOCK_SIZE) < BLOCK_SIZE)
    return 0;
  return EXTEND_SZ;
}

#if XENIX

public void native_cp(t_,s_,len)
	Byte * t_, *s_;
	Short  len;
{ register Byte * t, *s;
  
  if (t_ >= s_)
  { s = &s_[len-1];
    for (t = &t_[len]; t > t_; )
      *--t = *s--;
  }
  else 
  { s = &s_[-1]; t = t_;
//  s_ = &s_[len]; t_ = &t_[len]; 
    for (; t < t_; )
      *t++ = *++s;
  }
}

#endif


public Char * strmatch(const Char * t_, const Char * s_)

{ fast const Char * t = t_;
  fast const Char * s = s_;

#if CHARCODE != ASCII
  for (; *s != 0 and *t != 0 and toupper(*t) == toupper(*s); ++s)
    ++t;
#else
  for (; *s != 0 and *t != 0 and (*t | 0x20) == (*s | 0x20); ++s)
    ++t;
#endif
  return (Char*)t;
}



public Short struccmp(t_, s_)
	Char *t_, *s_;
{ fast Char * t = t_;
  fast Char * s = s_;

  for (; *s != 0 and *t != 0 and toupper(*t) == toupper(*s); ++s)
    ++t;
  return (unsigned)toupper(*t) - (unsigned)toupper(*s);
}



public Char * skipspaces(s_)
	Char *s_;
{ fast Char * s = s_;

  for (; *s != 0 and isspace(*s); ++s)
    ;
  return s;
}



public Char * skipalnum(s_)
	Char *s_;
{ fast Char * s = s_;

  for (; *s != 0 and isalnum(*s); ++s)
    ;
  return s;
}



public Char * skipto(s_, delim)
	Char *s_;
	Char  delim;
{ fast Char * s = s_;

  for (; *s != 0 and *s != delim; ++s)
    ;
  return s;
}



public Char * skipover(s_, delim)
	Char *s_;
	Char  delim;
{ fast Char * s = s_;

  for (; *s != 0 and *s != delim; ++s)
    ;
  return s == 0 ? s : s+1;
}



public Char * strpcpy(Char * t_, const Char * s_, Short n)
       
      /* Short  n;                     ** size of copy including the 0 */
{ fast const Char * s = s_;
  fast Char * t = t_;
  
  while (--n > 0)
    if ((*t++ = *s++) == 0)
      break;
  if (n == 0)
    *t = 0;
    
  return t_;
}




public Char * strpcpydelim(Char * t_, const Char * s_, Short n, Char delim)
   /*   Short  n;                     ** size of copy including the 0 */
   /*   Char   delim;		      ** delimiter char */
{ fast const Char * s = s_;
  fast Char * t = t_;
  
  for  ( ; --n > 0; ++t)
    if ((*t = *s++) == 0 or *t == delim)
      break;
  
  *t = 0;
    
  return t_;
}

public Char * strchcpy(t_, ch_, n)
        Char * t_;
        Char   ch_;
        Short  n;
{ fast Char * t = t_;
  fast Char  ch = ch_;
  
  while (--n >= 0)
    *t++ = ch;
  *t = 0;
  return t_;
}



public Char * strwcpy(t_, s_)
        Char * t_, * s_;
{ fast Char * t = t_;
  fast Char * s = s_;
 
 while (*s > ' ')
   *t++ = *s++;
   
 *t = 0;
 return t_; 
}

#undef malloc
#undef free
#undef realloc

#define BALLAST_SZ (0x400)

static Int malloc_ct = 0;
static Char * ballast;

static Char * * malloc_lst = null;

public void malloc_init()

{ ballast = malloc(BALLAST_SZ+4);
}



public Char * chk_malloc(n)
	Int n;
{ malloc_ct += n;
{ Char * res = malloc(n+12);
  if (res == null and n <= BALLAST_SZ and ballast != null)
  { free(ballast);
    ballast = null;
    res = malloc(n+12);
    if (res != null)
      i_log(n, "NOOFMEM");
  }
  if (res != null)
  { if (((Int)res & 3) != 0)
      gdb(res);
    ((Int*)res)[0] = (Int)malloc_lst;
    ((Int*)res)[1] = n;
    ((Int*)(res+n+8))[0] = 0x98765432;	/* LLONG_ALGNMT ?? */
    malloc_lst = (Char **)res;
  /*i_log(0, "MEMEMEM %x", res);*/
    return &res[8];
  }
  
  i_log(n, "Out of Memory");
  kill(getpid(), SIGTERM);
  sleep(20);
  return null;	/* never reached */
}}



Char ** malloc_fprev(bkt)
          int bkt;
{ fast Char ** mlst;
  fast Char ** prev = null;
       Char ** err = null;
  
  for (mlst = malloc_lst; mlst != null; mlst = (Char **)mlst[0])
  { if (((Int*)((char*)mlst+((Int*)mlst)[1]+8))[0] != 0x98765432)
      err = mlst;
    if (mlst == (Char**)bkt)
    { if (err != null)
        i_log(err, "MALLOC CORR.");
      return prev;
    }
    prev = mlst;
  }
  
  i_log(bkt, "Free NF");
  return (Char**)-1;
}



public void chk_free(bkt)
	Char * bkt;
{ bkt -= 8;
/*i_log(0, "FREEFREE %x", bkt);*/
{ Int tag = ((Int*)(bkt+((Int*)bkt)[1]+8))[0];  /* LLONG_ALGNMT ?? */
  if (tag != 0x98765432)
  { i_log(tag, "Illegal Free of %x", bkt);
    return;
  }
{ fast Char ** prev = malloc_fprev(bkt);
  if ((Int)prev == -1)
    ;
  else
  { Char ** nxt = (Char**)((Char**)bkt)[0];
    if (prev == null)
      malloc_lst = nxt;
    else
      prev[0] = (Char*)nxt;
    free(bkt);
  }
}}}


public Char * chk_realloc(bkt, n)
	Char * bkt;
	int  n;
{ bkt -= 8;
{ Char ** nxt = (Char**)((Char**)bkt)[0];
  Int tag = ((Int*)(bkt+((Int*)bkt)[1]+8))[0];  /* LLONG_ALGNMT ?? */
  if (tag != 0x98765432)
    i_log(tag, "Illegal RA of %x", bkt);

{ Char ** prev = malloc_fprev(bkt);

  if ((Int)prev == -1)
  { Char * rpl = chk_malloc(n+12);
    memcpy(&rpl[0], &bkt[8], n);
    gdb(0);
    return &rpl[0];
  } 
{ Char * res = realloc(bkt, n+12);
  if (res == null and n <= BALLAST_SZ and ballast != null)
  { free(ballast);
    ballast = null;
    res = realloc(bkt, n+12);
    if (res != null)
      i_log(n, "NOOFMEM");
  }

  if (res == null)
  { i_log(n, "CHKREALLOC");
    return &bkt[8];
  }
  if (prev == null)
    malloc_lst = (Char**)res;
  else
    prev[0] = res;
  
  ((Char ***)res)[0] = nxt;
  ((Int*)res)[1] = n;
  ((Int*)(res+n+8))[0] = 0x98765432;	/* LLONG_ALGNMT ?? */
  return &res[8];
}}}}



public Int tot_malloc()

{ return malloc_ct;
}


public Char * prepare_dfile(char * t, const char * root, int id)

{ int diry1 = abs(id) / 10000;
  int diry2 = abs(id) / 100;
  char fname[256];
  struct stat stat_;

  sprintf(&fname[0],"%s/%s%d0000", root, id > 0 ? "" : "N", diry1);

  if (-1 == stat(fname, &stat_))
  { Cc  cc = mkdir(fname, 0777);
    if (cc != 0)
      return null;
  }

  sprintf(&fname[0],"%s/%s%d0000/%d00", root, id > 0 ? "" : "N", diry1, diry2);
  if (-1 == stat(fname, &stat_))
  { Cc  cc = mkdir(fname, 0777);
    if (cc != 0)
      return null;
  }

  sprintf(&t[0],"%s/%s%d0000/%d00/%d", root, id > 0 ? "" : "N", diry1, diry2,id);
  return t;
}

#define RW_ME 0600

static Id sem0 = -1;
struct sembuf sem_lk, sem_unlk;


public Id global_lock()

{
  return OK;
{ fast Id id = semget((long)lk_base + 18, 1, RW_ME);
       Bool first = id < 0 /*and errno == ENOENT*/;
  if (first)
  {	id = semget((long)lk_base, 1, IPC_CREAT | RW_ME);
	  if (id < 0)
	    return -1;
  { Cc cc = semctl(id, 0, (long)SETVAL, 1);
		if (cc != OK)
			return cc;
    sem0 = id;
  }}

/*sem_lk.sem_num = 0;invariant */
  sem_lk.sem_op  = -1;
  sem_lk.sem_flg = SEM_UNDO;
  sem_lk.sem_op  = -1;
  sem_lk.sem_flg = SEM_UNDO;

/*sem_unlk.sem_num = 0;invariant */
  sem_unlk.sem_op  = 1;
  sem_unlk.sem_flg = SEM_UNDO;

  if (semop(sem0, &sem_lk, 1L) < 0)
  { Cc cc = semop(sem0, &sem_lk, 1L);
		if (cc < 0)
			return cc;
	}

  return OK;
}}


public Id global_unlock()

{ // return OK;
  if (semop(sem0, &sem_unlk, 1L) < 0)
  { Cc cc = semop(sem0, &sem_unlk, 1L);
		if (cc < 0)
			return cc;
	}

  return OK;
}

