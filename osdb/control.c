#include  <stdio.h>
#include  <signal.h>
#include  <string.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <sys/fcntl.h>
#include  <unistd.h>
#include "version.h"
#include "../h/defs.h"
#include "../h/errs.h"
#include "../h/prepargs.h"
#include "../h/tcap_reserve.h"
#include "../form/h/form.h"
#include "../form/h/verify.h"
#include "../db/cache.h"
#include "../db/b_tree.h"
#include "../db/recs.h"
#include "../db/memdef.h"
#include "domains.h"
#include "schema.h"
#include "rights.h"
#include "generic.h"
#include "trail.h"

/*

public Int force_appln(lock, db)
     Bool    lock;
     Id      db;
{ if (db != -1)
  { if (lock)
      rd_lock(db);
    alarm(0);
    force_db(db);
    
    if (db == mainupn)
      force_appln(lock, invupn);

    if (lock)
      rd_unlock(db);
  }
  return 0;
}

*/

int softa = 0;

public void softalarm(int n)

{ softa = n;
  alarm(n);
}



public Cc softterm(int n)

{ alarm(0);
  signal(SIGTERM, softterm);
  signal(SIGALRM, softterm);

  if (n == 0)
    return OK;

  alarm(30);

  return cache_term();
}



public void softfini()

{ fini_log(false);
  tcapclose();
  exit(1);
}



public Cc setup_sig(c)
   Cc  (*c)();			/* obsolete */
{ 
  (void)softterm(0);
}

public const Char * do_commit(state)
	Quot  state;
{ if (this_at[0] == 0 or getenv("NOFLOP") != null)
    return null;
{ Int fildes = open(this_at, O_CREAT + O_RDWR + O_SYNC, 0777);
  Au_header_t  auh;
  Cc cc;
  if (fildes < 0)
    return "Audit file missing";
  
  lseek(fildes, WASTE_AREA, 0);
  cc = read(fildes, (Char *)&auh, sizeof(auh));

{ static Char buf[100];
  sprintf(&buf[0], "Last Save %s\nFile size %ld bytes     \n", 
  						ctime(&auh.time), auh.size);
  if (auh.size + 10000 > maxfdsize)
    strcat(&buf[0], "Audit Disk needs changing\n");

  return cc != sizeof(auh) or auh.tag != TRAIL_TAG ? "Not an Audit File" 
                                                   : (const char*)buf;
}}}


public const Char * do_backup()

{ Sale_t sale;
  Lra_t lra_t;
  wr_lock(mainupn);
{ Unique un = find_uniques(mainupn, &lra_t);
  if (un != null)
  { un->backupct += 1;
    write_rec(mainupn, CL_UNIQ, lra_t, un);
  }
  wr_droplock(mainupn);
  rd_lock(invupn);
  sale = null_sct(Sale_t);
  sale.kind = 0;
  sale.id   = un->backupct;
  sale.date = today;
  at_xactn((Sorp)&sale);
  force_db(invupn);
  
{ Char * res = do_commit(Q_LOCKED);
  salert(res);
  printf("\n\r");
{ Char cmdline[80]; sprintf(&cmdline[0], "./ttar cv %s %s", maindb, invdb);
{ Cc cc = do_osys(cmdline);

  rd_unlock(invupn);
  rd_unlock(mainupn);
  return cc == OK ? "Done, Any Key" : "Backup failed, Any Key";
}}}}}
