#include   <stdio.h>
#include   <string.h>
#include   "version.h"
#include   "../h/defs.h"
#include   "../h/errs.h"
#include   "page.h"
#include   "recs.h"
#include   "memdef.h"
#include   "cache.h"

static Ldn      last_upn = 0;
static Page_nr  last_page = 0;
				/* The first byte is clean but */
				/* subsequent bytes are dirty  */

private Set is_free_bit(p, ix)
	 unsigned char *  p;
	 Short            ix;
{ if (ix < 8)
    return p[RP_FREEVEC0] & (1 << ix);
    
  return p[RP_FREEVEC - 1 + (ix >> 3)] & (1 << (ix & 7));
}



public Set rec_is_free(p, offs)
	 char             p[16];
	 Short            offs;
{ Vint cl_id = s_sgrp(p);
  if (ddict[cl_id].size_cl == 0)
  { i_log(cl_id, "rif ill class");
    return -1;
  }
{ Vint fix = (offs - ddict[cl_id].cl_soffs) / ddict[cl_id].size_cl;
  if (fix < 8)
    return p[RP_FREEVEC0] & (1 << fix);
    
  return p[RP_FREEVEC - 1 + (fix >> 3)] & (1 << (fix & 7));
}}



private void tog_bit(p, ix)
	 unsigned char *  p;
	 Short            ix;
{ if (ix < 8)
    p[RP_FREEVEC0] ^= (1 << ix);
  else
    p[RP_FREEVEC - 1 + (ix >> 3)] ^= (1 << (ix & 7));
}



static const Char bit_count[] =
/*0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 */
{ 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 
};

private Vint ct_free_bits(p, sz)
	 unsigned char *  p;
	 Short            sz;
{ fast Vint res = bit_count[ p[RP_FREEVEC0] & 15 ] +
    		  bit_count[(p[RP_FREEVEC0] >> 4) & 15];

  for (; (sz -= 8) > 0; ++p)
  { fast Short msk = p[RP_FREEVEC];
    if (sz < 8)
      msk &= (1 << sz) - 1;

     res += bit_count[ msk & 15 ] +
            bit_count[ msk >> 4 ];
  }

  return res;
}

static const Short bit_offs[] =
/*0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 */
{-1, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0 
};

					/* -ve not found */
private Vint find_bit(p, sz)
	 unsigned char *   p;
	 Short             sz;   /* first illegal index */ 
{ fast Vint res;
  fast Vint x = p[RP_FREEVEC0];

  if (x != 0)
  { if (x & 15)
      return bit_offs[x & 15];
    return bit_offs[x >> 4] + 4;
  }

  p += RP_FREEVEC;
				/* res is now mirrored: sz - res */
  for (res = sz; (res -= 8) > 0; )
  { x = *p++;
    if (x != 0)
    { if      ((x & 15) != 0)
        res -= bit_offs[x & 15];
      else
        res -= bit_offs[x >> 4] + 4;
      return res <= 0 ? -1 : (sz - res);
    }
  }

  return -1;
}

static Char myrbuff[BLOCK_SIZE];

public Byte * read_rec(upn, cl_id, lra, rec)
	Ldn        upn;
	Sid        cl_id;
	Lra_t      lra;
	void *     rec;
{ Byte * s = page_ref(upn, s_page(lra));

  if (s_sgrp(s) != cl_id and cl_id != 0)
  { i_log(s_sgrp(s), "*loose read from %d", cl_id);
  }
  
  if (rec_is_free(s, s_offs(lra)))
    i_log(upn, "Read free rec %d, %d (%d) (%d)", 
                 s_page(lra), s_offs(lra), s[RP_FREEVEC0], cl_id);

  if (cache_err != OK)
    return null;

  if (rec == null)
    rec = &myrbuff[0];
  memcpy(rec, &s[s_offs(lra)], ddict[cl_id].size_cl);

  return (Byte*)rec;  
}



