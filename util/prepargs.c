#include "../h/base.h"
#include "../h/prepargs.h"

private Int argc = -1;
private Char ** argv;


#define MININT 0x80000000

public Int args[NARGS];


public Bool moreargs( argc_,argv_, srcfn)
	Short  argc_;
        Char * argv_[];
	Char * *srcfn;
{ Short i,v;

  if (argc == -1)
  { argc = argc_;
    argv = argv_;
  
    for (i = 0;i < NARGS; ++i)
      args[i] = MININT;
  }

  while (--argc > 0)
    if ((++argv)[0][0] != '-')
    { *srcfn = &argv[0][0];
      return true;
    }
    else 
      for (i = 1; true; ++i)
      { Char ch = argv[0][i] & 0xdf;
        if ( not in_range(ch,'A','Z') )
          break;
        
        for (v = 0; in_range(argv[0][i+1], '0', '9'); ++i)
          v = v * 10 + argv[0][i+1] - '0';   /* negatives to go in */
        ARGS(ch) = v;
      }
  return false;
}  
