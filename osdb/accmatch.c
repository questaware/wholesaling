#if 0
282764 Consider
32145 Consider
43745 Consider
36511 Consider
1848 Consider
11576 Consider
30922 Consider
42633 Consider
24079 Consider
-89314 Consider

#endif


#include  <stdio.h>
#include  <signal.h>
#include   "version.h"
#include   "../h/defs.h"
#if SALONE == 0
#include "../h/errs.h"
#include "domains.h"
#include "schema.h"
#include "../db/b_tree.h"
#include "generic.h"
#endif

#define private

#define SPPRIME 13

#define USECONGS 1
#define CONVIX 1

#if SALONE 
Char dbg_file_name[] = "auditlog";
#endif

#define EMPTY 0
			/* MAXCHG restricts the number of changes match/not */
#define MAXCHG 12
			/* MAXDPTH restricts the number of entries to consider*/
#define MAXDPTH 28

Cash aarr[MAXDPTH+1];
Cash amin[MAXDPTH+1][2];		/* sum of all negative values */
Cash amax[MAXDPTH+1][2];		/* sum of all positive values */

/*Shortset o_l_td_m[MAXDPTH+1];		** 1 << (tot_dpth-ix) */


static Int   tot_dpth;				/* <= MAXDPTH */
static Int   tot_dpth_m_1;			/* tot_dpth - 1 */


static const Short primes [] = 
                    { 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47,
			53,59,61,67,71, 73, MAXSINT};
#define P1P2   3
#define P1P2B2 4
static Short pr_tbl[5];		/* 2, p1, p2, p1*p2, p1*p2*2 */

#define COPTSZ     16   /* size of congruency optimisation zone */
#define MAXCONGSZ 750
static Short * cong_tbl; /* of size 2 * MAXCONGSZ */
	/* Values are looked up in this table using pr_tbl[1] and pr_tbl[2]   */
	/* The indeces are v % pr_tbl[1] and v % pr_tbl[2]		      */
	/* The entry is a bit map; bit n shows whether or not any combination */
	/* of the deepest n entries in aarr can form v 			      */
#define cong_tbl_sz pr_tbl[P1P2B2]

static Shortset cong_ct[MAXDPTH+2];
static Set      cong_set;     /* bit vector of cong_ct */

#if SALONE
static Int mct = 0;
static Int mmct = 0;
#endif

/* with MAXCHG 8, MAXDPTH 24, BORDERW 9
    the match algorithm takes a maximum of 1.1 seconds, mct == 1110
*/


private void closedown(n)

{ fprintf(stderr, "Signal %d\n", n);
  exit(0);
}


#define one_l_td_m(ix) (1 << (tot_dpth - ix))


private void mk_lims()

{ Int min_odd = 0x7fffffff; /* maxint */
  Int max_odd = 0x80000000; /* minint */
  Int tot_odd = 0;
  Int noo_odds = 0;

       Int currmax = 0;
       Int currmin = 0;
  fast Int ix = tot_dpth_m_1 + 1;
  fast Int * p;
  amax[ix][0] = -1;
  amax[ix][1] = -1;
  amin[ix][0] = 0x7fffffff;
  amin[ix][1] = 0x7fffffff;

  for (; --ix >= 0;)
  { Int v = aarr[ix];
    if (v > 0)
      currmax += v;
    else
      currmin += v;

    if (v & 1)
    { tot_odd += v;
      ++noo_odds;
      if (v > max_odd)
        max_odd = v;
      if (v < min_odd)
        min_odd = v;
    }

    p = &amin[ix][0];
    p[0] = currmin;
    p[1] = currmin;

    p = &amax[ix][0];
    p[0] = currmax;
    p[1] = currmax;
					/* no positives and has negatives */
    if      (in_range(max_odd, 0x80000001, 0))
      amin[ix][(noo_odds & 1) ^ 1] -= max_odd;
					/* no negs and has positives */
    else if (in_range(min_odd, 0, 0x7ffffffe))
      amax[ix][(noo_odds & 1) ^ 1] -= min_odd;
    else if (noo_odds == 2)		/* one of each */
    { if      (tot_odd < 0)		/* and of different sign, see above */
      { amin[ix][0] += max_odd;		/* min has both */
        amax[ix][0] -= max_odd;		/* max has none */
      }
      else
      { amin[ix][0] -= min_odd;		/* min has none */
        amax[ix][0] += min_odd;		/* max has both */
      }
    }
    else if (noo_odds == 3)		/* one, and two */
    { Int mid = tot_odd - min_odd - max_odd;
      if (tot_odd < 0) 
      { if      (mid > 0)
          amax[ix][0] -= mid;		/* max cant have both */
        else if (-mid < max_odd)
          amin[ix][1] -= mid;		/* min drops mid */
        else
          amin[ix][1] += max_odd;	/* min gains max_odd to keep mid */
      }
      else
      { if      (mid < 0)
          amin[ix][0] -= mid;		/* min cant have both */
        else if (mid < -min_odd)
          amax[ix][1] -= mid;		/* max drops mid */
        else
          amax[ix][1] += min_odd;	/* max gains min_odd to keep mid */
      }
    }
					/* convert max to len */
    p[0] -= amin[ix][0];
    p[1] -= amin[ix][1];
  }
/*for (ix = -1; ++ix <= MAXDPTH; )
  { printf("%d   %d %d %d %d\n", aarr[ix],
  			    amin[ix][0], amin[ix][1], 
  			    amax[ix][0], amax[ix][1]);
  }*/
}

