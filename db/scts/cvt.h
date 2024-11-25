extern Cc ii_log(Short, Char *, Int, Int);
extern void prt_int(Int *, Short);
extern void prt_n(Int);
extern void prt_str(Char *, Short);
extern void prt_hex(Int *, Short);
extern void prt_key(Key, Short);
extern void prt_ix(Sid, Sid, Nullproc);
extern void prt_cl(Sid, Sid, Nullproc);
extern Int rd_char();
extern Int rd_past();
extern Int rd_int(Short, Int *);
extern void rd_str(Short, Char *);
extern void rd_name(Short, Char *);
extern void rd_hex(Short, Int *);


extern   void rbld_db(Sid, Char *);
extern   Cc   d_db_class(Sid, Char *);
extern   void o_db(Sid, Char *);
extern   Cc   i_db(Sid);
extern  Short r_ix(Sid, Sid, Nullproc, Nullproc);
