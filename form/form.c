#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include  <term.h>
/*#include  <signal.h>*/
#include  "version.h"
#include  "../h/defs.h"
#include  "../h/errs.h"
#include  "../env/tcap.h"
#include  "../h/tcap_reserve.h"
#include    <stdarg.h>

#define IN_FORMC 1
#include  "h/form.h"
#if !(MSDOS)
#include  <sys/ioctl.h>
#include  <fcntl.h>
#endif

void wputctrl(char * cmd, int p1, int p2);


extern Char * this_ttype;
					/* module screen */
int g_rows = SCRN_LINES;

Short g_top_win, g_bot_win;
static Short bsyrow, bsycol;

#define MAXCE 8

#define MAXBOX 10

#define NOPAD (-2)

#define A_ESC (0x1b)
/* This module uses nibbles (beginend, mode, color, color, hichar, lochar)
*/
#define B 0x100000L
#define E 0x200000L

#define A__REV  (A_REV << 8)
#define A__BOLD (A_BOLD << 8)
  		/* A bug in the 2l model prevents use of short literals !! */
static Int  BEREV;
static Int  BEREVBOLD;
static Int  BANNERCHROM;
static Int  INFOCHROM;

#define IV 1

static Line_t screen [SCRN_LINES];
static Line_t saved_fld, rest_fld;
static Short  saved_row, rest_row;
static Short  saved_col, rest_col;
static Short  saved_len, rest_len;


static Short fldinitch = '.'; 
/*static Int  fldfillch = '.';   ** not currently used */
static Bool is_ansi;
static Int  gfldfillch;
 
private Char *   last_p;
private Line_t * last_ln;
private Field alertfld;

public Tabled this_tbl;

/*
Char * BRV =  "[1;7m";
Char * BOLD = "[1m";
Char * NMODE ="[m";

Char * GS = "[12m";  ** these are experimental **
Char * GE = "[10m";
Char * G1 = "?";
Char * G2 = "Z";
Char * G3 = "@";
Char * G4 = "Y";
*/

#define s_row(p) ((p)->pos >> 9)
#define s_col(p) ((p)->pos & (MWIDTH - 1))

void init_form()

{ init_fmtform();
  BEREV = A_REV;		/* compiler bug */
  BEREV = (BEREV << 8) + B+E;
  BEREVBOLD = A_REV + A_BOLD;
  BEREVBOLD = (BEREVBOLD << 8) + B+E;
  BANNERCHROM = 0x25;
  BANNERCHROM = (BANNERCHROM  << 8) + B+E;
  INFOCHROM = A_GREEN;
  INFOCHROM = (INFOCHROM  << 8) + B+E;
/* 
{ Char * SS = SO;
  SO = SE;
  SE = SS;
*/
  fldinitch = '.';
/*fldfillch = '.';*/
  gfldfillch = '.';
  is_ansi = strncmp(this_ttype, "ansi", 4) == 0;
  if (is_ansi)
  { fldinitch = '°';
  /*fldfillch = A__REV + '°';*/
    gfldfillch = '°';
  }

  char * v = getenv("LINES");
  if (v != NULL)
  { g_rows = atoi(v);
    if (g_rows > SCRN_LINES)
      g_rows = SCRN_LINES;
  }
}


void form_fini()

{ char * v = getenv("LINES");
  if (v != NULL)
    g_rows = atoi(v);

  wputctrl(CS,0, g_rows-1);
}

                                       /* MODULE basic_screen */
                                       
#define clearscreen()  { tteeop(); }


private void wgoto(row, col)
	 Short   row, col;			/* both have origin 0 */
{ ttmove(row, col);
  last_ln = &screen[row];
  last_p = &last_ln->c[col*3];
}



#define erasechar(row, col)  /* both have origin 0 */ \
{ wgoto(row, col); \
  ttputc(' ');     \
  ttmove(row, col);\
  *last_p = ' ';   \
}

