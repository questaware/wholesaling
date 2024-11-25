#include	<stdio.h>
#include	<string.h>
#include	"version.h"
#include	"../h/defs.h"
#include	"page.h"
#include	"recs.h"
#include	"memdef.h"
#include	"cache.h"
#include	"b_tree.h"
#include	"../h/errs.h"

#define rpc_return(typ, v) return (v);

#define MAX_MULT ((1L << (16-LOG_BLOCK_SIZE))-1)

#define s_pg(lra) (((lra) >> (LOG_BLOCK_SIZE)) & 0xffff)
#define s_mult(lra) (((lra) >> (LOG_BLOCK_SIZE+16)) & MAX_MULT)

#define MULT_MASK ~((1L << (LOG_BLOCK_SIZE + 16))-1)
#define MULT_UNIT   (1L << (LOG_BLOCK_SIZE + 16))

#define mk_blra(pg, o) (((Int)(pg) << LOG_BLOCK_SIZE) + (o))


Cc dbcc;


static Int	  b_ix;
static Skind  b_key_type;
static Short  b_key_size;
static Short  b_key_cl;		/* record the index indexes */
			
private Short kv_compare(key_value, addr)
    Key_t      *key_value;
    Byte *      addr;
{ fast Int x;
       Int y;
  switch (b_key_type & (E_NU-1))
  { case INTEGER: x = key_value->integer;
					  		  y = r_quad(addr, 0);
								  return x == y ? 0 : x < y ? -1 : 1;
    case ENUM:    return key_value->enumeration - r_word(addr, 0);
    case FXD_STR: return strcmp(key_value->string, addr);
    case FXD_STR+UCKEY:
								  return struccmp(key_value->string, addr);
    case LRA_KEY: x = key_value->lra;
					  		  y = r_quad(addr, 0);
								  return x == y ? 0 : x < y ? -1 : 1;
    default:		  return i_log(0,"kv_compare:");
  }
}


private void kv_insert(addr, key_value)
    Byte *       addr;
    Key_t	*key_value;
{
  switch (b_key_type & (UCKEY-1))
  { case INTEGER:  w_quad(addr, 0, key_value->integer);
    when ENUM:     w_word(addr, 0, key_value->enumeration);
    when FXD_STR:  (void)strpcpy(addr, key_value->string, b_key_size+1);
    when LRA_KEY:  w_quad(addr, 0, key_value->lra);

    otherwise       (void)i_log(0,"kv_insert");
  }
}



/* private */ void kv_extract(key_value, addr)
    Key_t   * key_value;
    Byte *    addr;
{ switch (b_key_type & (UCKEY-1))
  { case INTEGER:  key_value->integer = r_quad(addr, 0);
    when ENUM:     key_value->enumeration = r_word(addr, 0);
    when FXD_STR:  (void)strpcpy(key_value->string, addr, b_key_size+1);
    when LRA_KEY:  key_value->lra = r_quad(addr, 0);

    otherwise      (void)i_log(0,"kv_extract");
  }
}

  /* Care, the following macros use global variable b_key_size */

#define TP(blk) (blk[SEG_TYPE] & 3)

#define addr_of_1  (BT_BASE+PTR_SIZE)

int gen_addr(blk, ix)
	Byte * blk;
	Int    ix;
{ return ((ix)-1) * (b_key_size+LRA_SIZE+TP(blk))+addr_of_1;
}


int addr_to_pos(blk, addr)
	Byte * blk;
	Short  addr;
{ return (Short)(addr - addr_of_1) / (Short)(b_key_size+LRA_SIZE+TP(blk)) + 1;
}


Lra_t  lra_o(base, mult)
	Lra_t  base;
	Short  mult;			/* zero based */
{ Short vsz = ddict[b_key_cl].cl_vecsz;
  Short rec_size = align(ddict[b_key_cl].size_cl, RP_ALIGN);
 
  Short ix = (s_offs(base) - ddict[b_key_cl].cl_soffs) / rec_size + mult;

  return mk_blra(s_pg(base) + ix / vsz, 
                 ddict[b_key_cl].cl_soffs + (ix % vsz) * rec_size);
}


#define next_addr_of(p, pos) ((pos)+b_key_size+LRA_SIZE+TP(p))

#define i_le(kv, addr) ((kv)->integer <= (long)r_quad(addr, 0))
#define s_le(kv, addr) (strcmp((kv)->string, addr) <= 0)
#define s_ule(kv, addr) (struccmp((kv)->string, addr) <= 0)


static void shift_in_node(block, lb, ub, delta)
    Byte * block;
    Short  lb, ub, delta;
{ Short sz = b_key_size + LRA_SIZE + TP(block);
  Int toffs = gen_addr(block, lb);
  if (toffs + delta * sz > BLOCK_SIZE)
    i_log(0, "S Node Too Big");
  else
    memmove(block+toffs+delta*sz, block+toffs, (Short)(ub-lb+1) * sz);
}


				/* node types must be the same */
private void move_to_node(to_blk, lb1, from_blk, lb2, ub2)
	Short   lb1, lb2, ub2;
	Byte *to_blk, *from_blk;
{ if (lb2 <= ub2)
  { Short typ = to_blk[SEG_TYPE];
    if (typ != from_blk[SEG_TYPE])
      i_log(typ, "Non cong move");
  { Int len = ((ub2-lb2) + 1) * (b_key_size + LRA_SIZE + TP(to_blk));
    
    Int toffs =  gen_addr(to_blk,  lb1); 
    if (toffs + len > BLOCK_SIZE)
      i_log(0, "Node Too Big");
    else
      memcpy(to_blk+toffs, from_blk + gen_addr(from_blk,lb2), len);
  }}
}

			  /* result 0 => not found,              */
			  /* else   abs is offset+1 in multiples */
			  /* -ve => would fit at the end of multiples */

private Short b_pg_srch(block_, key_val, lra, ord_ref)
    Byte *      block_;
    Key_t      *key_val;
    Lra_t      lra;		/* non inserts: 0 else lra to insert */
    Short      *ord_ref;        /* first is 1; match pos or 1st pos greater */
{
  fast Key_t * key_value = key_val;
  fast Byte *  block = block_;

  fast Int u = *(unsigned short*)(block+BT_NKEYS) & 0x7fff;
  fast Int l = 1;
       Int m;
  			      /* decision on key type hoisted from kv_compare */
  if ((b_key_type & (UCKEY-1)) == INTEGER)  
    while (u >= l)
    { m = (u + l) >> 1;
    { Int addr = gen_addr(block, m);
      Int mem = r_quad(block, addr);

      if      (key_value->integer <  mem)
				u = m - 1;
      else if (key_value->integer == mem)
				break;
      else 
      { l = m + 1;
      { Int got = s_mult(r_quad(block, addr+sizeof(Int))) + 1;
				if (key_val->integer <= mem + got and (b_key_type & E_NU) == 0)
				{ Int mult = key_val->integer - mem + 1;
				  if (mult <= got)
          { *ord_ref = m;
            return mult;
				  }
				  if (mult <= MAX_MULT)
				  { Lra_t tlra = r_quad(block, addr+sizeof(Int)) & ~MULT_MASK;
				    if (lra_o(tlra, got) == lra)
            { *ord_ref = m;
              return -mult;
				    }
				  }
				}
      }}
    }}
  else 
    while (u >= l)      
    { m = (u + l) >> 1;
    { Short res = kv_compare(key_value, &block[ gen_addr(block, m) ]);

      if      (res < 0)
				u = m - 1;
      else if (res > 0)
				l = m + 1;
      else
				break;
    }} 
	if (u >= l)
  { *ord_ref = m;
    return 1;
  }
  else 
  { *ord_ref = u+1;
    return 0;
  }
}

