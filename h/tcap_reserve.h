
#define CTRL_	0x0100	/* Control flag, or'ed in	    */
#define META	0x0200	/* Meta flag, or'ed in		    */
#define CTLX	0x0400	/* ^X flag, or'ed in		    */
#define SPEC	0x0800	/* special key (function keys)	    */
#define MOUS	0x1000	/* alternative input device (mouse) */
#define	SHFT	0x2000	/* shifted (for function keys)	    */
#define	ALTD	0x4000	/* ALT key...			    */

#define A_BLACK   0
#define A_RED     1
#define A_GREEN   2
#define A_YELLOW  3
#define A_BLUE    4
#define A_MAGENTA 5
#define A_CYAN    6
#define A_WHITE   7

#define A_BOLD   0x100
#define A_USCORE 0x200
#define A_BLINK  0x400
#define A_REV    0x800

/* The form to ttXXXX interface is
        Nibble   3         2        1        0
                         Mode     BG col   FG col
*/

int ttgetc(Id);
int hold(void);

extern void tcapopen(const Char *);
extern void tcapclose(void);

extern void ttputc(char);
extern void ttmove(register int, register int);
extern void tteeol(void);
extern void tteeop(void);
extern void ttrev(int);
extern void ttfcol(int);
extern void ttbcol(int);
extern void ttbeep(void);

extern void ttscup(void);
extern void ttscdn(void);
extern Bool ttcansc(void);
