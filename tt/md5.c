#include <stdlib.h>   
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <openssl/md5.h>

// unsigned char c[MD5_DIGEST_LENGTH];



int g_inbound[2],     // inbound to child
    g_outbound[2];

FILE * filter_cmd(const char * cmd, const char * s)

{ pipe(g_inbound);
  pipe(g_outbound);
  int i;
  pid_t pid = fork();
  if (pid == -1)        //error
  { char *error = strerror(errno);
    printf("error fork!!\n");
    return NULL;
  } 

  if (pid == 0)         // child process
  { close(g_inbound[1]);
    dup2(g_inbound[0], 0);
    dup2(g_outbound[1], 1);
//  sleep(30);

    execlp(cmd, cmd, (char *)NULL); // run command AFTER pipe char in userinput
    char *error = strerror(errno);
    printf("unknown command\n");
    return NULL;
  } 
  else                  // parent process
  { int wlen = write(g_inbound[1],(const void*)s, strlen(s));
    if (wlen < 0)
    { char *error = strerror(errno);
      printf("Write failed %s\n", error == NULL ? "???" : error);
    }
//  sleep(30);
    close(g_inbound[1]);
    int status;
    int cc = wait(&status);
    close(g_outbound[1]);
    
    return fdopen(g_outbound[0], "r");
  }
}

void fini_cmd()

{ close(g_inbound[0]);
  close(g_inbound[1]);
  close(g_outbound[0]);
}



char * calc_md5(char buf[MD5_DIGEST_LENGTH*2+4], 
                const char * input)
{ 
  const char hexdigs[16] = "0123456789abcdef";
  unsigned char c[MD5_DIGEST_LENGTH];
  MD5_CTX mdContext;

  MD5_Init (&mdContext);

  const char * s = input-1;
  while (*++s != 0)
    MD5_Update (&mdContext, s, 1);
  MD5_Final (c,&mdContext);

  for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
  { buf[i*2]   = hexdigs[c[i] / 16];
    buf[i*2+1] = hexdigs[c[i] & 15];
  }
  buf[MD5_DIGEST_LENGTH*2] = 0;

  FILE * in = filter_cmd("md5sum", input);
//printf("input %x\n", in);
  char buf5[MD5_DIGEST_LENGTH*2 + 10];
  char * ins = in == NULL ? NULL : fgets(buf5, sizeof(buf5), in);
  if (ins == NULL)
    printf("Md5sum failed\n");
//else
//  printf("Md5sum says %s\n", ins);
  if (in != NULL)
    fclose(in);
  fini_cmd();
  return buf;
}