private Bool last_nu;

		/* Node address stack used during insertion and deletion */
#define MAX_DEPTH_B_TREE 20

static Int   node_stk[MAX_DEPTH_B_TREE+1];          /* initially zero */
static Int*  stk_ptr;
					/* final page pushed for res -1 only */

#define clear_stk_() stk_ptr = &node_stk[MAX_DEPTH_B_TREE]


#define empty_stk_() (*stk_ptr == 0)


#define push_stk_ptr(page_nr) \
 if (stk_ptr == &node_stk[0]) \
   i_log(page_nr, "BT Stk oflo"); \
 else \
   *--stk_ptr = page_nr


#define pop_stk_ptr() *stk_ptr++
 
			  /* result 0 => not found,                  */
			  /* -1 => would fit at the end of multiples */
			  /* else   abs is offset+1 in multiples     */

private Cc b_srch(ipn, page, key_val, lra_ref)
    Sid	  	ipn;			/* locked */
    Page_nr	page;
    Key_t  *key_val;
    Lra_t  *lra_ref;		/* INPUT lra needed, (INSERTS ony) */ 
                              /* OUTPUT location (sic) of lra */ 
{ fast Short	addr;
			 Short	maddr;
	     Int *	stk_mix = null;
							/* this code unfolded for performance */
	cache_lock(); 		/* it uses _page_ref not page_ref !! */

	clear_stk_();
	last_nu = false;
	
	while (page != 0)
	{ fast Byte * block = _page_ref(ipn, page);
		fast Int u = *(short*)(block+BT_NKEYS) & 0x7fff;
		if (u == 0)
			break;

	{ fast Int l = 1;
						/* decision on key type hoisted from kv_compare */
		if ((b_key_type & (UCKEY - 1)) == INTEGER)	
			while (u >= l)
			{ Int m = (u + l) >> 1;
				addr = gen_addr(block, m);
			{ Int mem = r_quad(block, addr);

				if	(key_val->integer <  mem)
					u = m - 1;
				else if (key_val->integer == mem)
					break;
				else 
				{ l = m + 1;
				{ Int mul = s_mult(r_quad(block, addr+sizeof(Int))) + 1;
					if (key_val->integer <= mem + mul)
					{ Int mult = key_val->integer - mem + 1;
						if (mult <= mul)
						{ /*push_stk_ptr(page);*/
							*lra_ref = to_lra(page, addr + sizeof(Int));
							last_nu = s_offs(r_quad(block, addr+sizeof(Int))) == 0;
							cache_unlock();
							return mult;
						}
						if (mult <= MAX_MULT and (b_key_type & E_NU) == 0)
						{ Lra_t tlra = r_quad(block, addr+sizeof(Int)) & ~MULT_MASK;
							if (lra_o(tlra, mul) == *lra_ref)
							{ maddr = addr;
								stk_mix = stk_ptr;
							}
						}
					}
				}}
			}}
		else
			while (u >= l)			
			{ Int m = (u + l) >> 1;
				addr = gen_addr(block, m);
			{ Short res = kv_compare(key_val, &block[addr]);
				if			(res < 0)
				{ addr = gen_addr(block,l);
					res = kv_compare(key_val, &block[addr]);

					if			(res < 0)
						u = 0;									 /* exits inner loop "not found" */
					else if (res > 0) 
					{ u = m - 1;
						++l;
					}
				} 
				else if (res > 0)
				{ addr = gen_addr(block, u);
					res = kv_compare(key_val, &block[addr]);

					if			(res > 0)
						l = u + 1;								/* exits inner loop "not found" */
					else if (res < 0)
					{ l = m + 1;
						u--;
					}
				} 
				if (res == 0)
					break;
			}} 
		if (u >= l)
		{ *lra_ref = to_lra(page, addr + b_key_size);
			last_nu = s_offs(r_quad(block, addr+b_key_size)) == 0;
			cache_unlock();
			return 1;
		}
		push_stk_ptr(page);
		if (TP(block) == 0)
			break;
		page = r_word(block, gen_addr(block, l) - PTR_SIZE);
	}}

	if (stk_mix != null)
	{ stk_ptr = stk_mix-1;
		*lra_ref = to_lra(stk_mix[0], maddr + b_key_size);
	}

	cache_unlock();

	return stk_mix == null ? 0 : -1;
}

public Cc b_search(upn, ix_ix, key_val, lra)
    Sid	  	upn;
    Squot 	ix_ix;
    Key_t  *key_val;
    Lra_t  *lra;
{ b_key_type = ddict[ix_ix].kind_ix;
  b_key_size = ddict[ix_ix].size_ix;
  b_key_cl = ddict[ix_ix].cl_ix;

{ Lra_t zlra = 0;
  Byte * p = page_ref(-upn, ROOT_PAGE);
  Cc mu = b_srch(-upn,(Int)r_word(p, ix_offs(ix_ix)+IE_RP), key_val, &zlra);
  if (mu > 0)
  { if (lra != null)
    { vfy_islocked(upn, 1);
      p = rec_ref_unchecked(-upn, zlra);
      zlra = lra_o( r_quad(p, 0), mu-1);
      if (last_nu)
      { p = page_ref(-upn, s_pg(zlra));
        zlra = r_quad(p, BT_BASE+PTR_SIZE);
      }
      *lra = zlra;
      if (zlra >= 0x4000000)
      { i_log(zlra, "Corrupt ix: %d %d",ix_ix,-upn);
        return CORRUPT_DB;
      }
    }
    return OK;
  }
  return NOT_FOUND;
}}


public Bool was_mult(void) { return last_nu; }


static Byte mybuff[BLOCK_SIZE];


public Byte * b_srchrec(upn, ix_ix, key_val, lra)
    Sid	  	upn;			/* at least read locked */
    Squot 	ix_ix;
    Key_t  *key_val;
    Lra_t  *lra;
{ if (lra == null)
    lra = (Lra)&mybuff[0];
  dbcc = b_search(upn, ix_ix, key_val, lra);
  if (dbcc != OK)
    return null;
{ Char * s = page_ref(upn, s_pg(*lra));
  if (s_sgrp(s) != ddict[ix_ix].cl_ix)
  { i_log(*lra, "loose from %d is %d", ix_ix, s_sgrp(s));
    return null;
  }
  return &s[s_offs(*lra)];
}}



