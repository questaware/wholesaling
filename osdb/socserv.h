typedef union soc
{ struct sockaddr    un;
  struct sockaddr_in in;
  Char               buf[2+256];
} Soc_addr;

