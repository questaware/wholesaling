/* 
** Definitions for routines which allocate blocks of memory and 
** need to identify word boundaries inside it; on machines 
** where an integer is 4 bytes, the boundaries will be longwords 
*/

#define INT_LEN size_of(int)
#define NEXT_INT(p)  ((p + (INT_LEN-1)) & ~(INT_LEN-1))

#define WORD_LEN 2
#define LONG_LEN 4

#define r_word(b,o) (*(unsigned short *)((long)(b)+(long)(o)) )
#define w_word(b,o,v) *(short *)((long)(b)+(long)(o)) = v
#define r_quad(b,o) (*(unsigned long *)((long)(b)+(long)(o)) )
#define w_quad(b,o,v) *(long *)((long)(b)+(long)(o)) = v