public Byte * b_fetchrec(upn, ix, key_val, lra_, t)
    Sid	  	upn;
    Squot		ix;
    Key_t  *key_val;
    Lra_t  *lra_;
    void   *t;
{ if (lra_ != null)
    vfy_islocked(upn, 1);
  rd_lock(upn);
{ Byte * s = b_srchrec(upn,ix,key_val, lra_);
  if (s == null)
  { rd_unlock(upn); return null; }

  if (t == null)
    t = (void*)&mybuff[0];
  memcpy(t, s, ddict[ddict[ix].cl_ix].size_cl);
  rd_unlock(upn);
  return t;
}}

private Int find_best_size(blk)
	  Char * blk;
{ Int lim = (BLOCK_SIZE-BT_BASE-PTR_SIZE)/
	    	     (Short)(b_key_size + LRA_SIZE + TP(blk));
  Int mid_ = ((*(short*)(blk+BT_NKEYS) & 0x3fff) + 2) >> 1;
  Int mid = mid_;
  if ((b_key_type & (UCKEY - 1)) != INTEGER)
    return mid;

{ Int clamp = 20;

  while (--clamp >= 0)
  { Int lo = r_quad(blk, addr_of_1);
    Int hi = r_quad(blk, gen_addr(blk, mid));
    Int growth = hi - (lo + mid - 1);
      
    mid = *(short*)(blk+BT_NKEYS) - growth;
    if (mid < mid_)
      return mid_;
    if (growth <= 0)
      return mid_;
  }
  
  if (mid <= 0) 
  { i_log(mid,"find_best_size");
    mid = mid_;
  }
    
  return mid * 10 < lim * 9 ? mid  :
         lim <= 5           ? mid_ :
                              lim * 9 / 10;
}}


private Cc b_ins(Sid, Page_nr, Key, Short, Lra_t * ) forward;

/* private */ Cc nu_insert(ipn, tgt_lra, key_lra)
    Sid    ipn;
    Lra   tgt_lra, key_lra;
{ Key_t kv_t;
  Squot skey_type = b_key_type;
  Short skey_size = b_key_size;
  Byte * p = rec_ref_unchecked(ipn, *tgt_lra);
  Lra_t ext = r_quad(p, 0);
  Cc cc;

  b_key_type = LRA_KEY;
  b_key_size = 0;

  while (true)			/* once only */
  { if (s_offs(ext) != 0)
    { kv_t.lra = ext;				/* existing lra */
      if (ext == *key_lra)
        return DUP_UK_ATTR;
    { cc = b_ins(ipn, (Page_nr)0, &kv_t, 1, &kv_t.lra);
      if (cc != SUCCESS)
				break;
      ext = kv_t.lra;
    }}

    kv_t.lra = *key_lra;			/* new lra */
  { Page_nr root = s_pg(ext);
    cc = b_ins(ipn, root, (Key)key_lra, 1, &kv_t.lra);
    if (cc == OK)
    { p = page_mark_ref(ipn, s_page(*tgt_lra));
      w_quad(p, s_offs(*tgt_lra), kv_t.lra);
    }						/* bug: new root page? */
    break;
  }}

  b_key_type = skey_type;
  b_key_size = skey_size;
  return cc;
}

static Char  key_svalue_[BLOCK_SIZE];
static Key_t key_value_;


private Cc b_ins(ipn, root, key_val, by, lra)
	 Sid 	    ipn;
	 Page_nr  root;
	 Key_t *  key_val;
	 Short    by;			/* # entries to add in */
	 Lra_t	 *lra;        /* in out */ /* lra->page returns new root */
{      Byte * blk_, *twin_blk;
  fast Byte * blk;
  fast Short	addr;

       Bool     done = false;
       Page_nr  curr_page = 0;
       Page_nr  ins_page = 0;
       Lra_t    s_lra = *lra; 
       Lra_t    add_lra = s_lra;

  if (s_offs(add_lra) == 0)
    return INVALID;

{ Short mult = b_srch(ipn, root, key_val, &s_lra);
  if (mult > 0)	     
  { *lra = to_lra(root,0);
    return (b_key_type & E_NU) ? nu_insert(ipn, &s_lra, &add_lra)
			  									   	 : DUP_UK_ATTR; 
  }

  add_lra |= (by-1) * MULT_UNIT;

  while (not done and not empty_stk_())
  { curr_page = pop_stk_ptr();

    blk = page_mark_ref(ipn, curr_page);
    if (blk == null)
      return PI_MISSING; 

    if (ins_page != 0 and TP(blk) == 0)
    { i_log(0, "Ins into leaf node");
      ins_page = 0;
    }
  { Int actual = *(short*)(blk+BT_NKEYS) & 0x7fff;
    Short pos;
    mult = b_pg_srch(blk, key_val, add_lra, &pos);
    addr = gen_addr(blk, pos);
    
    if (mult < 0)
    { Int la = r_quad(blk, addr+b_key_size);
      w_quad(blk,          addr+b_key_size, la + by*MULT_UNIT);
      *lra = to_lra(root,0);
      if (by > 1)
        i_log(by, "Int.Err.was.void");
      return SUCCESS;
    }
    if (mult > 0)
      i_log(b_key_cl, "ins clash");
    				/*is current node full? */
    if ((actual+1) * (b_key_size + LRA_SIZE + TP(blk)) 
                  <= BLOCK_SIZE-BT_BASE-PTR_SIZE)
    { if (pos != actual+1)		/* move all entries up one position */
        shift_in_node(blk, pos, actual, 1);
      *(short*)(blk+BT_NKEYS) += 1;
    /*printf("#K %d %x\n", *(short*)(blk+BT_NKEYS), curr_page);*/
      done = true;
    }
    else
    { Page_nr new_page;
      if (get_new_page(ipn, s_stype(blk), b_ix, &new_page, &twin_blk) != OK)
				return i_log(OUT_OF_MEMORY, "b_ins: get new page");

      blk = page_mark_ref(ipn, curr_page);
      if (blk == null)
				return PI_MISSING; 

    { Int mid = find_best_size(blk);
      if (mid == pos)                    /* break point for node splitting    */
      { w_word(twin_blk, BT_BASE, ins_page);
				move_to_node(twin_blk, 1, blk, mid, actual);
      }
      else 
      { Byte * tblk;
        if (mid > actual)
          i_log(55,"fbs wrong");
				if (mid < pos)
				{ if (TP(blk))
				    w_word(twin_blk, BT_BASE, r_word(blk, gen_addr(blk, mid+1)-PTR_SIZE));
				  move_to_node(twin_blk, 1, blk, mid+1, pos-1);
				  move_to_node(twin_blk, (pos-mid) + 1, blk, pos, actual);
				  addr = gen_addr(blk, pos-mid);
				  tblk = twin_blk;
				}
				else
				{ if (TP(blk))
	    			w_word(twin_blk, BT_BASE, r_word(blk, gen_addr(blk, mid)-PTR_SIZE));
				  move_to_node(twin_blk, 1,   blk, mid, actual);
				  shift_in_node(blk, pos, mid-1, 1);
				  tblk = blk;
				}
				kv_insert(&tblk[addr], key_val);
				w_quad(tblk, addr + b_key_size, add_lra);
				if (ins_page != 0)
				  w_word(tblk, addr + b_key_size + LRA_SIZE, ins_page);
					/* extract new entry */
				key_val = &key_value_;			
				key_val->string = &key_svalue_[0];
				addr = gen_addr(blk, mid);
				kv_extract(key_val, &blk[addr]);
				add_lra = r_quad(blk, addr + b_key_size);
      }
      *(short*)(blk+BT_NKEYS) = mid-1;
    /*printf("#ko %d %x\n", *(short*)(blk+BT_NKEYS), curr_page);*/
      *(short*)(twin_blk+BT_NKEYS) = (actual-mid) + 1;
    /*printf("#kn %d %x\n", *(short*)(twin_blk+BT_NKEYS), new_page);*/
      twin_blk[BT_KEY_SIZE] = (char)b_key_size;
      ins_page = new_page;
    }}
  }}

  if (not done)
  { if (get_new_page(ipn, curr_page == 0 ? INDEX_SEG : INODE_SEG, 
                       b_ix, &root, &blk_) != OK)
			return i_log(DISK_SPACE, "b_ins: get new root");

    blk = blk_;									/* create new root node for B-tree */
    *(short*)(blk+BT_NKEYS) = 1;
    blk[BT_KEY_SIZE] = b_key_size;
    addr = BT_BASE + PTR_SIZE;
    w_word(blk, BT_BASE, curr_page);
  }
  kv_insert(&blk[addr], key_val);
  w_quad(blk, addr + b_key_size, add_lra);
  if (ins_page != 0)
    w_word(blk, addr + b_key_size + LRA_SIZE, ins_page);
  *lra = to_lra(root,0);
  return SUCCESS;
}}



