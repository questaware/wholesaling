#define AT_REC_SZ 64

#define MSGKEY ((key_t)103)

#define SPECIAL_MSGLEN 1

#define WASTE_AREA (10 * AT_REC_SZ)

#define FORMAT_TAG 0xf6f6f6f6 
#define TRAIL_TAG  0xa0d1a0d1

#define Q_CHG_DSK 1
#define Q_PSH_DSK 2
#define Q_POP_DSK 3
#define Q_DATA    4
		/* otherwise the pid and 'D' => die, 
					 'R' => signal reply on SIGUSR1 
		*/
#define maxfdsize 737280


typedef struct Msgbuf 
{ long	mtype;		        /* message type */
  char	mtext[BLOCK_SIZE];	/* message text */
} Msgbuf_t, *Msgbuf;


typedef struct Au_header
{ Int  tag;
  Int  size;		/* of contents only */
  Int  time;
  Char filler[AT_REC_SZ - 3 * sizeof(Int)];
} Au_header_t, * Au_header;


extern Int c_sum();

extern Fildes open_read_at __((Char *));
extern Char * rd_next __((Fildes));
extern Cc rd_nxt_rec __((Fildes, Char **));

