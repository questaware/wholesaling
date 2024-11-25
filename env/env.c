#include  <stdio.h>
#include  <string.h>
#include  <signal.h>
#include  <term.h>
#include  <termio.h>
#include  <fcntl.h>
#include  <errno.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  "version.h"

#include <sys/types.h>
#include  <sys/stat.h>


#if NONDIR
 #include <dirent.h>
#else
 #if XENIX
 #include  <sys/ndir.h>
 #else
 #include <ndir.h>
 #endif
#endif

#include  "../h/errs.h"
#include  "../h/defs.h"
#include  "tcap.h"
/*
#include  "../h/env.h"
*/
#if XENIX
#include  <sys/vid.h>
#endif

Char *   this_ttype;

#define A_ESC (0x1b)

Int kbdpoll;			/* in O_NDELAY mode		*/
Int kbdflgs;			/* saved keyboard fd flags	*/
Bool kbd_is_file;

struct	termio	otermio;	/* original terminal characteristics */
struct  termio  ntermio;	

#define TCAPSLEN 1024
Char tcapbuf[TCAPSLEN];
Char *CM, *CE, *CL, *SO, *SE, *CS, *SR, *SF, *IS, *KS, *KE;

Bool screxist;
Bool vt100;
Bool ansi;
Bool lincon;

Int  prefixlen;

Term_t term;

Id pen_fildes = -1;

FILE * dbg_file = null; 

/*
extern	unsigned char *tparm(), *tgoto(), *tgetstr(), *tigetstr(),
		    term_parm_err[], *term_err_strings[], *Def_term, *termname();
extern	unsigned char *boolnames[], *boolcodes[], *boolfnames[],
		   *numnames[], *numcodes[], *numfnames[],
		   *strnames[], *strcodes[], *strfnames[];
extern	void  tputs(), putp(), vidputs(), vidattr();

extern	Int   resetterm(), fixterm(), saveterm(), restartterm(), del_curterm(),
		delterm();

extern	void  termerr(), tinputfd();
*/

public void ttopen(fildes)
      int   fildes;
{ struct stat stat_;
  Cc cc = fstat(fildes, &stat_);
  kbd_is_file = cc == OK and (stat_.st_mode & S_IFREG) != 0;
  cc = ioctl(fildes, TCGETA, &otermio);	/* save old settings */
  if (cc != 0 and errno != ENOTTY)
    i_log(cc,"Ioctl failed %d", errno);
#if 0
  ntermio = otermio;
  ntermio.c_iflag = IGNBRK;	/* was BRKINT *//* setup new settings */
  ntermio.c_lflag = ISIG;
  ntermio.c_oflag = 0;
  ntermio.c_cc[VMIN]  = 1;
  ntermio.c_cc[VTIME] = 2;
#else
    
  /*ntermio = otermio;*/
  ntermio.c_cflag = otermio.c_cflag;
  ntermio.c_line = otermio.c_line;
  ntermio.c_cc[VMIN] = 1;
#if 0  
  ntermio.c_lflag = ISIG;
  ntermio.c_oflag = 0;
  ntermio.c_cc[VTIME] = 2;
#endif
#endif

  cc = ioctl(0, TCSETAW, &ntermio); /* and activate them */ /* was TCSETAW */
  if (cc != 0 and errno != ENOTTY)
    i_log(cc,"Ioctl failed %d", errno);
  if (fildes == 0)
  { kbdflgs = fcntl(fildes, F_GETFL, 0);
    kbdpoll = false;
    setbuf(stdout, null);
  }
}


public void ttignbrk(fildes)
      int   fildes;
{
#if 0
  ntermio.c_iflag = 0; /* IGNBRK */;
  ntermio.c_lflag = 0;
{ Cc cc = ioctl(fildes, TCSETA, &ntermio);
  if (cc != 0)
    i_log(cc,"Ioctl failed");
}
#endif
}



public void ttclose (fildes)
         int    fildes;
{ if (otermio.c_cflag != 0)
  { ioctl(fildes, TCSETA, &otermio); 		/* restore terminal settings */
    fcntl(fildes, F_SETFL, kbdflgs);
  }
}