public Cc b_insert(upn, ix_ix, key_val, lra)
	 Sid 	    upn;
	 Squot    ix_ix;
	 Key_t *  key_val;
	 Lra_t   *lra;		/* input only */
{ Byte * block = page_ref(-upn, ROOT_PAGE);

  b_ix = ix_ix;
  b_key_type = ddict[b_ix].kind_ix;
  b_key_size = ddict[b_ix].size_ix;
  b_key_cl   = ddict[b_ix].cl_ix;

  vfy_islocked(upn, 2);

{ Lra_t  slra = *lra;
  Short i_offs = ix_offs(ix_ix);
  Cc cc = b_ins(-upn, (Int)r_word(block,i_offs+IE_RP), key_val, 1, lra);
  if (cc != SUCCESS)
  { *lra = slra; return cc; }

  block = page_mark_ref(-upn, ROOT_PAGE);
  w_word(block, i_offs + IE_RP, s_pg(*lra));
  *lra = slra;
{ Int v = r_quad(block, i_offs + IE_VERSION) + 1;
  if ((v & 0xffffff) == 0)
    v = v - 0x1000000;
  w_quad(block, i_offs + IE_VERSION, v);
  return cc;
}}}

public Cc b_replace(upn, ix_ix, key_val, lra)
    Sid	  	upn;
    Squot 	ix_ix;
    Key_t  *key_val;
    Lra_t * lra;      /* input; output = lra deleted */
{ Cc rescc;
  b_key_size = ddict[ix_ix].size_ix;
  b_key_type = ddict[ix_ix].kind_ix;
  b_key_cl   = ddict[ix_ix].cl_ix;
  if (b_key_type & E_NU)
    return ILLEGAL_SRCH;
  wr_lock(upn);
  if (*lra == 0)
    rescc = b_delete(upn, ix_ix, key_val, lra);
  else
  { Byte * p = page_ref(-upn, ROOT_PAGE);
    Lra_t la = 0;
    rescc = b_srch(-upn, r_word(p, ix_offs(ix_ix)+IE_RP), key_val, &la);
    if (rescc <= 0)
      rescc = OK;
    else
    { Offs offs = s_offs(la);
      p = page_mark_ref(-upn, s_page(la));
      if (rescc == 1 and s_mult(r_quad(p, offs)) == 0)
      { w_quad(p, offs, *lra);
        rescc = OK;
      }
      else
      { rescc = b_delete(upn, ix_ix, key_val, &la);
        if (rescc == OK)
        { rescc = b_insert(upn, ix_ix, key_val, lra);
          *lra = la;
        }
      }
    }
  }
  wr_unlock(upn);
  return rescc;
}

			/* b_first() and b_last() functions */

public Cc b_first(upn, ix_ix, key_val, lra)
    Sid	  	upn;
    Squot 	ix_ix;
    Key_t  *key_val;    /* in (default), out */
    Lra_t  *lra;        /* in (default), out */
{ rd_lock(upn);
  cache_lock();
{ fast Byte * p = _page_ref(-upn, ROOT_PAGE);
       Page_nr pg;
       Cc cc = NOT_FOUND;

  b_key_type = ddict[ix_ix].kind_ix;
  b_key_size = ddict[ix_ix].size_ix;

  for (pg  = r_word(p, ix_offs(ix_ix) + IE_RP); 
       pg != 0; pg = r_word(p, BT_BASE))
  { p = _page_ref(-upn, pg);
    cc = OK;
  }
  if (cc == OK)
  { kv_extract(&key_val[0], &p[addr_of_1]);
    *lra = r_quad(p, addr_of_1 + b_key_size) & ~MULT_MASK;
  }
  cache_unlock();
  rd_unlock(upn);
  return cc;
}}


public Cc b_last(upn, ix_ix, key_val, lra)
    Sid	  	upn;
    Squot 	ix_ix;
    Key_t      *key_val;    /* in (default), out */
    Lra_t      *lra;        /* in (default), out */
{ rd_lock(upn);
  cache_lock();
{ fast Byte * p = _page_ref(-upn, ROOT_PAGE);
  fast Page_nr pg;
       Offs offs = 0;

  b_key_type = ddict[ix_ix].kind_ix;
  b_key_size = ddict[ix_ix].size_ix;
  b_key_cl   = ddict[ix_ix].cl_ix;

  for (pg  = r_word(p, ix_offs(ix_ix)+IE_RP); 
       pg != 0; 
       pg = r_word(p, offs+LRA_SIZE))
  { p = _page_ref(-upn, pg);
    offs = gen_addr(p, *(short*)(p+BT_NKEYS)) + b_key_size;
    if (TP(p) == 0)
      break;
  }
  if (offs != 0)
  { kv_extract(&key_val[0], &p[offs - b_key_size]);
    *lra = r_quad(p, offs);
  { Short mult = s_mult(*lra);
    if (mult > 0)
    { *lra = lra_o(*lra & ~MULT_MASK, mult-1);
      key_val->integer += mult;
    }
  }}
  cache_unlock();
  rd_unlock(upn);
  return offs == 0 ? NOT_FOUND : SUCCESS;
}}
 
