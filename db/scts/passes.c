#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  "../../h/base.h"
#include  "../../h/prepargs.h"

#define SUCCESS 0

#define offsetof(sct, fld) ((Int)&(sct).fld - (int)&(sct))


//extern Char * malloc();
extern Char * strmatch();

extern Ptr newdict();
extern Ptr dict();
extern Ptr rddict();


Char dbg_file_name[50];

typedef struct Fielddescr_t
{ Char * modename;
  Char * fieldname;
  Char * keyquot;
  Bool   repeated;
  struct Fielddescr_t * next;
} Fielddescr_t, * Fielddescr;


typedef struct Sctdescr_t
{ Char *     name;
  Squot      su;
  Bool       seq;
  Char *     sctname;
  Char *     keyquot;
  Fielddescr fldlst;
  struct Sctdescr_t * next;
} Sctdescr_t, *Sctdescr;


typedef struct Strdescr_t
{ Char *     name;
  Squot      su;
  Short      size;
} Strdescr_t, *Strdescr;



#define K_SCT 1 
#define K_UNI 2
#define K_STR 3
#define K_NXT 4
#define K_UKN 5


#define P printf

Char nulls[512];

private Char * alloc_word(s_)
       Char * s_;
{ fast Char * s = s_;
  
  while (*s != 0 and *s <= ' ')
    ++s;
    
{ Char * stt = s;

  while (*s == '_' or isalpha(*s) or isdigit(*s))
    ++s;
    
{ Short len = s - stt;
  Char * t_ = malloc(len + 2);
  Char * t = t_;
  s = stt;

  while (--len >= 0)
    *t++ = *s++;
  *t = 0;
  return t_;
}}}



public Char * skipspace(s)
    Char * s;
{ while (*s != 0 and *s <= ' ')
    ++s;
  return s;
}    
  
static char line[400];
static Ptr modes;
Sctdescr_t sctlst; 


static Cc bld_dict()

{ Bool cont = true;
  Sctdescr   prevsct = &sctlst;
  sctlst.next = null;

  modes = newdict(8);
  
  while (cont and fgets(line, 400, stdin) != null)
   
    if (*strmatch("typedef", line) == 0)
    { Char * ln = skipspace(&line[7]);
      Squot suc = *strmatch("struct", ln) == 0 ? K_SCT :
                  *strmatch("union", ln)  == 0 ? K_UNI :
                  *strmatch("Char", ln)   == 0 ? K_STR : 0;
      if (suc == 0)
        continue;
        
      if (suc == K_STR)
      { Char * mname = alloc_word(&line[12]);
        ln = skipspace(&ln[4]);
        ln = strmatch(ln,mname);
        ln = skipspace(ln);
        if (*ln == '[')
        { Strdescr sd = new(Strdescr_t);
          sd->name = mname;
          sd->su   = suc;
          sd->size = 0;
        { Ptr eny = dict(&modes, mname);
          eny[0] = (Int)mname;
          eny[1] = (Int)sd;
        }}
        continue;
      }

      ln = skipspace(&ln[6]);
    { Char * sctname = alloc_word(ln);
      Char * ln_ = skipspace( strmatch(ln,sctname) );
      Bool key = *strmatch("KEY(",ln_) == 0;
      Char * keyquot = not key ? null : alloc_word(strmatch(ln_, "KEY("));
      Bool seq = false;
      Fielddescr_t fldlst;
      Fielddescr   prevfld = &fldlst;
      fldlst.next = null;

      while (true)
      { ln = skipspace(fgets(line, 400, stdin)); 
        if (*ln == /* { */ '}')
          break;
				if (ln[0] == '/' and ln[1] == '*')
				  continue;
        if (prevfld == &fldlst and *ln++ != '{' /* } */)  
          i_log(55, "struct bracket must be first on next line");
        
        ln = skipspace(ln);
      { Char * mdname = alloc_word(ln);
        ln = skipspace(strmatch(ln, mdname));
      { Char * fldname = alloc_word(ln);
        Bool repeated = false;
        ln = skipspace(strmatch(ln, fldname));
        if      (*ln == '[')
          repeated = true;
        else if (*ln != ';')
          i_log(55, "expected semicolon");
        ln = skipspace(ln+1);
      { Bool key_ = *strmatch("KEY(", ln) == 0;
        Fielddescr fld = new(Fielddescr_t);
        fld->modename  = mdname;
        fld->fieldname = fldname;
        fld->keyquot   = not key_ ? null : alloc_word(strmatch(ln, "KEY("));
        fld->repeated  = repeated;
        fld->next      = 0;
        prevfld->next = fld;
        prevfld = fld;
        
        if (strcmp(mdname, "Dba_t") == 0 and strcmp(fldname,"next") == 0)
          seq = true;
      }}}}
    { Char * msctname = alloc_word(&ln[1]);
      Sctdescr sct  = new(Sctdescr_t);
      sct->name     = msctname;
      sct->su       = suc;
      sct->sctname  = sctname;
      sct->keyquot  = keyquot;
      sct->seq      = seq;
      sct->fldlst   = fldlst.next;
      sct->next     = 0;
    { Ptr eny = dict(&modes, msctname);
      eny[0] = (Int)msctname;
      eny[1] = (Int)sct;
      prevsct->next = sct;
      prevsct = sct;
    }}}}
    
  return SUCCESS;
}
      
