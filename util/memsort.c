
/******************************************************************/
/* Since sorting is always time critical, there is little point   */
/* in parameterising for sort key size and offset as users will   */
/* always want to optimise their own situation by copying and     */
/* changing code.  These customisations are reasonably easy to do */
/* Similarly, there is no point catering for sort keys aligned    */
/* incorrectly for machine architectures.  This code is not       */
/* machine independant in this respect.                           */
/******************************************************************/
#include   <stdio.h>
#include   <string.h>
#include   "../h/base.h"

#ifndef SORTTYPE
#define SORTTYPE 0
#endif

#define BIGCODE  1
#define SLTBL    0
#define VERBOSE  0

#define Keytype Int
#define Vint Int


#define MININT 0x80000000
#define MAXINT 0x7fffffff


static Int memsort_stride;


#define  CMP_OP >
#define OCMP_OP <=

#if SALONE || 1

 extern Keytype * testtbl;
 void prttbl();
#else

 void prttbl() {}
#endif

void mysrt(Byte * lb, Int count);


char blank[] = "                                        ";

#define sps(n) &blank[40 - (n)]

#define MAXUNS 4

static Int depth = 0;
static Int swaps = 0;

static Int step_in_bytes;			/* sort step */
static Int step2_in_bytes;			/* sort step */
/*static Int sfield;			** field offset */
static Byte sbuff[1024];

#if VERBOSE == 0 && 0
 #define inc_swap(n)
#else
 #define inc_swap(n) swaps += (n)
#endif


#define step  step_in_bytes
#define step2 step2_in_bytes
#define COPY(arrt, t, arrs, s) \
    memcpy(&arrt[(t)*step],  &arrs[(s)*step], step)
#define SWAP(arr, a, b) \
  { memcpy(&sbuff[0],       &arr[(a)*step], step); \
    memcpy(&arr[(a)*step],  &arr[(b)*step], step); \
    memcpy(&arr[(b)*step],  &sbuff[0], step);    \
  }
#define SWAPABS(arr, a, b) \
  { memcpy(&sbuff[0],  &arr[(a)], step); \
    memcpy(&arr[(a)],  &arr[(b)], step); \
    memcpy(&arr[(b)],  &sbuff[0], step);    \
  }
#define SWAPABSM(arr, a, b, sz) \
  { memcpy(&sbuff[0],  &arr[(a)], sz); \
    memcpy(&arr[(a)],  &arr[(b)], sz); \
    memcpy(&arr[(b)],  &sbuff[0], sz);    \
  }

#if SALONE == 0

#define dsp_tbl(stt, mid, ct)

#else

void dsp_tbl(Int stt, Int mid, Int ct)

{ Keytype * tbl = &testtbl[stt];
  Int ix;
  printf("{BM %d mid %d ct %d\n", stt, mid, ct);
  for (ix = -1; ++ix < ct; )
    printf(" %d", tbl[ix]);
  printf("\n");
}


static Cc chk_tbl(Int stt, Int ct, Char * msg)

{ Keytype * tbl = &testtbl[stt];
  Int ix;
  printf("BF %d %d\n", stt, ct);
  for (ix = ct; --ix > 0; )
    if (tbl[ix-1] > tbl[ix])
      break;
 
  if (ix > 0)
  { printf("%s\n", msg);
    printf("%d %d %d XX", stt, ct, ix);
    prttbl();
    return -1;
  }
  
  return OK;
}

#endif


#if SORTTYPE <= 1

#if SORTTYPE == 0

#define ttbl ((Byte*)testtbl)

#define SLOTQSZ (16) /* power of 2 */

