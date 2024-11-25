
#include "version.h"

#include <fcntl.h>
#include "../h/defs.h"
#include   "h/page.h"
#include   "h/memdef.h"
#include "../db/cache.h"


char dbg_file_name[40] = "cleanuplog";

char ths[108];			/* ditto */

const char nulls[512] = "";

void ps_dump()

{ 
}

void tcapclose()

{ tteeop();
}


void softfini()

{ 
}



int explain()

{ printf(
 "cleanup [-u ] [-b   |-p |-r   |-s |-z ]\n"
 "cleanup [-u } [-a # |-c |-f # |-o # ] dbname\n"
 "	-a # db	     Adjust readers (dangerous)\n"
 "      -b	     Basic semaphore statistics\n"
 "      -c db	     Display readers/writers on db\n"
 "      -f # [-#] db Show frees for record type # (at most #)\n"
 "      -o # db	     dump block #\n"
 "      -p	     Dump the cache\n"
 "      -r	     Restore shared memory (from /tmp/osdbshm)\n"
 "      -s	     Save shared memory (to /tmp/osdbshm)\n"
 "      -u	     Unlock the cache (v.dangerous)\n"
 "      -z	     Zap semaphores and shared memory\n"
 " Defaults: cleanup -b\n"
 "           cleanup -c db\n"
        );
  exit(0);
}





void sho_frees(Id upn, Int cl_id, Int clamp)

{ Short acl_id = cl_id < 0 ? -cl_id : cl_id;
  Short mi_eny = class_offs(acl_id);
  Byte * mi = page_ref(upn, ROOT_PAGE);

  Page_nr pg;
 
  printf(cl_id <= 0 ? "Freelist %d\n" : "Used list %d", cl_id);
  
  for (pg = cl_id == 0 ? r_word(mi, MI_FREE_LIST)   :
            cl_id  > 0 ? r_word(mi, mi_eny + CE_RP) :
                         r_word(mi, mi_eny + CE_FREE_LIST);
       pg != 0 and --clamp >= 0;)
  { Byte * p = page_ref(upn, pg);
    printf("Page %d FV %x\n", pg, p[RP_FREEVEC0]);
    pg = cl_id < 0 ? s_wifr(p)
    		   : r_word(p, FL_NEXT);
  }
}

void main(argc_,argv_)
	Int argc_;
	Char ** argv_;
{ Int argc = argc_;
  Char ** argv = argv_;
  Bool a_opt = false;
  Bool b_opt = false;
  Bool c_opt = false;
  Bool f_opt = false;
  Bool o_opt = false;
  Bool p_opt = false;
  Bool r_opt = false;
  Bool s_opt = false;
  Bool u_opt = false;
  Bool z_opt = false;
  Int clamp = 10;
  Int a_opt_val = 0;
  Int f_opt_val = 0;
  Int o_opt_val = 0;
  Char * dbname = null;

  for ( ; --argc > 0; ++argv)
  { Char * arg = argv[1];
    if (arg[0] == '-')
    { if      (arg[1] == 'a')
      { a_opt = true;
        a_opt_val = MININT;
      }
      else if (arg[1] == 'b')
        b_opt = true;
      else if (arg[1] == 'c')
        c_opt = true;
      else if (arg[1] == 'f')
      { f_opt = true;
        f_opt_val = MININT;
      }
      else if (arg[1] == 'm')
        r_opt = true;
      else if (arg[1] == 'o')
      { o_opt = true;
        o_opt_val = MININT;
      }
      else if (arg[1] == 'p')
        p_opt = true;
      else if (arg[1] == 's')
        s_opt = true;
      else if (arg[1] == 'u')
        u_opt = true;
      else if (arg[1] == 'z')
      { shm_del();
        locks_del();
        exit(0);
      }
      else if (in_range(arg[1], '0', '9'))
        clamp = atol(&arg[1]);
      else
        explain();
    }
    else if (a_opt_val == MININT)
      a_opt_val = atol(arg);
    else if (f_opt_val == MININT)
      f_opt_val = atol(arg);
    else if (o_opt_val == MININT)
      o_opt_val = atol(arg);
    else if (dbname == null)
      dbname = arg;
  }
  
  if (dbname == null)
  { if (not b_opt and not p_opt and not r_opt and not s_opt and not z_opt)
      b_opt = true;
  }
  else
  { if (not a_opt and not c_opt and not f_opt and not o_opt)
      c_opt = true; 
  }
  
  if (b_opt)
  { Int ix;
    Id cid = get_cache_lock(false);
    if (cid < 0)
      printf("No semaphore present\n");
    else
      for (ix = 0; ix <= 4*2; ix += 2)
      { Int c_ct = ct_rdrs(ix);
				printf("Upn %d	count %d\n", ix >> 1, c_ct);
      }
  } 
  else
  { Int users;
    Int iter;
    for (iter = 2; --iter >= 0; )
    { alarm(3);
      users = init_cache(false);
      alarm(0);
      if (users < 0)
      { if (iter > 0 and users == -7 and u_opt)
        { printf("Unlocking cache");
          cache_unlock();
          continue;
        }
        printf("Too many users(cc = %d), Any Key\n", users);
        exit(0);
      }
      break;
    }
    if (r_opt)
    { Int sz;
      extern Byte * give_shm();
      Byte * mem = give_shm(&sz);
      Id op = open("/tmp/osdbshm", O_RDWR + O_CREAT, 0644);
      if (op >= 0)
      { Int done = write(op, mem, sz);
        printf("Written %d to %s\n", done, "/tmp/osdbshm");
      }
      close(op);
      fini_cache();
      exit(0);
    }
    if (s_opt)
    { Int sz;
      extern Byte * give_shm();
      Byte * mem = give_shm(&sz);
      Id op = open("/tmp/osdbshm", O_RDONLY);
      if (op >= 0)
        read(op, mem, sz);
      sleep(600000);
      fini_cache();
      exit(0);
    }

    if      (p_opt)
      dump_cache();
    else
    { Set props = F_SYNC + F_WRITE + F_IXOPT + F_WRIX + F_EXISTS;
      Int mainupn = init_db(dbname, props);
      if (mainupn < 0)
      { printf("Couldnt access (%d) db %s\n", mainupn, dbname);
	exit(1);
      }
      if      (f_opt)
      { sho_frees(mainupn, f_opt_val, clamp);
      }
      else if (o_opt)
      { Byte * p = page_ref(mainupn, o_opt_val);
	Int ix;
	printf("Page %d:\n", o_opt_val);
	for (ix = -1; ++ix < BLOCK_SIZE; )
	  printf("%3x ", p[ix]);
	printf("\n");
      }
      else if (a_opt or c_opt)
      { printf("Upn %d Number of users %d\n", mainupn, users-1);
      { Int rdrs = ct_rdrs(mainupn);
	printf("Writers %d, Readers %d\n", rdrs / 8, rdrs & 7);
	if (a_opt and (rdrs != 0 or a_opt_val != 0))
	  raw_unlock(mainupn, a_opt_val != 0 ? a_opt_val : rdrs);
      }}
      chan_unlink(mainupn);
    }
    fini_cache();
  }
}