private Cc process_dict()

{ Sctdescr slst;
  Fielddescr flst;
    
  P("BEGIN\n");
  
  for (slst = sctlst.next; slst != null; slst = slst->next)
  { P("Structure %s\n", slst->name);
    for (flst = slst->fldlst; flst != null; flst = flst->next)
    { 
      Sctdescr sctd = (Sctdescr)rddict(&modes,flst->modename);
      Char * kind = sctd == null  ? "unknown" :
                    sctd->su == 1 ? "struct " : 
                    sctd->su == 2 ? "union  " : 
                    sctd->su == 3 ? "string " : "error";
      
      P("Field %s Mode %s %s %s\n", 
                flst->fieldname, flst->modename, 
                flst->keyquot == null ? "" : flst->keyquot,
                kind);
    }
  }
  P("END\n");
  
  return SUCCESS;
}

private Cc gen_mkdests()

{ Sctdescr slst;

  for (slst = sctlst.next; slst != null; slst = slst->next)
  { if (slst->sctname[0] == '_')
    { Fielddescr flst;
      Short  keyct_ = 0;
      Short  keyct = 0;
      for (flst = slst->fldlst; flst != null; flst = flst->next)
        if (flst->keyquot != null)
          keyct += 1;

      P("Cc d_%s(Sid u, %s* s);\n\n", slst->name, slst->name);

      P("Cc i_%s(Sid u,%s * s,Lra plr)\n", slst->name, slst->name);
      
      P("{ Key_t kv_t;  Char kvstr[512];\n");
      P("  Lra_t lra;\n");
      P("  Cc cc;\n");

      if (slst->seq)
      { P("  s->next = *plr == 0 ? 0 : Q_EFLAG;\n");
      }
        P("  cc = new_init_rec(u, CL%s, s, &lra);\n", slst->sctname);
        P("  if (cc)\n    return cc;\n");
      if (slst->seq)
      { P("  if (*plr != 0)\n");
        P("    upd_nxt_fld(u, CL%s, *plr, offsetof(*s, next), lra);\n",
                                slst->sctname);
      }
    
      for (flst = slst->fldlst; flst != null; flst = flst->next)
      { 
        Sctdescr mdd = (Sctdescr)rddict(&modes,flst->modename);
        Quot k = mdd == null ? K_UKN : mdd->su;
        Quot kind = k == K_SCT and strcmp(flst->fieldname,"next") == 0 ? K_NXT 
                                                                       : k;
        if (flst->keyquot != null)
        { if     (kind == K_UKN)
            P("  kv_t.integer = s->%s;\n", flst->fieldname);
          else if (kind == K_STR)
          { P("  strpcpy(&kvstr[0], s->%s, sizeof(%s));\n", 
                               flst->fieldname, slst->name);
            P("  kv_t.string = &kvstr[0];\n");
          }
          else 
            P("????");
        
          if (slst->seq)
            P("  if (*plr == 0)\n");
          P("  cc = ix_insert(u, %s, &kv_t, &lra);\n",flst->keyquot);
          P("  if (cc)\n  { d_%s(u,s); return cc;\n  }\n", slst->name);
        }
      }           /* for (flst ...) */
        
      P("  *plr = lra;\n");
      P("  return 0;\n}\n\n");                /* end of i_%s() */

      if (keyct == 0)
        continue;
        
      P("Cc d_%s(Sid u, %s* s)\n", slst->name, slst->name);
    
      P("{ Key_t kv_t;  Char kvstr[512];\n");
      P("  Lra_t lra, plra;\n");
      P("  Cc cc = OK, c = OK;\n");

      for (flst = slst->fldlst; flst != null; flst = flst->next)
      { 
        Sctdescr mdd = (Sctdescr)rddict(&modes,flst->modename);
        Quot k = mdd == null ? K_UKN : mdd->su;
        Quot kind = k == K_SCT and strcmp(flst->fieldname,"next") == 0 ? K_NXT 
                                                                       : k;
        if (flst->keyquot != null)
        { if     (kind == K_UKN)
            P("  kv_t.integer = s->%s;\n", flst->fieldname);
          else if (kind == K_STR)
          { P("  strpcpy(&kvstr[0], s->%s, sizeof(%s));\n", 
                               flst->fieldname, slst->name);
            P("  kv_t.string = &kvstr[0];\n");
          }
          else 
            P("????");

	  P(keyct_ == 0 ? "  cc" : "  c");        
          P("  = b_delete(u,%s,&kv_t,&lra);\n",
                                     flst->keyquot,flst->keyquot,flst->keyquot);
          if (++keyct_ > 1)                                   
          { P("  if (lra != plra) c = CORRUPT_DB;\n");
            P("  if (c < cc)\n    cc = c;\n");
          }
          if (keyct_ < keyct)
            P("  plra = lra;\n");
        }
      }           /* for (flst ...) */

      P("  if (cc == OK)\n");
      if (not slst->seq)
        P("    cc = del_rec(u, CL%s, lra);\n", slst->sctname);
      else 
      { P("  { Lra_t nlra;\n");
        P("    while (lra != 0)\n");
        P("    { s = (%s *)rec_ref(u, lra);\n", slst->name);
        P("      nlra = s == null ? 0 : (s->next & (Q_EFLAG-1));\n");
        P("      cc |= del_rec(u, CL%s, lra);\n", slst->sctname);
        P("      lra = nlra;\n");
        P("    }\n");
        P("  }\n");
      }
        
      P("  return cc;\n}\n\n");                /* end of d_%s() */
     
    }
  }
}

