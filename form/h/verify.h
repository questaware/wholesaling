extern Char * vfy_barcode __((const Char *));
extern Id     asc_to_bcsup __((Char *, Id *));
extern  Int    asc_to_pw(Char *);
extern Char * bc_to_asc __((Id, Id, Char *));
extern Char * get_bc __((Char*, Id *, Id *));
extern Char * get_qty __((Char *, Id *));
extern Char * get_cash __((Char *, Cash *));
extern Char * get_b_sname __((Char *, Char *));

extern Char * vfy_str __((Char *, const Char *, Quot));
extern Bool vfy_integer(const Char *);
extern Bool vfy_nninteger(const Char *);
extern Bool vfy_ointeger(const Char *);
extern Bool vfy_cash(const Char *);
extern Bool vfy_real(const Char *);
extern Bool vfy_zrate(const Char *);
extern Bool vfy_postcode(const Char *);
extern Char * get_b_sname(Char*,Char*);

extern const Char scats[];

#define E_ALPHA     1
#define E_NUM       2
#define E_TEL       4
#define E_FNAME     8    /* File names */
#define E_SPACE    16
#define E_SNAME    32