private Int wputs(row, col, str, ct, attr)
 	 Short    row, col;			/* both have origin 0 */
	 Char *   str;
	 Short    ct;
	 Set      attr;
{      last_ln = &screen[row];
{ fast Char space = ' ';
       Char chrom = attr >> 8;
       Char mode  = (attr & ~ (B+E))>> 16;
  fast Char * p = &last_ln->c[col * 3];
       Char * p_ = p;
       Short llen = last_ln->len;

  if (llen < col * 3)
  { p = &last_ln->c[llen];
    while (p < p_)
    { *p++ = space;  *p++ = 0;     *p++ = 0;
    }
  }

  ttmove(row, col);
  if (attr & B)
    ttrev((Short)(attr >> 8));

{      Int iter;
  fast Char * s = str;

  while (*s != 0 and --ct != -1)
  { ttputc(*s);
    *p++ = *s++;     *p++ = chrom; *p++ = mode;
  }

  for (iter = 1; ; --iter)
  { if (attr & 255)
      while (--ct >= 0)
      { ttputc(attr);
        *p++ = attr;   *p++ = chrom; *p++ = mode;
      }
    
    if (iter == 0)
      break;
  
    if (attr & E)
      ttrev((Short)-1);

    if (p >= p_)
    { p_[2] |= (attr & ~E) >> 16;
      p[-1] |= (attr & ~B) >> 16;
    }
    
    if (attr & 255)
      break;
    attr = space;
  }

  if (p > &last_ln->c[llen])
  { last_ln->len = p - &last_ln->c[0];
 
    if (last_ln->len > 85 * 3)
      i_log(last_ln->len, "lc too l");
  }
  last_p = p;
  return s - str;
}}}

private void wclrfld(row, col, size, attr)
	Short    row, col;			/* both have origin 0 */
	Short    size;
	Set      attr;
{      last_ln = &screen[row];
{ fast Char space = ' ';
       Char chrom = attr >> 8;
       Char mode  = attr >> 16;
  fast Char * p = &last_ln->c[col * 3];
       Char * p_ = p;
       Int llen = last_ln->len;

  if (llen < col * 3)
  { p = &last_ln->c[last_ln->len];
    while (p < p_)
    { *p++ = space;  *p++ = chrom;  *p++ = mode;
    }
  }

  ttmove(row, col);
  if (attr & B)
    ttrev((Short)(attr >> 8));

{ Short ct = size;
  while (--ct >= 0)
  { ttputc(attr);
    *p++ = attr;    *p++ = chrom;   *p++ = mode;
  }
  
  if (attr & E)
    ttrev((Short)-1);

  if (p > &last_ln->c[llen])
  { last_ln->len = p - &last_ln->c[0];
    if (last_ln->len > 85 * 3)
      i_log(last_ln->len, "lc too l");
  }

  last_p = p;
  if (p > p_)
  { p_[2] |= (attr & ~E) >> 16;
    p[-1] |= (attr & ~B) >> 16;
  }
}}}



private void savefld(row, col, size)
	Short    row, col;			/* both have origin 0 */
	Short    size;
{ saved_fld.len = size;
  saved_row = row;
  saved_col = col;
  saved_len = screen[row].len;
  memcpy(&saved_fld.c[0], &screen[row].c[col * 3], saved_fld.len * 3);
}  

private void redoline(row)
           Short row;
{      Short  len = screen[row].len;
       Char * z = &screen[row].c[len];
  fast Char * p;

  ttmove(row, 0);
  tteeol();

  for (p = &screen[row].c[0]; p < z; p += 3)
  { if ((p[2] & (B+E >> 16)) == 0)
      ttputc(*p);      
    else 
    { if (p[2] & (B >> 16))
      { ttrev(p[2] * 256 + p[1]);
      }
      ttputc(*p);      
      if (p[2] & (E >> 16))
      { ttrev((Short)-1);
      }
    }
  }
}



private void wputmore(ch)       /* to be called afer wputs, wclrfld, wgoto */
	Char   ch;
{ fast Char * p;
  if (&last_ln->c[last_ln->len] <= last_p)
  { last_ln->len += 3;
    if (last_ln->len > 85 * 3)
      i_log(last_ln->len, "lc too l");
  }
  p = last_p;
  p[0] = (int)ch;
  p[1] = p[-2];
  p[2] = p[-1];
  last_p = p + 3;
  
  ttputc(ch);
}



void wputc(ch)
        Char ch;
{ ttputc(ch);
}


void wputstr(str)
        const Char * str;
{ ttputs(str);
}

void wclr()

{ fast Short i;
  for (i = 0; i < SCRN_LINES; ++i)
  { screen[i].len = 0;
    screen[i].c[0] = 0;
  }
  clearscreen();
}



void wclreol()

{ Short len = last_p - &last_ln->c[0];
  if (last_ln->len > len)
    last_ln->len = len;

  tteeol();
}



void wclrgrp()