public Cc write_rec(upn, cl_id, lra, rec)
	Ldn        upn;
	Sid        cl_id;		/* must not be 0 */
	Lra_t      lra;
	void *     rec;
{ Offs offs = s_offs(lra);
  Vint fix = (offs - ddict[cl_id].cl_soffs) / ddict[cl_id].size_cl;
  Byte * p = page_ref(upn, s_page(lra));

  if (   cache_err != OK
      or p[SEG_TYPE] != REC_SEG + (cl_id << 2))
  { i_log(p[SEG_TYPE], "Write wrong type %d (%d)", lra, cl_id);
    return REC_GONE;
  }
      
  if (is_free_bit(p, fix))
  { i_log(cl_id, "Write to free %d (%d)", lra, cl_id);
    return REC_GONE;
  }

  if (rec != null)
  { p = page_mark_ref(upn, s_page(lra));
    memcpy(&p[offs], rec, ddict[cl_id].size_cl);
  }
  return OK;
}


						/* app. specific */
public void upd_nxt_fld(upn, cl_id, lra, offs, vlra)
	Ldn        upn;
	Sid        cl_id;
	Lra_t      lra;
	Offs	     offs;
	Lra_t      vlra;
{ Byte * p = rec_mark_ref(upn, cl_id, lra & (Q_EFLAG-1));
  Int v = (r_quad(p, offs) & Q_EFLAG)+(vlra & (Q_EFLAG-1));
  w_quad(p, offs, v);
}

/* pages with free record slots available are chained together through
the with-free field: wifr
This is the first free slot in the page */

Vint sf_wifr_ix;		/* auxilliary result */
				/* -1 => no wifr */
public Vint sf_wifr(Char * p)

{ Vint cl_id = s_sgrp(p);

  sf_wifr_ix = find_bit(p, ddict[cl_id].cl_vecsz);
  if (sf_wifr_ix < 0)
    return -1;
  return ddict[cl_id].cl_soffs + sf_wifr_ix * ddict[cl_id].size_cl;
}

				/* only used by cleanup.c */
				/* no wifr => return 0 */
public Vint s_wifr(Char * p)

{ Vint cl_id = s_sgrp(p);

  Vint fld = find_bit(p, ddict[cl_id].cl_vecsz);
  if (fld < 0)
    return 0;
{ Offs offs = ddict[cl_id].cl_soffs + fld * ddict[cl_id].size_cl;
  return *(Short*)&p[offs];
}}



public void vfy_wifr(Id upn, Char * p, Short wifr)

{ Vint cl_id = s_sgrp(p);

  Vint fld = find_bit(p, ddict[cl_id].cl_vecsz);
  if (fld >= 0)
  { Offs offs = ddict[cl_id].cl_soffs + fld * ddict[cl_id].size_cl;
    if (*(Short*)&p[offs] != wifr)
    { i_log(cl_id, "Wifr %d field %d corrupted from %d to %d", 
                    upn, offs, *(Short*)&p[offs], wifr);
      *(Short*)&p[offs] = wifr;
    }
  }
}



#define NEW_REC_CLAMP    300
#define EMPTY_PAGE_OHEAD  10

static Short rec_size;        /* private to new_rec and new_init_rec */

