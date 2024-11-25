#include   <execinfo.h>
#include   <stdio.h>
#include   <ctype.h>
#include   <time.h>
#include   <errno.h>
#include   "version.h"

#define LVAR_OFFS 4

#define MIN_FRAME 4
#define MAX_FRAME 8000

extern void ie_log();

Int * top_fp;


private Int * this_fp()

{ Char buf[4];
  return *(Int**)&buf[LVAR_OFFS];
}



public void ps_init()

{ 
//top_fp = this_fp();
//top_fp = (Int*)top_fp[0];
}



private Int this_caller(fp, cfp_ref, ra_ref, p_area_ref)
	Int *     fp;
	Int *   * cfp_ref;
	Int *   * ra_ref;
	Int *   * p_area_ref;
{ Int * cfp = (Int *)fp[0];
  *cfp_ref = cfp;
  *ra_ref = (Int *)fp[1];
  *p_area_ref = &fp[2];
{ Int diff = (Int)cfp - (Int)fp;
  return cfp >  top_fp 				? -1 : 
         cfp == top_fp				?  0 : 
         in_range(diff, MIN_FRAME, MAX_FRAME)   ?  1 : -1;
}}



public void ps_dump()

{ void *trace[16];
  int i;
  int trace_size = backtrace(trace, 16);
													  /* overwrite sigaction with caller's address */
//char ** messages = backtrace_symbols(trace, trace_size);
												  /* skip first stack frame (points here) */
  printf("[bt] Execution path:\n");
  for (i=1; i<trace_size; ++i)
  {
    ie_log("%x", trace[i]);
//  printf("[bt] #%d %s\n", i, messages[i]);

    char syscom[256];
    sprintf(syscom,"addr2line %p -e sighandler", trace[i]); //last parameter is the name of this app
    system(syscom);
  }
#if 0
{ Int * fp = this_fp();
  Int * cfp;
  Int * ra;
  Int * p_area;
  
  while (this_caller(fp, &cfp, &ra, &p_area) >= 0)
  { ie_log("%x (%8x, %8x, %8x, %8x, %8x)", ra, 
	      p_area[0], p_area[1], 
	      p_area[2], p_area[3], p_area[4]);
    if (cfp  == top_fp)
      break;
    fp = cfp;
  }
}
#endif
}