{ fast Short i;
  for (i = g_top_win; i <= g_bot_win; ++i)
    if (screen[i].len != 0)
    { screen[i].len  = 0;
      screen[i].c[0] = 0;
      ttmove(i, 0);
      tteeol();
    }
}



void wputctrl(char * cmd, int p1, int p2)

{ char * buf = tparm(cmd, p1, p2);
  if (buf != NULL)
    for (; *buf != 0; ++buf)
      ttputc(*buf);
}




void wrefresh()

{ fast Short i;
  clearscreen();
  
  for (i = 0; i < g_rows-1; ++i)
    redoline(i);
}



void put_eeol()

{ if (this_tbl != null)
  { direct_data(&this_tbl->c[this_tbl->curr_fld], "");
    wclreol();
  }
}



void clr_item(coord)
	Int	 coord;
{ Int spos = save_pos();
  Field fld = goto_fld(coord);
  if (not (fld->props & FLD_BOL))
    left_fld(FLD_BOL, &fld);
  put_eeol();
  restore_pos(spos);
}

void mend_fld()

{ if (saved_fld.len != 0)
  { rest_fld = saved_fld;
    rest_row = saved_row;
    rest_col = saved_col;
    rest_len = saved_len;
  }
}
 
 
private void patch_fld(fld)
	Field  fld;		/* field not to patch */
{ if (rest_row != s_row(fld) or rest_col != s_col(fld))
  { Line_t * line = &screen[rest_row];

    if (rest_row >= g_rows)
    { i_log(rest_row, "Rest Row");
      rest_row = g_rows-1;
    }
    if (rest_col + rest_fld.len >= CHARWDTH)
    { i_log(rest_col, "Rest Col");
      return;
    }
    memcpy(&line->c[rest_col * 3], rest_fld.c, rest_fld.len * 3);
    line->len = rest_len / 3;
    redoline(rest_row);
    rest_fld.len = 0;
  }
}

void wscroll(n)
	Short n;
{ fast Short i;

  if (n > g_bot_win - g_top_win)
  { n = g_bot_win - g_top_win + 1;
    clearscreen();
  }
  else 
    for (i = n; i > 0; --i)
      ttscdn();

  for (i = g_top_win; i <= g_bot_win - n; ++i)
    memcpy((Char*)&screen[i], (Char*)&screen[i+n], sizeof(Line_t));

  for (i = g_bot_win - n + 1; i <= g_bot_win; ++i)
  { ttmove(i, 0);
    tteeol();
    screen[i].len  = 0;
  }
}

void push_form(Screenframe_t * frame)

{ memcpy(&frame->c[0], screen, g_rows * sizeof(Line_t));
  frame->tbl      = this_tbl;
  frame->fld      = this_tbl->curr_fld;
  frame->alertfld = alertfld;
  frame->top_win  = g_top_win;
  frame->bot_win  = g_bot_win;
  saved_fld.len = 0;
}


Field pop_form(Screenframe_t * frame)

{ memcpy(screen, &frame->c[0], g_rows * sizeof(Line_t));
  alertfld = frame->alertfld;
  g_top_win = frame->top_win;
  g_bot_win = frame->bot_win;
{ Tabled tbl = frame->tbl;  
  tbl->curr_fld = frame->fld;
  this_tbl = tbl;

  wrefresh();
{ Field fld = &tbl->c[tbl->curr_fld];
  wgoto(s_row(fld), s_col(fld));
  return fld;
}}}

					/* MODULE forms */
Cc add_field(tbl, class, id, props, row_, col)
	Tabled   tbl;
	const Fieldrep_t * class;
	Sid      id;
	Shortset props;
	Short    row_, col;
{      Short row = row_ >= g_rows ? 0 : row_;
       Short pos = row * MWIDTH + col;
  fast Field tbl_p = (Field)&tbl->c[tbl->tablesize];
  fast Field tbl_z = (Field)&tbl->c[tbl->tablesize - tbl->noo_eny + 1];

  tbl->noo_eny += 1;  
  if (tbl->noo_eny >= tbl->tablesize)
    return TBL_FULL;

  while (tbl_p >= tbl_z and tbl_p->pos <= pos)
    --tbl_p;
    
  if (tbl_p >= tbl_z)
  { if (tbl_p->pos == pos)
      return ENY_EXISTS;

  { Field tbl_p_ = (Field)&tbl->c[1];
    for (; tbl_p_ < tbl_p; ++tbl_p_)
      memcpy(&tbl_p_[0], &tbl_p_[1], sizeof(Field_t));
  }}
  
  tbl_p->id    = id;
  tbl_p->props = props;
  tbl_p->pos   = pos;
  tbl_p->ecol  = col + class->size - 1;
  tbl_p->class = (Fieldrep)class;
  return OK;
}    



