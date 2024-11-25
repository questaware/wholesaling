#ifndef FILE
#include <stdio.h>
#endif

#if S_LINUX
#define O_SYNCW O_SYNC
#endif

#define file_open(path,mode)	open(path, O_RDWR | O_CREAT, 0777)
#define file_close(fid)		close(fid)

#define F_EXISTS   1
#define F_NEW      2		/* the file must not exist */
#define F_WRITE    4
#define F_APPEND   8
#define F_EXCL    16
#define F_SYNC    32
#define F_SIMPLE  64
#define F_IXOPT  128		/* open the index */
#define F_CHECK	 256
#define F_NEWIX  512		/* the index must not exist already */
#define F_WRIX  1024		/* open the index for writing */

#define F_VERBOSE 0x8000
#define MAXCLIENTS 12

typedef struct Cache_det_t
{ 
  Int    page_upn;
  Short  lru_forwards;
  Short  lru_backwards;
  Ushort overflow;   	/* hash extension list */
  Short  busy_ct;			/* bitvec of procs guaranteed this page */
  Short  wifr;	    	/* value of wifr field */
  Ushort memory;      /* short pointer to memory */ 
}
Cache_det_t, * Cache_det;

typedef struct 			        /* stored in shared memory */
{ Int    id;
  Int    num_held;
  Char	 tty[8];
} Proc_t;

extern Int cache_err;

extern Id global_lock(void);
extern Id global_unlock(void);

extern Short init_cache __((Bool));
extern void fini_cache();
extern Cc cache_term();
extern int get_cache_users(Proc_t * *);
extern Cc init_db(Char*,Set);

extern Int * tty_map();

#if SINGLE_USER
#define locks_del()
#define shm_del()
#define cache_lock() OK
#define cache_unlock()
#define ct_rdrs(upn) 0
#define ct_wrrs(upn)
#define flip_vfywr()
#define vfy_wrlock(upn)
#define vfy_lock(upn)
#define vfy_islocked(upn, min)

#define rd_lock(upn)
#define rd_unlock(upn)
#else
extern Int count_busys(Int * all_ct);

extern void locks_del();
extern void shm_del();
extern Cc cache_lock();
extern void cache_unlock();
extern Short ct_rdrs(Sid);
extern Int ct_wrrs(Sid);
extern void flip_vfywr();
extern void vfy_wrlock(Sid);
extern void vfy_lock(Sid);
extern void vfy_islocked(Sid, Int);

extern Cc rd_lock(Sid);
extern void rd_unlock(Sid);
#endif

extern void wr_lock_window(Sid);
extern void flip_writedelay();

extern Short ct_cache();
extern Cc   wr_lock __((Sid));
extern Bool wr_lock_try __((Sid, Short));
extern void wr_unlock __((Sid));
extern void wr_droplock(Sid);

extern Bool read_db(Sid, Page_nr, Byte *);

extern Bool page_in_strms(Sid upn, Page_nr pg);
extern void page_from_strms(Sid upn, Page_nr pg);
extern Cc page_to_strms(Sid upn, Page_nr pg);

extern Id next_pid __(());
extern Char * chan_dbname  __((Char *, Sid));
extern Fildes chan_fileno __((Sid));
extern Int chan_filesize __((Sid));
extern Sid chan_link  __((Shortset, Char *));
extern Cc chan_erase_file __((Char *));
extern Cc chan_unlink  __((Sid));
extern void set_db_size __((Sid,Int));
extern Short busy_ct __((Sid, Page_nr));
extern Cc vfy_page __((Sid, Page_nr));
extern Byte * page_ref __((Sid, Page_nr));
extern Byte * _page_ref __((Sid, Page_nr));
extern Byte * page_mark_ref(Sid, Page_nr);
extern Byte * rec_ref_unchecked __((Sid, Lra_t));
extern Byte * rec_ref __((Sid, Lra_t));
extern Cache_det p_r_eny;
extern Bool page_mark __((Sid, Page_nr));
extern Byte * page_mark_ref __((Sid, Page_nr));
extern Byte * rec_mark_ref __((Sid, Quot, Lra_t));
extern Byte * new_page_mark __((Sid, Page_nr, Short)) ;
extern Bool put_page __((Sid, Page_nr, Byte *));
extern Int do_db __((Sid,Set));
extern Int force_db __((Sid));
extern void dest_db __((Sid));
extern void prt_cpage __((Sid,Int));
extern void dump_blk(Byte *, int);
extern void dump_cache __(());

extern Int raw_lock(Sid, Int);   /* actually in rawlocks.c */
extern Int raw_unlock(Sid, Int); /* ditto */
