#include  <stdio.h>
#include  <string.h>
#include  <unistd.h>
#include  "version.h"
#include  "../h/defs.h"
#include  "page.h"
#include  "memdef.h"
#include  "cache.h"
#include  "../h/errs.h"


Szdcr_t ddict[MAX_BOTH];


#define PAGE_SZ 1

public Cc get_new_page(upn, type, seg_grp, page_nr_ref, blk_ref)
    Sid       upn;               /* write locked */
    Squot     type;
    Short     seg_grp;
    Int      *page_nr_ref;  
    void *    blk_ref;
{ Byte * mi = page_mark_ref(upn, ROOT_PAGE);

    fast Page_nr fpage = r_word(mi, MI_FREE_LIST);
    fast Byte * p;
         Int next;
					/* first try MI_FREE_LIST */
  if (fpage != 0)
  { p = page_mark_ref(upn, fpage);
    next = r_word(p, FL_NEXT);
    if (p[SEG_TYPE] != FREE_LIST_SEG)
      i_log(p[SEG_TYPE], "Data in free-list");
    w_word(mi, MI_FREE_LIST, next);
    if (fpage > r_word(mi, MI_FIRST_UNUSED))
      i_log(upn, "Fl %d FU %d", fpage, r_word(mi, MI_FIRST_UNUSED));
    memset(&p[0], 0, BLOCK_SIZE);
  }
  else					/* use M_FIRST_UNUSED */
  { Int sz = r_word(mi, MI_FILE_SZ);
    Int asz = chan_filesize(upn);
    fpage = r_word(mi, MI_FIRST_UNUSED);
    next =  r_word(mi, MI_FILE_SZ);
    if (next != asz)
      return i_log(upn, "Page 0 Corr %d %d", next, chan_filesize(upn));

    if (fpage > 0xffff)
      return TOO_BIG;

    if (fpage >= sz)
    { Short fdesc = chan_fileno(upn);
      if (fdesc < 0)
        return i_log(false, "lost db fd");
			      /* i_log(upn, "Bigger DB %x", fpage); */
      next = extend_file(fdesc, fpage);
      if (next == 0)
      { Short i;
        Char ln[280];
        sprintf(&ln[0], "rm %s/balast", COMDIR);
        system(&ln[0]);
        i_log(DISK_SPACE, "Disk Space Needed");
        for (i = 10; --i > 0;)
        { fprintf(stderr, "FREE UP DISK SPACE\n");
      	  sleep(20);
      	  next = extend_file(fdesc, fpage);
      	  if (next != 0)
      	    break;
        }
        if (next == 0) 
      	  exit(i_log(DISK_SPACE, "Disk Space Panic"));
      }
      w_word(mi, MI_FILE_SZ, fpage + next);
      set_db_size(upn, fpage + next);
    }

    w_word(mi, MI_FIRST_UNUSED, fpage + PAGE_SZ);

    p = page_mark_ref(upn, fpage);
  }

  p[SEG_TYPE] = mk_type_grp(type, seg_grp);
  *(Byte**)blk_ref = p;
  *page_nr_ref = fpage;

  return OK;
}

public Cc page_now_free (upn, page_nr)
    Sid         upn;
    Page_nr     page_nr;
{      Byte * mi = page_mark_ref(upn, ROOT_PAGE);
  fast Byte * block = page_mark_ref(upn, page_nr);

  if (cache_err != OK)
    return CORRUPT_DB;
  if (block[SEG_TYPE] == FREE_LIST_SEG)
    return CORRUPT_DB;

  block[SEG_TYPE] = FREE_LIST_SEG;
{ Short nxt = r_word(mi, MI_FREE_LIST);
  fast Byte * b;
  if (nxt == page_nr)
    i_log(page_nr, "PNF loop");
  w_word(block, FL_NEXT, nxt);
  for (b = &block[FL_NEXT+2]; b < &block[BLOCK_SIZE]; ++b)
    *b = 0;

  w_word(mi, MI_FREE_LIST, page_nr);
  return OK;
}}

public Cc init_roottbl(upn)
    Sid   upn;
{ Byte block[BLOCK_SIZE];
  fast Byte * p;
  for (p = &block[BLOCK_SIZE]; --p >= block; )
    p[0] = 0;

{ Int ext = extend_file(chan_fileno(upn), 0);
  if (ext == 0)
    return -1;
  
  block[SEG_TYPE]  = M_INDEX_SEG;

  w_word(block, MI_FIRST_UNUSED, 1);
  w_word(block, MI_FILE_SZ, ext);
  set_db_size(upn, (Int)ext);  

  write_page(chan_fileno(upn), ROOT_PAGE, block);
{ Byte * p = page_mark_ref(upn, ROOT_PAGE);

  memcpy(p, block, BLOCK_SIZE);
  return OK;
}}}