#define out_lims(x, ix) \
  ((unsigned)(x - amin[ix][x & 1]) > amax[ix][x & 1])

static int do_once = 0;

#if USECONGS

#if 0
 
private void find_primes()

{ Short lim = tot_dpth_m_1;
  if (lim > 16)
     lim = 16;

{ Int iter;
  pr_tbl[1] = 1;

  for (iter = 0; ++iter <= 2; )
  {      Int bestsave = 0;
	 Short bestprime = MAXSINT;
	 Short *prref;
    for (prref = &primes[0]; ; ++prref)
    { Short prime = *prref;
      if (prime == pr_tbl[1])
	continue;
      if (prime * pr_tbl[1] > MAXCONGSZ)
	break;
    {      Short lowest = MAXSINT;
	   Short worst = 0;
      fast Int up;

      for (up = -1; ++up < lim;)
      { fast Short add = aarr[tot_dpth_m_1 - up] % prime;
	if (add < 0)
	  add += prime;
	if (lowest > add)
	  lowest = add;

	worst += add;
	if (worst - lowest > prime)
	  break;
      { Int savebyprime = (2 * (prime - worst) + lowest) << up;
	if (savebyprime > bestsave * prime)
	{ bestsave = savebyprime / prime;
	  bestprime = prime;
#if SALONE && 0
          printf("At %d, %d Get %d\n", up, iter, bestprime);
#else
        /*i_log(0, "At %d, %d Get %d\n", up, iter, bestprime);*/
#endif
	}
      }}
    }}
    pr_tbl[iter] = bestprime;
  }

  if (pr_tbl[1] == MAXSINT or pr_tbl[2] == MAXSINT)
  { pr_tbl[1] = 7;
    pr_tbl[2] = 73;
  }

#if SALONE
    if (++do_once <= 2)
      ;
    printf("Chopping with 0 at %d %d\n", pr_tbl[1], pr_tbl[2]);
#endif
  
  pr_tbl[P1P2]   = pr_tbl[1] * pr_tbl[2];
  pr_tbl[P1P2B2] = pr_tbl[P1P2] * 2;
}}

/*static Int congo_ct;*/
/*static Int cong_tbl_occ1;	** size of used part */
/*static Int cong_tbl_occ2;     */

private Cc find_congs()

