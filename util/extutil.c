#include   <stdio.h>
#include   <ctype.h>
#include   <sys/types.h>
#include   <sys/stat.h>
#include   <sys/fcntl.h>
#include   <unistd.h>
#include   "version.h"
#include   "../h/defs.h"
#include   "../h/tcap_reserve.h"

#include <errno.h>

#if MSDOS
#include   <dos.h>
#include   <process.h>
#else 
#include   <dirent.h>
#endif

extern long lseek();

extern Char * this_ttype;

Int trash;

Cc do_osys(cmd)
     Char * cmd;
{ if (cmd == NULL)
		system("/home/osdb/bin/shutdown.sh");
	else
	{ tcapclose();
	{ Cc cc = 
#if MSDOS
          	shellprog(cmd);
#else 
          	system(cmd);
#endif
  	tcapopen(this_ttype);
  	return cc;
  }}
}



Cc callprog(conc, cmd, argv)       /* problems with ipc facilities */
        Bool   conc;
        Char * cmd;
        Char **argv;
{ tcapclose();
{ Id pid
#if MSDOS 
  	 = spawnv(P_WAIT, cmd, argv);
#else           
		 = fork();
  if (pid == 0)
    execv(cmd, argv);
#endif

{ int rc = 0;
  Int pid_ = 
#if MSDOS
             pid;
  rc = pid;
#else             
             conc ? pid : wait(&rc);
#endif
  tcapopen(this_ttype);
  return pid == pid_ and rc == 0 ? OK : -1;
}}}        



Int last_file(dir)
        Char *    dir;
#if MSDOS
{ 
}
#else
{ DIR * dirp = opendir(dir);
  struct dirent * dp;
  Int last = 0;
  if (dirp == null)
    return -1;

  while ((dp = readdir(dirp)) != null)
  { Int i = 0;
    sscanf(dp->d_name, "%d", &i);
    if (i > last)
      last = i;
  }
  
  closedir(dirp);
  return last;
}
#endif

private Char utilbuff[132];


FILE * new_file(dir)
        Char *    dir;
{ FILE * file = null;
  int clamp = 3;

  while (--clamp >= 0)
  { Int last = last_file(dir);
    if (last == -1)
      return null;

    sprintf(&utilbuff[0], "%s/%d", dir, last+1);
    Fildes filix = open(utilbuff, O_CREAT + O_EXCL + O_RDWR, 0644);
    if (filix >= 0)
    { FILE * file = fdopen(filix, "w");
      if (file != null)
        break;
    }
  }
  if (file == null)
    file = fopen("rm /tmp/osdbtmp.txt","w");

  return file;
}



int file_exists(const char * name)

{ struct stat statrec;
  Cc cc = stat(name, &statrec);
	return cc != OK 								   ? 0 :
				 (statrec.st_mode & S_IFREG) ? 1 : 0;
}