private Cc b_del(Sid, Page_nr, Key_t *, Lra_t * ) forward;

private Cc nu_delete(ipn, tgt_lra, key_lra_ref)
      Sid     ipn;
      Lra_t  tgt_lra;
      Lra    key_lra_ref;
{ Byte * p = rec_ref_unchecked(ipn, tgt_lra);
  Squot skey_type = b_key_type;
  Short skey_size = b_key_size;
  Lra_t  ext = r_quad(p , 0);
  Page_nr page = s_pg(ext);

  b_key_type = LRA_KEY;
  b_key_size = 0;

{ Cc cc = b_del(ipn, page, (Key)key_lra_ref, key_lra_ref);
  if (cc != SUCCESS)
    i_log(cc, "nu_delete: rec %d lost", b_ix);
  else
  { Byte * block;

    while (page != 0)
    { block = page_ref(ipn, page);
      if (*(short*)(block+BT_NKEYS) != 0)
				break;

    { Page_nr npage = r_word(block, BT_BASE);    
      (void)page_now_free(ipn, page);
      page = npage;
      ext = page;
    }}
    if (page == 0)
    	return i_log(CORRUPT_PF, "nu_delete: extn empty");

    if (*(short*)(block+BT_NKEYS) == 1 and r_word(block,BT_BASE) == 0)
    { ext = r_quad(block, BT_BASE+PTR_SIZE);
      (void)page_now_free(ipn, page);
    } 

    p = page_mark_ref(ipn, s_page(tgt_lra));
    w_quad(p, s_offs(tgt_lra), ext);    
  }
  b_key_type = skey_type;
  b_key_size = skey_size;
  return cc;
}}

			/* pages on either side of a page */
			/* returns offset in the middle */
private Short find_siblings(lr_sib, block_, child_page)
    Page_nr   lr_sib[3];
    Byte *    block_;
    Page_nr   child_page;
{ fast Byte * block = block_;
  fast Short addr = addr_of_1 - PTR_SIZE - LRA_SIZE;
  fast Short i;

  lr_sib[1] = 0;
  lr_sib[2] = 0;
					/* i is mirrored */
  for (i = *(short*)(block+BT_NKEYS); i >= 0; --i)
  { 
    if (child_page == r_word(block, addr + LRA_SIZE))
    { if (i != *(short*)(block+BT_NKEYS))
        lr_sib[1] = r_word(block, addr - b_key_size - PTR_SIZE);
      if (i != 0)
        lr_sib[2] = r_word(block, addr + b_key_size + PTR_SIZE + 2*LRA_SIZE);

      return *(short*)(block+BT_NKEYS) - i + 1;
    }
    addr = next_addr_of(block, addr);
  }
  i_log(0, "Imp Err");
  return -1;
}


#define copy_entry(to_blk, pos, from_blk, pos2) \
 memcpy((to_blk)+gen_addr(to_blk,pos), \
        (from_blk)+gen_addr(from_blk, pos2), b_key_size+LRA_SIZE)


#define leaf_node(block) (r_word(block, BT_BASE) == 0)
		
					           /* result in the stack */
                                                   /* write locks on */
private Bool find_successor(ipn, page, block_, pos)     
    Sid 	  ipn;
    Page_nr page;
    Byte *  block_;
    Short   pos;
{ if (TP(block_) == 0)
    return false;

{ fast Byte * blk = block_;
  fast Page_nr npg = r_word(blk, gen_addr(block_, pos)+b_key_size+LRA_SIZE);
  if (npg == 0)
    return false;

  push_stk_ptr(page);

  for (; npg != 0;
         npg = r_word(blk, BT_BASE))
  { push_stk_ptr(npg);
    blk = page_ref(ipn, npg);
  }

  return true;
}}


private Cc b_del(ipn, root, key_val, lra)
    Sid        ipn;
    Page_nr    root;
    Key_t *    key_val;
    Lra_t     *lra;            /* E_NU: lra = rec to be dltd; out rec dltd */