/********************************/
/* encoding of keytbl index     */
/* 				*/
/*  1 bit : spare	        */
/*  2 bits: 1: 1 dig		*/
/*	    2: 2 dig	        */
/* 	    3: [ dig		*/
/*  5 bits: dig - '0'		*/
/********************************/

static Char keytbl[128];		/* key value */
static Char klentbl[128];		/* length of sequence */

static Vint prefixcode;			/* see ttgetc() for the meaning */
static Vint seqlen;

Vint kseq_to_index(s)
	Char * s;
{ 		/* assume s[0] == 0x1b, s[1] == 'o' or '[' */
  Vint res = 0;
  if      (s[2] == '[')
  { res |= 0x60;
    ++s;
  }
  if (in_range(s[2], '1', '9') and
      in_range(s[3], '0', '9'))
  { if      (s[2] == '1')
      res |= 0x20;
    else if (s[2] == '2')
      res |= 0x40;
    ++s;
  }
  return res | (s[2] - '0') & 0x1f;
}

void def_seq(s, val)
	Char *  s;
	Vint    val;
{ Vint ix = kseq_to_index(s);
  keytbl[ix] = val;
{ Vint len = strlen(s);
  klentbl[ix] = len;
  ix = (s[2] - '0') & 0x1f;
  if (klentbl[ix] < len)
    klentbl[ix] = len;
}}

#define putpad(str) putp(str)
public void ttputc(char) forward;
extern int strncmp ( const char *, const char *, size_t);


public void tcapopen(termtype)        /* The initialisation routine */
        const Char * termtype;
{ Char tcbuf[1024];

  if (termtype == NULL or tgetent(tcbuf, termtype) != 1) 
  { sprintf(&tcbuf[0], "Unknown terminal type %s!", termtype);
		i_log(ENV_FAULT,tcbuf);
    exit(ENV_FAULT);
  }
  vt100 = strncmp(termtype, "vt100", 5) == 0;
  ansi  = strncmp(termtype, "ansi", 4)  == 0;  
  lincon  = strncmp(termtype, "console", 4)  == 0;
  
  term.t_mrow = term.t_nrow = (short)tgetnum("li");
  if (term.t_nrow <= 0) 
	{	i_log(ENV_FAULT, "termcap entry incomplete (lines)");
		exit(ENV_FAULT);
	}
  if (term.t_nrow == 24)
    term.t_nrow = 25;		/* we know best */

  term.t_mcol = term.t_ncol = (short)tgetnum("co");
  if (term.t_ncol <= 0)
	{	i_log(ENV_FAULT, "Termcap entry incomplete (columns)");
		exit(ENV_FAULT);
	}
  
{ Char * p = tcapbuf;
  CL = tgetstr("cl", &p); /* clear screen and cursor home */
  CM = tgetstr("cm", &p); /* cursor motion */
  CE = tgetstr("ce", &p); /* clear to end of line */
  SO = tgetstr("so", &p); /* start stand out */
  SE = tgetstr("se", &p); /* end stand out */
  CS = tgetstr("cs", &p); /* set scroll region */
  if (vt100 && 0 == 1)	  /* scroll reverse */
    SR = "\033[25A\033M";
  else
    SR = tgetstr("sr", &p);

  if (vt100 && 0 == 1)	  /* scroll forward */
    SF = "\033[25B\033D";
  else
    SF = tgetstr("sf", &p);

  screxist = SF != NULL && SR != NULL;
  IS = tgetstr("is", &p); /* extract init string */
  KS = tgetstr("ks", &p); /* extract keypad transmit string */
  KE = tgetstr("ke", &p); /* extract keypad transmit end string */

  if (CL == NULL or CM == NULL or CE == NULL)
	{	i_log(ENV_FAULT, "Incomplete termcap entry\n");
		exit(ENV_FAULT);
	}

  prefixlen = 0;
{ Vint noobasic = 0;
  Char * tgs = tgetstr("ku", &p);
  if (tgs != null)
    def_seq(tgs, -'u');

  tgs = tgetstr("kd", &p);
  if (tgs != null)
    def_seq(tgs, -'d');
  tgs = tgetstr("kl", &p);
  if (tgs != null)
    def_seq(tgs, -'l');
  tgs = tgetstr("kr", &p);
  if (tgs != null)
    def_seq(tgs, -'r');
  tgs = tgetstr("PU", &p);
  if (tgs != null)
  { noobasic += 1;
    def_seq(tgs, -'U');
  }
  tgs = tgetstr("PD", &p);
  if (tgs != null)
  { noobasic += 1;
    def_seq(tgs, -'D');
  }
  tgs = tgetstr("kP", &p);
  if (tgs != null)
    def_seq(tgs, -'U');
  tgs = tgetstr("kN", &p);
  if (tgs != null)
    def_seq(tgs, -'D');
  tgs = tgetstr("kh", &p);
  if (tgs != null)
    noobasic += 1;
  else
    tgs = "\033[H";
  def_seq(tgs, -'H');
  tgs = tgetstr("k1", &p);
  if (tgs != null and tgs[1] == 'O')   /* It's vt100, has no HOME, PU, PD */
    def_seq(tgs, -'H');
  tgs = tgetstr("k2", &p);
  if (tgs != null and tgs[1] == 'O')   /* It's vt100, has no HOME, PU, PD */
    def_seq(tgs, -'D');
  tgs = tgetstr("k3", &p);
  if (tgs != null and tgs[1] == 'O')   /* It's vt100, has no HOME, PU, PD */
    def_seq(tgs, -'U');
   	        
  ttopen(fileno(stdin));
  if (p >= &tcapbuf[TCAPSLEN])
	{	i_log(ENV_FAULT, "Terminal description too big!\n");
		exit(ENV_FAULT);
	}

  if (IS != NULL)			/* send init strings if defined */
    putpad(IS);
#if 0
  if (KS != NULL)
    putpad(KS);
#endif
}}}
 