static void merge(left, leftnbystep, totnbystep)
   Byte *  left;
   Vint    leftnbystep,  totnbystep;		/* leftn < totn */
{ 
#if SLTBL
     Vint slotqueue[SLOTQSZ];
#endif
  fast Int riteoub = totnbystep - step;
  fast Int luub = leftnbystep - step;

       Int leftoub /*= 0 */;
/*					** this is slower
  while (*(Int*)left[leftoub] < *(Imt*)&left[luub+step] and leftoub <= luub)
    leftoub += step;
  left += leftoub / step;
*/
  leftoub = (luub);
  /*printf("%s{MG left %d  mid %d right %d\n",  
                                  sps(depth),(left-ttbl)/step,luub, riteoub);*/
 while (true) /* (luub > left)		** there is a left wedge */
 {fast Int leftnlb = luub + step;
  fast Int leftnub = luub;
#if SLTBL
       Int slotqueuesz = 0;
       Vint slotq = 0;
  slotqueue[0] = -1;
#endif
  while (true) /* (riteoub > luub) */
  { /*printf("%sLeftoub %d  Leftnlb %d  Leftnub  %d", 
	                       sps(depth), leftoub, leftnlb,  leftnub);*/
    /*printf(" Riteoub %d\n",   riteoub);*/
  { Int maxp = leftnub;
    if (maxp < leftnlb)			/* the middle wedge has run out */
    { 
#if SLTBL
     maxp = slotqueue[slotq];
     if (maxp < 0)
#endif
			/* the trash at luub is always highest */
     { if (leftnlb < luub   and leftnub - leftoub > 0) /* => merge:p2 > 0 */
       { if (luub - (leftoub + step) <= 0)
           printf("Bad imp\n");
#if SLTBL
	if (slotqueuesz > 1)
	{ /*dsp_tbl((left-ttbl+leftoub)/step+1, (leftnlb-leftoub)/step-1,(luub-leftoub)/step);*/
	  mysrt(&left[leftoub+step], (luub - (leftoub+step)) / step);
          /*printf("}DS %d %d\n", (left-ttbl+leftoub)/step+1, (luub-leftoub)/step);*/
	 /*chk_tbl((left-ttbl+leftoub)/step+1, (luub-leftoub)/step, "The Sort");*/
        }
	else
#endif
	{ /*if (*(Int*)&left[leftnlb] > *(Int*)&left[leftoub+step])
	      printf("It's not so\n");*/
	  /*dsp_tbl((left-ttbl+leftoub)/step+1, (leftnlb-leftoub)/step-1,(luub-leftoub)/step);*/
	  merge(&left[leftoub+step], leftnub-leftoub, luub-(leftoub+step));
         /*printf("}DM %d %d\n", (left-ttbl+leftoub)/step+1, (luub-leftoub)/step);*/
	 /*chk_tbl((left-ttbl+leftoub)/step+1, (luub-leftoub)/step, "The Merge");*/
	}
       }
#if SLTBL
       slotqueuesz = 0;
       slotq = 0;
       slotqueue[0] = -1;
#endif
       leftnlb = leftoub + step;
       leftnub = luub;
       maxp = leftnub;
       if (maxp < leftnlb)
         maxp = leftoub;
     }
    }

    if (leftoub >= 0 and *(Int*)&left[leftoub] > *(Int*)&left[maxp])
      maxp = leftoub;

  { Keytype maxval = *(Int*)&left[maxp];
/*{ Int p;
    for (p = luub+step; (p -= step) >= 0; )
      if (*(Int*)&left[p] > maxval)
      { printf("Nothi %d %d %d\n", (left-testtbl)/step,maxval,*(Int*)&left[p]);
	break;
      }
  }*/
    riteoub += step;
    while ((riteoub -= step) > luub and *(Int*)&left[riteoub] >= maxval)
      ;

    if (riteoub <= luub)
      break;
  /*printf("%sSwap  %d  %d\n", sps(depth),
               		(left-ttbl+maxp)/step, (riteoub+left-ttbl)/step);*/
    inc_swap(1);
    SWAPABS(left, maxp, riteoub);
    riteoub -= step;

#if SLTBL
    if      (maxp == slotqueue[slotq])
    { slotq = (slotq + 1) & (SLOTQSZ - 1);
      --slotqueuesz;
    }
    else 
#endif
         if (maxp != leftoub)
      leftnub -= step;
    else
    { leftoub -= step;
    /*printf("leftoub %d leftnlb %d leftnub %d\n",
                 leftoub-testtbl, leftnlb-testtbl, leftnub-testtbl);*/
      if (leftnub == luub and leftnlb == leftoub+step2_in_bytes)
      { /*printf("--leftnlb\n");*/
        leftnlb -= step;
        continue;
      }
    }

#if SLTBL
    if (slotqueuesz >= SLOTQSZ - 1)
    { slotqueuesz = MAXINT;
     /*printf("Overfloed\n");*/
    }
    else
    {/*if (slotqueue[(slotq+slotqueuesz) & (SLOTQSZ-1)] != -1)
        printf("Overwrite not -1\n");*/
      slotqueue[(slotq+slotqueuesz) & (SLOTQSZ-1)] = maxp;
      slotqueue[(slotq+slotqueuesz+1) & (SLOTQSZ-1)] = -1;
      slotqueuesz += 1;
    }
#endif
    /*prttbl();*/
    /*printf("Riteoub %d luub %d\n", riteoub, luub);*/
  }}}				/* while something on right */
  /*printf("DINN\n");*/
  if (leftoub >= riteoub)	/* there is no right wedge */
    break;

  if (leftnub < luub and leftnub - leftoub > 0) /* => merge:p2 > 0 */
  { 
  /*dsp_tbl((left-ttbl+leftoub)/step+1, leftnlb - leftoub, luub-leftoub);*/
#if SLTBL
    if (slotqueuesz > 0)
       mysrt(&left[leftoub+step], (luub - leftoub) / step);
    else
#endif
      merge(&left[leftoub+step], leftnub - leftoub, luub - leftoub);

  /*printf("}DM %d %d\n", leftoub/step+1, luub/step);*/
    
  /*chk_tbl((left-ttbl+leftoub)/step+1, (luub-leftoub)/step, "THe Merge");*/
  }
/*if (riteoub != (luub))
    printf("riteoub not correct\n");*/
  luub = leftoub;
  if (luub < 0)
    break;
 }
 /*printf("}MG Done\n");*/
}

