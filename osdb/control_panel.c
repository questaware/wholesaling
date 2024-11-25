#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "version.h"
#include "../h/defs.h"
#include "../h/errs.h"
#include "../h/prepargs.h"
#include "../h/defs.h"

#include "../db/cache.h"

extern Int lk_base;


Cc i_log(void) {};
void sf_wifr() {};
void vfy_wifr() {};
void tcapclose() {};
void softfini() { };
Bool write_page(Short i, Int n, void * p) { return 0; };
Bool rec_is_free() { return 0; };

char ths [108];



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

/*
typedef struct 			        // stored in shared memory
{ Int    id;
  Int    num_held;
} Proc_t;
*/

void explain()

{ fprintf(stderr, "control_panel [ -j ]\n");
  exit(0);
}

void main(argc,argv)
	Int argc;
	Char ** argv;
{ int j_opt = 0;
  int argsleft = argc - 1;
  Char ** argv_ = &argv[1];
	Char * user = getenv("PWD");

  lk_base += strcmp(user,"/home/osdb") == 0  ?  0 :
             strcmp(user,"/home/osdb2") == 0 ? 20 :
             *strmatch("/home/peter",user) == 0 ? 40 : 60;

  for (; argsleft > 0 and argv_[0][0] == '-'; --argsleft)
  { Char * flgs;
    for (flgs = &argv_[0][1]; *flgs != 0; ++flgs)
      if      (*flgs == 'j')
        j_opt = 1;
      else
        explain();
    ++argv_;
  }

{ Proc_t * pc_tbl = NULL;
	int ct = get_cache_users(&pc_tbl);
	printf("Number of users %d\n------------------\n", ct);

	if (ct > 0 && pc_tbl != NULL)
		for (int ix = -1; ++ ix < MAXCLIENTS; )
		{ if (pc_tbl[ix].id == 0)
				continue;
			printf("Pid %5d NH %d on %s\n", 
						  pc_tbl[ix].id, pc_tbl[ix].num_held, 
						  pc_tbl[ix].tty); 
		}	
}}