{ fast Short * p;
  pr_tbl[0] = 2;
  find_primes();
  
  if (cong_tbl_sz > 2 * MAXCONGSZ)
    i_log(cong_tbl_sz, "CTS NOT OK");

  for (p = &cong_tbl[cong_tbl_sz]; (p -= 2) >= cong_tbl; )
    *(Int*)p = 0;
  for (p = (Short*)&cong_ct[MAXDPTH+2]; --p >= (Short*)cong_ct; )
    *p = 0;

  cong_tbl[0] = -1;

{ fast Short rdcur = 1;
       Short congo_ct = 1;
       Short ctol = 1;
       Short wrcur = -1;
       Short up;
  for (up = -1; ++up < 15 and up <= tot_dpth_m_1; rdcur = rdcur << 1)
  { fast Int c1;
    Int v = aarr[tot_dpth_m_1 - up];
  #if CONVIX
    Short cong = (v % pr_tbl[P1P2]);
    if (cong < 0)
      cong += pr_tbl[P1P2];
  
    wrcur *= 2;
    p = &cong_tbl[ctol * 2];

    for (c1 = (ctol/*pr_tbl[P1P2]*/)*2; --c1 >= 0;)
    { p -= 1;
      if (*p & rdcur)
      { Short c = cong + (c1 >> 1);
	if (c >= pr_tbl[P1P2])
	  c -= pr_tbl[P1P2];
      { Short * pp = &cong_tbl[c * 2 + ((v+c1) & 1)];
	*pp |= wrcur;
	if ((*pp & rdcur) == 0)
	{ if (c >= ctol)
	    ctol = c + 1;
	  ++congo_ct;
	}
      }}
    }
  #else
  X fast Int c2;
  X Short cong1 = v % pr_tbl[1];
  X Short cong2 = v % pr_tbl[2];
  X if (cong1 < 0)
  X   cong1 += pr_tbl[1];
  X if (cong2 < 0)
  X   cong2 += pr_tbl[2];
  X
  X  for (c2 = /*pr_tbl[2]*/ cong_tbl_occ2; --c2 >= 0;)
  X { c1 = /*pr_tbl[1]*/ cong_tbl_occ1;
  X { Short ix = (c2 * pr_tbl[1] + c1) * 2;
  X   p = &cong_tbl[ix];
  /*  if (ix >= 2 * MAXCONGSZ)
	i_log(ix, "Bound Check");
  */
  X   for (c1 *= 2; --c1 >= 0;)
  X     if (*--p & rdcur)
  X     { Short c1_ =  cong1 +(c1 >> 1);
  X       Short c2_ =  cong2 + c2;
  X       if (c1_ >= pr_tbl[1])
  X         c1_ -= pr_tbl[1];
  X       if (c2_ >= pr_tbl[2])
  X         c2_ -= pr_tbl[2];
  X       if (c2_ >= cong_tbl_occ2)
  X         cong_tbl_occ2 = c2_ + 1;
  X       if (c1_ >= ctol)
  X         ctol = c1_ + 1;
  X     { Short * pp = &cong_tbl[(c2_ * pr_tbl[1] + c1_)*2 + ((v+c1) & 1)];
  X       if ((*pp & rdcur) == 0)
  X         ++congo_ct;
  X       *p |= wrcur;
  X       *pp |= wrcur;
  X     }}
  X }}
  #endif
    /*cong_tbl_occ1 = ctol; */
    cong_ct[tot_dpth_m_1 - up] = 1;
  }
  return OK;
  cong_ct[tot_dpth_m_1] = 0;		/* too expensive for rem */
}}

#else

/* new */
/*static Int congo_ct;*/
/*static Int cong_tbl_occ1;	** size of used part */
/*static Int cong_tbl_occ2;     */

static const Int cong_weights[16] =

{ 
0x1,
0x2,
0x4,
0x8,
0x18, 
0x2c,
0x50,
0xb0,
0x1e0, 
0x400,
0xb00,
0x1f00,
0x4000,
0xa000,
0x10000,/* this slot not used */
};

int gddbb(int n)
{}

private Int find_one_congs()