public Cc new_rec(upn, cl_id, lra_ref)
	Ldn        upn;
	Sid        cl_id;
	Lra_t     *lra_ref;
{      Short mi_eny = class_offs(cl_id);
       Vint vsz = ddict[cl_id].cl_vecsz;
       Page_nr flp, next, prev = ROOT_PAGE;
       Int clamp = NEW_REC_CLAMP;
       Byte * p;
  fast Byte *root_p = page_mark_ref(upn, (Int)ROOT_PAGE);
       Vint wifr_fld;

  vfy_islocked(upn, 2);
  w_quad(root_p, mi_eny + CE_VERSION, r_quad(root_p, mi_eny + CE_VERSION) + 1);
  rec_size = ddict[cl_id].size_cl;

  last_page = -1;

  for (flp = r_word(root_p, mi_eny + CE_FREE_LIST); 
       flp != 0 and --clamp >= 0;
       flp = next)
  { p = page_mark_ref(upn, flp);
    if (cache_err != OK)
      break; 

    wifr_fld = sf_wifr(p);		/* ct == 0   : all allocated */
  { Vint ix = sf_wifr_ix;
    Vint ct = ct_free_bits(p, vsz);	/* ct == 1   : will be all allocated */
    					/* ct == vsz : already unallocated */
    if      (ct == 0)
      i_log(cl_id, "Int.Err.Wifr(1) %d", flp);
    if (wifr_fld + rec_size > BLOCK_SIZE)
      i_log(cl_id, "OFLO with %d\n", wifr_fld);

    next = r_word(p, wifr_fld);

    if      (ct == vsz and not page_in_strms(upn, flp) or ct == 1)
      							/* take it out */
    { Char * prev_p = page_mark_ref(upn, prev);
    /*i_log(cl_id, prev==ROOT_PAGE ? "Unfree page %d":"UNFREE page %d", flp);*/
      p_r_eny->wifr = *(Short*)&p[wifr_fld];
      if (prev == ROOT_PAGE)
        w_word(prev_p, mi_eny + CE_FREE_LIST, p_r_eny->wifr);
      else
      { Vint wifr_fldp = sf_wifr(prev_p);
        if (wifr_fldp < 0)
          i_log(cl_id, "Int.Err.Wifr(2) %d", prev);
        else
          w_word(prev_p, wifr_fldp, p_r_eny->wifr);
      }
      if (ct != vsz or ct == 1)
      { tog_bit(p, ix);
        break;
      }
    { Page_nr prv = r_word(p, RP_PREV);
      Page_nr nxt = r_word(p, RP_NEXT);
      (void)page_now_free(upn, flp);		/* references ROOT, flp */
    /*i_log(cl_id, "Recovered page");*/
      prev_p = page_mark_ref(upn, prv);
      w_word(prev_p, prv == ROOT_PAGE ? mi_eny + CE_RP : RP_NEXT, nxt);
      if (nxt != 0)
      { prev_p = page_mark_ref(upn, nxt);
        w_word(prev_p, RP_PREV, prv);
      }
    }}
    else if (ct < vsz or next == 0 or clamp < NEW_REC_CLAMP - EMPTY_PAGE_OHEAD)
    { tog_bit(p, ix);
    { Vint n_fld = sf_wifr(p);
      if (n_fld < 0)
        i_log(prev, "Int.Err.Wifr");
      else
      { w_word(p, n_fld, *(Short*)&p[wifr_fld]);
      /*i_log(cl_id, "Routed %d from %d to %d, val %d", 
                       flp, wifr_fld, n_fld, (Int)*(Short*)&p[wifr_fld]);*/
        if (*(Short*)&p[wifr_fld] != p_r_eny->wifr)
          i_log(cl_id, "%d %d Already corrupted from %d to %d", 
                    upn, wifr_fld, *(Short*)&p[wifr_fld], p_r_eny->wifr);

      }
      break;
    }}
    else	/* => ct == vsz and page_in_strms(upn, flp) */
    { /*i_log(cl_id, "Prot Free %d", flp);*/
      prev = flp;
    }
  }}

  if (clamp < 0)
  { i_log(cl_id, "Big Fre List");
    flp = 0;
  }

  if (flp != 0)
  { /*i_log(cl_id, "NRec %d [ %x ] ", flp, p[RP_FREEVEC0]);*/

    *lra_ref = to_lra(flp, wifr_fld);
    return OK;
  }
  else
  { if (get_new_page(upn, REC_SEG, cl_id, &flp, &p) != OK or
				cache_err != OK)
    	return i_log(OUT_OF_MEMORY, "new_rec: out of memory");

    root_p = page_mark_ref(upn, (Int)ROOT_PAGE);
    prev = r_word(root_p, mi_eny + CE_RP);
    if (prev == flp)
      i_log(cl_id, "*Loop shit %d", prev);

    if (prev != ROOT_PAGE)
    { Char * n = page_mark_ref(upn, prev);
      w_word(n, RP_PREV, flp);
    }
    w_word(p, RP_NEXT, prev);
    w_word(root_p, mi_eny + CE_RP, flp);

    p[RP_FREEVEC0] = -2;

  { Int toffs = ddict[cl_id].cl_soffs + vsz * ddict[cl_id].size_cl;
    if (toffs < BLOCK_SIZE)
      memset(&p[toffs], 0xfe, BLOCK_SIZE - toffs);
    
    if (vsz <= 8)
      p[RP_FREEVEC0] &= ((1 << vsz) - 1);
    else
    { Int top = ((vsz - 9) >> 3); 
      p[RP_FREEVEC + top] = -1;
      vsz &= 7;
      if (vsz > 0)
        p[RP_FREEVEC + top] &= ((1 << vsz) - 1);
      for ( ;--top >= 0; )
        p[RP_FREEVEC + top] = -1;
    }

    if (vsz > 1)
    { Cache_det pr = p_r_eny;
      Vint wifr_fld_ = sf_wifr(p);
      pr->wifr = r_word(root_p, mi_eny + CE_FREE_LIST);
      w_word(p, wifr_fld_, pr->wifr);
      w_word(root_p, mi_eny + CE_FREE_LIST, flp);
    /*i_log(cl_id, "Unfree new page %d", flp);*/
    }

    *lra_ref = to_lra(flp, ddict[cl_id].cl_soffs);
  }}
  return SUCCESS;
}