private Cc gen_ios()

{ Sctdescr slst;
  Fielddescr flst;

  P("#ifdef DBASCIO\n");

#ifdef CLNM
  P("char * class_Names[] = {\n");
  
  for (slst = sctlst.next; slst != null; slst = slst->next)
    P(" \"%s\",\n", slst->name);
  

  P(" 0\n};\n\n");
#endif
  P("#define Q_EFLAG 0x40000000\n\n");

  for (slst = sctlst.next; slst != null; slst = slst->next)
  { Bool need_nl = false;
    P("void o_%s(Sid u,%s * s,char * k)\n{ char * t;\n",
                                              slst->name, slst->name);
    if (slst->seq)
      P("  if (s->next & Q_EFLAG) return;\n");
    if (slst->sctname[0] == '_')
    { P("  printf(\"CL%s \");\n", slst->sctname);
      /*need_nl = true;*/
    }
    if (slst->keyquot != null)
    { P("  if (k != null)\n    { printf(\" \"); prt_key(k,%s); }\n", slst->keyquot);
      need_nl = true;
    }
    if (need_nl)
      P("    prt_nl();\n");
    if (slst->seq)
      P("  while (s != 0)\n  {\n    printf(\"*\");\n");
    for (flst = slst->fldlst; flst != null; flst = flst->next)
    { 
      Sctdescr mdd = (Sctdescr)rddict(&modes,flst->modename);
      Quot k = mdd == null ? K_UKN : mdd->su;
      Squot kind = k == K_SCT and strcmp(flst->fieldname,"next") == 0 ? K_NXT 
                                                                      : k;
      if (not flst->repeated)
        P("    t = (char*)&s->%s;", flst->fieldname);
      else 
      { P("    for (t =(char*)&s->%s; t < (char*)(s+1); t += sizeof(%s))\n",
            flst->fieldname, flst->modename);
        P("    { prt_rptr();\n");
      }

      if      (kind == K_UKN)
        if (not flst->repeated)
          P("      prt_int(t,sizeof(%s));\n", flst->modename);
        else
          P("      prt_hex(t,sizeof(%s));\n", flst->modename);
      else if (kind == K_SCT)             /* struct */
        P("      o_%s(u,t,0);\n", flst->modename);
      else if (kind == K_UNI)             /* union */
        P("      prt_hex(t,sizeof(%s));\n", flst->modename);
      else if (kind == K_STR)             /* string */
        P("      prt_str(t,sizeof(%s));\n", flst->modename);
      else if (kind == K_NXT)
        ;
      else 
        P("  ????;\n");
      if (flst->repeated)
        P("    }; prt_endr();\n");
    }
    if (slst->seq)
    { P("\n    s = s_offs(s->next) == 0 ? 0 :\n");
      P("       (%s*)rec_ref(u,s->next & (Q_EFLAG - 1));\n", slst->name);
      P("  }\n  prt_endr();\n");
    }
    P(slst->sctname[0] == '_' ? "  prt_nl();\n}\n\n" : "}\n\n");
  }
    
  for (slst = sctlst.next; slst != null; slst = slst->next)
  { 
    P("Cc g_%s(Sid u,%s * s, Bool ins)\n", slst->name, slst->name);

    P("{ fast char * t;\n");
    P("  Lra plra = 0;\n");

    if (slst->seq)
    { P("  Cc  nc, cc = 0;\n");
      P("  while (rd_past() == '*')\n  {\n");
    }

    for (flst = slst->fldlst; flst != null; flst = flst->next)
    { 
      Sctdescr mdd = (Sctdescr)rddict(&modes,flst->modename);
      Squot k = mdd == null ? K_UKN : mdd->su;
      Squot kind = k == K_SCT and strcmp(flst->fieldname,"next") == 0 ? K_NXT 
                                                                      : k;
      if (not flst->repeated)
        P("    t = (char*)&s->%s;", flst->fieldname);
      else 
      { P("    for (t =(char*)&s->%s; rd_past() == '*'; t += sizeof(%s))\n",
                                               flst->fieldname, flst->modename);
        P("    { if (t >= (char*)(s+1)) break;\n");
      }
      
      if      (kind == K_UKN)
        if (not flst->repeated)
          P("  (void)rd_int(sizeof(%s),t);\n", flst->modename);
        else
          P("      rd_hex(sizeof(%s),t);\n", flst->modename);
      else if (kind == K_SCT)             /* struct */
        P("  g_%s(u,t,false);\n", flst->modename);
      else if (kind == K_UNI)             /* union */
        P("  rd_hex(sizeof(%s),t);\n", flst->modename);
      else if (kind == K_STR)             /* string */
        P("  rd_str(sizeof(%s),t);\n", flst->modename);
      else if (kind != K_NXT)
        P("  ????;\n");

      if (flst->repeated)
        P("    };\n");
    }
    if (slst->sctname[0] == '_')
      if (slst->seq) 
      { P("    if (ins)\n    { nc = i_%s(u,s,&plra);\n", slst->name);
        P("      if (nc) cc = nc;\n    }\n");
      }
      else
        P(" return i_%s(u,s,&plra);\n", slst->name);
    
    if (slst->seq)
    { P("  }\n");
      P("  return cc;\n");
    }
    P("}\n\n");                /* end of g_%s() */
  }

  for (slst = sctlst.next; slst != null; slst = slst->next)
  { if (slst->sctname[0] != '_')
      continue;
    P("Cc r_%s(u,lr,s) Sid u; Lra lr; %s * s;\n", slst->name, slst->name);

    P("{ Cc cc = 0;\n");
    P("  Key_t kv_t;\n");
    
    if (slst->seq)
      P("  if (s->next & Q_EFLAG) return;\n");
      
    for (flst = slst->fldlst; flst != null; flst = flst->next)
    { 
      Sctdescr mdd = (Sctdescr)rddict(&modes,flst->modename);
      Squot k = mdd == null ? K_UKN : mdd->su;
      Squot kind = k == K_SCT and strcmp(flst->fieldname,"next") == 0 ? K_NXT 
                                                                      : k;
      if (flst->keyquot != null)
      { if      (kind == K_UKN)
          P("  kv_t.integer = s->%s;\n", flst->fieldname);
        else if (kind == K_STR)             /* string */
          P("  kv_t.string = &s->%s[0];\n", flst->fieldname);
        
        P("  cc |= ix_insert(u, %s, &kv_t, lr);\n",flst->keyquot);
      }
    }
    P(" return cc;\n");
    P("}\n\n");

    P("Cc di_%s(Sid u)\n{", slst->name);
    P("  Cc cc = 0;\n");

    for (flst = slst->fldlst; flst != null; flst = flst->next)
    { 
      Sctdescr mdd = (Sctdescr)rddict(&modes,flst->modename);
      Squot k = mdd == null ? K_UKN : mdd->su;
      Squot kind = k == K_SCT and strcmp(flst->fieldname,"next") == 0 ? K_NXT 
                                                                      : k;
      if (flst->keyquot != null)
        P("  cc |= b_delall(u, %s);\n",flst->keyquot);
    }
    P("  return cc;\n");
    P("}\n\n");
  }
  
  P("void o_db(Sid upn, char * clnm)\n{\n");
  
  for (slst = sctlst.next; slst != null; slst = slst->next)
   if (slst->sctname[0] == '_')
   { if (slst->keyquot == null)
       P(" if (clnm == 0 || !strcmp(\"%s\", clnm)) prt_cl(upn, CL%s, o_%s);\n", 
           slst->name, slst->sctname, slst->name);
     else 
       P(" if (clnm == 0 || !strcmp(\"%s\", clnm)) prt_ix(upn, %s, o_%s);\n", 
           slst->name, slst->keyquot, slst->name);
   }
  
  P("}\n\n");
  
  P("static Char buf[512];\n\n");
  
  P("Cc i_db(Sid upn)\n{ Cc cc = 1;\n");
  P("  int lct = 0;\n");
  
  P("  while (rd_past() == 'C')\n");
  P("  { Cc cc = 0;\n");
  P("    Char cl_name[40];\n    rd_name(39,&cl_name[0]);\n");
  P("    /*POSS_WLOCK*/\n");
  P("  { Lra_t lra;\n");
  P("    ++lct;\n");
  
  for (slst = sctlst.next; slst != null; slst = slst->next)
    if (slst->sctname[0] == '_')
    { P("    if (strcmp(&cl_name[1], \"%s\") == 0) {\n",slst->sctname);
      if (slst->keyquot != null)
      { Sctdescr mdd = (Sctdescr)rddict(&modes,slst->keyquot);
        Squot k = mdd == null ? K_UKN : mdd->su;
        Squot kind = k == K_SCT and strcmp(flst->fieldname,"next") == 0 ? K_NXT 
                                                                        : k;
        P("      Key_t kv_t;\n");
        P("      Char keystr[200+0+0];\n");
        if (kind == K_UKN)
          P("      rd_int(sizeof(Int),&kv_t.integer);\n");
        else 
        { P("      rd_str(size_ix[\"%s\"],&keystr[0]);", slst->keyquot);
          P("      kv_t.string = &keystr[0];\n");
        }
      }
      P("      cc |= g_%s(upn, buf, true);\n", slst->name);
      
      if (slst->keyquot != null)
        P("    ix_insert(upn, %s, &kv_t, &lra);\n", slst->keyquot);
        
      P("  } else\n");
    }
  
  P("   i_log(0, \"Unknown Class CL%%s\", cl_name);\n");
  P("   if (cc)\n   { char buf[20]; sprintf(buf, \"%%ld rec %%ld\", (Int)cc, (Int) lct);\n");
  P("     ii_log(cc,\"Error no %%s at %s with %%s\",(Int)buf,(Int)&cl_name[2]);\n");
  P("     break;\n");
  P("   }\n");
  P("    /*POSS_WUNLOCK*/\n");
  P("}}}\n\n");

  P("Nullproc di_Type_arr [] =\n");
  P("{0,\n");
  
  for (slst = sctlst.next; slst != null; slst = slst->next)
   if (slst->sctname[0] == '_')
     P(" di_%s,\n", slst->name);
  
  P("};\n\n");

  P("Nullproc r_Type_arr [] =\n");
  P("{0,\n");
  
  for (slst = sctlst.next; slst != null; slst = slst->next)
   if (slst->sctname[0] == '_')
     P(" r_%s,\n", slst->name);
  
  P("};\n\n");
    
  P("void rbld_db(Sid upn, char * clnm)\n{\n");
  
  for (slst = sctlst.next; slst != null; slst = slst->next)
   if (slst->sctname[0] == '_')
     P(" if (clnm == 0 || !strcmp(\"%s\",clnm))"
       "   r_ix(upn, CL%s, di_%s, r_%s);\n", 
         slst->name, slst->sctname, slst->name, slst->name);
  
  P("}\n\n");

  P("Cc d_db_class(Sid upn, char * sctnm)\n");
  P("{ Cc cc = NOT_FOUND;\n");
  
  for (slst = sctlst.next; slst != null; slst = slst->next)
   if (slst->sctname[0] == '_')
   { P(" if (!strcmp(sctnm, \"%s\"))\n"
       " { cc = di_%s(upn);\n", 
         slst->name, slst->name);
     P("   del_all_recs(upn,CL%s);\n  }\n",slst->sctname);
   }
  
  P("  return cc;\n}\n\n");
  P("#endif\n");
}



int main (argc,argv)
	Int argc;
        Char ** argv;
{
  Char * ofn = "/tmp/pjsdbcf";
  Char * srcfn = null;

  Int argsleft = argc - 1;
  Char ** argv_ = &argv[1];
  
  for (; argsleft > 0 and argv_[0][0] == '-'; --argsleft)
  { Char * flgs;
    for (flgs = &argv_[0][1]; *flgs != 0; ++flgs)
      if      (*flgs == 'o' && argsleft > 1)
      { --argsleft;
        ++argv_;
        ofn = argv_[0];
      }
      else if (*flgs == 'a')
        ;
    ++argv_;
  }

  if (argsleft <=- 0)
  { fprintf(stderr, "passes [-o fn] inputfn\n");
    exit(1);
  }

  srcfn = argv_[0];

  if (srcfn != null)
  { FILE * cc = freopen(srcfn, "r", stdin);
    if (cc == null)
    { i_log(1,"Cannot find %s", srcfn);
      exit(3);
    }
  }
  
  bld_dict();
  process_dict();

  fclose(stdout);
  (void)freopen(ofn, "w", stdout);
  gen_mkdests();
  gen_ios();
  exit(0);
  return 0;
}
