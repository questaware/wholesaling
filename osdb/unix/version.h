				/*   
				** Configuration File
				   (Externals)
				*/
#ifndef IN_VERSIONH
#define IN_VERSIONH
					/* key properties */
#define S_UNIX5 0
#define XENIX   0
#define MSDOS   0

#define S_LINUX 1

#define MSC 0
/*#define __STDC__ 0*/

#define COLOR 1

#define SCRN_COLS 80
#define SCRN_LINES 40

#define ANSI	0			/* ANSI escape sequences	*/
#define TERMCAP 1			/* Use TERMCAP			*/
#define	IBMPC	0			/* IBM-PC CGA/MONO/EGA/VGA drvr	*/

#define NONDIR 1
#define SINGLE_USER 0
#define USE_CONTCP 1

#include "../../h/base.h"

#define TMP_PREFIX ','

#define NO_SHARE  
#define DEV_TTY  "/dev/tty"
#define DEV_PEN  "/dev/tty1a"
#define DEV_NULL "/dev/null"

#define OSGOOD_SUGRO_NO "M00550"

#define LONG_ALGNMT 1		/* smallest alignment for longs */

#define LITTLEENDIAN 1

#define WORDSIZE 32
#define BYTESIZE 8

#define CHARCODE ASCII
#define A_BEL   7
#define A_BS    8
#define A_HT    9
#define A_LF    10
#define A_FF    12
#define A_CR	13
#define	A_SP	32

#define MAXINT 0x7fffffff
#define MININT 0x80000000

#define MAXSINT 0x7fff
#define MINSINT 0x8000

#define NaN MININT

#define NFILEN 256

#define CHK_MALLOC 1
							/* conditionals */
#if MSDOS & (TURBO | MSC)
#define	NEAR
#define	DNEAR
#define	PASCAL
#define	CDECL cdecl
#else
#define NEAR
#define	DNEAR
#define	PASCAL
#define	CDECL
#endif

#define CONST const

#if MSDOS
#include <stdlib.h>
#include <io.h>

#else
#if S_LINUX
 #include <stdlib.h>
#else
 #include <prototypes.h>
#endif

#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#if	MSDOS & MSC
#include	<dos.h>
#include	<memory.h>
#define	peek(a,b,c,d)	movedata(a,b,FP_SEG(c),FP_OFF(c),d)
#define	poke(a,b,c,d)	movedata(FP_SEG(c),FP_OFF(c),a,b,d)
#endif

typedef int Vint;       /* set this to short int if it is more efficient */

#define NSTRING 256

#if    S_BSD4_x
typedef int Uid;
#define STRCHR  index
#define STRRCHR rindex
#define MEMCPY(t,s,n) (void)bcopy(s,t,n)
#define memcpr(t,s,n)       bcmp(s,t,n)
#define MEMCLR(s,n)   (void)bzero(s,n)
#define memcpy(t,s,n) (void)bcopy(s,t,n)
#endif 

#if    XENIX 
#define const
#include   <memory.h>

typedef unsigned short Uid;
#define STRCHR  strchr
#define STRRCHR strrchr

#define memcpy(t,s,n) (void)native_cp(t,s,n)
#define memmove(t,s,n) (void)native_cp(t,s,n)
#endif

#ifndef STRCHR
typedef  int Uid;
#define STRCHR  index
#define STRRCHR rindex
//#define memcpy(t,s,n) (void)bcopy(s,t,n)
//#define memcpr(t,s,n)       bcmp(s,t,n)
#endif

#if 0
extern char * malloc();
extern void free();
extern char * realloc();
#endif

#if CHK_MALLOC
#undef malloc
#define malloc chk_malloc
#undef free
extern void chk_free(char* bkt);
#define free(p)  chk_free((char*)p)
#define realloc(s, sz) chk_realloc((char*)s, sz)
#endif

#if __STDC__
#define __(x) x
#else 
#define __(x) ()
#endif

#define CANCHOWN  (S_UNIX5)

#define MSG 1
#define SOCKET 2

#define IPC_TYPE MSG

#define ASMSTR 0

#if	ATARI || MSDOS || OS2
#define	DIRSEPSTR	"\\"
#define	DIRSEPCHAR	'\\'
#else
#define DIRSEPSTR	"/"
#define	DIRSEPCHAR	'/'
#endif

                                      /* Internals */
#define PROGID 1

#define MUSER 1

#define DBFILEM "./dbmain"
#define CMDFILE "./top"

#define COMDIR  "./comments"
#define LOGFILE "./ilog"

#define RACCTSZ 5

#define ACCTXT 0
#define TAKTXT 0
					/* At least MIN_HIST days tb kept */
#define MIN_HIST 100

#define PRT_TO_FILE 0

#endif /* VERSION */
