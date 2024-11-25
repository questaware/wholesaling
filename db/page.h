		/*** Low level formats
		*/
#define  PTR_SIZE  2
#define  LRA_SIZE  4

		/*** Standard Page Header 
		*/
#define  SEG_TYPE 0

#define s_stype(p) ( (p)[SEG_TYPE] & 3)
#define s_sgrp(p)  (((p)[SEG_TYPE] >> 2) & 0x3f)

#define mk_type_grp(t,g) (((g)<<2)|(t))

#define  REC_SEG	1     /* Class Segment (a property bit)*/
#define  INDEX_SEG	0     /* Primary Index Segment */
#define  INODE_SEG	2     /* Primary Index Segment, non leaf */

#define  M_INDEX_SEG   0xf6   /* Master Index Segment   (whole byte) */
#define  FREE_LIST_SEG 0xfe   /* Free Page List Segment (whole byte) */

#if (INDEX_SEG & 3) != 0 || (INODE_SEG & 3) != PTR_SIZE
  unacceptible values
#endif

#define non_leaf(p)     ((p)[SEG_TYPE] & 3)
#define is_leaf(p)     (((p)[SEG_TYPE] & 3) == 0)

#define Q_EFLAG  0x40000000
#define non_u_page(lra) (lra & Q_EFLAG)


		/* Root Page */
#define  MI_FREE_LIST	 2	/* 2 bytes */
#define  MI_FIRST_UNUSED 4	/* 2 bytes */
#define  MI_REC_TBL	 6	/* open ended */
#define  MI_FILE_SZ    510      /* Size of file on disk */

#define  CE_SIZE     10
#define  CE_RP_SIZE   0
#define  CE_RP	      2
#define  CE_FREE_LIST 4
#define  CE_VERSION   6

#define  IE_SIZE    8
#define  IE_RP_SIZE 0
#define  IE_RP	    2
#define  IE_VERSION 4		

		/* Free Pages */
#define FL_NEXT		2  	/* 2 bytes */

		/* Index Page */

#define  BT_KEY_SIZE    1       /* 1 byte */
#define  BT_NKEYS	2	/* 2 bytes */
#define  BT_BASE        4

		/* Records Page */
#define  RP_FREEVEC0    1	/* 1 byte */
		
#define  RP_NEXT	2	/* 2 bytes */
#define  RP_PREV        4	/* 2 bytes */
#define  RP_FREEVEC     6	/* 2 bytes */  

#define  RP_ALIGN     2		/* alignment for records */
#define  RP_LOG_ALIGN 1

  /* ------------------ ddict ------------------ */

#define MAX_CLASS 25
#define MAX_IX    25

#define MAX_BOTH MAX_CLASS
#if MAX_IX > MAX_CLASS 
#undef MAX_BOTH
#define MAX_BOTH MAX_IX
#endif

typedef struct 
{ Short size_cl;
  Short size_ix;
  Char  kind_ix;
  Char  cl_ix;
  Char  cl_vecsz;
  Char  cl_soffs;
} Szdcr_t, *Szdcr;

extern Szdcr_t ddict[MAX_BOTH];
		
#define  RP_ALIGN 2
#define  RP_LOG_ALIGN 1

#define class_offs(id) (MI_REC_TBL + CE_SIZE * id)
#define ix_offs(id)    (MI_REC_TBL + IE_SIZE * id)

#define infreevec(p,fix) (((fix) < 8 ? (p)[RP_FREEVEC0] \
                                     : (p)[RP_FREEVEC + ((fix)>>3)-1]) \
    			  & (1 << (fix & 7)))


extern Cc get_new_page __((Sid, Squot, Sid, Int *, void *));
extern Cc page_now_free  __((Sid, Int));
extern void limbo_page __((Sid, Int));
extern Cc init_roottbl __((Sid));
extern Cc load_roottbl __((Sid, Bool));
extern Page_nr class_freel __((Sid, Sid));
extern Cc add_class __((Sid, Sid, Short));
extern Cc del_class __((Sid, Sid));
extern Cc add_index __((Sid, Sid, Squot, Short, Sid));
extern Cc del_index __((Sid, Sid));
extern Set vfy_roottbl(Sid, Bool);