#else

#define ttbl ((Byte*)testtbl)

static void merge(left, leftnbystep, totnbystep)
   Byte *  left;
   Vint    leftnbystep,  totnbystep;		/* leftn < totn */
{ 
  fast Int riteoub = totnbystep - step;
  fast Int luub = leftnbystep - step;

       Int leftoub = luub;
  Keytype maxval = *(Int*)&left[luub];

  /*printf("%s{MG left %d  mid %d right %d\n",  
                                  sps(depth),(left-ttbl)/step,luub, riteoub);*/
  riteoub += step;
  while ((riteoub -= step) > luub and *(Int*)&left[riteoub] >= maxval)
    ;

{ Int hdiff = riteoub - luub;
  if (hdiff <= 0)
    return;

/*if (hdiff == step)
  { ** could do better **
    return;
  }*/
  
  hdiff = hdiff >> 1;

  if (hdiff < step)
  { Int ix = luub + step;
    maxval = *(Int*)&left[ix];
    COPY(sbuff, 0, left, ix);
    while ((ix -= step) >= 0 and *(Int*)&left[ix] > maxval)
    { COPY(left, ix + step, left, ix);
    }
    COPY(left, ix + step, sbuff, 0);
  }
  else if (luub - hdiff + step < 0)
  { printf("Too small\n");
  }
  else
  { if (hdiff <= sizeof(sbuff))
      SWAPABSM(left, luub - hdiff + step, luub + step, hdiff)
    else
    { Int hi = riteoub - hdiff;
      while ((hi -= sizeof(sbuff)) > luub)
        SWAPABSM(left, hi - hdiff, hi, sizeof(sbuff))
      hi = sizeof(sbuff) - (luub + step - hi);
      if (hi > 0)
        SWAPABSM(left, luub - hdiff, luub + step, hi);
    }
    merge(&left[luub - hdiff],  hdiff*2, hdiff * 3);
    if (luub >= hdiff)
      merge( left,                luub - hdiff, riteoub + step);
  }
}}

#endif

void mysrt(Byte * lb, Int count)

