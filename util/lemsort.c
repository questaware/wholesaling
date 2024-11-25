#include   "../h/base.h"

#define SORTTYPE 0
#define BIGCODE 1

#define Keytype Int
#define Vint Int


#define MININT 0x80000000
#define MAXINT 0x7fffffff


static Int memsort_stride;


#define  CMP_OP >
#define OCMP_OP <=

#if SALONE || 1

extern Keytype testtbl[];

void prttbl();

#else

void prttbl() {}

#endif

void mysrt(Keytype * lb, Int count);


char blank[] = "                                        ";

#define sps(n) &blank[40 - (n)]

#define MAXUNS 4

Int depth = 0;
Int swaps = 0;

/*
void dsp_tbl(Int stt, Int mid, Int ct)

{ Keytype * tbl = &testtbl[stt];
  Int ix;
  printf("{BM %d mid %d ct %d\n", stt, mid, ct);
  for (ix = -1; ++ix < ct; )
    printf(" %d", tbl[ix]);
  printf("\n");
}
*/

void chk_tbl(Int stt, Int ct, Char * msg)

{ Keytype * tbl = &testtbl[stt];
  Int ix;
  printf("BF %d %d\n", stt, ct);
  for (ix = ct; --ix > 0; )
    if (tbl[ix-1] > tbl[ix])
      break;
 
  if (ix > 0)
  { printf("%s\n", msg);
    printf("%d %d XX", stt, ct);
    prttbl();
  }
}



#if SORTTYPE < 1

static void mymerge(left, leftn, totn)
   Keytype * left;
   Vint    leftn,  totn;		/* leftn < totn */
{
  fast Keytype * riteoub = left + totn - 1;
       Keytype * luub = left + leftn;

  fast Keytype * leftoub /* = left*/;
/*					** this is slower
  while (*leftoub < luub[0] and leftoub < luub)
    ++leftoub;
  left = leftoub;
*/
  leftoub = (luub-1);
/*printf("%sMERGE left %d  mid %d right %d\n", 
	         sps(depth), left-testtbl, luub-testtbl,  riteoub-testtbl);*/

 while (true) /* (luub > left)		** there is a left wedge */
 {fast Keytype * leftnlb = leftoub + 1;		/* empty */
  fast Keytype * leftnub = leftnlb - 1;

  while (riteoub >= luub)
  {/*printf("RITEOUB %d luub %d\n", riteoub-testtbl, luub-testtbl);
    printf("%sLeftoub %d  Leftnlb %d  Leftnub  %d\n", 
	     sps(depth), leftoub-testtbl, leftnlb-testtbl,  leftnub-testtbl);
    printf("%sRiteoub %d\n", 
	     sps(depth), riteoub-testtbl);*/
    if (leftnlb > leftnub)		/* the middle wedge has run out */
    { if (luub - leftnub > 1 and luub - leftoub - 1 > 1)
      { if (leftnub > leftoub) /* => mymerge:p2 > 0 */
        { /*dsp_tbl(leftoub-testtbl+1, leftnlb - leftoub - 1, (luub-1) - leftoub);*/
          mymerge(leftoub+1, leftnlb - leftoub - 1, (luub-1) - leftoub);
          /*printf("}DM %d %d\n", leftoub-testtbl+1, (luub-1) - leftoub);*/
        }
       /*chk_tbl(leftoub-testtbl+1, (luub-1) - leftoub, "The Merge");*/
       /*mysrt(leftoub+1, (luub-1) - leftoub);*/
      }
      leftnlb = leftoub + 1;
      leftnub = (luub-1);
    }
  { Keytype * p;
    Keytype * maxp = leftoub;
    Keytype maxval = *maxp;

    if (leftnlb <= leftnub and (left >  leftoub or *leftoub <= *leftnub))
    { maxp = leftnub;
      maxval = *maxp;
    }
/*
    for (p = left-1; ++p < luub;)
      if (*p > maxval)
      { printf("Not highest %d %d %d\n", left-testtbl, maxval, *p);
	break;
      }
*/
    ++riteoub;
    while (--riteoub >= luub and *riteoub >= maxval)
      ;
  /*while (*riteoub >= maxval and --riteoub >= luub)
      ;*/

    if (riteoub < luub)
      break;
    /*printf("%sSwap  %d  %d\n", sps(depth),maxp-testtbl,riteoub-testtbl);*/
    /*printf("Swap %d %d\n", maxp-testtbl,riteoub-testtbl);*/
    ++swaps;
    *maxp = *riteoub;
    *riteoub = maxval;
    --riteoub;
    if      (maxp != leftoub)
      --leftnub;
    else
    { --leftoub;
    /*printf("leftoub %d leftnlb %d leftnub %d\n",
                 leftoub-testtbl, leftnlb-testtbl, leftnub-testtbl);*/
      if (leftnub+1 == luub and leftnlb-1 == leftoub+1)
      { /*printf("--leftnlb\n");*/
        --leftnlb;
      }
    }
    /*prttbl();*/
    /*printf("Riteoub %d luub %d\n", riteoub-testtbl, luub - testtbl);*/
  }}
  if (leftoub >= riteoub)	/* there is no right wedge */
    break;

  if (luub - leftoub - 1 > 1 and (leftoub+1 < leftnlb or leftnub < (luub-1)))
  { /*printf("Shit\n");*/
#if 0
    mysrt(leftoub+1, (luub-1) - leftoub);
#else
    if (leftnub > leftoub) /* => mymerge:p2 > 0 */
    { /*dsp_tbl(leftoub-testtbl+1, leftnlb - leftoub, (luub-1) - leftoub);*/
      mymerge(leftoub+1, leftnub - leftoub, (luub-1) - leftoub);
    /*printf("}DM %d %d\n", leftoub-testtbl+1, (luub-1) - leftoub);*/
    }
     /*chk_tbl(leftoub-testtbl+1, (luub-1) - leftoub, "The Merge");*/
#endif
  }
/*if (riteoub != (luub-1))
    printf("riteoub not correct\n");*/
  luub = leftoub + 1;
  if (luub <= left)
    break;
 }
}