#define init_fld_strm(tbl) tbl->curr_fld = tbl->tablesize + 1

Tabled mk_form(recipe,tbl_sz)
	const Fieldrep_t *  recipe;
	Int       tbl_sz;
{ fast const Fieldrep_t * f;
       Short ct = 1;	/* I think it is safe to have this 0, pjs */
       
  for (f = recipe; f->id != 0; ++f)
    ct += f->rptct >= 0 ? f->rptct : tbl_sz;
    
{ Short enydpth = f->rptct;
  Cc cc = OK;
  Tabled tbl = (Tabled)malloc(sizeof(Tabled_t) + sizeof(Field_t)*ct);
  tbl->tablesize = ct;
  tbl->tabledepth = tbl_sz;
  tbl->noo_eny   = 0;

  int top_win = 0;
  int bot_win = 2;

  for (f = recipe; f->id != 0; ++f)
  { Short rpt;
    Shortset props = f->props + ((f->rptct & 0x7f) ? 0 : FLD_SGL) + FLD_FST; 
    Short tblsz = f->rptct >= 0 ? f->rptct : tbl_sz;
    Short row = f->row;
    if (row < 0)
      row = tbl_sz * enydpth - row;

    if (top_win == 0 && tblsz > 1)
    { top_win = row >= g_rows ? 0  : row;
      bot_win = row - 1 + enydpth * tblsz;
      if (bot_win >= g_rows)
          bot_win = g_rows;
    }
                                  /* FLD_FST required on single fields */
    for (rpt = 0; rpt + 1 < tblsz; ++rpt)
    { cc |= add_field(tbl, f, f->id + rpt, props, row, f->col);
      row += enydpth;
      props &= ~FLD_FST;
    }
    if (tblsz > 1)
      props |= FLD_LST;
    cc |= add_field(tbl, f, f->id + rpt, props, row, f->col);
  }

{ Tabled   stbl = this_tbl;
  Field fld;  
  this_tbl = tbl;
  init_fld_strm(tbl);
  tbl->msgfld  = right_fld(FLD_FD, &fld) != OK ? null : fld;
  tbl->top_win = top_win;
  tbl->bot_win = bot_win;
  this_tbl = stbl;
  if (cc != OK)
  { i_log(cc, "mk_form failed");
    if (tbl != null)
      free(tbl);
    tbl = null;
  }
  return tbl;
}}}

Cc right_fld(props, fld_ref)
	Short     props;
	Field  *  fld_ref;
{ fast Tabled    tbl = this_tbl;
       Bool skipping = false;
  fast Field f;
  Short i;
  
  for (i = 1; i >= 0; --i)
  { while (tbl->curr_fld - 1 > tbl->tablesize - tbl->noo_eny)
    { tbl->curr_fld -= 1;
      f = (Field)&tbl->c[tbl->curr_fld];
    
      if      (f->props & FLD_SGL)
        skipping = false;
      else if ((f->props & (FLD_BOL + FLD_FST)) == FLD_BOL)
        skipping = true;
  
      if (not skipping and (f->props & props) == props)
      { *fld_ref = f;
        return OK;
      }
    }
    tbl->curr_fld = tbl->tablesize;
  }
  return NOT_FOUND;
}

Cc down_fld(props_, fld_ref)
	Short     props_;
	Field *   fld_ref;
{      Short   props = props_;
       Tabled  tbl = this_tbl;

  if (tbl->curr_fld == tbl->tablesize + 1)
    return right_fld(props, fld_ref);
    
{ fast Field tbl_p = (Field)&tbl->c[tbl->curr_fld];
  fast Field tbl_z = (Field)&tbl->c[tbl->tablesize - tbl->noo_eny + 1];
       Short npos = tbl_p->pos + MWIDTH;
       Short col  = tbl_p->pos & (MWIDTH-1);
       Set sngl = (tbl_p->props & FLD_SGL);

  while (tbl_p >= tbl_z and tbl_p->pos < npos)
    --tbl_p;
  while (tbl_p >= tbl_z and ((tbl_p->props & props) != props or 
				(sngl ? tbl_p->ecol < col
				      : s_col(tbl_p) != col)))
    --tbl_p;

  if (tbl_p < tbl_z)
  { tbl->curr_fld = tbl->tablesize;
    return NOT_FOUND;
  }
  tbl->curr_fld = tbl_p - &tbl->c[0];		
  *fld_ref = tbl_p;
  return OK;
}}