public Cc new_init_rec(upn, cl_id, rec, lra_ref)
	Ldn        upn;
	Sid        cl_id;
	Char *     rec;
	Lra_t     *lra_ref;
{ Cc cc = new_rec(upn, cl_id, lra_ref);
  if (cc == SUCCESS)
    memcpy(rec_mark_ref(upn, cl_id, *lra_ref), rec, rec_size);

  return cc;
}

public Cc del_rec(upn, cl_id, lra)
      Ldn     upn;
      Sid     cl_id;			/* must not be 0 */
      Lra_t   lra;
{ vfy_islocked(upn, 2);
{ Short mi_eny = class_offs(cl_id);
  Page_nr page = s_page(lra);
  Short offs = s_offs(lra);
/*Vint vsz =   ddict[cl_id].cl_vecsz;*/
  Vint fix = (offs - ddict[cl_id].cl_soffs) / ddict[cl_id].size_cl;

  Byte * root_p = page_mark_ref(upn, (Int)ROOT_PAGE);
  Byte * p = page_mark_ref(upn, page);
  Cc cc = OK;

  if      (cache_err != OK)
    cc = i_log(LOST_BLOCK, "CErr %d", cache_err);
  else if (p[SEG_TYPE] != REC_SEG + (cl_id << 2))
    cc = i_log(LOST_BLOCK, "seg %d %d", p[SEG_TYPE], cl_id);
  else if (is_free_bit(p, fix))
    cc = i_log(LOST_BLOCK, "Already gone %x %x %x", lra, p[SEG_TYPE], fix);

  if (cc != OK)
    return cc;

  w_quad(root_p, mi_eny + CE_VERSION, r_quad(root_p, mi_eny + CE_VERSION) + 1);
  last_page = -1;

{ Vint fb = sf_wifr(p);
  if (fb < 0)					/* put it on the free list */
  { p_r_eny->wifr = r_word(root_p, mi_eny + CE_FREE_LIST);
    w_word(p, offs, p_r_eny->wifr);
    w_word(root_p, mi_eny + CE_FREE_LIST, page);
  /*i_log(cl_id, "Delfree new page %d at %d", page, offs);*/
  }
  else if (offs < fb)				/* reroute the free list */
  { cc = r_word(p, fb);
    w_word(p, offs, cc);
  /*i_log(cl_id, "DelRouted %d from %d to %d, val %d", 
                       page, fb, offs, *(Short*)&p[fb]);*/
  }

  tog_bit(p, fix);

  return cache_err == 0 ? OK : CORRUPT_DB;
}}}



public Cc del_all_recs(upn, cl_id)
      Ldn     upn;
      Sid     cl_id;
{ wr_lock(upn);
{ Byte * root_p = page_ref(upn, (Int)ROOT_PAGE);
  Int dbsize = r_word(root_p, MI_FIRST_UNUSED);
  Page_nr page;

  for (page = 0; ++page < dbsize; )
  { Byte * p = page_ref(upn, page);

    if (p[SEG_TYPE] == (REC_SEG | (cl_id << 2)))
      (void)page_now_free(upn, page);
  }

{ Short mi_eny = class_offs(cl_id);
  Byte * root_p_ = page_mark_ref(upn, (Int)ROOT_PAGE);
  w_word(root_p_, mi_eny + CE_RP, 0);
  w_quad(root_p_, mi_eny + CE_VERSION, r_quad(root_p_, mi_eny + CE_VERSION) + 1);
  w_word(root_p_, mi_eny + CE_FREE_LIST, 0);
  wr_unlock(upn);
  return OK;
}}}



