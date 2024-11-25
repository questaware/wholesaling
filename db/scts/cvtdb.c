#include  <string.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <signal.h>
#include	"version.h"
#if XENIX
#include  <sys/ascii.h>
#endif
#include	"../../h/defs.h"
#include	"../../h/errs.h"
#include	"../../db/page.h"
#include	"../../db/recs.h"
#include	"../../db/memdef.h"
#include	"../../db/cache.h"
#include        "../../db/b_tree.h"
#include	"schema.h"
#include	"domains.h"
#include        "../../h/prepargs.h"
#include        "cvt.h"

extern Bool cvt_sho_page;
extern Int lk_base;

char msg[100];			/* to resolve the symbol */
char ths[108];			/* ditto */

const Char nulls[BLOCK_SIZE];
Char dbg_file_name[50]; 

#define PRT_WDTH 80

static Int prt_col = 0;


#define DQUOTE 0x22
#define BSLASH 0x5c


#define DBASCIO 1

#define offsetof(sct, fld) ((Int)&(sct).fld - (int)&(sct))

#include "dump.c.c"

extern Char * getenv();

int tcapclose()

{ 
}


void softalarm()

{ /* to resolve the symbol */
}



void softfini()

{ /* to resolve the symbol */
}


public Cc softterm(n)

{ alarm(0);
  signal(SIGTERM, softterm);
  signal(SIGALRM, softterm);

  if (n == 0)
    return OK;

   alarm(10);

{ Cc cc = cache_term();
  if (cc == OK)
  { fini_log(false);
    exit(1);
  }
}}



Short r_ix(upn, cl, di_Type, r_Type)
         Sid    upn;
         Sid    cl;
         void  (*di_Type)();
         void  (*r_Type)();
{ di_Type(upn);

{ Char * ustrm = init_brec_strm(upn, cl);
  Page_nr page;
  Offs offs;
  Short ct = 0;
  Char * p;

  wr_lock(upn);

  while (next_brec(ustrm, &page, &offs, &p) >= OK)
  { Lra_t lra = to_lra(page, offs);
    fprintf(stderr, ".");
    r_Type(upn, &lra, p);
    ct += 1;
  }
    
  wr_unlock(upn);

  fini_brec_strm(ustrm);
  return ct;
}}


Short r_ix_all(Sid upn)
{ 
  Int cl_ix;
  wr_lock(upn);
#if 0
  for (cl_ix = CL_stt - 1; ++cl_ix <= CL_end; )
    (di_Type_arr[cl_ix])(upn);
#endif
{ Char * ustrm = init_brec_strm(upn, 0);
  Page_nr page;
  Offs offs;
  Short ct = 0;
  Char * p;

  while ((cl_ix = next_brec(ustrm, &page, &offs, &p)) > OK)
  { Lra_t lra = to_lra(page, offs);
    fprintf(stderr, " %d", cl_ix);
    if (cl_ix > CL_end)
      i_log(cl_ix, "Ix OOR");
    (r_Type_arr[cl_ix])(upn, &lra, p);
    ct += 1;
  }
    
  wr_unlock(upn);

  fini_brec_strm(ustrm);
  return ct;
}}


static Id upn;

void closedown()

{ chan_unlink(upn);
  fini_cache();
  exit(1);
}



static void report_sizes(set)
	Set   set;
{ Int ix = 1;
  if (set == 0)
    fprintf(stderr, "All records are of correct size\n");
  else
  { Set top = set & 0x80000000;
    set &= 0x7fffffff;
    fprintf(stderr, "The following %s records were the wrong size\n",
             top ? "IX_" : "CL_"
           );
    for ( ; set != 0; ++ix)
    { if (set & 1)
				fprintf(stderr, " %d ", ix);
      set = set >> 1;
    }
    fprintf(stderr, "\n");
  }
}


static Cc upd_class_size(upn, cl)
    Sid   upn;
    Short cl;
{ Short c_offs = class_offs(cl);
  if (c_offs >= BLOCK_SIZE)
    return -1;
{ Byte * p = page_ref(upn, ROOT_PAGE);
  if (r_word(p, c_offs + CE_RP) != 0)
    return HALTED;
  wr_lock(upn);
{ Byte * p = page_mark_ref(upn, ROOT_PAGE);
  
  w_word(p, c_offs + CE_RP_SIZE, ddict[cl].size_cl);
  wr_unlock(upn);
  return OK;
}}}



static void upd_class_sizes(upn, set)
	Id   upn;
	Set  set;
{ Int ix = 1;
  if (set & 0x80000000)
    return;			/* we dont update index sizes */

  for ( ; set != 0; ++ix)
  { if (set & 1)
    { Cc cc = upd_class_size(upn, ix);
      if (cc != OK)
        fprintf(stderr, "The CL_ class %d is wrong and non empty\n");
    }
    set = set >> 1;
  }
  fprintf(stderr, "\n");
}


