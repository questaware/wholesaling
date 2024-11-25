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

#if USE_CONTCP
#define SOCTYP SOCK_STREAM
#else
#define SOCTYP SOCK_DGRAM
#endif


#define SOCKNAME "2100"

Char dbg_file_name[] = "notused";


#define BUFLEN 513

Soc_addr soc_addr;

Char  in_buf[BUFLEN + sizeof(Int)];
Char out_buf[BUFLEN + sizeof(Int)];



ps_dump() {}



static void explain()

{ fprintf(stderr, "socserv [# | file ]\n"
                  "     #     port number to offer\n"
                  "     file  name of socket\n"
      	       );
  exit(0);
}

Cc serve_reply(Id sid, Char type, Char * msg)

{ out_buf[sizeof(Int)-1] = type;
  strpcpy(&out_buf[sizeof(Int)], msg, sizeof(out_buf)- sizeof(Int));
}



Cc serve_serve(Id sid, Int len, Char type, Char * msg)

{ printf("Got %d:%d: %s\n", len, type, msg);
  
  return serve_reply(sid, 'R', "hello");
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
 int addrlen = sizeof(struct sockaddr_in);
 Int port = not in_range(srcfn[0], '0', '9') ? -1 : atol(srcfn);

  int sid = socket(port > OK ? AF_INET : AF_UNIX, SOCK_STREAM, 0/*IPPROTO_UDP*/);

  struct sockaddr addr_s;
  struct sockaddr_in addr_sin;
  int on = 1;
  
  if (sid < 0)
  { fprintf(stderr, "Cannot create socket");
    exit(1);
  }
  
  if (setsockopt(sid, SOL_SOCKET, SO_REUSEADDR,
                      (char *) &on, sizeof(on)) < 0) /* endian? */
  { fprintf(stderr, "setsockopt failed %d", errno);
    exit(1);
  }            
  
  if (port < 0)
  { soc_addr.un.sa_family = AF_UNIX;
    strpcpy(&soc_addr.un.sa_data[0], srcfn, sizeof(soc_addr.un.sa_data)); 
#if USE_CONTCP == 0
    addrlen = sizeof(soc_addr);
#endif
  }
  else
  { memset(&soc_addr, 0, sizeof(soc_addr));
    soc_addr.in.sin_family = AF_INET;
    soc_addr.in.sin_port = htons(port);
  /*soc_addr.in.sin_addr.s_addr = addr;*/
  }

{ int cc = bind(sid, (void*)&soc_addr, port < 0 ? sizeof(soc_addr) 
                                                : sizeof(soc_addr.in));
  if (cc < OK)
  { fprintf(stderr, "Bind failed %d\n", errno);
    exit(1);
  }  

{ union 
  { Char               b[256];
    struct sockaddr_in sin; 
  } u;
  int salen = 256;
  
  Cc cc = getsockname(sid, (void*)&u, &salen);
  if (cc != OK)
  { fprintf(stderr, "gsn failed %d\n", errno);
    exit(1);
  }
  
  if (u.sin.sin_family != AF_INET)
  { fprintf(stderr, "Is not AF_INET %d\n", u.sin.sin_family);
    exit(1);
  }
  
  printf("Port %d:%x Addr %x\n", 
          u.sin.sin_family, u.sin.sin_port, u.sin.sin_addr.s_addr);
  

{ 
  fd_set  inputs;
  Int     max_input = sid;
  memset(&inputs, 0, sizeof(inputs));
  
  FD_SET(sid, &inputs);
#if USE_CONTCP
  listen(sid, 5);
#endif

  while (true)
  { Cc cc;
#if USE_CONTCP
    fd_set inputs_v;
    fd_set excep_v;
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    memcpy(&inputs_v, &inputs, sizeof(inputs));
    memcpy(&excep_v, &inputs, sizeof(inputs));
    
    cc = select(max_input + 1, &inputs_v, null, &excep_v, &timeout);
    /*printf("RETURNED\n");*/
    if (cc < OK)
    { fprintf(stderr, "Error %d from select\n", errno);
    }
    if (cc > OK)
#endif
    { Int closeix = 0;
      Int ix;
      printf("SEEN\n");
#if USE_CONTCP == 0
      ix = max_input;
#else
      for (ix = max_input + 1; --ix >= 0; )
        if (ix == sid)
        { if      (FD_ISSET(ix, &inputs_v))
          { struct sockaddr client_soc;
            int alen = sizeof(client_soc);
            Int cid = accept(sid, &client_soc, &alen);
            if (cid < OK)
              fprintf(stderr, "accept err %d", errno);
            else
            { FD_SET(cid, &inputs);
              if (max_input < cid)
                max_input = cid;
              printf("ACCEPTING %d\n", cid);
            }
          }
        }
        else if (FD_ISSET(ix, &inputs_v))
#endif            
        { /*printf("Receiving\n");*/
        {
#if USE_CONTCP
          Int got = recv(ix, &in_buf[sizeof(Int)-1], sizeof(in_buf), 0);
#else
#define ix sid
          int rlen = addrlen;
          Int got = recvfrom(sid, &in_buf[sizeof(Int)-1], sizeof(in_buf), 0,
                               (void*)&soc_addr, &rlen
                            );
#endif            
          printf("S Recvd %d\n", got);
          if (got <= OK)
          { if (got == 0 or errno == ENOTCONN)
            { sleep(10);
#if USE_CONTCP
              closeix = ix;
#endif
            }
            else
              fprintf(stderr, "Error %d reading sock %d\n", errno, ix);
          }
          else /* if (got > OK) */
          { 
            printf("Server Recvd\n");
#if USE_CONTCP
            if (in_buf[sizeof(Int)-1] == 0)
              closeix = ix;
            else
#endif
            { serve_serve(ix, got, in_buf[sizeof(Int)-1], 
                                 &in_buf[sizeof(Int)]);
	    { Int len = strlen(&out_buf[sizeof(Int)]) + 2;

#if 0
	      Cc cc = write(ix, &out_buf[sizeof(Int)-1], len);
#elif USE_CONTCP
	      Cc cc = send(ix, &out_buf[sizeof(Int)-1], len, 0);
#else
	      Cc cc = sendto(sid, &out_buf[sizeof(Int)-1], len, 0,
		               (void*)&soc_addr, sizeof(soc_addr));
#endif
	      if (cc < len)
		fprintf(stderr, "serv-send err (%d) %d %d\n", errno, len, cc);
            }}
          }
        }}
#if USE_CONTCP
        else if (FD_ISSET(ix, &excep_v))
          closeix = ix;
#endif
      if (closeix != 0)
      { 
        fprintf(stderr, "Closing %d", closeix);
        FD_CLR(closeix, &inputs);
        close(closeix);
        if (max_input == ix)
          max_input = closeix - 1;
      }
    }
  }

}}}}}
