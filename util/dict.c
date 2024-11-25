#include   <stdlib.h>
#include   "../h/base.h"


#define inithashsze 256   /* must be a power of two */
#define prime1  37
#define primestp 7        /* primestp mod inithashsze != 0 */

private Ptr free_dicts = null;    /* empty dictionaries */

#define zalloc malloc


typedef struct
{ Char * dom;
  Int    rng;
} Hasheny_t, *Hasheny;

public void freedict(p)
        Ptr  p;
{ p[0] = (Int)free_dicts;
  free_dicts = p;
}



public Ptr newdict(logsze)
        Int  logsze;
{ Int sze = 1 << (logsze-1);

  if (free_dicts != null and free_dicts[-1] + 1 >= sze)
  { Ptr res = &free_dicts[ free_dicts[-1]*2 + 2];
    Ptr lim = free_dicts;
    
    free_dicts = (Ptr)free_dicts[0];
    for (; res > lim;)
      *--res = 0;
    return res;
  }
  else
  { Ptr res = (Ptr)zalloc(sze*sizeof(Hasheny_t)+4);
    res[0] = sze - 1;
    return &res[1];
  }
}



private Int hashstr(s)
       Char * s;
{ Int res = 0;
  while (*s != 0)
    res += *s++ * res;
  return res;
}

#define dictbody(hashtblref,dom)                   \
\
  Hasheny hashtbl = (Hasheny)*hashtblref;          \
  Int hashszem1 = ((Ptr)hashtbl)[-1];              \
  Int hashv = hashstr(dom);                        \
  Int miss;                                        \
  Hasheny eny = &hashtbl[ hashv ];                 \
\
  if (eny->dom != 0 and strcmp(eny->dom,dom) != 0) \
    for (miss = 2; ; ++miss)                       \
    { hashv = (hashv + primestp) & hashszem1;      \
      eny = &hashtbl[ hashv ];                     \
      if (eny->dom == 0 or strcmp(eny->dom,dom) == 0) \
        break;                                     \
\
      if (miss * 8 /* say */ >= hashszem1)         \
      { Ptr nhashtbl = (Ptr)zalloc(hashszem1*2*sizeof(Hasheny_t)+20); \
        Short lim = (hashszem1 + 1) * 2;           \
        Short i;                                   \
        *nhashtbl++ = lim -1;                      \
        for (i = hashszem1; i >= 0; --i)           \
        { Char * dom = hashtbl[i].dom;             \
          if (dom != 0)                            \
          { Hasheny eny = (Hasheny)dict(&nhashtbl,dom); \
            eny->dom = dom;                        \
            eny->rng = hashtbl[i].rng;             \
          }                                        \
        }                                          \
        freedict(hashtbl);                         \
        *hashtblref = nhashtbl;                    \
        eny = (Hasheny)dict(hashtblref,dom);       \
        break;                                     \
      }                                            \
    }

  
public Ptr dict(hashtblref,dom)
        Ptr *  hashtblref;
        Char * dom;        /* != 0 */
{ dictbody(hashtblref,dom);
  return (Ptr)eny;
}



public Int rddict(hashtblref,dom)
        Ptr * hashtblref;
        Char * dom;        /* != 0*/
{ dictbody(hashtblref,dom);
  return eny->rng;
}

