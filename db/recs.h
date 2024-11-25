#ifndef s_page

#define Q_EFLAG  0x40000000

#define s_page(lra) (((lra) >> (LOG_BLOCK_SIZE)) & 0xffff)
#define s_offs(lra) ((lra) & (BLOCK_SIZE-1))

#define to_lra(page,offs) ((((int)page & 0xffff) << LOG_BLOCK_SIZE) + (offs))

#define same_lra(a,b) (*a == *b)

extern Bool vfy_is_occ __((Byte*, Short));


extern Vint s_wifr(Char * p);

extern void vfy_wifr(Id upn, Char * p, Short wifr);

extern Cc new_rec  __((Sid, Sid, Lra));
extern Cc del_rec  __((Sid, Sid, Lra_t));
extern Char * new_stream __((Sid, Sid));
extern Cc new_init_rec(Ldn, Sid, char * , Lra_t *);
extern Char * next_rec __((void *, Lra, void *));
extern Cc next_rec_read __((/*strm, rec_ref*/));
extern Char * init_brec_strm __((Sid, Sid));
extern Cc next_brec __((/*strm, lra_ref, rec_ref*/));
extern void fini_stream(/*Ix_Strm*/);

extern Byte * read_rec  __((Sid, Sid, Lra_t, void * ));
extern Cc write_rec  __((Sid, Sid, Lra_t, void *));
extern void upd_nxt_fld(Ldn, Sid, Lra_t, Offs, Lra_t);
extern Set rec_is_free(char p[16], Short);
extern Vint sf_wifr(Char *);
extern void frame_blks(Id, Id);
extern void fini_brec_strm(/*Ix_Strm*/);

#define init_rec_strm(u,c) new_stream(u,c)

#endif
