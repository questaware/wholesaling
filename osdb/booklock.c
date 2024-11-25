#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "version.h"

#include "../h/defs.h"
#include "../db/cache.h"

char dbg_file_name[30+3];

char ths[108];

#define DBNAME "dbmain"

int g_locklen = 0;

int g_lock = false;
int g_unlock = false;

const char nulls[1024];

static void explain()

{ fprintf(stderr, "toplock [-i | -l | -u | -s <secs> |] [<db>]\n"
                  "     -i        -- show readers\n"
                  "     -l        -- lock\n"
                  "     -u        -- unlock\n"
                  "     -s secs   -- seconds to lock\n"
                  "where the default db is '%s'\n", DBNAME
      	       );
  exit(0);
}



tcapclose()

{
}


Cc softalarm(int n)

{
}


void softfini()

{
}


softterm(int n)

{ exit(0);
}


ps_dump()

{ /* to resolve the symbol */
}

public void vfy_wifr(Char * p, Short wifr)

{ 
}



public Set rec_is_free(Char * p)

{ return 0;
}


public Vint sf_wifr(Char * p)

{ return 0;
}



void main(argc,argv)
	Vint argc;
        Char ** argv;
{ Vint argsleft = argc - 1;
  Char ** argv_ = &argv[1];
  Bool i_opt = false;

  for (; argsleft > 0 and argv_[0][0] == '-'; --argsleft)
  { Char * flgs;
    for (flgs = &argv_[0][1]; *flgs != 0; ++flgs)
      if      (*flgs == 'l')
        g_lock = true;
      else if (*flgs == 'u')
        g_unlock = true;
      else if (*flgs == 's')
      { if (in_range(flgs[1], '0', '9'))
	  g_locklen = atoi(&flgs[1]);
	else if (argsleft > 0)
	{ --argsleft;
	  if (argsleft <= 0)
	    explain();
	  g_locklen = atoi(*++argv_);
	}
      }
      else if (*flgs == 'i')
        i_opt = true;
      else
        explain();
    ++argv_;
  }
  
{ Char * srcfn = argsleft <= 0 ? DBNAME : argv_[0];
  Short tally = i_opt+g_lock+g_unlock+(g_locklen != 0);
  if (tally == 0)
    g_lock = true;
  if (tally > 1)
    explain();
  sprintf(&dbg_file_name[0], "%slog", srcfn);  
{ Int users = init_cache((Bool)0);
  if (users < 0)
  { printf("Too many users\n");
    exit(0);
  }

{ Set props = F_SYNC + F_WRITE + F_IXOPT + F_WRIX + F_EXISTS;
  Int mainupn = init_db(srcfn, props);
  if (mainupn < 0)
  { printf("Couldnt access db %s\n", srcfn);
    exit(1);
  }
  if (i_opt)
    printf("Lock count: %d\n", ct_rdrs(mainupn));

  if (g_lock or g_locklen > 0)
    raw_lock(mainupn, 1);

  if (g_locklen > 0)
     sleep(g_locklen);

  if (g_unlock or g_locklen > 0)
    raw_unlock(mainupn, 1);

  chan_unlink(mainupn);
  fini_cache();
}}}}