{ fast Short * p;
  pr_tbl[P1P2]   = pr_tbl[1]*pr_tbl[2];
  pr_tbl[P1P2B2] = pr_tbl[P1P2] * 2;

  /*if (cong_tbl_sz > 2 * MAXCONGSZ)
      i_log(cong_tbl_sz, "CTS NOT OK");*/

  /*for (p = (Short*)&cong_ct[MAXDPTH+2]; --p >= (Short*)cong_ct; )
      *p = 0;*/
  for (p = &cong_tbl[cong_tbl_sz]; (p -= 2) >= cong_tbl; )
    *(Int*)p = 0;

  cong_tbl[0] = -1;
  /*cong_tbl_occ1 = 1;*/
  /*cong_tbl_occ2 = 1;*/
#if COPTSZ > 16
  COPTSZ too big
#endif
{ fast Short rdcur = 1;
       Short wrcur = -1;
  fast Short ctolb2 = 2;
  fast Int c1;
       Int bestsave = 0;
       Short congo_ct = 1;
       Short up;
#define v c1
  for (up = -1; ++up < COPTSZ and up <= tot_dpth_m_1; )
  { v = aarr[tot_dpth_m_1 - up];
  {
  #if CONVIX
    Short congb2 = (v % pr_tbl[P1P2]) * 2 + ctolb2 + (v & 1);
    if (congb2 >= pr_tbl[P1P2B2])
      congb2 -= pr_tbl[P1P2B2];

    wrcur *= 2;
    c1 = ctolb2 >> 1;
    p = &cong_tbl[c1 * 2];

    for (; (c1 -= 1) >= 0;)
    { congb2 -= 2;
      if (congb2 < 0)
        congb2 += pr_tbl[P1P2B2];
      p -= 2;
      if (p[1] & rdcur)
      { Short * pp = &cong_tbl[congb2 ^ 1];
	if ((*pp & rdcur) == 0)
	{ *pp |= wrcur;
	  ++congo_ct;
	  if (congb2 >= ctolb2)
	    ctolb2 = (congb2 + 2);
	}
      }
      if (*p & rdcur)
      { Short * pp = &cong_tbl[congb2];
	if ((*pp & rdcur) == 0)
	{ *pp |= wrcur;
	  ++congo_ct;
	  if (congb2 >= ctolb2)
	    ctolb2 = (congb2 + 2);
	}
      }
    }
    
    rdcur = rdcur << 1;
    ctolb2 &= -2;		/* remove (v & 1) */
  #else
  X fast Int c2;
  X Short cong1 = v % pr_tbl[1];
  X Short cong2 = v % pr_tbl[2];
  X if (cong1 < 0)
  X   cong1 += pr_tbl[1];
  X if (cong2 < 0)
  X   cong2 += pr_tbl[2];
  X
  X  for (c2 = /*pr_tbl[2]*/ cong_tbl_occ2; --c2 >= 0;)
  X { c1 = /*pr_tbl[1]*/ ctol;
  X { Short ix = (c2 * pr_tbl[1] + c1) * 2;
  X   p = &cong_tbl[ix];
  /*  if (ix >= 2 * MAXCONGSZ)
	i_log(ix, "Bound Check");
  */
  X   for (c1 *= 2; --c1 >= 0;)
  X     if (*--p & rdcur)
  X     { Short c1_ =  cong1 +(c1 >> 1);
  X       Short c2_ =  cong2 + c2;
  X       if (c1_ >= pr_tbl[1])
  X         c1_ -= pr_tbl[1];
  X       if (c2_ >= pr_tbl[2])
  X         c2_ -= pr_tbl[2];
  X       if (c2_ >= cong_tbl_occ2)
  X         cong_tbl_occ2 = c2_ + 1;
  X       if (c1_ >= ctol)
  X         ctol = c1_ + 1;
  X     { Short * pp = &cong_tbl[(c2_ * pr_tbl[1] + c1_)*2 + ((v+c1) & 1)];
  X       if ((*pp & rdcur) == 0)
  X         ++congo_ct;
  X       *p |= wrcur;
  X       *pp |= wrcur;
  X     }}
  X }}
  #endif
    /*cong_tbl_occ1 = ctol; */
#if SALONE && 0
    if (do_once < 2)
      printf("cgo %d\n", congo_ct);
#endif
    /*cong_ct[tot_dpth_m_1 - up] = congo_ct;*/
    v = (long)(pr_tbl[P1P2B2] - congo_ct) * cong_weights[up];
    if (v == 0)
      break;
    if (v > bestsave)
    { bestsave = v;
#if SALONE && 0
      if (do_once < 2)
        printf("best %d up %d\n", save, up);
#endif
    }
  }}
  cong_set = (rdcur - 1) & 0xffff;
  return bestsave;
#undef v
}}


#define IGN_SZ 1

void set_cong_ct(Set bv)

{ memset(&cong_ct[0], 0, sizeof(cong_ct));
  bv = bv >> IGN_SZ;		/* ignore extremities */
{ Int up;
  for (up = tot_dpth_m_1 + 1 - IGN_SZ; --up >= 0 and (bv & 1);)
  {  bv = bv >> 1;
     cong_ct[up] = 1;
  }
  if (bv != 0)
    i_log(0, "tdpth Bd");
}}


private Cc find_congs()