public void tcapclose()

{ 
#if 0
  if (KE != NULL)		/* send end-of-keypad-transmit string */
    putpad(KE);
#endif
  ttclose(fileno(stdin)); /* surely 0 by definition ! */
}

public void ttputc(c)
	Char c;
{ write(1, &c, 1);
}


public void ttputs(s)
	const Char * s;
{ while (*s != 0)
    write(1, s++, 1);
}
	  

#define endit(x) kill(getpid(), x)


int ttgetc(fildes)
        Id   fildes;
{ Char c, c1 = 0;

  do
  { if (read(fildes, &c, 1) == 0)
      continue;

    if	    (c == A_ESC)
    { Vint ix = 0;
      if (read(fildes, &c, 1) == 0)
	continue; 	 /* O or [ */
      if (read(fildes, &c, 1) == 0)
	continue;
      if (c == '[' or klentbl[(c - '0') & 0x1f] > 3)
      { if (read(fildes, &c1, 1) == 0)
	  continue;
	if	(c == '[')
	  ix |= 0x60;
	else if (ix == 0 and in_range(c1, '0', '9'))
	{ ix |= ((c - '0') << 5);
	  c = c1;
	}
      }
      ix |= (c - '0') & 0x1f;
      if (klentbl[ix] >= 4 and (klentbl[ix] > 4 or c1 == 0))
	if (read(fildes, &c1, 1) == 0)
	  continue;
      return keytbl[ ix ];
    }
    else if (c == 'Z'-'@')
     return -'U';
    else if (c == 'X'-'@')
     return -'D';
    else if ((c & 255) == 0x9b) 	  /* support vt arrow keys */
    { Char b;
      if (read(fildes, &c, 1) ==0)
	continue;
      if (in_range(c, 'A', 'D'))
	return - "udrl"[ 255 & ((int)c - 'A')];
      else
      { if (read(fildes, &b, 1) == 0)
	  continue;
	return - "?HU??D" [ 255 & ((int)c - '1')];
      }
    }
    else
      return c;
  } while (not kbd_is_file);			/* break => endit() */
  endit(SIGTERM);
  sleep(10);
  return 0;
}



