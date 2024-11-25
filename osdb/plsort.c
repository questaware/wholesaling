#include <stdio.h>
#include <version.h>
#include "../h/prepargs.h"


static Int tabsz = 8;
static Int pagelen = 66;


Char * dbg_file_name = "pl_sort";


#define MAX_LINE_SIZE  80
#define SORT_ARR_SIZE 500

#define STT_STEXT 18
#define STP_STEXT 55
/*
    10160   24   .30                7-Up Cherry Cans   4.47

     bloh   12   .65         R.Whites 1.5ltr Bottles   4.72
*/

static Char linearr[SORT_ARR_SIZE][MAX_LINE_SIZE+4];



#define VT 12


#define Q_EOF    (-1)
#define Q_IGNORE 0
#define Q_FORM   1


Bool sort_pline(t_, s_)
     Char *t_, *s_;
{ Char * s = &s_[STT_STEXT];
  Char * t = &t_[STT_STEXT];
  
  while (*s == ' ')
    ++s;
    
  while (*t == ' ')
    ++t;
    
  while (*s == *t and s <= &s_[STP_STEXT] and
	              t <= &t_[STP_STEXT])
  { ++s; ++t;
  }
  
  return s > &s_[STP_STEXT] ? (t > &t_[STP_STEXT] ? 0 : 1) :
	 t > &t_[STP_STEXT] ? -1                           :
	 *t > *s            ? 1                            : -1;
}     



Quot get_line(file, line)
   FILE * file;
   Char * line;
{ Char * s = fgets(&line[0], MAX_LINE_SIZE, file);
  if (s == null)
  { line[0] = 0;
    return Q_EOF;
  }
    
  while (*s == ' ' or *s == '\t')
    ++s;
  if (*s != '@' and not in_range(*s, 'a', 'z') and not in_range(*s, '0','9'))
    return Q_IGNORE;
  while (*s != ' ' and *s != 0)
    ++s;
  
  return *s != ' ' or s - &line[0] != 6 ? Q_IGNORE : Q_FORM;
}


void explain()

{ fprintf(stderr, "Usage: plsort [-wN ] [-lN ] [-v] { file } *\n"
		  "  where in -wN  N is the tab size\n"
		  "  where in -lN  N is the page length\n");
}

#define MININT 0x80000000

void main (argc,argv)
	Int argc;
	Char ** argv;
{
  Char * srcfn = null;
  Bool more = true;

  while (more)
  { more = moreargs(argc,argv,&srcfn);
    
    if (not more and srcfn != null)
      break;

    if (ARGS('W') != MININT)
      tabsz = ARGS('W');

    if (ARGS('L') != MININT)
      pagelen = ARGS('L');

    if (ARGS('X') != MININT)
    { explain();
      exit(0);
    }

  { FILE * file = not more ? stdin : fopen(srcfn, "r");
    if (file == null)
      fprintf(stderr, "Cannot open file %s\n", more ? srcfn : "stdin");
    else 
    { Int last_sa;
      Int arr_ix;
    
      while (true)
      { Quot kind = Q_FORM;
      
	for (last_sa = 0; kind == Q_FORM and last_sa < SORT_ARR_SIZE; ++last_sa)
	  kind = get_line(file, &linearr[last_sa][0]);

	--last_sa;
   
	if (last_sa > 0)
	  sort(sort_pline, MAX_LINE_SIZE+4,
	                         &linearr[0][0], &linearr[last_sa-1][0]);

	for (arr_ix = 0; arr_ix <= last_sa; ++arr_ix)
	  printf("%s", &linearr[arr_ix][0]);
	if (kind == Q_EOF)
	  break;
      }
    }
  }}                   
}    