{      Int bestsave = 0;
       Int prAix;
  fast Int prBix;
       Int prAbix = 0;
       Int prBbix = 1;
  fast Int lim;

  pr_tbl[0] = 2;

  for (prAix = -1; ++prAix < (sizeof(primes) / sizeof(Short)) - 2; )
  { pr_tbl[1] = primes[prAix];
    /* Int */ lim = MAXCONGSZ / pr_tbl[1];
    for (prBix = prAix; ; )
    { pr_tbl[2] = primes[++prBix];
      if (pr_tbl[2] >= lim)
        break;
    { Int save = find_one_congs();
#if SALONE && 0
       if (do_once < 2)  ;
         printf("%x P %d %d saves %d\n", cong_set, pr_tbl[1], pr_tbl[2], save);
#endif
      if (save > bestsave)
      { bestsave = save;
        prAbix = prAix;
        prBbix = prBix;
      }
    }}
  }
#if 1
  pr_tbl[1] = primes[prAbix];
  pr_tbl[2] = primes[prBbix];
#else
  pr_tbl[1] = 7;
  pr_tbl[2] = 73;
#endif
  (void)find_one_congs();
  set_cong_ct(cong_set);    

#if SALONE
  if (++do_once <= 2)
    printf("Chp w %d d %d @ %d, %d\n", bestsave, up+1, pr_tbl[1], pr_tbl[2]);
#else
  /*i_log(0, "Chp w %d d %d @ %d, %d\n", bestsave, up+1, pr_tbl[1], pr_tbl[2]);*/
#endif
				  /* will be too expensive to do rem  at end */
  return OK;
#undef up
}

#endif

#endif

Set result;

private void chose_result(Set set, Int ix)

{ if (set < result)
  { result = set;
#if SALONE
    printf("At Nd %d Chose %x ", mct, set);
#else
  /*i_log(0, "At Nd  Chose %x ", set);*/
#endif
    if (tot_dpth_m_1 >= ix and ix > 0)
    { tot_dpth_m_1 = ix -1; 
      mk_lims();
      (void)find_one_congs();
      set_cong_ct(cong_set);    
    }
  }
}


private void mtch_amount(ix, amt, set_)
  fast	 Int       ix;
#define ix1 ix		/* ix incremented */
	 Int	   amt;		/* never 0 */
	 Int 	   set_;
{      Short c1 = -1;
       Short congset;
  fast Int x = amt;
  fast Int set = set_;
#if SALONE
  ++mct;
#endif
  for (; ; ++ix)
  { /* printf("Ix %ld Val %ld\n", ix, x); */
    if (out_lims(x, ix))
    { /*printf("Maxing %ld %ld %ld %ld\n", ix, x, amin[ix][x & 1],
						  amax[ix][x & 1]); */
      return;
    }
  #if 1
  #if USECONGS
    if (c1 < 0 and /*cong_ct[ix] + */ cong_ct[ix+1] > 0)
    					/* +1: its used below */
  #if CONVIX
    { c1 = x % pr_tbl[P1P2];
      if (c1 < 0)
	c1 += pr_tbl[P1P2];
      congset = cong_tbl[c1*2 + (x & 1)];
    }
  #else
  X { c1 = x % pr_tbl[1];
  X   if (c1 < 0)
  X	c += pr_tbl[1];
  X { Short c2 = x % pr_tbl[2];
  X   if (c2 < 0)
  X	c += pr_tbl[2];
  X   congset = cong_tbl[(c2*pr_tbl[1]+c1)*2 + (x & 1)];
  X }}
  #endif
    if (c1 >= 0 and (congset & one_l_td_m(ix)) == 0)
	break;
  #endif
  { Int oldx = x;
    x -= aarr[ix];				  /* First deducting */
    if (x == 0)
    { chose_result(set | (1 << ix), ix);
      return;
    }
    /* printf("Ix %ld - %ld = %ld\n", ix, v, x);*/
    ++ix;
    if (ix1 >= tot_dpth_m_1 and x != aarr[ix1])
      break;
    if (not out_lims(x, ix1))
    while (true)					/* once only */
    {
  #if USECONGS
      if (cong_ct[ix1] > 0)
  #if CONVIX
      { Short c2 = x % pr_tbl[P1P2];
				if (c2 < 0)
				  c2 += pr_tbl[P1P2];
				if ((cong_tbl[c2*2 + (x & 1)] & one_l_td_m(ix)) == 0)
				  break;
      }
  #else
  X   { Short c2 = x % pr_tbl[1];
  X			if (c2 < 0)
  X	  		c2 += pr_tbl[1];
  X   { Short c2 = x % pr_tbl[2];
  X			if (c2 < 0)
  X			  c2 += pr_tbl[2];
  X			if ((cong_tbl[(c2 * pr_tbl[1]+c2)*2 + (x & 1)] & one_l_td_m(ix)) == 0)
  X	  		break;
  X   }}
  #endif
  #endif
      if (x == aarr[ix1])
      { chose_result(set | (3 << (ix1-1)), ix1);
        return;
      }
	/*printf("Pix %ld - %ld\n", ix, aarr[ix]);*/
      mtch_amount(ix1 + 1, x - aarr[ix1], set | (3 << (ix1-1)));
	/*printf("Pix %ld (+) %ld\n", ix, aarr[ix]);*/
      mtch_amount(ix1 + 1, x, set | (1 << (ix1-1)));

      break;
    }				  /* c1, congset now valid again */
       				  /* then not deducting v */
    x = oldx;			  
    /*printf("Ix %ld + %ld\n", ix-1, v);*/
    if (out_lims(x, ix1))
      break;
    if (x == aarr[ix1])
    { chose_result(set | (1 << ix1), ix1);
      break;
    }
  #if USECONGS
    if (c1 >= 0 and (congset & one_l_td_m(ix1)) == 0)
      break;
  #endif
      /*printf("Pix %ld - %ld\n", ix-1, aarr[ix-1]);*/
   
    mtch_amount(ix1 + 1, x - aarr[ix1], set | (1 << ix1));
  }}
#else
X			      /* the above is an optimisation of the below */
X { set = mtch_amount(ix, x);
X   if (set != EMPTY)
X	return set | (1 << ix);
X   x += v;
X }
X return EMPTY;
#endif
}

