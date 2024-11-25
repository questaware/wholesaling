#include  "version.h"
#include  <stdio.h>
#include  <ctype.h>
#include	"../h/defs.h"
#include	"../h/errs.h"
#include	"../db/page.h"
#include	"../db/recs.h"
#include	"../db/memdef.h"
#include	"../db/cache.h"
#include        "../db/b_tree.h"
#include        "../h/prepargs.h"
#include	"schema.h"
#include	"domains.h"

#define offsetof(sct, fld) ((Int)&(sct).fld - (int)&(sct))

#include "dump.c.c"