/* private */ Cc streamtree(ipn, page, indent)
    Sid     ipn;
    Page_nr page;
    Short   indent;
{ Char * strm_a = ix_new_stream(ipn, page, null);
  Lra_t  lra_a;
  Char   buf[512];
  Char * sale;

  while ((sale = (Char *)ix_next_rec(strm_a, &lra_a, &buf[0])) != null)
  { printf("K %d\n", stream_last_key(strm_a)->integer);
  }

  ix_fini_stream(strm_a);
}

void explain()

  { fprintf(stderr,
    "cvtdb {-i} {-w | -d | } {-f n} {-b n} {-r} dbname {-k structure} {input}\n"
    " -b n   -- Dump index in ascii\n" 
    " -B n   -- Dump index in ascii (using streaming)\n" 
    " -C n   -- Dump page of DB\n"
    " -c n   -- Dump page of DB index\n"
    " -i     -- must not exist (-w: db, -r: index)\n"
    " -w     -- write to dbname\n"
    " -d     -- delete structure\n" 
    " -v     -- verify record sizes\n"
    " -u     -- update record sizes\n"
    " -V     -- verify record and key sizes\n"
    " -f n   -- from integer key value n\n" 
    " -t n   -- to integer key value n\n"
    " -k sct -- Structure to deal with\n"
    " -r     -- reindex\n"
    " -s     -- Special\n"
    " -T     -- Test\n"
    " -K     -- Scan\n"
    " -p     -- Profile the indeces\n"
    " -P     -- Show page numbers\n"
    " -X     -- Try to use the unique integer index for output order\n"
    " where structure may be Invoice_t for invoices\n\n"
    " Shared memory at %x\n",lk_base);
    exit(0);
  }

Bool i_opt;
Bool t_opt;
Bool k_opt;
Bool w_opt;
Bool d_opt;
Bool v_opt;
Bool u_opt;
Bool r_opt;
Bool s_opt;
Bool p_opt;
Bool x_opt;


Int d_tree = 0;
Int f_val = -1;
Int t_val = -1;
Int blk_ix = 0x7fffffff;

Char * sctid_ = null;


static Char fnbuf[256];


public Id last_id(upn, ix_ix)
      Id    upn;
      Id    ix_ix;
{ Lra_t lra;
  Key_t  kvi;  kvi.integer = 0;
 (void)ix_last(upn, ix_ix, &kvi, &lra);
  return kvi.integer <= 0 ? 0 : kvi.integer;
}


