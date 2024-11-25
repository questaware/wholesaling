#ifndef s_page
#include "recs.h"
#endif

/* Index seg formats: 

   key
   lra
 { page }
*/

			/*  Basic Types - values of the type Basic_type */
#define UCKEY    64
#define E_NU    128

#define INTEGER	0
#define FXD_STR 3
#define ENUM    4
#define LRA_KEY 5
#define NU_INT	(INTEGER+E_NU)
#define NU_FSTR (FXD_STR+E_NU)


extern Cc dbcc;

typedef union 
{ Int      integer;
  Int      enumeration;
  Char *   string;
  Lra_t    lra;
} Key_t, *Key;

extern Cc b_search __((Sid, Squot, Key, Lra));
extern Byte * b_srchrec  __((Sid, Squot, Key, Lra));
extern Byte * b_fetchrec __((Sid, Squot, Key, Lra, void*));
extern Cc b_last   __((Sid, Squot, Key, Lra));
extern Bool was_mult(void);
extern Cc b_first  __((Sid, Squot, Key, Lra));
extern Cc b_replace __((Sid, Squot, Key, Lra));
extern Cc b_insert __((Sid, Squot, Key, Lra));
extern Cc b_delete __((Sid, Squot, Key, Lra));

extern Byte * b_new_stream __((Sid, Squot, Key));
extern void ix_fini_stream __((/*stream_*/));
extern Cc next_of_ix  __((/*stream_, key_val, lra*/));
extern Byte * ix_next_rec  __((Byte *, Lra, void *));
extern Cc prev_of_ix  __((/*stream_, key_val, lra*/));
extern Byte * ix_prev_rec  __((Byte *, Lra, void *));
extern Key stream_last_key(/*strm*/);
extern void dumptree __((/*upn, page, indent*/));


#define ix_search(u,i,kv,lra)    b_search(u,i,kv,lra)

#define ix_srchrec(u,i,kv,lra)    b_srchrec(u,i,kv,lra)
#define ix_fetchrec(u,i,kv,lra,t) b_fetchrec(u,i,kv,lra,t)

#define ix_first(u,i,kv,lra) b_first(u,i,kv,lra)
#define ix_last(u,i,kv,lra) b_last(u,i,kv,lra)

#define ix_replace(u,i,kv,lra)  b_replace(u,i,kv,lra)

#define ix_insert(u,i,kv,lra) b_insert(u,i,kv,lra)

#define ix_delete(u,i,kv,lra) b_delete(u,i,kv,lra)

#define ix_new_stream(u,i,kv) b_new_stream(u,i,kv)