{ Lra_t d_lra = 0;
  Cc  mult = b_srch(ipn, root, key_val, &d_lra);
  if (mult <= 0)
    return KEY_MISSING;

{ Page_nr cpage = s_pg(d_lra);
  Byte * cpage_p = page_mark_ref(ipn, cpage); 		/* always refers to cpage */
  if (cpage_p == null)
    return PI_MISSING; 

{ Lra_t entry = r_quad(cpage_p, s_offs(d_lra));
  if      (s_offs(entry) == 0)
    return nu_delete(ipn, d_lra, lra);
  else if ((b_key_type & E_NU) and entry != *lra)
    return NOT_FOUND;

  *lra = lra_o(entry & ~MULT_MASK, (mult-1));

{ Short offs = s_offs(d_lra);
  Short pos = addr_to_pos(cpage_p, offs);

  if (s_mult(entry) > 0)
  { Short mult_ = s_mult(entry) + 1;
    if      (mult == 1)
    { w_quad(cpage_p, offs - b_key_size, key_val->integer+1); 
      w_quad(cpage_p, offs,              lra_o(*lra, 1) + (mult_-2)*MULT_UNIT);
      return OK;
    }
    else 
    { entry &= ~MULT_MASK; 
      w_quad(cpage_p, offs,              entry + (mult-2)*MULT_UNIT);
      if (mult_ == mult)
        return SUCCESS;
      key_val->integer += 1;
      entry = lra_o(entry, mult);
    { Cc rcc = b_ins(ipn, root, key_val, mult_-mult, &entry);
      key_val->integer -= 1;
      if (rcc != OK)
      { cpage_p = page_mark_ref(ipn, cpage);
        w_quad(cpage_p, offs,              entry + (mult_-1)*MULT_UNIT);
        return rcc;
      }
      cpage_p = page_mark_ref(ipn, ROOT_PAGE);
      w_word(cpage_p, ix_offs(b_ix) + IE_RP, s_pg(entry));
      return rcc;
    }}
  }
  				/* find successor and swop with target, then */
  if (find_successor(ipn, cpage, cpage_p, pos))	
  {				/* redesignate the successor as the target   */
    cpage_p = page_mark_ref(ipn, cpage);
    if (cpage_p == null)
      return PI_MISSING; 

    cpage = pop_stk_ptr();

  { Byte * succ = page_mark_ref(ipn, cpage);
    if (succ == null)
      return PI_MISSING; 

    copy_entry(cpage_p, pos, succ, 1);
    key_val = &key_value_;
    key_val->string = &key_svalue_[0];
    kv_extract(key_val, &succ[BT_BASE + PTR_SIZE]);
    pos = 1;
  }}

  while (true)
  { Page_nr  p_page, lr_sib[3];

    cpage_p = page_mark_ref(ipn, cpage);
    if (cpage_p == null)
      return PI_MISSING;
					/* remove target; shift down in node */
  { Short min_ = (BLOCK_SIZE-BT_BASE-PTR_SIZE)/
								  (Short)(2 * (b_key_size + LRA_SIZE + TP(cpage_p)));
    Short actual = *(short*)(cpage_p+BT_NKEYS);
    shift_in_node(cpage_p, pos+1, actual--, -1);
    *(short*)(cpage_p+BT_NKEYS) = actual;
  /*printf("#kd %d %x\n", actual, cpage);*/

    p_page = pop_stk_ptr();    	/* get parent node */
																/* do we have underflow in the node? */
    if (p_page == 0 or actual >= min_)
      break;

  { Byte * parent_p = page_mark_ref(ipn, p_page);

    Short p_pos = find_siblings(&lr_sib[0], parent_p, cpage);
    Short sibix;
    
 /* i_log(p_page, "delredist %d", cpage); */
    
    for (sibix = 3; --sibix > 0; )
      if (lr_sib[sibix] != 0)
      { Byte * sibling = page_ref(ipn, lr_sib[sibix]);
				if (sibling == null)
				  return PI_MISSING;

      { Short sib_nr = *(short*)(sibling+BT_NKEYS);
				if (sib_nr > min_)             /* can we redistribute from sibling? */
				{ Short total = actual + sib_nr;
				  Short l_nr = (total + 1) >> 1;
				  Short r_nr = total - l_nr;

				  sibling = page_mark_ref(ipn, lr_sib[sibix]);
				  if (sibix == 2)				/* right sibling */
				  { Short ct = l_nr - actual;
				    actual += 1;				/* add in parent entry*/
				    copy_entry(cpage_p, actual, parent_p, p_pos);
				    if (TP(cpage_p))
				      w_word(cpage_p, gen_addr(cpage_p,actual) + b_key_size + LRA_SIZE,
	             	  			      r_word(sibling, BT_BASE));
	    			move_to_node(cpage_p, actual + 1, sibling, 1, ct-1);
				    copy_entry(parent_p, p_pos, sibling, ct);
            if (TP(cpage_p))
							w_word(sibling, BT_BASE, 
	            		   r_word(sibling, gen_addr(sibling,ct+1) - PTR_SIZE));
				    shift_in_node(sibling, ct+1, sib_nr, -ct);
				    *(short*)(cpage_p+BT_NKEYS) = l_nr;
         /* printf("#k0 %d %x\n", cpage_p[BT_NKEYS], cpage); */
	    			*(short*)(sibling+BT_NKEYS) = r_nr;
         /* printf("#ks %d %x\n", sibling[BT_NKEYS], lr_sib[sibix]); */
				  }
				  else						/* left sibling */
				  { Short ct = r_nr - actual;				/* > 0 */
				    shift_in_node(cpage_p, 1, actual, ct);
            if (TP(cpage_p))
				      w_word(cpage_p, gen_addr(cpage_p,ct+1) - PTR_SIZE, 
	             	                 r_word(cpage_p, BT_BASE));
				    copy_entry(cpage_p, ct, parent_p, p_pos-1);
				    move_to_node(cpage_p, 1, sibling, l_nr + 2, sib_nr);
            if (TP(cpage_p))
				      w_word(cpage_p, BT_BASE, r_word(sibling, gen_addr(sibling,l_nr+2)-PTR_SIZE));
				    copy_entry(parent_p, p_pos-1, sibling, l_nr + 1);
				    *(short*)(cpage_p+BT_NKEYS) = r_nr;
          /*printf("#kA %d %x\n", cpage_p[BT_NKEYS], cpage);*/
				    *(short*)(sibling+BT_NKEYS) = l_nr;
          /*printf("#kB %d %x\n", l_nr, lr_sib[sibix]);*/
			  	}
				  sibix = -1;				/* indicate a break */
			  }
      }}

    if (sibix < 0)
      break;

    for (sibix = 3; --sibix > 0; )
      if (lr_sib[sibix] != 0)			/* can we concatenate ? */
      { Byte * sibling = page_mark_ref(ipn, lr_sib[sibix]);
        if (sibling == null)
				  return PI_MISSING;
							/* sib_nr <= min_ */
      { Short sib_nr = *(short*)(sibling+BT_NKEYS);
				if (sibix == 2)					/* right sibling */
				{ copy_entry(cpage_p, actual+1, parent_p, p_pos);
          if (TP(cpage_p))
				    w_word(cpage_p, gen_addr(cpage_p,actual+2)-PTR_SIZE,
	            			r_word(sibling, BT_BASE));
				  move_to_node(cpage_p, actual+2, sibling, 1, sib_nr);
				  sibling = cpage_p;		/* use cpage */
				  cpage = lr_sib[2];		/* free sibling */
				}
				else
				{ --p_pos;
				  copy_entry(sibling, sib_nr+1, parent_p, p_pos);
          if (TP(cpage_p))
				    w_word(sibling, gen_addr(sibling,sib_nr+2)-PTR_SIZE,
	           			 r_word(cpage_p, BT_BASE));
				  move_to_node(sibling, sib_nr+2, cpage_p, 1, actual);
				}
				*(short*)(sibling+BT_NKEYS) = actual + sib_nr + 1;
      /*printf("#k- %d %x\n", sibling[BT_NKEYS], -1);*/
				(void)page_now_free(ipn, cpage);
				pos = p_pos;
				cpage = p_page;
				break;
      }}
    if (sibix <= 0)
    { i_log(0, "Neither Did");
      return ELOGIC;
    }
  }}}

  return OK;
}}}}

public Cc b_delete(upn, ix_ix, key_val, lra)
    Sid		  upn;
    Squot	  ix_ix;
    Key_t *	key_val;
    Lra_t * lra;							/* in E_NU: rec to be dltd; output rec dltd */
{ Byte * root_p = page_ref(-upn, ROOT_PAGE);
  Short i_offs = ix_offs(ix_ix);

  b_ix = ix_ix;
  b_key_type = ddict[b_ix].kind_ix;
  b_key_size = ddict[b_ix].size_ix;
  b_key_cl   = ddict[b_ix].cl_ix;

  vfy_islocked(upn, 2);

{ Page_nr page = r_word(root_p, i_offs+IE_RP);

  Cc cc = b_del(-upn, page, key_val, lra);
  if (cc < SUCCESS)
    return cc;

  root_p = page_mark_ref(-upn, ROOT_PAGE);
{ Int v = r_quad(root_p, (Int)i_offs + IE_VERSION) + 1;
  if ((v & 0xffffff) == 0)
    v = v - 0x1000000;

  w_quad(root_p, i_offs + IE_VERSION, v);
  page = r_word(root_p, i_offs+IE_RP);
  if (page != 0)
  { Byte * block = page_ref(-upn, page);
    if (*(short*)(block+BT_NKEYS) == 0)
    { w_word(root_p, i_offs+IE_RP, r_word(block, BT_BASE));
      (void)page_now_free(-upn, page);
    }
  }
  return cc;
}}}