void main (argc,argv)
	Short argc;
        Char ** argv;
{ Char * dbfn = null;
  Char * inputfn = null;
  Char sctid[50];

  Bool didfile = false;

  Set props = F_EXISTS;

  Int argsleft = argc;
  Char ** argv_ = &argv[0];
  Char * user = getenv("USER");

  lk_base += strcmp(user,"osdb") == 0  ?  0 :
             strcmp(user,"osdb2") == 0 ? 20 :
             strcmp(user,"peter") == 0 ? 40 : 60;
  
  for ( ; --argsleft >= 0; )
  { ++argv_;
    
    if      (argsleft <= 0)
    { if (didfile)
        break;
    }
    else if (argv_[0][0] != '-')
    { if (dbfn == null)
        dbfn = argv_[0];
      else
        inputfn = argv_[0];
      didfile = true;
    }
    else
    {Char * iarg = &argv_[0][0];
     Char opt; 
     while ((opt = *++iarg) != 0)
     {if (opt == 'i') 
      	i_opt = true;
      else if (opt == 'w')
      	w_opt = true;
      else if (opt == 'd')
      	d_opt = true;
      else if (opt == 'v')
      	v_opt = true;
      else if (opt == 'u')
      { u_opt = true;
        props |= F_WRITE;
      }
      else if (opt == 'V')
      { v_opt = true;
        props |= F_IXOPT;
      }
      else if (opt == 'r')
      	r_opt = true;
      else if (opt == 's')
      	s_opt = true;
      else if (opt == 'T')
      	t_opt = true;
      else if (opt == 'K')
      	k_opt = true;
      else if (opt == 'p')
      	p_opt = true;
      else if (opt == 'P')
      	cvt_sho_page = true;
      else if (opt == 'X')
        x_opt = true;
      else if (opt == 'b')
      { ++argv_;
        --argsleft;
      	d_tree = atol(argv_[0]);
      }
      else if (opt == 'B')
      { ++argv_;
        --argsleft;
      	d_tree = -atol(argv_[0]);
      }
      else if (opt == 'c')
      { ++argv_;
        --argsleft;
      	blk_ix = -atol(argv_[0]);
      }
      else if (opt == 'C')
      { ++argv_;
        --argsleft;
      	blk_ix = atol(argv_[0]);
      }
      else if (opt == 'f')
      { ++argv_;
        --argsleft;
      	f_val = atol(argv_[0]);
      }
      else if (opt == 't')
      { ++argv_;
        --argsleft;
      	t_val = atol(argv_[0]);
      }
      else if (opt == 'k')
      { ++argv_;
        --argsleft;
      	sctid_ = argv_[0];
      }
      else
        explain();
     }
    }
  }

  if (not didfile)
    explain();

  if (w_opt and r_opt)
  { fprintf(stderr, "Not both writing and reindexing");
    exit(0);
  }

  if      (t_val >= 0)
  { while (true)
    { Char buf[25];
      printf(">>");
      rd_hex(10, &buf[0]);
      if (buf[0] = 0)
        break;
      prt_hex(buf, 10);
    }
  }

  if (inputfn != null)
    (void)freopen(inputfn, "r", stdin);
    
  signal(SIGINT, closedown);
  signal(SIGTERM, closedown);

  int ct = init_cache(null);
  if (ct != 1)
  { fprintf(stderr, "init_cache failed %d", ct);
    exit(1);
  }

  flip_vfywr();
  strcpy(&dbg_file_name[0],"cvtlog");
  if (dbfn == null or dbfn[0] == 0)
    explain();
  if (r_opt and sctid_ == null)
    unlink(strcat(strcpy(&fnbuf[0], dbfn), "_"));

  if (w_opt or d_opt or t_opt)
  { props |= F_WRITE+F_IXOPT+F_WRIX;
    if (i_opt)
    { props |=  (F_NEW+F_NEWIX);
      props &= ~F_EXISTS;
    }
  }

  if (r_opt)
  { props |= F_WRITE+F_IXOPT+F_WRIX;
    if (i_opt)
    { props |=  F_NEWIX;
    }
    else
    { Char fnmix[80];
      Char * fnmx_ = strcat(strpcpy(&fnmix[0],dbfn,sizeof(fnmix)-1),"_");
      FILE * e = fopen(fnmx_, "rw");
      if (e != null)
        fclose(e);
      else
      { fprintf(stderr, "Index %s not there or unsuitable\n", fnmx_);
        exit(1);
      }
    }
  }
  if (d_tree != 0 or blk_ix < 0 or x_opt)
    props = F_IXOPT;

  upn = init_db(dbfn, props | F_VERBOSE);
  if (upn < 0)
  { fini_cache();
    exit( i_log(upn,"init_db failed") );
  }

  if (v_opt)
    report_sizes(vfy_roottbl(upn, props & F_IXOPT));

  if (u_opt)
  { Set set = vfy_roottbl(upn, props & F_IXOPT);
    if (set != 0)
    { report_sizes(set);
      upd_class_sizes(upn, set);
    }
    return;
  }
  if      (d_tree != 0)
  { setbuf(stdout, 0);
    if (d_tree > 0)
      dumptree(upn, d_tree);
    else
      streamtree(upn, -d_tree);
  }
  else if (blk_ix != 0x7fffffff)
  { Id u = upn;
    if (blk_ix < 0)
    { u = -upn;
      blk_ix = -blk_ix;
    }
    prt_cpage(u, blk_ix);
  }
  else if (r_opt)
#if 1
  { if (sctid_ != null)
      rbld_db(upn, sctid_);
    else
      r_ix_all(upn);
  }
#else
  { if (sctid_ == null)
    { Cc cc = init_ix(upn);
      if (cc != OK)
        closedown();
    }
    rbld_db(upn, sctid_);
  }
#endif
  else if (d_opt)
  { if (sctid_ == null)
      fprintf(stderr, "structure not specified\n");
    else
    { Cc cc = d_db_class(upn, sctid_);
      if (cc != OK)
        fprintf(stderr, "Error (%d)\n", cc);
    }
  }
  else if (t_opt)
  { Sale_t sale;
    Key_t kv_t;
    Lra_t lra;
    int i;
    for (i = 0; ++i <= 1000000; )
    { wr_lock(upn);
      sale.id = last_id(upn, IX_SALE) + 1;
    { Cc cc = new_init_rec(upn, CL_SALE, &sale, &lra);
      if (cc != OK)
      { fprintf(stderr,"CrCc %d at %d\n",cc,i);
        break;
      }
      kv_t.integer = sale.id;
    { Cc cc = ix_insert(upn, IX_SALE, &kv_t, &lra);
      if (cc != OK)
      { fprintf(stderr,"InsCc %d at %d\n",cc,i);
        break;
      }
      wr_unlock(upn);
      
      Int all_ct;
      Int ct = count_busys(&all_ct);
      if (ct > 0)
        printf(stderr,"Got busy %d\n", ct);
      
      if (i > 1000000 - 16)
      { fprintf(stderr,"Sleep\n");
        sleep(2);
      }
    }}}
  }
  else if (not w_opt)
    o_db(x_opt ? -upn : upn, sctid_);
  else 
  { 
//  wr_lock(upn);
//  flip_writedelay();
    i_db(upn);
    
    Int ch = rd_past();
    
//  flip_writedelay();
//  wr_unlock(upn);
    
    dest_db(upn);

    if (ch != EOF)
    { Char line[140];
      Short i;
      i_log(0, "Stopped prematurely, next 10 lines:");
      for (i = 10; i > 0 and fgets(line, 140, stdin) != null; --i)
        fprintf(stderr, "%s", line);
    }        
  }

{ extern int hwm_pages;
  fprintf(stderr,"Hwm %d\n", hwm_pages);
}
  chan_unlink(upn);
  
  fini_cache();
  exit(0);
}

