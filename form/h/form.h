#define  FLD_FST  1
#define  FLD_LST  2
#define  FLD_BOL  4
#define  FLD_EOL  8
#define  FLD_RD   16
#define  FLD_WR   32
#define  FLD_GRAB 64
#define  FLD_NOCLR 128
#define  FLD_MSG  256
#define  FLD_FD   512
#define  FLD_SGL  1024
#define  FLD_NOECHO 2048
#define  FLD_S     FLD_NOECHO+FLD_NOCLR
#define  FLD_PEN   4096
#define  FLD_BUSY  9192


#define T_DAT  (1 << 6)
#define T_FAT  (2 << 6)
#define T_INT  (3 << 6)
#define T_OINT (4 << 6)
#define T_CSH  (5 << 6)
#define T_OCSH (6 << 6)

#define MWIDTH (1 << 9)

#define ID_FAC 512
#define LOG_ID_FAC 9
#define ID_MSK (0x3f)
#define FMT_MSK (0x1c0)
//                        bottom 6 bits row
//                        next   3 bits format
//                        next   7 bits field
#define s_fid(fld)  ((fld)->id & ~(ID_FAC - 1))
#define s_frow(fld) ((fld)->id &  ID_MSK)


#define W_CMD   (1 * ID_FAC)
#define W_MSG   (2 * ID_FAC)
#define W_SEL   (3 * ID_FAC)

#define W_BAN  (50 * ID_FAC)

#define CHARWDTH (SCRN_COLS+10)

typedef struct
{ Short    id;
  Shortset props;
  Char     row, col;
  Char     size;
  Char     rptct;
  const Char * name;
} Fieldrep_t, * Fieldrep;

typedef struct
{ Char   typ;
  Char   row;
  Char   ct;
  Char   col;
  const Char * c;
} Over_t, * Over;


typedef struct
{ Short    id;
  Short    props;
  Short    pos;
  Short    ecol;
  Fieldrep class;
} Field_t, * Field;


typedef struct 
{ Short   curr_fld;
  Short   dummy;
  Field   msgfld;
  Short   noo_eny;
  Short   tablesize;
  Short   tabledepth;
  Over    curr_over;
  Short   top_win;
  Short   bot_win;
  Char	  banner[82];
  Field_t c[1];     /* c[0] is unused, c[1], c[2] ... are the fields */
} Tabled_t, * Tabled;


typedef struct 
{ Short  len;		/* times 3 */
  Char   c[CHARWDTH*3];
} Line_t, *Line;

typedef struct Screenframe_s
{ /*struct Screenframe_s  * next;*/
  Tabled        tbl;
  Short         fld;
  Char          top_win;
  Char          bot_win;
  Field         alertfld;
  Line_t        c [SCRN_LINES];
} Screenframe_t, *Screenframe;

Tabled this_tbl;

extern int g_rows;

extern Int get_fopt(const Char*,Field);
extern Char salerth(const Char *);
extern void alert(const char *);
extern void salert(const Char * s);
extern void form_fini();
extern void wscroll(Short);
extern void wrefresh();
extern void wclr(), wclrgrp();
extern Cc add_field(Tabled, const Fieldrep_t * , Sid, Shortset, Short, Short);
extern void wopen  (Tabled, const Over_t *);
extern void wopen_row(Tabled);
extern void wputc(char);
extern void wputstr(const char *);
extern void wreopen(Tabled);
extern void wclreol(void);
extern void wsetover(Tabled, const Over_t*);
extern void end_interpret();
#define clr_interpret end_interpret
extern void clr_item __((Int));
extern void clr_alert(void);
extern void idleform(void);
extern void init_form(void);
extern void init_fld_strm(/*tbl*/);
extern Cc right_fld(Short, Field *);
extern Cc down_fld(Short, Field *);
extern Cc left_fld(Short, Field *);
extern Cc up_fld(Short, Field *);
extern Field goto_fld __((Short));
extern Short save_pos();
extern Field restore_pos(Short);
extern void helpfld();
extern void mend_fld(void);
extern Short gad_ct;		/* moves before selecting */
extern Cc get_any_data (Char *, Field *);
extern Cc get_data (Char *);
extern void get_b_str(Short, Bool, Char *);
extern void append_data (Char *);
extern void direct_data (const Field, const Char *);
extern void put_data (const Char *);
extern void fld_put_data (Short, Char *);
extern void fld_put_int  (Short, Int);
extern Tabled_t * mk_form(const Fieldrep_t *, Int);
extern void put_choices __((const Char const *[]));

extern void push_form(Screenframe_t * frame);
extern Field pop_form(Screenframe_t * frame);
extern void put_free_data(const char * str);

extern void ipfld(void);

extern Char * fmt_dec(Char *, Short, Int);
extern Char * fmt_cash(Char *, Short, Cash);
extern Char * fmt_lstr(Char *, Short, Char *);
extern Char * fmt_rstr(Char *, Short, Char *);
extern Char * fmt_data();
extern Char * fmt_str(char *,
                      const char *,
                      va_list args);
extern void put_integer (Int);
extern void put_ointeger (Int);
extern void put_cash (Cash);
extern void put_ocash (Cash);
extern Char * to_client_ref(Char *, Int);

extern Char * rgt_to_asc(Int, Char *);

extern Int asc_to_rgt(const Char *, Bool);

#if 1
#ifndef IN_FORMC
#if __STDC__
 int form_put( Int, ...);
#else
 int form_put();
#endif
#endif
#endif

extern Cc to_integer (Char *, Int *);
extern Cc get_integer (Int *);
extern Cc get_money (Cash *);
extern Cc get_ointeger (Int *)    /* not used */;
extern Cc get_any_integer (Int *, Field *);
extern Cc get_ocref (Int *, Char *);

extern Cash strtocash(Char * s_);


extern void prt_set_title(Char *);
extern void prt_set_footer(Char *);
extern void prt_close_printer();
extern Char * prt_open_printer(void *);
extern void prt_init_report();
extern Char * prt_one_report(void *, Char *, Char *);
extern void prt_set_cols(Char *, Char *);
extern void prt_head();
extern void prt_need(Short);
extern void prt_raw(Char *);
extern void prt_text(Char *);
extern void prt_fmt(const char *, ...);
extern void prt_row(Int, ...);
extern void prt_fini(const char * email);
extern void prt_fini_idle(const char * email);
extern Char * prt_lpt_file(Char *, Char *);


extern Date day_no(Short, Short, Short);
extern const Char * date_to_weekday (Date);
extern Short to_week(Short) ;
extern Date easter(Short);
extern Short next_month(Short);
extern Short this_month(Short);
extern Date asc_to_date(Char *);
extern Char * date_to_asc(Char *, Date, Int);

extern Time asc_to_time(Char *);
extern Char * time_to_asc(Char *, Time);
