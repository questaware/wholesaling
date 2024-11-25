
typedef long Int;
typedef short int Short;
typedef unsigned short int Ushort;
typedef char Char;
typedef unsigned char Byte;
typedef char Sbyte;
typedef short int Bool;
typedef short int Squot;
typedef long Quot;
typedef short int Skind;
typedef long Kind;
typedef short int Cc;
typedef long * Ptr;
typedef unsigned short Shortset;
typedef unsigned long Set;
typedef short Fildes;
typedef int Sid;
typedef long Id;
typedef Short Ldn;
typedef Int Page_nr;
typedef short Offs;
typedef long Cash;

#define fast register

#define emptyset 0

#define in_range(a,b,c) ((unsigned)((a) - (b)) <= (c) - (b))
// #define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)
#define bits(n)  ((1 << n) - 1)
#define OK 0
#define null (void *)0

#ifdef __cplusplus

#else 

#define true 1
#define false 0

#define and &&
#define or ||
#define not !

#define public 
#define readpublic 
#define private
#define forward

#endif

#define when break; case
#define otherwise break; default:

#define align(v, sz) (((v) + (sz) - 1) & ~((sz) - 1))

#define new(typ)     (typ *)malloc(sizeof(typ))
#define null_sct(typ) (*(typ*)&nulls[0])

#define minv(a,b) (a < b ? a : b)
#define maxv(a,b) (a > b ? a : b)

//#define foreach (a,b) for(a = b; a != null; a = a->next)
//#define following (f,a,b) for (a = b; a != null; a = a->f)

#ifdef IN_VERSIONH
extern Int trash;
extern const Char nulls[1024];
#endif