private Set mat_amount(amount, sz)
	 Int  amount;
	 Int  sz;
{      
  fast Cash * iip = &aarr[0]-1;
  fast Set cursor;
  Short congtbl[2 * MAXCONGSZ];
  cong_tbl = & congtbl[0];

#if SALONE
  mct = 0;
#endif
  tot_dpth = sz;
  tot_dpth_m_1 = tot_dpth - 1;
  if (tot_dpth_m_1 < 0)
    return EMPTY;

  aarr[tot_dpth] = 0x7fffffff;	/* for faster boundary checks */
  aarr[tot_dpth+1] = 0x7fffffff;/* for faster boundary checks */

  mk_lims();
  result = (Set)-1;
{
#if USECONGS
  Cc cc = find_congs();
#else
  Cc cc = OK;
#endif
  Int i;
#if SALONE && 0
  Int adj;
  for (adj = minv(tot_dpth, 16); --adj > 0; )
    printf("Count at %d is %d Density %f\n", tot_dpth - adj,
                  cong_ct[tot_dpth - adj],
                  cong_ct[tot_dpth - adj] / (double)cong_tbl_sz);
#endif

  cong_ct[0] = 0;

  if (tot_dpth > 0)
    mtch_amount(0, amount, 0);

  if (result == (Set)-1)
    result = 0;

#if SALONE == 0
  /*if (mct > 400000)
      i_log(66, "TSlen %d res %x", mct, result);*/
#else
    mmct += mct;
    printf("Nd %d,%d res %x\n", mmct, mct, result);
    fprintf(stderr, "Nd %d res %x\n", mct, result);
#endif
  return result;
}}

#if SALONE == 0

    /* > 0 => all up to offset - 1 */  

