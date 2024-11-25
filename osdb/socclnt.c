#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <resolv.h>
#include "version.h"

#include "socserv.h"

extern Char * strchr();


#if USE_CONTCP
#define SOCTYP SOCK_STREAM
#else
#define SOCTYP SOCK_DGRAM
#endif



#define SOCKNAME "2100"


Char dbg_file_name[] = "notused";

#define BUFLEN 513

Char  in_buf[BUFLEN + sizeof(Int)];
Char out_buf[BUFLEN + sizeof(Int)];



Soc_addr soc_addr;


ps_dump() {}


static void explain()

{ fprintf(stderr, "socclnt [ [#][@server] | file ]\n"
                  "     #      port number to offer   DEFAULT 2100\n"
                  "     server name of server machine DEFAULT localhost\n"
                  "     file   name of socket\n"
      	       );
  exit(0);
}

void client_req(Char type, Char * msg)

{ out_buf[sizeof(Int)-1] = type;
  strpcpy(&out_buf[sizeof(Int)], msg, sizeof(out_buf)- sizeof(Int));
}




void client_reply(Char in_buf[], Int got)

{ printf("Type %c: %s\n", in_buf[sizeof(Int)-1], &in_buf[sizeof(Int)]);
}

void main(argc,argv)
	Vint argc;
        Char ** argv;
{ Vint argsleft = argc - 1;
  Char ** argv_ = &argv[1];
  Bool i_opt = false;

  for (; argsleft > 0 and argv_[0][0] == '-'; --argsleft)
  { Char * flgs;
    for (flgs = &argv_[0][1]; *flgs != 0; ++flgs)
      if (*flgs == 'i')
        i_opt = true;
      else
        explain();
    ++argv_;
  }

{ Char * srcfn = argsleft <= 0 ? SOCKNAME : argv_[0];
  Int port = not in_range(srcfn[0], '0', '9') ? -1 : atol(srcfn);

  int sid = socket(port >= OK ? AF_INET : AF_UNIX, SOCTYP, 0/*IPPROTO_UDP*/);
  if (sid < 0)
  { fprintf(stderr, "Cannot create socket");
    exit(1);
  }
  
  memset(&soc_addr, 0, sizeof(struct sockaddr_in));
  soc_addr.in.sin_family = AF_INET;
  soc_addr.in.sin_addr.s_addr = htonl(INADDR_ANY);
    
{ int cc = bind(sid, (void*)&soc_addr, port < 0 ? sizeof(soc_addr) 
                                                : sizeof(soc_addr.in));
  if (cc < OK)
  { fprintf(stderr, "Bind failed %d\n", errno);
    exit(1);
  }  

{ int addrlen;
  Char * at = argsleft <= 0 ? null : strchr(srcfn, '@');

  if (port < 0)
  { soc_addr.un.sa_family = AF_UNIX;
    strpcpy(&soc_addr.un.sa_data[0], srcfn, sizeof(soc_addr.un.sa_data)); 
    addrlen = sizeof(soc_addr);
  }
  else
  { memset(&soc_addr, 0, sizeof(soc_addr));
  { Cc cc = inet_aton(at == null ? "127.0.0.1" : at+1, &soc_addr.in.sin_addr);
    if (cc == 0) /* sic(k) */
    { fprintf(stderr, "Cannot find address (%d:%d) %s\n", cc, errno,
                        at == null ? "127.0.0.1" : at+1);
      exit(1);
    }

    soc_addr.in.sin_family = AF_INET;
    soc_addr.in.sin_port = htons(port);
    addrlen = sizeof(soc_addr.in);
  }}
    
{ int ix;
#if USE_CONTCP
  Cc cc = connect(sid, &soc_addr.un, addrlen);

  if (cc < OK)
    fprintf(stderr, "Connection failed %d\n", errno);
  else
#endif
  { for (ix = 0; ++ix <= 10; )
    { client_req('?', "whos there?");
    { Int len = strlen(&out_buf[sizeof(Int)]) + 2;

#if USE_CONTCP
      Cc cc = send(sid, &out_buf[sizeof(Int)-1], len, 0);
#else
      Cc cc = sendto(sid, &out_buf[sizeof(Int)-1], len, 0, 
                      (void*)&soc_addr, addrlen);
#endif
      if (cc < OK)
	fprintf(stderr, "Client send failed (%d) %d\n", errno, cc);
      else
      {
#if USE_CONTCP
        Int got = recv(sid, &in_buf[sizeof(Int)-1], sizeof(in_buf), 0);
#else
        int rlen = addrlen;
        Int got = recvfrom(sid, &in_buf[sizeof(Int)-1], sizeof(in_buf), 0,
                               (void*)&soc_addr, &rlen
                            );
#endif            
	if (got < OK)
	{ fprintf(stderr, "C.Error %d reading sock %d\n", errno);
	}
	else
          client_reply(in_buf, got);
      }
      sleep(1);
    }}
    client_req(0, "bye");
#if USE_CONTCP
    if (  send(sid, &out_buf[sizeof(Int)-1], 6, 0) < 6)
#else
    if (sendto(sid, &out_buf[sizeof(Int)-1], 6, 0, 
                 (void*)&soc_addr, addrlen)      < 6)
#endif
      ;
    close(sid);
  }
}}}}}

