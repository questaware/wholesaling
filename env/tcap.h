typedef struct
{	short	t_mrow; 		 /* max number of rows allowable */
	short	t_nrow; 		/* current number of rows used	*/
	short	t_mcol; 		/* max Number of columns.	*/
	short	t_ncol; 		/* current Number of columns.	*/
	short	t_margin;		/* min margin for extended lines*/
	short	t_scrsiz;		/* size of scroll region "	*/
	int	t_pause;		/* # times thru update to pause */
} Term_t;

extern Term_t term;

#define A_BOLD   0x100
#define A_USCORE 0x200
#define A_BLINK  0x400
#define A_REV    0x800

extern Char *CM, *CE, *CL, *SO, *SE, *CS, *SR, *SF, *IS, *KS, *KE;
extern Short g_top_win, g_bot_win;

extern void tcapopen(const Char *);
extern void tcapclose(void);

extern void ttputc(char);
extern void ttputs(const char *);

extern int ttgetc(Id id);

extern int hold(void);
extern void init_pen();

extern void ttmove(register int,register int);
extern void tteeol(void);
extern void tteeop(void);
extern void ttrev(int);
extern void ttfcol(int);
extern void ttbcol(int);
extern void ttbeep(void);

extern void ttscup(void);
extern void ttscdn(void);
extern Bool ttcansc(void);
extern void ttignbrk(int);