typedef struct 
{ Sid     upn;
  Sid     ix_ix;
  Squot   key_typ;
  Int     version;
  Short   ukey_bot;			    /* base of an extender */
  Lra_t   last_lra;
  Key_t   last_key;
  Short   stk_top;
  Page_nr page_stk [ MAX_DEPTH_B_TREE ];   
  Short   ord_stk  [ MAX_DEPTH_B_TREE ];   /* ordinal for next work */
  Short   m_stk    [ MAX_DEPTH_B_TREE ];   /* mult    for next work */
  Char    keystr[BLOCK_SIZE/2];
} Ix_strm_t, *Ix_strm;



public void ix_fini_stream(stream_)  
       Byte * stream_;
{ free(stream_);
}



public Key stream_last_key(strm)
	Byte * strm;
{ return &((Ix_strm)strm)->last_key;
}

/* Work is grouped into a 'frame':
    page, key, lra
*/

#define extender(lra) (s_offs(lra) == 0)
					/* When caled after a version change
					   strm->last_lra is non null */

private Ix_strm new_strm(strm, key_val)
	Ix_strm     strm;
	Key_t *     key_val;
{ Sid ipn = -(strm->upn);
  Offs ioffs = ix_offs(strm->ix_ix);
  Byte * p = _page_ref(ipn, ROOT_PAGE);

  b_key_type = strm->key_typ;
  b_key_size = ddict[strm->ix_ix].size_ix;
  strm->version = r_quad(p, ioffs + IE_VERSION);
  strm->ukey_bot = MAX_DEPTH_B_TREE + 1;

{ Page_nr page = r_word(p, ioffs + IE_RP);
  Short ord = 1;
  Short top = -1;

  for (; page != 0; )
  { p = _page_ref(ipn, page);
  { Cc mult = key_val == null ? 1 : b_pg_srch(p, key_val, 0, &ord);
    strm->page_stk [++top] = page;
    strm->ord_stk  [  top] = ord;
    if ((strm->m_stk [top] = mult-1) < 0)
      strm->m_stk    [  top] = 0;
					  /* the one you have NOT visited */
    if (mult > 0 and key_val != null)
    { /*i_log(0, "Exact");*/
      break;
    }
  /*i_log(top, "NPUSH %d %d", page, ord);*/
    page = 0;
    if (non_leaf(p))
      page = r_word(p, gen_addr(p,ord) - PTR_SIZE);
  }}
  strm->page_stk[++top] = page;    /* say */
  strm->ord_stk [  top] = MAXSINT;  /* say */
  strm->m_stk   [  top] = MAXSINT;

  if (page != 0)			/* Exact */
  { Lra_t ext = r_quad(p, gen_addr(p,ord) + b_key_size);
    if (not extender(ext))
      strm->last_lra = ext - 1;
    else
    { Squot s_kt = b_key_type;
      b_key_type = LRA_KEY;
      b_key_size = 0; 
      --top;
      strm->ukey_bot = top;

      for (page = s_pg(ext); 
				   page != 0; )/* and !break*/
      { p = _page_ref(ipn, page);
      { Cc mult = b_pg_srch(p, &strm->last_lra, 0, &ord); 
				strm->page_stk [++top] = page;
				strm->ord_stk  [  top] = ord;
				strm->m_stk    [  top] = 0;

				if (mult > 0)
				  break;
        page = 0;
        if (non_leaf(p))
          page = r_word(p, gen_addr(p,ord) - PTR_SIZE);
      }}

      strm->page_stk[++top] = page;
      strm->ord_stk [  top] = MAXSINT;
      strm->m_stk   [  top] = MAXSINT;

      b_key_type = s_kt;
    }
  }

  strm->stk_top = top;
  return strm;
}}

public Byte * b_new_stream(upn, ix_ix, key_val)
	Sid       upn;
	Squot     ix_ix;
	Key_t *   key_val;
{ Ix_strm strm   = (Ix_strm_t *)malloc(sizeof(Ix_strm_t));
  strm->upn      = upn;
  strm->ix_ix    = ix_ix;
  strm->last_lra = 0;
  strm->key_typ  = ddict[ix_ix].kind_ix;
  strm->last_key.integer = 0;
  if ((strm->key_typ & (UCKEY-1)) != FXD_STR)
  { if (key_val != null)
      strm->last_key = *key_val;
  }
  else 
  { Short l = ddict[ix_ix].size_ix;
    strm->last_key.string = strm->keystr;
    strpcpy(strm->last_key.string, key_val == null ? "" : key_val->string, l);
  } 
  rd_lock(upn);
  cache_lock();
{ Ix_strm res = new_strm(strm, key_val);
  cache_unlock();
  rd_unlock(upn);
  return (Byte *)res;
}}


static Byte nrbuff[BLOCK_SIZE];