Cc down_left_fld(props, fld_ref)
	Short     props;
	Field  *  fld_ref;
{ fast	Tabled    tbl = this_tbl;
loop:
  if (tbl->curr_fld == tbl->tablesize + 1)
    return right_fld(props, fld_ref);
    
{ fast Field tbl_p = (Field)&tbl->c[tbl->curr_fld];
  fast Field tbl_z = (Field)&tbl->c[tbl->tablesize - tbl->noo_eny + 1];
       Short npos = (tbl_p->pos + MWIDTH) & ~(MWIDTH - 1);

  while (tbl_p >= tbl_z and ((tbl_p->props & props) != props or 
	                      tbl_p->pos < npos))
    --tbl_p;

  if (tbl_p < tbl_z)
  { tbl->curr_fld = tbl->tablesize + 1;
    goto loop;		/* return NOT_FOUND; */
  }
  tbl->curr_fld = tbl_p - &tbl->c[0];		
  *fld_ref = tbl_p;
  return OK;
}}

Cc left_fld(props, eny_ref)
	Short     props;
	Field  *  eny_ref;
{ fast Tabled  tbl = this_tbl;
       Bool skipping = false;
  fast Field f;
       Short i;
       
  for (i = 1; i >= 0; --i)
  { while (true)
    { tbl->curr_fld += 1; 
      if (tbl->curr_fld > tbl->tablesize)
      { tbl->curr_fld = tbl->tablesize - tbl->noo_eny;
        break;
      }
      f = (Field)&tbl->c[tbl->curr_fld];
    
      if      (f->props & FLD_SGL)
        skipping = false;
      else if ((f->props & (FLD_EOL + FLD_LST)) == FLD_EOL)
        skipping = true;
    
      if (not skipping and (f->props & props) == props)
      { *eny_ref = (Field)&tbl->c[tbl->curr_fld];
        return OK;
      }
    }
  }
  return NOT_FOUND;
}


Cc up_fld(props, eny_ref)
	Short     props;
	Field  *  eny_ref;
{ fast Tabled  tbl = this_tbl;
  if (tbl->curr_fld == tbl->tablesize - tbl->noo_eny)
    return left_fld(props, eny_ref);

{ fast Field tbl_p = (Field)&tbl->c[tbl->curr_fld];
  fast Field tbl_z = (Field)&tbl->c[tbl->tablesize];
       Short col  = tbl_p->pos & (MWIDTH - 1);
       Short row  = tbl_p->pos &~(MWIDTH - 1);
  
  while (++tbl_p <= tbl_z and               (tbl_p->pos & ~(MWIDTH - 1)) == row)
    ;

  while (tbl_p <= tbl_z and 
         ((tbl_p->props & props) != props or (tbl_p->pos & (MWIDTH - 1)) > col))
    ++tbl_p;
  
  if (tbl_p > tbl_z)
  { tbl->curr_fld = tbl->tablesize - tbl->noo_eny;
    return NOT_FOUND;
  }
  tbl->curr_fld = tbl_p - &tbl->c[0];		
  *eny_ref = tbl_p;
  return OK;
}}

Field goto_fld(id_)
	Short    id_;
{ fast Field f_z = &this_tbl->c[this_tbl->tablesize - this_tbl->noo_eny + 1];
  fast Field f = &this_tbl->c[this_tbl->curr_fld]; 
       Shortset id_mask;
  fast Short id = id_;

  if      (id <= ID_MSK)
  { id_mask = 0; id = 0;
  }
  else if ((id & ID_MSK) == ID_MSK)
  { Short row  = f->pos &~(MWIDTH - 1);
  
    for ( ; f < &this_tbl->c[this_tbl->tablesize] and
	    (f->pos &~(MWIDTH - 1)) == row; ++f)
      ;
    --f;
    id_mask = ~ID_MSK; id &= ~ID_MSK; 
  }
  else 
    id_mask = -1;

  for ( ; f >= f_z and (f->id & id_mask) != id; --f)
    ;
      
  if (f < f_z)
    for (f = &this_tbl->c[this_tbl->tablesize]; (f->id & id_mask) != id; --f)
      if (f <= f_z)
        return null;
   
  this_tbl->curr_fld = f - &this_tbl->c[0];		
  return f;
}