{/*printf("%s{ mysrt  %d  (%d)\n", sps(depth), (lb - ttbl) / step, count);*/
  if      (count <= 2)
  { /* if (count == 2) */
    { if (*(Int*)&lb[0] > *(Int*)&lb[step])
      { 
        SWAP(lb, 0, 1);
      /*printf("%sDwap %d %d\n", sps(depth+1), lb-testtbl, lb-testtbl+1);*/
      /*printf("Dwap %d %d\n", lb-testtbl, lb-testtbl+1);*/
        inc_swap(1);
      }
    }
  }
#if BIGCODE
  else if (count == 3)
  { Keytype val = *(Int*)&lb[0];
    if      (val > *(Int*)&lb[step])
      if (*(Int*)&lb[step] > *(Int*)&lb[step2]) /* lb[1] is in the middle */
      { SWAP(lb, 0, 2);
	inc_swap(1);
      /*printf("%sDwap %d %d\n", sps(depth+1), lb-testtbl, lb-testtbl+2);*/
      /*printf("Dwap %d %d\n", lb-testtbl, lb-testtbl+2);*/
      }
      else				/* lb[1] is at the bottom */
      { COPY(sbuff, 0, lb, 0);
        COPY(lb, 0, lb, 1);
	if (val < *(Int*)&lb[step2])	/* lb[2] is at the top */
	{ COPY(lb, 1, sbuff, 0);
	  inc_swap(1);
	/*printf("%sDwap %d %d\n", sps(depth+1), lb-testtbl, lb-testtbl+1);*/
	/*printf("Dwap %d %d\n", lb-testtbl, lb-testtbl+1);*/
	}
	else
	{ COPY(lb, 1, lb, 2);
	  COPY(lb, 2, sbuff, 0);
	  inc_swap(2);
	/*printf("%s3wap %d %d\n", sps(depth+1), lb-testtbl, lb-testtbl+2);*/
	/*printf("3wap %d %d\n", lb-testtbl, lb-testtbl+2);*/
	}
      }
    else 
    { if (val < *(Int*)&lb[step2])	/* lb[0] is on the bottom */
      { if (*(Int*)&lb[step] > *(Int*)&lb[step2])
	{ SWAP(lb, 1, 2);
	  inc_swap(1);
	/*printf("%sDwap %d %d\n", sps(depth+1), lb-testtbl+1, lb-testtbl+2);*/
	/*printf("Dwap %d %d\n", lb-testtbl+1, lb-testtbl+2);*/
	}
      }
      else				/* lb[1] is at the top */
      { COPY(sbuff, 0, lb, 0);
        COPY(lb, 0, lb, 2);
	COPY(lb, 2, lb, 1);
	COPY(lb, 1, sbuff, 0);
        inc_swap(2);
	/*printf("%s3wap %d %d\n", sps(depth+1), lb-testtbl, lb-testtbl+2);*/
	/*printf("3wap %d %d\n", lb-testtbl, lb-testtbl+2);*/
      }
    }
  }
#endif
  else
  { /*++depth;*/
  { Vint half = (count + 1) >> 1;
    mysrt(lb, half);
#if BIGCODE == 0
    if (count != 3)
#endif
      mysrt(&lb[half*step], count - half);
    merge(lb, half*step, count*step);
    /*--depth;*/
  }}

/*prttbl();*/
/*printf("%s} mysrt  %d  (%d)\n", sps(depth), (lb - ttbl) / step, count);*/
}

#elif SORTTYPE == 2
				/* Stone sort */
void mysrt(Byte * lb, Int count)

{ fast Int ix;
  fast Int pow;
  fast Int pow_;
  for (pow = 1; pow < count; pow = pow * 2)
    ;
    
  for ( ; pow >= 1; pow = pow >> 1)
  { for ( pow_ = pow; pow_ >= 1; pow_ = pow_ >> 1)
    { Int pow__ = pow_ * step;
      for (ix = (count - pow_) * step; (ix -= step) >= 0;)
        if (*(Int*)&lb[ix] > *(Int*)&lb[ix+pow__])
        { SWAPABS(lb, ix, ix+pow__);
          /*printf("Swap %d %d\n", ix, ix+pow_);*/
          inc_swap(1);
        }          
    } 
  }
}

#else
			/* doesn't work */
void mysrt(Byte * lb, Int count)