public Cc next_of_ix (stream_, key_val, lra)
	Byte *  stream_;		/* strm->upn at least read locked */
	Key_t * key_val;
	Lra_t * lra;
{ 
	fast Ix_strm strm = (Ix_strm)stream_;
       Sid   upn = strm->upn;

  b_key_type = strm->key_typ;
  b_key_cl   = ddict[strm->ix_ix].cl_ix;
  cache_lock();

{      Byte * p = _page_ref(-upn, ROOT_PAGE);
  fast Int version = r_quad(p, ix_offs(strm->ix_ix) + IE_VERSION);

  if (version != strm->version)
  { strm->version = version;
    if (strm->stk_top != -1)
    { strm->last_lra += 1;
      i_log(strm->ix_ix, "Version chg");
#if 0
      if ((b_key_type & (UCKEY-1)) == FXD_STR)
				i_log(4, "Version CHG %s", key_val->string);
#endif
      kv_extract(key_val,
							   (b_key_type & (UCKEY-1)) != FXD_STR ? (Char*)&strm->last_key
																							       : strm->last_key.string);
      strm = new_strm(strm, key_val);
      cache_unlock();
      (void)next_of_ix(strm, key_val, lra);
      cache_lock();
    }
  }
#define top version

  while ((top = strm->stk_top) >= 0)
  { Page_nr pg = strm->page_stk[top];
    Short ord  = strm->ord_stk[top];
    Short ext_line = top - strm->ukey_bot;

    p = _page_ref(-upn, pg);
    if (ord > *(short*)(p+BT_NKEYS))
    { strm->stk_top -= 1;
      continue;
    }
    b_key_size = p[BT_KEY_SIZE];
  { Short offs = gen_addr(p, ord) + b_key_size; /* care: uses b_key_size! */
    Short mstk = strm->m_stk [top];
    Lra_t tgt = r_quad(p, offs);

    if (b_key_size > 0)
    { kv_extract(key_val, &p[offs - b_key_size]);
      if ((b_key_type & (UCKEY - 1)) == INTEGER)
      { key_val->integer += mstk;
      }
    }
    
    if (extender(tgt) and ext_line != 0)	/* an uninhibited extender */
    { strm->ukey_bot = top;
      pg = s_pg(tgt);				/* go down it */
    }
    else
    { pg = 0;		
      strm->m_stk [top] += 1;
      if (strm->m_stk [top] > s_mult(tgt))
      { strm->m_stk [top] = 0;
        strm->ord_stk [top] += 1;
        if (not is_leaf(p))			/* go down RHS */
				  pg = r_word(p, offs + LRA_SIZE);
        if (ext_line == 0)
          strm->ukey_bot = MAX_DEPTH_B_TREE + 1;
      }
    }
    while (pg != 0)
    { strm->stk_top = ++top;
      strm->page_stk[top] = pg;      
      strm->ord_stk [top] = 1;
      strm->m_stk   [top] = 0;
      p = _page_ref(-upn, pg);
      pg = r_word(p, addr_of_1 - PTR_SIZE);
      if (pg != 0 and TP(p) == 0)
      { i_log(p[SEG_TYPE], "Page in leaf");
        break;
      }
    }
    if (not extender(tgt))
    { tgt = lra_o(tgt & ~MULT_MASK, mstk);
      strm->last_lra = tgt;
      *lra = strm->last_lra;
      cache_unlock();
      if ((b_key_type & (UCKEY-1)) != FXD_STR)
        strm->last_key = *key_val;
      else 
				strpcpy(strm->last_key.string, key_val->string,
																			 ddict[strm->ix_ix].size_ix);
      return OK;
    }
  }}
  cache_unlock();
  return NOT_FOUND;
#undef top
}}


public Byte * ix_next_rec(stream, lra_ref, r_ref)
	Byte *     stream;
	Lra_t     *lra_ref;
	void      *r_ref;
{ 
	Id upn = ((Ix_strm)stream)->upn;
  if (lra_ref == null)
    lra_ref = (Lra)&nrbuff[0];
  rd_lock(upn);
{ Cc cc = next_of_ix(stream, &((Ix_strm)stream)->last_key, lra_ref);
  if (cc != SUCCESS)
  { rd_unlock(upn); return null;
  }
{ Id cl = ddict[((Ix_strm)stream)->ix_ix].cl_ix;
  Byte * s = page_ref(upn, s_pg(*lra_ref));
  if (r_ref == null)
    r_ref = &nrbuff[0];
  if (s_sgrp(s) != cl)
  { i_log(((Ix_strm)stream)->ix_ix, "*loose sindex %x page %d", 
                                         *s, s_pg(*lra_ref));
    r_ref = null;
  }
  else 
    memcpy(r_ref, &s[s_offs(*lra_ref)], ddict[cl].size_cl);
  rd_unlock(upn);
  return (Byte*)r_ref;  
}}}

private Cc delall(ipn, page, depth)
    Sid      ipn;
    Page_nr page;
    Sid     depth;
{ Short skey_size = b_key_size;
  Byte * block = page_mark_ref(ipn, page);
  if (block == null)
  	return i_log(ANY_ERROR, "delall: can't find page");

  if (depth > 50)
  	return i_log(-9, "Cycle in Index");

  b_key_size = block[BT_KEY_SIZE];

{ Short actual = *(short*)(block+BT_NKEYS);
  Short addr = addr_of_1;
  Cc cc = SUCCESS;
  Page_nr page_ = r_word(block, BT_BASE);
  if (page_ != 0)
    cc = delall(ipn, page_, depth+1);

  for (; actual > 0; --actual)
  { block = page_ref(ipn, page);

  { Int ext = r_quad(block, addr + b_key_size);
    if (s_offs(ext) == 0)
      cc |= delall(ipn, s_pg(ext), depth+1);

    if (block[SEG_TYPE] & 3)
    { Short page__ = r_word(block, addr + b_key_size + LRA_SIZE);
      if (page__ != 0)
        cc |= delall(ipn, page__, depth+1);
    }
    addr = next_addr_of(block, addr);
  }}
  b_key_size = skey_size;
  return cc | page_now_free(ipn, page);
}}


public Cc b_delall(upn, ix_ix)
	Short    upn;
	Short    ix_ix;     
{ wr_lock(upn);
  Byte * p = page_ref(-upn, ROOT_PAGE);
{ Cc cc = SUCCESS;
  Page_nr root = r_word(p, ix_offs(ix_ix) + IE_RP);
  if (root != 0)
  { cc = delall(-upn, root, 0);
    p = page_mark_ref(-upn, ROOT_PAGE);
    w_word(p, ix_offs(ix_ix) + IE_RP, 0);
  }
  wr_unlock(upn);
  rpc_return(cc, cc);
}}

//#define DUMP_INDEX

#ifdef DUMP_INDEX

private Cc b_dumpix(ipn, page, level)
    Sid	  	ipn;			/* locked */
    Page_nr	page;
    Int         level;

{ fast Short addr;

{ fast Byte * block = _page_ref(ipn, page);
  fast Int u = *(short*)(block+BT_NKEYS) & 0x7fff;

  fast Int l;
			      /* decision on key type hoisted from kv_compare */
  if ((b_key_type & (UCKEY - 1)) == INTEGER)  
    for (l = 0; ++l <= u; )
    { addr = gen_addr(block, l);
    { Int mem = r_quad(block, addr);
      Int mul = s_mult(r_quad(block, addr+sizeof(Int))) + 1;
      Int npage = r_word(block, addr - PTR_SIZE);
      if (npage != 0)
      { Cc cc = b_dump_ix(ipn, npage, level+1);
      }
      printf("%4d %d %d\n", level,mem,mul);
    } 
  else 
    for (l = 0; ++l <= u; )
    { addr = gen_addr(block, l);
      Int npage = r_word(block, addr - PTR_SIZE);
      if (npage != 0)
      { Cc cc = b_dump_ix(ipn, npage, level+1);
      }
      printf("%4d Key\n", level);
    }
  }

  return stk_mix == null ? 0 : -1;
}}


private void b_dump_index(upn, ix_ix)
    Sid	  	upn;			/* locked */
    Squot 	ix_ix;
{ 
  cache_lock();			/* it uses _page_ref not page_ref !! */

{ b_key_type = ddict[ix_ix].kind_ix;
  b_key_size = ddict[ix_ix].size_ix;
  b_key_cl = ddict[ix_ix].cl_ix;

{ Byte * p = page_ref(-upn, ROOT_PAGE);
  b_dumpix(-upn,(Int)r_word(p, ix_offs(ix_ix)+IE_RP), 0);

  cache_unlock();
}}}

#endif