public Quot match_amount(set_ref, acc, amount)
         Set      *set_ref;
	 Accdescr  acc;
	 Cash	   amount;
{      Int szm1 = acc->curr_last;
       Int ix = 0;
       Int dpth = 0;
       Int matix = -2;
       Int after_bal = 0;           /* paids after matching balance */
       Int unpaid = 0;              /* unpaids */
       Int unpaid_at_b = 0;       /* unpaids before the matching balance */
       Cash bal = 0;

  for (; szm1 >= 0; --szm1)
  { Accitem iip = &acc->c[ix];
    Cash v = iip->val;
    if (not (iip->props & (K_BFWD + K_SALE + K_GIN)))
      v = -v;
    if (iip->props & K_ANTI)
      v = -v;
    if (iip->props & K_PAID)
      ++after_bal;
    if (not (iip->props & (K_PAID + K_CASH + K_IP + K_PD)))
    { if (dpth < MAXDPTH)
      { aarr[dpth++] = v;
      /*i_log(v, "Consider");*/
      }
      ++unpaid;
    }
    if (not (iip->props & (K_CASH + K_IP + K_PD)))
      bal += v;
    if (iip->balance != bal)
      i_log(iip->id, "Bal %d not bal %d %d", acc->id, iip->balance, bal);
      
  /*i_log(iip->id, "MAB %d", iip->balance);*/
    if (iip->balance == amount)
    { matix = ix;
      after_bal = (iip->props & K_PAID) ? 1 : 0;
      unpaid_at_b = unpaid;
    /*i_log(ix, "MB %d %d", after_bal, unpaid_at_b);*/
    }
    ix += 1;
  }				/* up to 10 entries after */
{ Set res = matix >= 0 and after_bal < unpaid_at_b ? EMPTY 
                                                   : mat_amount(amount, dpth);
#if 0
  i_log(0, "Gives %x\n", res);
#endif
  if (res != EMPTY or matix >= 0)
  { commit_match(0, acc, res, matix+1, amount);
    *set_ref = res;
   /* i_log(0, "Res %x", *set_ref); */
    return matix >= 0 ? matix+1 : MATCHSET;
  }

  return NOMATCH;  
}}

#else 

#define PRIME (2 * 3 * 4 * 5 * 6 * 7 - 1)


public void ps_dump()
{ 
}


private Int gen_rand()

{ static Int seed = 4582849;
  Int r = seed * PRIME;
  seed = (r << 4) ^ r;
  return seed;
}

static Int begin;

private void prttime()

{
#define fm "T %d s\n", time(0) - begin
  printf(fm);
  fprintf(stderr, fm);
}


Int a_amount = 0;

main (argc, argv)
	int argc;
	char * argv[];
{ Int dix;
  Int iter;

  FILE * ip = fopen("pjsin", "r");
  i_log(0, "Standalone");

  if (argc > 1)
    a_amount = atol(argv[1]);

  for (dix = 0; ip != null and dix < MAXDPTH; ++dix)
  { if (fscanf(ip, "%d", &aarr[dix]) <= 0)
      break;
    printf("II %d II            %d\n", aarr[dix], aarr[dix] % SPPRIME);
  }

  if (dix == 0)
    exit(0);

  printf("Noo vals %ld, USECONGS %d CONVIX %d\n", dix, USECONGS, CONVIX);

  aarr[dix] = 0;
  signal(SIGINT, closedown);
  signal(SIGQUIT, closedown);

  begin = time(0);
  while (begin == time(0))
    ;
  ++begin;

  for (iter = 0; iter < 20; ++iter)
  { Set set = gen_rand() & ((1 << (dix - 1)) - 1);
    Set cursor = 1;
    Int set_ = set;
    Int amount = 0;
    Int ix = 0;
/*
    sleep(1);
*/
    while (set_ != 0)
    { if (set_ & cursor)
				amount += aarr[ix];

      set_ &= ~cursor;
      ++ix;
      cursor *= 2;
    }

    if ((set & 0x30) == 0)
      amount += 17;
  
    if (a_amount != 0)
      amount = a_amount;  
    
    printf("%d Set %x Amt %d%s\n", iter,
    		set, amount, (set & 0x30) ? "" : "        ODDAMT");

  { Int start = time(0); 
    Set res_ = mat_amount(amount, dix);
    Set res = res_;
    Int tot = 0;
    Int j;
    printf("%x Gives %x in %d secs\n", set, res, time(0) - start);
    if (res == 0)
    { printf("NOT FOUND %x\n\n", set);
    }
    else
    { for (j = 0; j < 30 and aarr[j] != 0; ++j)
      { if (res & 1)
	{
#if 0
	  printf("%d\n", aarr[j]);
#endif
	  tot += aarr[j];
	}
	res = res >> 1;
      }
      if (tot == amount)
        printf("\n");
      else
      { printf("WRONG Total is %d\n\n", tot);
      }
      prttime();
      fflush(stdout);
    }
    if (a_amount != 0)
      break;
  }}
}

#endif
