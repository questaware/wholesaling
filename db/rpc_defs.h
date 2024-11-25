 /* RPC_ROLE 0: no rpc; 
             1: server; 
            -1: client;
            -2: client on the same machine
  */
#ifndef RPC_ROLE 
#define RPC_ROLE 0
#endif
/*--------------------------------*/
#if RPC_ROLE <= 0
#define RPC_STAR
#define rpc_return(typ, v) return v
#define rpc_retvoid        return
#else
#define RPC_STAR *
#define rpc_return(typ, v) rpc_res.typ = (v); return &rpc_res.typ
#define rpc_retvoid(v)     v; return (void*)&rpc_res
#endif

/*--------------------------------*/
#if RPC_ROLE != 0

#undef SUCCESS
#include        "dn.h"
#define SUCCESS 0

typdef union
{ int	            void_;
  Cc	            cc;
  Sid	            sid;
  Set	            set;
  Short	            short_;
  Byte              block[1024];
  struct 
  { Int    Vbyte_len;
    Char * Vbyte_val;
  }     vbyte;
  struct CcLra      cclra;
  struct CcKeyLra   cckeylra;
  struct Key__t     key__t;
  StrmId            strmid;
  struct VbyteLra   vbytelra;
  struct Rec_strm_t rec_strm_t;  
} Rpc_res_t;

extern Rpc_res_t rpc_res;

#endif

/*--------------------------------*/
#if RPC_ROLE < 0
extern CLIENT * db_cl;
#endif