Short save_pos()
   
{ return this_tbl->curr_fld;
}

	
Field restore_pos(pos)
	Short   pos;	
{ this_tbl->curr_fld = pos;
  return &this_tbl->c[this_tbl->curr_fld];
}

void direct_data(const Field fld, const Char * str)

{ Int fillch = /* fld->props & FLD_WR ? fldfillch : */ ' ';
  Fieldrep rpn = fld->class;
  if (rpn != null)
                                          /* ordinary, padded with spaces */ 
    (void)wputs(s_row(fld), s_col(fld), str, rpn->size, fillch);
}



void direct_so_data (fld, str)
	Field   fld;
	Char   *str;
{ Int fillch = /* fld->props & FLD_WR ? fldfillch : */ BANNERCHROM;
  Fieldrep rpn = fld->class;
  if (rpn != null)
                                          /* RV padded with ordinary spaces */ 
    (void)wputs(s_row(fld), s_col(fld), str, rpn->size, fillch);
} 


void direct_hl_data (fld, str)
	Field   fld;
	Char   *str;
{ Int fillch = /*fld->props & FLD_WR ? fldfillch : ' '*/ INFOCHROM;
  Fieldrep rpn = fld->class;
  if (rpn != null)
                                          /* BRV padded with ordinary spaces */ 
    (void)wputs(s_row(fld), s_col(fld), str, rpn->size, fillch);
} 

void put_data (str)
	const Char   *str;
{ direct_data(&this_tbl->c[this_tbl->curr_fld], str);
}


void fld_put_data (field, str)
        Short   field;
	Char   *str;
{ goto_fld(field);
  put_data(str);
}


void fld_put_int (field, v)
        Short   field;
	Int     v;
{ goto_fld(field);
  put_integer(v);
}



void put_sodata (str)               /* field not size protected */
	Char   *str;
{ direct_so_data((Field)&this_tbl->c[this_tbl->curr_fld], str);
}



void put_free_data(str)
	const Char *str;
{ Field fld = (Field)&this_tbl->c[this_tbl->curr_fld];
  wputs(s_row(fld), s_col(fld), str, NOPAD, ' ');
}


void form_put( Int dcr, ... )
    
{
  Char * val;

  va_list ap;
  va_start(ap,dcr);
 
  do
  { Short wh = (dcr & FMT_MSK);
    Field f = goto_fld(dcr & ~FMT_MSK);
    if (f == null)
    {  dcr = va_arg(ap, Int);
       continue;
    }
    if      (wh + f->id * 0 == T_DAT)
      put_data(va_arg(ap, char *));
    else if (wh == T_FAT)
      put_free_data(va_arg(ap, char *));
    else if (wh == T_INT)
      put_integer(va_arg(ap, Int));
    else if (wh == T_OINT)
      put_ointeger(va_arg(ap, Int));
    else if (wh == T_CSH)
      put_cash(va_arg(ap, Int));
    else if (wh == T_OCSH)
      put_ocash(va_arg(ap, Int));
    else
    { dcr = va_arg(ap, Int);
      put_data("IllT");
    }
  } while ((dcr = va_arg(ap, Int)) != 0);

  va_end(ap);
}



/*************************************************************************/

void salert(const Char * s) { direct_data(alertfld, s); }
Char salerth(const Char * s) { salert(s); return toupper(hold());}
void alert(const Char * s) { ttbeep();  salert(s);}
void clr_alert() { salert(""); }

static Char * ich = ">";
void idleform()  { if (bsyrow != -1) wputs(bsyrow, bsycol, ich, 1,0); }
void busyform()  { ich = "?"; idleform(); ich = ">"; }

static Id last_help_ix;

