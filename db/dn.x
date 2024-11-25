#include "../../h/base.h"
			/* unfortunately rpc #include paths are fucked */

typedef char *Null;			/* literal NULL */
typedef Char B16[16];

const FNLEN = 256;

typedef string Filename <FNLEN>;
typedef string Keystring <64>;

typedef opaque Vbyte<512>;


#if 0
			/*  Basic Types - values of the type Basic_type */
const UCKEY    = 64;
const E_NU     = 128;

const INTEGER	= 0;
const FXD_STR   = 3;
const ENUM      = 4;
const LRA_KEY   = 5;
const NU_INT	= 128;
const NU_FSTR   = 131;

#endif

union Key_u switch (int typ)  /* types above minus 1 */
{ case 1: Int        integer;
  case 2: Int        enumeration;
  case 3: Keystring  str;
  case 4: Lra_t      lra;
};

typedef union Key_u  Key__t;

typedef long StrmId;

struct CcLra
{ Cc     cc;
  Lra_t  lra;
};
typedef struct CcLra CcLra;


struct CcLraBool
{ Cc     cc;
  Lra_t  lra;
  Bool   bool_;
};
typedef struct CcLraBool CcLraBool;

struct VbyteLra
{ Vbyte  vbyte;
  Lra_t  lra;
};
typedef struct VbyteLra VbyteLra;

struct VbyteLraBool
{ Vbyte  vbyte;
  Lra_t  lra;
  Bool   bool_;
};
typedef struct VbyteLraBool VbyteLraBool;


struct VbytePageOffs
{ Vbyte  vbyte;
  Int    page;
  Offs   offs;
};
typedef struct VbytePageOffs VbytePageOffs;


struct CcKeyLra
{ Cc      cc;
  Key__t  key;
  Lra_t   lra;
};
typedef struct CcKeyLra CcKeyLra;


struct Rec_strm_t
{ Sid        upn;
  Char       cl_id;
  Char       written;    
  Int        versn;
  Int        page;
  Offs       offs;
  VbyteLra   res;
};
typedef struct Rec_strm_t Rec_strm_t;



program DBPROG
{ version DBVERS
  { 
    Set   rpc_init_cache(Bool)				= 4;
    void  rpc_fini_cache(Set)				= 5;
    Sid   rpc_chan_link(Shortset, Filename)		= 6;
    Cc    rpc_chan_unlink (Sid)				= 7;

    void rpc_rd_lock(Sid)				= 10;
    void rpc_rd_unlock(Sid)				= 11;
    void rpc_wr_lock(Sid)				= 12;
    void rpc_wr_unlock(Sid)				= 13;
  /*void rpc_wr_lock_window(Sid)			= 14;*/


    Set    rpc_rec_is_free(B16, Short) 			= 20;
    void   rpc_upd_nxt_fld(Sid, Sid, Lra_t, Offs, Lra_t)= 21;
    Vbyte  rpc_read_rec(Sid, Sid, Lra_t, Null) 		= 24;
    Cc     rpc_write_rec(Sid, Sid, Lra_t, Vbyte) 	= 25;
    CcLra  rpc_new_rec(Sid, Sid) 			= 26;
    CcLra  rpc_new_init_rec(Sid, Sid, Vbyte)		= 27;
    Cc     rpc_del_rec(Sid, Sid, Lra_t)			= 28;

    Rec_strm_t rpc_new_stream(Sid, Sid)			= 29;
    void       rpc_fini_stream(Rec_strm_t)		= 30;
    Rec_strm_t rpc_next_rec(Rec_strm_t)		        = 31;

    Rec_strm_t   rpc_init_brec_strm(Sid, Sid)		= 32;
    void         rpc_fini_brec_strm(Rec_strm_t)		= 33;
    Rec_strm_t   rpc_next_brec(Rec_strm_t)		= 34;

    CcLraBool    rpc_b_search(Sid, Sid, Key__t)		= 41;
    /*Bool       was_mult();*/
    VbyteLraBool rpc_b_fetchrec(Sid, Sid, Key__t)	= 42;
    CcLra        rpc_b_insert (Sid, Sid, Key__t)	= 43;
    CcLra        rpc_b_replace(Sid, Sid, Key__t, Lra_t)	= 44;

    CcKeyLra     rpc_b_first  (Sid, Sid)		= 45;
    CcKeyLra     rpc_b_last   (Sid, Sid)		= 46;
    CcLra        rpc_b_delete (Sid, Sid, Key__t, Lra_t)	= 47;

    void         rpc_ix_fini_stream(StrmId)		= 48;
    Key__t       rpc_stream_last_key(StrmId)		= 49;
    StrmId       rpc_b_new_stream(Sid, Sid, Key__t)	= 50;
    VbyteLra     rpc_ix_next_rec(StrmId)		= 51;
    Cc           rpc_b_delall(Sid, Sid)			= 52;
  } = 1;
} = 0x20000001;
