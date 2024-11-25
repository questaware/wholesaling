#define SUCCESS 0
#define NOT_FOUND 1
#define HALTED 2
#define PAGE_UP 3
#define PAGE_DOWN 4
#define BAD_BUTTON 5


typedef short int Date;
typedef Int       Time;

typedef void  (*Nullproc)();

typedef long int Lra_t;
typedef Lra_t * Lra;

#define BLOCK_SIZE 1024
		/* the above must be a power of two */
#define LOG_BLOCK_SIZE 10

#define bad_ptr(x) (((int)x & 0xffffff00) == 0)

#define ULIM_MAX 36700160

#define ROOT_PAGE 0L

#define LDATE    10

#ifdef __cplusplus

extern "C" {

#endif

#define mrealloc(p,n) realloc((char *)(p), n)

extern void malloc_init(void);
extern void ps_dump(void);

extern Char * chk_malloc __((Int));

extern void native_cp(Byte *, Byte *, Short);
extern Int read_page(Short, Int, void *);
extern Bool write_page(Short, Int, void *);
extern Int extend_file(Short, Page_nr);

extern Char * strmatch(const Char *, const Char *);
extern Char * prepare_dfile(char * t, const char * root, int id);

extern Short struccmp(Char *, Char *);
extern Char * strpcpy(Char *, const Char *, Short);
extern Char * strpcpydelim(Char * t_, const Char * s_, Short n, Char delim);
extern Char * strchcpy(Char *, Char, Short);
//extern Char * strcat(/*t, s*/);
//extern Char * strchr(/*t, c*/);
extern Char * skipspaces(/*s*/);
extern Char * skipalnum(/*s*/);
extern Char * skipto(Char * s_, Char delim);
extern Char * skipover(Char * s_, Char delim);

extern void init_fmtform(void);
extern void chop3(Char**, Char *, Char);

extern Date day_no(Short, Short, Short);
extern Short to_week(Date);
extern Short next_month(Date);
extern int hold(void);
extern void ps_init(void);
extern void fini_log(Bool);
extern Cc i_log();
extern Cc do_osys(Char*);
extern void softfini();
extern Int last_file(Char *);

#ifdef __cplusplus
}
#endif

extern const Char bigspaces60[];
#define spaces(n) ((char*)&bigspaces60[60-n])

#define Q_UNLOCKED 0
#define Q_LOCKED   1

#define M_FORCE 4
#define M_DUMMY 8

