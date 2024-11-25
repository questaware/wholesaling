#define ID_FAC 256


typedef struct
{ Set    props;
  Short  id;
  Short  row, col;
  Short  size;
  Short  rptct;
  Char * value;
} Fieldrep_t, * Fieldrep;

#define W_CMD  (1)
#define W_MSG  (2)
#define W_KEY  (2 * ID_FAC)
#define W_Z    (3 * ID_FAC)
#define W_PACK (4 * ID_FAC)
#define W_TKT  (5 * ID_FAC)
#define W_DSCN (6 * ID_FAC)
#define W_PRC  (7 * ID_FAC)
#define W_STK  (8 * ID_FAC)


Fieldrep_t SIform [] = { {W_CMD, FLD_WR, 1, 1, 10,  1, null},
			 {W_MSG, FLD_RD, 1,30, 30,  1, null},
			 {W_KEY, FLD_WR, 3, 0, 16, 15, null},
			 {W_Z,   FLD_WR, 3,17,  1, 15, null},
			 {W_PACK,FLD_WR, 3,19,  3, 15, null},
			 {W_TKT, FLD_WR, 3,23,  4, 15, null},
			 {W_DSCN,FLD_WR, 3,28, 40, 15, null},
			 {W_PRC, FLD_WR, 3,69,  6, 15, null},
			 {W_STK, FLD_WR, 3,76,  4, 15, null},
			 {0,     0,      0, 0,  0,  0, null),
		       };