void mysrt(Keytype * lb, Int count)

{ /*printf("%s{ mysrt  %d  (%d)\n", sps(depth), lb - testtbl, count);*/
  Int wba[40];
  Int hba[40];
  Int cba[40];
    
 while (true)
 {if      (count <= 2)
  { /* if (count == 2) */
    if (lb[0] > lb[1])
    { Keytype val = lb[0];
      lb[0] = lb[1];
      lb[1] = val;
    /*printf("%sDwap %d %d\n", sps(depth+1), lb-testtbl, lb-testtbl+1);*/
    /*printf("Dwap %d %d\n", lb-testtbl, lb-testtbl+1);*/
      ++swaps;
    }
  }
#if BIGCODE
  else if (count == 3)
  { Keytype val = lb[0];
    if      (val > lb[1])
      if (lb[1] > lb[2])		/* lb[1] is in the middle */
      { lb[0] = lb[2];
	lb[2] = val;
      /*printf("%sDwap %d %d\n", sps(depth+1), lb-testtbl, lb-testtbl+2);*/
      /*printf("Dwap %d %d\n", lb-testtbl, lb-testtbl+2);*/
	++swaps;
      }
      else				/* lb[1] is at the bottom */
      { lb[0] = lb[1];
	if (val < lb[2])		/* lb[2] is at the top */
	{ lb[1] = val;
	/*printf("%sDwap %d %d\n", sps(depth+1), lb-testtbl, lb-testtbl+1);*/
	/*printf("Dwap %d %d\n", lb-testtbl, lb-testtbl+1);*/
	  ++swaps;
	}
	else
	{ lb[1] = lb[2];
	  lb[2] = val;
	  swaps += 2;
	/*printf("%s3wap %d %d\n", sps(depth+1), lb-testtbl, lb-testtbl+2);*/
	/*printf("3wap %d %d\n", lb-testtbl, lb-testtbl+2);*/
	}
      }
    else 
    { if (val < lb[2])		/* lb[0] is on the bottom */
      { val = lb[1];
        if (val > lb[2])
	{ lb[1] = lb[2];
	  lb[2] = val;
	/*printf("%sDwap %d %d\n", sps(depth+1), lb-testtbl+1, lb-testtbl+2);*/
	/*printf("Dwap %d %d\n", lb-testtbl+1, lb-testtbl+2);*/
	  ++swaps;
	}
      }
      else				/* lb[1] is at the top */
      { lb[0] = lb[2];
	lb[2] = lb[1];
	lb[1] = val;
	swaps += 2;
	/*printf("%s3wap %d %d\n", sps(depth+1), lb-testtbl, lb-testtbl+2);*/
	/*printf("3wap %d %d\n", lb-testtbl, lb-testtbl+2);*/
      }
    }
  }
#endif
  else
  { Vint half = (count + 1) / 2;
    wba[depth] = 2;
    hba[depth] = half;
    cba[depth] = count;
    ++depth;
    count = half;
    continue;
  }
  --depth;
  if (depth >= 0)
  { 
    half = hba[depth];
    count = cba[depth] - half;
    if (--lba[depth] > 0)
    { lb += half;
      
#if BIGCODE == 0
    if (count != 3)
#endif
      mysrt(lb+half, count - half);
    mymerge(lb, half, count);
    --depth;
  }}
/*prttbl();*/
/*printf("%s} mysrt  %d  (%d)\n", sps(depth), lb - testtbl, count);*/
}

#else
				/* Stone sort */
void mysrt(Keytype * lb, Int count)

{ fast Int ix;
  fast Int pow;
  fast Int pow_;
  for (pow = 1; pow < count; pow = pow * 2)
    ;
    
  for ( ; pow >= 1; pow = pow >> 1)
  { for ( pow_ = pow; pow_ >= 1; pow_ = pow_ >> 1)
    { for (ix = count - pow_; --ix >= 0;)
        if (lb[ix] > lb[ix+pow_])
        { Keytype val = lb[ix+pow_];
          lb[ix+pow_] = lb[ix];
          lb[ix] = val;
          /*printf("Swap %d %d\n", ix, ix+pow_);*/
          ++swaps;
        }          
    } 
  }
}

#endif


#if SALONE || 1

Keytype testtbl[] = 
{ 23, 65, 43, 54, 73, 45, 34, 21, 77, 84, 55, 88, 23, 37, 91, 83, 12, 18,
  23, 65, 63, 56, 73, 65, 36, 86, 77, 21, 55, 88, 66, 37, 91, 83, 12, 18,
  23, 65, 63, 56, 73, 65, 36, 46, 77, 21, 55, 48, 66, 37, 91, 43, 12, 14,
  21, 65, 61, 56, 71, 65, 16, 46, 77, 21, 55, 48, 66, 17, 91, 41, 12, 14,
 199,198,197,196,195,194,193,192,191,190,
 189,188,187,186,185,184,183,182,181,180,
 179,178,177,176,175,174,173,172,171,170,
 169,168,167,166,165,164,163,162,161,160,
 159,158,157,156,155,154,153,152,151,150,
 149,148,147,146,145,144,143,142,141,140,
 139,138,137,136,135,134,133,132,131,130,
 129,128,127,126,125,124,123,122,121,120,
 119,118,117,116,115,114,113,112,111,110,
 109,108,107,106,105,104,103,102,101,100,
  99, 98, 97, 96, 95, 94, 93, 92, 91, 90,
  89, 88, 87, 86, 85, 84, 83, 82, 81, 80,
  79, 78, 77, 76, 75, 74, 73, 72, 71, 70,
  69, 68, 67, 66, 65, 64, 63, 62, 61, 60,
  59, 58, 57, 56, 55, 54, 53, 52, 51, 50,
  49, 48, 47, 46, 45, 44, 43, 42, 41, 40,
  39, 38, 37, 36, 35, 34, 33, 32, 31, 30,
  29, 28, 27, 26, 25, 24, 23, 22, 21, 20,
  19, 18, 17, 16, 15, 14, 13, 12, 11, 10,
   9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
};

void prttbl()

{ Keytype * lb;

  for (lb = &testtbl[-1]; ++lb <= &testtbl[sizeof(testtbl)/sizeof(Int)-1]; )
      printf(" %d", lb[0]);
    printf("\n");  
}

void main(int argc, char * argv[])

{ Int ix;
  mysrt(&testtbl[0], sizeof(testtbl)/sizeof(Int));

  printf("Elems %d Swaps %d\n", sizeof(testtbl)/sizeof(Int), swaps);
  for (ix = -1; ++ix < sizeof(testtbl)/sizeof(Int); )
    printf("%d\n", testtbl[ix]);

  chk_tbl(0, sizeof(testtbl)/sizeof(Int), "The Res");
  exit(0);
}


#endif