void wopen(tbl, overlay_)
        Tabled tbl;
        const Over_t * overlay_;
{ const Over_t * overlay = overlay_;
  Int  tblsz = tbl->tabledepth;
  tbl->curr_over = (Over)overlay;
  bsyrow = -1;
  last_help_ix = -1000;
  wclr();
  this_tbl = tbl;
  
  if (overlay != null)
  { bsyrow = overlay->row;
    bsycol = overlay->col;
    
    for (; overlay->typ >= 0; ++overlay)
    { const Char * str = overlay->c;
      Short ct;
      Short row = overlay->row;
      if (row < 0)
        row = tblsz - row;
      if (row >= g_rows)
				row = 0;

      if (str != null)
       for (ct = overlay->ct; --ct >= 0; ++row)
         wputs(row,overlay->col,str, NOPAD, overlay->typ != IV ? 0 : BANNERCHROM);
    }
    if (overlay->c != NULL)
    { strchcpy(&tbl->banner[0], '-', 81);
      strcpy(&tbl->banner[10], overlay->c);
      tbl->banner[strlen(tbl->banner)] = '-';
    }
  }
  init_fld_strm(tbl);

  while ((tbl->curr_fld -= 1) > tbl->tablesize - tbl->noo_eny)
  { Field f = (Field)&tbl->c[tbl->curr_fld];
    
    if ((f->props & (FLD_WR+FLD_NOCLR)) == FLD_WR)
      wclrfld(s_row(f), s_col(f), f->class->size, fldinitch);
  }

  init_fld_strm(tbl);
  if (right_fld(FLD_RD, &alertfld) != OK)
    alertfld = null;

  if (tbl->banner[0] != 0)
  { goto_fld(W_BAN);
    put_sodata(tbl->banner);
  }
  
  g_top_win = tbl->top_win;
  g_bot_win = tbl->bot_win;

  wputctrl(CS,g_top_win,g_bot_win);

  tbl->curr_fld = tbl->tablesize;
  saved_fld.len = 0;
}


void wopen_row(tbl)
        Tabled tbl;
{ Field f = (Field)&tbl->c[tbl->curr_fld];
	Int row = s_row(f);
	for (; s_row(f) == row; ++f)
		wclrfld(s_row(f), s_col(f), f->class->size, '.');
}


void wreopen(tbl)
        Tabled tbl;
{ wopen(tbl, tbl->curr_over);
}

void wsetover(tbl, over)
              Tabled  tbl;
        const Over_t *over;
{ tbl->curr_over = (Over_t*)over;
}

void helpfld()

{ fast Tabled  tbl = this_tbl;
  Short ix = tbl->tablesize - tbl->curr_fld;

  if (last_help_ix != ix and
                       tbl->msgfld != null and in_range(ix, 0, tbl->noo_eny-1))
  { direct_hl_data(tbl->msgfld, ((Field)&tbl->c[tbl->curr_fld])->class->name);
    last_help_ix = ix;
  }
}



void ipfld()

{ if (this_tbl->msgfld != null)
    direct_hl_data(this_tbl->msgfld, "In Progress");
  last_help_ix = -1;
}



void end_interpret()

{ if (this_tbl->msgfld != null)
    direct_data(this_tbl->msgfld, "");
}

Short gad_ct;			/* number of moves */
static Char kb_ch;


private Cc interpret_move(fld_ref)
	Field  *fld_ref;
{ fast Tabled tbl = this_tbl;
  const Char * name = null;
       Cc cc = OK;

  gad_ct = 0;
  
  while (true)
  { Field fld = (Field)&tbl->c[tbl->curr_fld];
    if (tbl->curr_fld > tbl->tablesize - tbl->noo_eny and
        tbl->curr_fld < tbl->tablesize + 1)
    { if (name != fld->class->name and tbl->msgfld != null)
      { name = fld->class->name;
        last_help_ix = -1000;
        helpfld();
      }
      wgoto(s_row(fld), s_col(fld));
      *fld_ref = fld;
    }
    kb_ch = ttgetc(0);
    if (rest_fld.len > 0)
      patch_fld(fld);
    if      (kb_ch == A_FF)
      wrefresh();
    else if (kb_ch == 'P'-'@')
      return PAGE_UP;
    else if (kb_ch == 'B'-'@')
      return PAGE_DOWN;
    else if (kb_ch == A_HT)
      cc = down_left_fld(FLD_WR + FLD_FST, fld_ref);
    else if (kb_ch > 0)
      return -kb_ch;
    else 
    { gad_ct += 1;
      switch (-kb_ch)
       { case  'r': cc = right_fld(FLD_WR, fld_ref);
         when  'l': cc = left_fld(FLD_WR, fld_ref);

         when  'u': if ((fld->props & (FLD_FST + FLD_SGL)) == FLD_FST)
                      return HALTED;
                    cc = up_fld(FLD_WR, fld_ref);

         when  'd': if ((fld->props & (FLD_LST + FLD_SGL)) == FLD_LST)
                      return HALTED;
                    cc = down_fld(FLD_WR, fld_ref);
         when  'U': return PAGE_UP;
         when  'D': return PAGE_DOWN; 
         when  'H': init_fld_strm(tbl);
                   cc = right_fld(FLD_WR, fld_ref);
         otherwise  return BAD_BUTTON;
       }
     }
  }
}

