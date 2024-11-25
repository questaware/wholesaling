/b_delete(u,IX_STKLN,&kv_t,&lra);$/a\
  if (not (s->props & P_OBS))\
  { char bc[30];\
    c = b_delete(u,IX_STKLN,&kv_t,&lra);\
    kv_t.string = bc_to_asc(s->supplier,s->line,&bc[0]);\
    if (c == SUCCESS)\
      c = ix_delete(u, IX_STKBC, &kv_t, &lra);\
  }

/b_delete(u,IX_STKLN,&kv_t,&lra);$/d

/ix_insert(u, IX_STKLN, &kv_t, &lra);$/a\
  if (not (s->props & P_OBS))\
  { char bc[30];\
    cc = ix_insert(u, IX_STKLN, &kv_t, &lra);\
    kv_t.string = bc_to_asc(s->supplier,s->line,&bc[0]);\
    if (cc == SUCCESS)\
      cc = ix_insert(u, IX_STKBC, &kv_t, &lra);\
  }

/ix_insert(u, IX_STKLN, &kv_t, &lra);$/d

/b_delall(u, IX_STKLN);/a\
  b_delall(u, IX_STKBC);

/b_delall(u, IX_STKAL);/a\
  b_delall(u, IX_STKAB);

/grace;/s/(char\*)(s+1)/(char*)\&s->grace[8]/

/lastfive;  *prt_int/i\
  if (not (s->next & Q_EFLAG))\
  {

/lastfive;  *(void)rd_int/i\
  if (plra == 0)\
  {

/lastfive;  *(void)rd_int/a\
    s->lastfive = 0; /* cannot survive reformat */

/s->xactn; t < /i\
  }

/xactn; t </ s/xactn/xactn[s->next \& Q_EFLAG ? -ACCHDR : 0]/

/xactn; rd_past/i\
  }

/xactn; rd_past/ s/xactn/xactn[plra == 0 ? 0 : -ACCHDR]/

/tdate;  *prt_int/i\
  if (not (s->next & Q_EFLAG))\
  {

/tdate;  *(void)rd_int/i\
  if (plra == 0)\
  {

/+= sizeof(Tak_t))/i\
  }

/(s+1); t += sizeof(Tak_t))/s/eny/eny[s->next \& Q_EFLAG ? -TAKHDR : 0]/
/'; t += sizeof(Tak_t))/s/eny/eny[plra == 0 ? 0 : -TAKHDR]/

/cc |= g_Stockitem_t/a\
  if (cc==DUP_UK_ATTR)\
  { ii_log(cc,"Duplicate %ld %ld",(Int)((Stockitem)buf)->supplier, (Int)((Stockitem)buf)->line);\
    cc = 0;\
  }
/POSS_WLOCK/a\
    wr_lock(upn);

/POSS_WUNLOCK/a\
    wr_unlock(upn);