static Byte dum_rec[BLOCK_SIZE];

public Byte * rec_ref(upn, lra)
    Sid       upn;
    Lra_t     lra;
{ Char * p = page_ref(upn, s_page(lra));
  if (rec_is_free(p, s_offs(lra)))
  { i_log(p[SEG_TYPE], "*ref to free from %d", s_sgrp(p));
    return &dum_rec[0];
  }
  return &p[s_offs(lra)];
}

public Byte * rec_mark_ref(upn, cl_id, lra)
    Sid       upn;
    Quot      cl_id;
    Lra_t     lra;
{ Byte * p =  page_mark_ref(upn, s_page(lra));
  if (s_sgrp(p) != cl_id and cl_id != 0)
  { i_log(p[SEG_TYPE], "*loose ref from %d", cl_id);
    return &dum_rec[0];
  }
  if (rec_is_free(p, s_offs(lra)))
  { i_log(p[SEG_TYPE], "*mark to free from %d", s_sgrp(p));
    return &dum_rec[0];
  }
  return &p[ s_offs(lra) ];
}

			/* must be isomorphic to prefix of Rec_strm_t */
typedef struct 
{ Sid     upn;
  Char    cl_id;
  Char    written;    
  Int     version;
  Page_nr page;
  Offs    offs;
} Ix_strm_t, *Ix_strm;



public Char * new_stream(upn, cl_id)
	Ldn      upn;
	Sid      cl_id;			/* must not be 0 */
{ if (rd_lock(upn) != OK)
    return null;

  Short mi_eny = class_offs(cl_id);
  Byte * root_p = page_ref(upn, (Int)ROOT_PAGE);

  Ix_strm strm = new(Ix_strm_t);
  if (strm == null)
    return null;

  strm->upn     = upn;
  strm->cl_id   = cl_id;
  strm->written = false;
  strm->version = r_quad(root_p, mi_eny + CE_VERSION);
  strm->page    = r_word(root_p, mi_eny + CE_RP);
  strm->offs    = ddict[cl_id].cl_soffs - ddict[cl_id].size_cl;
  last_page = -1;
  if (page_to_strms(upn, strm->page) != OK)
    i_log(strm->page, "Out of stream space");
  rd_unlock(upn);
  return (Char *)strm;
}


public void fini_stream(strm)
	Ix_strm  strm;
{ page_from_strms(strm->upn, strm->page);
  free((Char*)strm);
}

public Char * next_rec(strm, lra_ref, rec_ref)
	void *   strm;
	Lra_t   *lra_ref;
	void    *rec_ref;	
{ Sid upn = ((Ix_strm)strm)->upn;
  rd_lock(upn);

  Byte * p = page_ref(upn, (Int)ROOT_PAGE);
  Vint cl_id = ((Ix_strm)strm)->cl_id;
  Vint mi_eny = class_offs(cl_id);
  Vint rec_sz = ddict[cl_id].size_cl;
  Vint soffs = ddict[cl_id].cl_soffs;
  Offs offs = ((Ix_strm)strm)->offs;
  Vint fix = (offs - soffs) / rec_sz;
  Int version = r_quad(p, mi_eny + CE_VERSION);
  Bool poss_wr = version != ((Ix_strm)strm)->version;
  Page_nr page = ((Ix_strm)strm)->page;

  ((Ix_strm)strm)->version = version;

  while (page != 0)                     /* and not return */
  { Byte * p = page_ref(upn, page);
    ((Ix_strm)strm)->page = page;

    if (s_sgrp(p) != cl_id)
      i_log(cl_id, "Mixed recs in strm (%d)", s_sgrp(p));

    if (poss_wr or page != last_page or
		   upn  != last_upn)
    { if (poss_wr)
			((Ix_strm)strm)->written = true;
      ((Ix_strm)strm)->version = version;
    }

    while (offs < BLOCK_SIZE)		/* and not break */
    { offs += rec_sz;
      fix += 1;
      if (not is_free_bit(p, fix))
				break;
    }

    if (offs + rec_sz <= BLOCK_SIZE)
    { last_upn  = upn;
      last_page = page;
      ((Ix_strm)strm)->offs = offs;
      *lra_ref = to_lra(page, offs);
      memcpy(rec_ref, (Char *)&p[offs], ddict[cl_id].size_cl);
      rd_unlock(upn); 
      return cache_err != OK ? null : rec_ref;
    }

    offs = soffs - rec_sz;
    fix = -1;
    page_from_strms(upn, page);
    page = r_word(p, RP_NEXT);
    if (page_to_strms(upn, page) != OK)
      i_log(page, "Out of stream space");
  /*i_log(page, "Nxt");*/
  }
  *lra_ref = 0;
  last_upn = -1;
  rd_unlock(upn);
  return null;
}

