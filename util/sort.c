#include "string.h"
#include "version.h"
#include "../h/defs.h"


#if 0

#define MAX_ELEMSZ 1000
static Char elembuf[MAX_ELEMSZ];



void sort(prc_cmp, elemsz, lb, ub)
  Bool   (*prc_cmp)();
  Vint   elemsz;
  Char * lb, * ub;
{ if (ub - lb > 0)
  { Char * low = lb;
    Char * upp = ub;
    Vint diff = (upp - low) / elemsz;
    Char * t = low + (diff / 2) * elemsz;

    while (upp > low)          
    { for (; low < upp; low += elemsz)
        if      (low == t)
        { diff = (upp - low) / elemsz;
          t = upp - (diff / 2) * elemsz;
          if (low == t)
            break;
          else
          { memcpy(&elembuf[0], t, elemsz);
            memcpy(t, low, elemsz);
            memcpy(low, elembuf, elemsz);
          }
        }   
        else if ((*prc_cmp)(low, t) >= 0)
          break;
         
      for (; low < upp; upp -= elemsz)
        if      (upp == t)
        { diff = (upp - low) / elemsz;
          t = low + (diff / 2) * elemsz;
          if (upp == t)
            break;
          else
          { memcpy(&elembuf[0], t, elemsz);
            memcpy(t, upp, elemsz);
            memcpy(upp, elembuf, elemsz);
          }
        }        
        else if ((*prc_cmp)(upp, t) <= 0)
          break;

      if (upp > low)
      { memcpy(&elembuf[0], upp, elemsz);
        memcpy(upp,low,elemsz);
        memcpy(low, elembuf, elemsz);
        low += elemsz;
        upp -= elemsz;
      }
    }
    if (low == upp)
    { if (low != t and (*prc_cmp)(low, t) > 0)
      { sort(prc_cmp, elemsz, lb, low - elemsz);
        sort(prc_cmp, elemsz, low, ub);
      }
      else
      { sort(prc_cmp, elemsz, lb, low);
        sort(prc_cmp, elemsz, low + elemsz, ub);
      }
    }
    else 
    { sort(prc_cmp, elemsz, lb, upp);
      sort(prc_cmp, elemsz, low, ub);
    }
  }
}

#endif

static Int (*my_prc_cmp)();
static Char * spare;
static Int elemsz;


void srt(lb, ub)
  Char * lb, * ub;
{ Int diff = ub - lb;
  if (diff > 0)
  { Char * t = lb + (diff / (elemsz * 2)) * elemsz;
    
    srt(lb, t);
    srt(t + elemsz, ub);
    memcpy(spare, lb, diff + elemsz);

  { fast Char * low = &spare[0];
    fast Char * upp = &spare[t - lb + elemsz];
         Char * uppstt = upp;
         Char * uppstp = &spare[diff];
   
    while (true)
    { Char * nxt;
      if      (upp > uppstp)
      { if (low >= uppstt)
          break;
        nxt = low;
        low += elemsz;
      }
      else if (low >= uppstt)
      { nxt = upp;
        upp += elemsz;
      }
      else if ((*my_prc_cmp)(low, upp) <= 0)
      { nxt = low;
        low += elemsz;
      }
      else 
      { nxt = upp;
        upp += elemsz;
      }
      memcpy(lb, nxt, elemsz);
      lb += elemsz;
    }
  }}
}


Bool sort(prc_cmp, elemsz_, lb, ub)
  Int    (*prc_cmp)();
  Vint   elemsz_;
  Char * lb, * ub;
{ my_prc_cmp = prc_cmp;
  elemsz = elemsz_;
  spare = malloc((ub - lb + 1) * elemsz);
  if (spare != null)
  { srt(lb, ub);
    free(spare);
    return true;
  }
  
  return false;
}