#define A_PFXCHR 2

private Cc g_data(fld, str_)
	Field   fld;
	Char   *str_;
{ fast Char * str = str_;
  Short row = s_row(fld);
  Short col = s_col(fld);
  Short size, fsize = fld->class->size;
  Bool noecho = (fld->props & FLD_NOECHO) != 0;

  for (size = fsize; ;)
  { if      (kb_ch < A_SP or kb_ch == 0x7f)
    { if (kb_ch == A_CR or kb_ch == A_LF or kb_ch < 0)
        break;
      if      (kb_ch == A_PFXCHR)
        *str++ = kb_ch;
      else if (kb_ch != A_BS and kb_ch != 0x7f)
        ttputc(A_BEL);
      else 
        if (size < fsize)
        { ++size; *--str = 0;
				  if (not noecho)
            erasechar(row, col + fsize - size);
        }
    }
    else if (size <= 0)
      ttputc(A_BEL);
    else 
    { --size;
      *str++ = kb_ch;
      if (not noecho)
        wputmore(kb_ch);
    }
    kb_ch = ttgetc(0);
  }

  while ((--str) > str_ and *str == ' ')
    ;
  str[1] = 0;
  wgoto(s_row(fld), s_col(fld));
  if (kb_ch > 0)
    clr_alert();

  return OK;
}

void put_choices(optext)
         const Char const * optext[];
{ fast Tabled   tbl = this_tbl;
  const Char ** opt = &optext[-1];
       Field fld;

  goto_fld(W_SEL);      

  while (*++opt != null)
  { put_data(*opt);
    down_fld(0, &fld);
  }

  goto_fld(W_CMD);
}



Cc get_any_data (str, fld_ref)
	Char   *str;
	Field  *fld_ref;
{ fast  Tabled  tbl = this_tbl;

  *str = 0;
  while (true)
  { Cc cc = interpret_move(fld_ref);
    if (cc > 0)
      return cc;

  { Field fld = *fld_ref;
    Short row = s_row(fld);
    Short col = s_col(fld);

    if (row >= g_rows)
      row = 0;

    if (fld->props & FLD_NOCLR)
      wclrfld(row, col, 0, gfldfillch);
    else
    { savefld(row, col, fld->class->size);
      wclrfld(row, col, fld->class->size, gfldfillch);
      wgoto(row, col);
    }

    cc = g_data(fld, str);
    if (kb_ch > 0)
      return cc;
  }}
}

Cc get_data(str)
	Char   *str;
{ fast Tabled  tbl = this_tbl;
       Field fld = (Field)&tbl->c[tbl->curr_fld]; 
       Short row = s_row(fld);
       Short col = s_col(fld);

  if (row >= g_rows)
    row = 0;

  helpfld();
  if (fld->props & FLD_NOCLR)
    wclrfld(row, col, 0, gfldfillch);
  else
  { savefld(row, col, fld->class->size);
    wclrfld(row, col, fld->class->size, gfldfillch);
    wgoto(row, col);
  }
  str[0] = 0;
  while ((kb_ch = ttgetc(0)) == A_FF)
  { wrefresh();
    wgoto(row, col);
  }
  if (rest_fld.len > 0)
    patch_fld(fld);
  return kb_ch < 0 ? kb_ch != -'H' ? BAD_BUTTON : HALTED :
                     g_data(fld, str);
}


void get_b_str(fsize, noecho, str)
        Short  fsize;
        Bool   noecho;
        Char * str;
{ fast Short size;
  
  for (size = fsize; ;)
  { /* read(0, &kb_ch, 1); */
    kb_ch = ttgetc(0);

    if      (kb_ch < A_SP)
    { if (kb_ch == A_CR or kb_ch == A_LF or kb_ch == A_ESC)
        break;
      if (kb_ch != A_BS)
        kb_ch = A_BEL;
      else 
        if (size < fsize)
        { ++size; *--str = 0;
          if (noecho)
            continue;
        }
    }
    else if (size <= 0)
      kb_ch = A_BEL;
    else 
    { --size;
      str[0] = kb_ch; *++str = 0;
      if (noecho)
        continue;
    }
    ttputc(kb_ch);
  }
  str[0] = 0;
}
