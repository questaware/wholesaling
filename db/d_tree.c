#include	<stdio.h>
#include	"version.h"
#include	"../h/defs.h"
#include	"page.h"
#include	"recs.h"
#include	"memdef.h"
#include	"cache.h"
#include	"b_tree.h"
#include	"../h/errs.h"


extern Char dbg_file_name[];

static FILE * dbg_file;

#define s_pg(lra) (((lra) >> (LOG_BLOCK_SIZE)) & 0xffff)
#define s_mult(lra) (((lra) >> (LOG_BLOCK_SIZE+16)) & MAX_MULT)
#define addr_of_1  (BT_BASE+PTR_SIZE)
#define MAX_MULT ((1L << (16-LOG_BLOCK_SIZE))-1)
#define MULT_MASK ~((1L << (LOG_BLOCK_SIZE + 16))-1)

#define next_addr_of(p, pos) ((pos)+b_key_size+LRA_SIZE+TP(p))

#define TP(blk) (blk[SEG_TYPE] & 3)

static Short  b_ix;
static Skind  b_key_type;
static Short  b_key_size;
static Short  b_key_cl;		/* record the index indexes */


private void indt(i)
    Short i;
{ if (dbg_file != null)
    while (--i >= 0)
      fprintf(dbg_file," ");
}

/* private */ Cc dtree(ipn, page, indent)
    Sid     ipn;
    Page_nr page;
    Short   indent;
{ Byte * block = page_ref(ipn, page);

{ Page_nr page_ = r_word(block, BT_BASE);
  Short actual = *(short*)(block+BT_NKEYS);
  Short j, addr = addr_of_1;

  Short skey_size = b_key_size;  
  b_key_size = block[BT_KEY_SIZE];

  if (dbg_file == null)
    return -1;

  indt(indent);
  fprintf(dbg_file, "Page %ld(%d) ct %ld\n", 
               page, block[SEG_TYPE], actual);

  if (s_stype(block) != INDEX_SEG and s_stype(block) != INODE_SEG)
  { 
    fprintf(dbg_file, "dtree: wr type %x pg %d", block[SEG_TYPE], page);
    return -1;
  }

  if (page_ != 0)
    dtree(ipn, page_, indent+1);

  for (; actual > 0; --actual)
  { block = page_ref(ipn, page);
  { Lra_t lra = r_quad(block, addr + b_key_size);
    Int offs = s_offs(lra);
    Short mult = s_mult(lra);
    lra &= ~ MULT_MASK;

    indt(indent);
    if (b_key_size <= 4)
      fprintf(dbg_file, b_key_size==0 ? "Lra K %lx"  :
                        mult != 0     ? "K %ld (%d)" :
                                        "K %ld",   r_quad(block,addr), mult+1);
    else
    { fprintf(dbg_file,"Key ");
      for (j = 0; j < b_key_size and block[addr+j] != 0; ++j)
	fputc(block[addr+j], dbg_file);
    }
    if      (offs == 0)
    { fprintf(dbg_file,"\n");
      dtree(ipn, s_pg(lra), indent+1);
    }
    else 
    { block = page_ref(-ipn, s_pg(lra));
      if (s_sgrp(block) != ddict[b_ix].cl_ix)
				fprintf(dbg_file, "Type is %d sb %d %d", s_sgrp(block), b_ix,
					     ddict[b_ix].cl_ix);
      if (rec_is_free(block, offs))
				fprintf(dbg_file, " (%d ERR: Index to free %x)", s_pg(lra), block[RP_FREEVEC0]);

      if (b_key_size != 0)
        fprintf(dbg_file," %lx\n", lra);
      else
        fprintf(dbg_file, " Id is %x", r_quad(block, offs));
    }
    block = page_ref(ipn, page);
    if (is_leaf(block))
      addr = next_addr_of(block, addr);
    else
    { Short page__ = r_word(block, addr + b_key_size + LRA_SIZE);
      addr = next_addr_of(block, addr);
      if (page__ != 0)
        dtree(ipn, page__, indent+1);
    }
  }}
  if (b_key_size == 0)
    fprintf(dbg_file,"\n");

  b_key_size = skey_size;  
}}


public void dumptree(upn, ix)
    Sid     upn;
    Sid     ix;
{ dbg_file = dbg_file_name[0] == 'c' and 
             dbg_file_name[1] == 'v' ? stdout : fopen(dbg_file_name, "a");

  if (dbg_file != null)
  { Byte * p = page_ref(-upn, ROOT_PAGE);
    Page_nr page = r_word(p, ix_offs(ix) + IE_RP);
    if (page == 0)
      fprintf(dbg_file, "Tree is Empty\n");
    else 
    { b_ix = ix;		/* misuse b_key_type */
      b_key_type = ddict[b_ix].kind_ix;
      (void)dtree(-upn, page, 0);
    }
    if (dbg_file != stdout)
      fclose(dbg_file);
  }
}	  