{ fast Int ix;
  fast Int pow;
  fast Int pow_;
  for (pow = 1; pow < count; pow = pow * 2)
  { for ( pow_ = pow; pow_ >= 1; pow_ = pow_ >> 1)
   /*for ( pow_ = 1; pow_ <= pow; pow_ *= 2)*/

    { Int pow__ = pow_ * step;
      for (ix = (count - pow_) * step; (ix -= step) >= 0;)
        if (*(Int*)&lb[ix] > *(Int*)&lb[ix+pow__])
        { SWAPABS(lb, ix, ix+pow__);
          /*printf("Swap %d %d\n", ix, ix+pow_);*/
          inc_swap(1);
        }          
    } 
  }
}

#endif

void memsort(Byte * lb, Int stp, Int count)

{ step_in_bytes = stp;
#if SORTTYPE < 1
  step2_in_bytes = step_in_bytes * 2;
#endif
  mysrt(lb, count);
}

#if SALONE

const Keytype testdata_[] = 
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
   9,  8,  7,  6,  5,  4,  3,  2,  1,  0, /**/
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
   9,  8,  7,  6,  5,  4,  3,  2,  1,  0, /**/
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
   9,  8,  7,  6,  5,  4,  3,  2,  1,  0, /**/
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
   9,  8,  7,  6,  5,  4,  3,  2,  1,  0, /**/
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
   9,  8,  7,  6,  5,  4,  3,  2,  1,  0, /**/
};

Keytype * testdata;
Keytype * testtbl;
Int testtblsz;
Int testtblused;

void prttbl()

{ Keytype * lb;

  for (lb = &testtbl[-1]; ++lb <= &testtbl[testtblused-1]; )
      printf(" %d", lb[0]);
    printf("\n");  
}


void prttbllbl()

{ const Keytype * lb;

  for (lb = &testdata[-1]; ++lb <= &testdata[testtblused-1]; )
      printf("%d\n", lb[0]);
    printf("\n");  
}


#define TESTTBLISZ 100
#define INC_SZ      24

void main(int argc, char * argv[])

{ Char * fn = null;
  Bool n_opt = false;
  Int aix = 1;
  Int ix;
  if (aix < argc and argv[aix][0] == '-' and 
                     argv[aix][1] == 'n')
  { n_opt = true;
    ++aix;
  }

  if (aix < argc)
    fn = argv[aix];

  if (fn == null)
  { testdata = testdata_;
    testtblsz = sizeof(testdata_)/sizeof(Int);
    testtblused = testtblsz;
  }
  else
  { testtblsz = TESTTBLISZ;
    testtblused = 0;
    testdata = (Keytype *)calloc(testtblsz, sizeof(Keytype));
  { Char line[133];
    Char * ln;
    FILE * ip = strcmp(fn, "-") == 0 ? stdin : fopen(fn, "r");
    if (ip == null)
    { printf("Cannot open input %s\n", fn);
      exit(1);
    }
    
    while ((ln = fgets(&line[0], sizeof(line)-1, ip)) != null)
    { while (*ln != 0 and isspace(*ln))
        ++ln;
      if (not in_range(*ln, '0', '9'))
        continue;
    { Int v = atol(ln);
      if (testtblused >= testtblsz)
      { Keytype * ntbl = (Keytype*)calloc(testtblsz+INC_SZ+1,sizeof(Keytype));
        if (ntbl == null)
          break;
        memcpy(&ntbl[0], testdata, testtblsz * sizeof(Keytype));
        testtblsz += INC_SZ;
        free(testdata);
        testdata = ntbl;
      }
      testdata[testtblused] = v;
      ++testtblused;
    }}
  }}

  testtbl = (Keytype *)calloc(testtblsz, sizeof(Keytype));

  if (n_opt)
  { prttbllbl();
    exit(0);
  }

  for (ix = 10; --ix >= 0; )
  { memcpy(&testtbl[0], testdata, testtblused * sizeof(Int));
    memsort((Byte*)&testtbl[0], sizeof(Int), testtblused);
  }

  printf("Elems %d Swaps %d\n", testtblused, swaps);
  for (ix = -1; ++ix < testtblused; )
    printf("%d\n", testtbl[ix]);

{ Cc cc = chk_tbl(0, testtblused, "The Res");
  exit(cc);
}}


#endif