public Char * init_brec_strm(upn, cl_id)
	Ldn      upn;
	Sid      cl_id;
{ Ix_strm strm = new(Ix_strm_t);
  if (strm == null)
    return null;

  strm->upn     = upn;
  strm->cl_id   = cl_id;
  strm->page    = 1;
  strm->offs    = 0;
  last_page = -1;
  return (Char *)strm;
}



public void fini_brec_strm(strm)
	Ix_strm  strm;
{ free((Char*)strm);
}

                     /* returns type of record found, -1 => EOS, -2 => error */

public Cc next_brec(strm, pg_ref, offs_ref, rec_ref)
	Ix_strm  strm;
	Page_nr *pg_ref;
	Offs    *offs_ref;
	Byte *  *rec_ref;	
{ Sid upn = strm->upn;
  rd_lock(upn);
{ char root_page[BLOCK_SIZE];
  Byte * p = page_ref(upn, (Int)ROOT_PAGE);
  if (p == null)
    return -2;
  
  memcpy(&root_page[0],p, BLOCK_SIZE); 
  
{ Vint cl_id_ = strm->cl_id;              /* 0 => all types */
  Vint cl_id = cl_id_;

  Page_nr page = strm->page;
  Offs offs = strm->offs;
  Page_nr dbsize = r_word(root_page, MI_FIRST_UNUSED);
  
  while (page < dbsize)                     /* and not return */
  { Byte * p = page_ref(upn, page);

    if (s_stype(p) == REC_SEG)
    { if (cl_id_ == 0)
        cl_id = s_sgrp(p);
      if (s_sgrp(p) == cl_id)
      {
        Int version = r_quad(root_page, class_offs(cl_id) + CE_VERSION);
        if (version != strm->version)
		  	  strm->written = true;
    
        strm->page = *pg_ref = page;
  
      { Vint soffs = ddict[cl_id].cl_soffs;
        Vint rec_sz = ddict[cl_id].size_cl;
        Vint fix;
        if (offs != 0)
          fix = (offs - soffs) / rec_sz;
        else
        { offs = soffs - rec_sz;
          fix = -1;
        }
  
        while (offs < BLOCK_SIZE)		/* and not break */
        { offs += rec_sz;
		  	  fix += 1;
		  	  if (not is_free_bit(p, fix))
		  	    break;
        }
  
        if (offs + rec_sz <= BLOCK_SIZE)
        { last_upn  = upn;
		  	  last_page = page;
		  	  strm->offs = *offs_ref = offs;
		  	  *rec_ref = (Byte *)&p[offs];
		  	  rd_unlock(upn); 
		  	  return cl_id;
        }
      }}
    }
  
    page += 1;
    offs = 0;
  }
  *offs_ref = 0;
  *rec_ref = null;
  last_upn = -1;
  rd_unlock(upn);
  return -1;
}}}

public void frame_blks(Id upn, Id cl_id)

{ wr_lock(upn);
{ Char * strm = init_brec_strm(upn, cl_id);
  Page_nr page;
  Offs offs;
  Int top =   ddict[cl_id].cl_vecsz;
  Int toffs = ddict[cl_id].cl_soffs + top * ddict[cl_id].size_cl;
  
  while (next_brec(strm, &page, &offs, myrbuff) > OK)
  { Byte * p = page_mark_ref(upn, page);
    Int to;
    for (to = toffs - 1; ++to < BLOCK_SIZE; )
      p[to] = 0xfe;
  }

  fini_brec_strm(strm);
  wr_unlock(upn);
}}