public Set vfy_roottbl(upn, vfyix)
    Sid   upn;
    Bool vfyix;
{ Byte * p = page_ref(upn, ROOT_PAGE);
  Set res = 0;
  Short cl;

  for (cl = 0; ++cl < MAX_CLASS; )
  { Short c_offs = class_offs(cl);
    if (c_offs > BLOCK_SIZE)
      break;
    if (p[c_offs + CE_RP_SIZE] != ddict[cl].size_cl)
      res |= (1 << (cl - 1));
  }
  if (vfyix and res == 0)
  { p = page_ref(-upn, ROOT_PAGE);

    for (cl = 0; ++cl < MAX_IX; )
    { Short c_offs = ix_offs(cl);
      if (c_offs > BLOCK_SIZE)
				break;
      if (p[c_offs + IE_RP_SIZE] != ddict[cl].size_ix)
				res |= (1 << (cl - 1)) | 0x80000000;
    }
  }
  
  return res;
}

#if 0
X
Xpublic Cc load_roottbl(upn, chk)
X	Sid   upn;
X	Bool  chk;
X{ Byte * p = page_mark_ref(upn, ROOT_PAGE);
X  if (cache_err != OK)
X    return CORRUPT_PF;
X
X  i_log(0, "load_roottbl called");
X
X{ Short mi_top = BLOCK_SIZE;
X  Short cl, ix;
X
X  for (cl = 0; ++cl < MAX_CLASS; )
X  { Short c_offs = class_offs(cl);
X    if (c_offs > mi_top)
X      break;
X  { Short sz = r_word(p, c_offs + CE_RP_SIZE);
X    if (chk and ddict[cl].size_cl != sz)
X      return CORRUPT_PF;
X    ddict[cl].size_cl = sz;
X  }}
X
X  p = page_ref(-upn, ROOT_PAGE);
X  if (cache_err != OK)
X    return CORRUPT_PF;
X
X  for (ix = 1; ix <= MAX_IX; ++ix)
X  { Short i_offs = ix_offs(ix);
X    if (i_offs > mi_top)
X      break;
X  { Short sz = r_word(p, i_offs + IE_CL);
X    if (chk and ddict[ix].cl_ix != sz)
X      return CORRUPT_PF;
X    ddict[ix].cl_ix   = sz;
X    ddict[ix].size_ix = r_word(p, i_offs + IE_RP_SIZE);
X  }}
X  return OK;
X}}
X
#endif

public Page_nr class_freel(upn, classid)
    Sid    upn;
    Sid    classid;
{ Byte * root_p = page_ref(upn, ROOT_PAGE);
  Offs mi_eny = class_offs(classid);

  return mi_eny > BLOCK_SIZE
			  ? OUT_OF_ROOT : r_word(root_p, mi_eny + CE_FREE_LIST);
}  


public Cc add_class(upn, class_id, size)
    Sid   upn;
    Sid   class_id;
    Short size;
{ fast Byte * p = page_mark_ref(upn, ROOT_PAGE);
  if (cache_err != OK)
    return CORRUPT_PF;    

{ Short mi_eny = class_offs(class_id);
  Short rp_size = r_word(p, mi_eny + CE_RP_SIZE);

  if (mi_eny + CE_SIZE > BLOCK_SIZE)
    return OUT_OF_ROOT;
  if (rp_size != size and rp_size != 0)
    return CLASS_EXISTS;
  if (r_word(p, mi_eny + CE_RP) != 0)
    return CLASS_EXISTS;

  w_word(p, mi_eny + CE_RP_SIZE, size);
  w_word(p, mi_eny + CE_RP, 0);
  w_word(p, mi_eny + CE_FREE_LIST, 0);
  w_quad(p, mi_eny + CE_VERSION, 0);
  return SUCCESS;
}}


/*   
public Cc del_class(upn, class_id)
    Sid    upn;
    Sid    class_id;
{ Byte * p = page_mark_ref(upn, ROOT_PAGE);
  Short mi_eny = class_offs(class_id);
  if (mi_eny + CE_SIZE > BLOCK_SIZE)
    return OUT_OF_ROOT;
  if (cache_err != 0)
    return CORRUPT_DB;

  w_word(p, mi_eny + CE_RP_SIZE, 0);
  return SUCCESS;
}
*/

public Cc add_index(upn, index_id, kind, size, class_id)
    Sid    upn;
    Sid    index_id;
    Squot  kind;   /* not used */
    Short  size;
    Sid    class_id;
{ Short mi_ix = ix_offs(index_id);

  if (mi_ix > BLOCK_SIZE-IE_SIZE)
    return OUT_OF_ROOT;

{ Byte * p = page_mark_ref(-upn, ROOT_PAGE);

  if (r_word(p, mi_ix + IE_RP) != 0)
    return IX_EXISTS;

  w_word(p, mi_ix + IE_RP_SIZE, size);
  w_quad(p, mi_ix + IE_VERSION, (Int)class_id << 24);

  return OK;
}}


/*  
public Cc del_index(upn, index_id)
    Sid    upn;
    Sid    index_id;
{ Byte * p = page_mark_ref(-upn, ROOT_PAGE);

  Short mi_ix = ix_offs(index_id);

  if (mi_ix > BLOCK_SIZE)
    return OUT_OF_ROOT;

  w_word(p, mi_ix, 0);
  return SUCCESS;
}
*/