int hold()

{ return ttgetc(0);
}

public void ttmove(row, col)
      register int row, col;
{
  putpad(tgoto(CM, col, row));
}


public void tteeol()

{ putpad(CE);
}


public void tteeop()

{ putpad(CL);
}

#if	COLOR

Char fcol[] = "\033[3 m\000";
Char bcol[] = "\033[4 m\000";

public void ttfcol(color)	/* no colors here, ignore this */
  int color;
{ fcol[3] = color + '0';
  ttputs(fcol);
}

public void ttbcol(color)	/* no colors here, ignore this */
  int color;
{ bcol[3] = color + '0';
  ttputs(bcol);
}
#endif

static Char attrbuf[40];
Short	cfcolor = 0x7;		/* current forground color */
Short	cbcolor = 0;		/* current background color */
Short	cmode = 0;
Short	ofcolor = 0x7;
Short	obcolor = 0;

public void ttrev(state)		/* uses ansi modes */
	int  state;
{ if ((state & 0xffff) == 0xffff)
  { ttputs("\033[0m");
    cfcolor = ofcolor;
    cbcolor = obcolor;
    cmode = 0;
  }
  else
  {      Short colf = state & 0xf;
         Short colb = (state & 0xf0) >> 4;
         Short mode = state & 0xf00;
    fast Char * p = &attrbuf[1];
    
    attrbuf[0] = A_ESC;
    if (mode != cmode)
    { if (mode & A_BOLD)
      { *p++ = ';'; *p++ = '1';
      }
      if (mode & A_USCORE)
      { *p++ = ';'; *p++ = '4';
      }
      if (mode & A_BLINK)
      { *p++ = ';'; *p++ = '5';
      }
      if (mode & A_REV)
      { *p++ = ';'; *p++ = '7';
      }
      cmode = mode;
    }
    if (colf != colb)
    { if (colf != cfcolor)
      { *p++ = ';'; *p++ = '3';
        *p++ = '0'+colf;
        cfcolor = colf;
      }
      if (colb != cbcolor)
      { *p++ = ';'; *p++ = '4';
        *p++ = '0'+colb;
        cbcolor = colb;
      }
    }
    *p++ = 'm';
    *p++ = 0;
    attrbuf[1] = '[';
    ttputs(attrbuf);
  }
}

public void ttbeep()

{ ttputc(A_BEL);
}


public void ttscdn()

{ ttmove(g_bot_win, 0);
  putpad(SF);
}


public void ttscup()

{ ttmove(g_top_win,0);
  putpad(SR);
}


public Bool ttcansc()

{ return screxist;
}

public void init_pen()

{/* if (pen_fildes == -1)
  { pen_fildes = open(DEV_PEN, O_RDONLY);
    if (pen_fildes == -1)
      printf("Cannot access Pen");
    ttopen(pen_fildes);
  } */
}


#if 0
/*
public Bool ip_was_pen()

{ fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  FD_SET(pen_fildes, &rfds);
{ Int cc = select(10, &rfds, 0, 0,  0);
  if (cc < 0)
    i_log(cc, "select failed");
  if      (FD_ISSET(0, &rfds))
    return false;
  else if (not FD_ISSET(pen_fildes, &rfds))
    return i_log(false, "select wrong result");
  return true;
}}


public void flush_pen()

{ if (pen_fildes != -1)
    ioctl(pen_fildes, TCFLSH, 0);
}
*/
#endif

#if 0
/*
struct stat fszstat;

public struct stat * examine_fildes(fildes)
	Fildes fildes;
{ 
  return fstat(fildes, &fszstat) < OK ? null : &fszstat;
}


public Int this_fsize()

{ return examf_cc != OK ? 0 : fszstat.st_size;
}



public Int this_moddate()

{ return examf_cc != OK ? 0 : fszstat.st_mtime;
}
*/
#endif
