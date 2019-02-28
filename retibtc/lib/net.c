#include <net.h>

/*NET_ENT UTILITY******************************************************************/

int getsock_net_ent(int fd, Net_ent ent)
{
  struct sockaddr_in sock_addr; // local address of socket
  socklen_t sock_addr_len = sizeof(sock_addr);

  if( ( getsockname(fd, (struct sockaddr*) &sock_addr, &sock_addr_len ) ) == -1)
  {
    perror("internal getsock_net_ent error(getsockname)\n");
    return -1;
  }
  /*Port of net_ent*/
  ent->port = (unsigned short) ntohs(sock_addr.sin_port) ; //network to host short
  /* costruzione indirizzo IP tramite inet_ntop (network to presentation) */
  if(inet_ntop(AF_INET, &sock_addr.sin_addr, ent->addr, sizeof(ent->addr)) == NULL)
  {
    perror("internal getsock_net_ent error(inet_ntop)");
    return -1;
  }
  return 0; // return 0 on success
}


int getpeer_net_ent(int fd, Net_ent ent)
{
  struct sockaddr_in sock_addr; // remote address of socket
  socklen_t sock_addr_len = sizeof(sock_addr);
  if( ( getpeername(fd, (struct sockaddr*) &sock_addr, &sock_addr_len ) ) == -1)
  {
    perror("internal getpeer_net_ent error(getpeername): ");
    return -1;
  }
  /*Port of net_ent*/
  ent->port = ((unsigned short) ntohs(sock_addr.sin_port) ); //network to host short

  /* costruzione indirizzo IP tramite inet_ntop (network to presentation) */
  if(inet_ntop(AF_INET, &sock_addr.sin_addr, ent->addr, sizeof(ent->addr)) == NULL)
  {
    perror("internal getpeer_net_ent error(inet_ntop)");
    return -1;
  }
  return 0; // return 0 on success
}


bool compare_net_ent(void *x, void *y)
{
  Net_ent a = (Net_ent) x;
  Net_ent b = (Net_ent) y;
  if( !(strncmp(a->addr, b->addr, LEN_ADDRESS)) && (a->port == b->port) )
    return true;
  return false;
}


void visit_net_ent(void *args)
{
  Net_ent n = (Net_ent)args;
  printf("Net_ent--> IP:port = %s:%hd\n", n->addr, n->port);
}


/*******NET UTILITY****************************/
void fill_address(
  struct sockaddr_in *socket_address,
  sa_family_t         family,
  char *              ip_address,
  unsigned short      port)
  {
    /* inizializza zona di memoria indirizzo */
    memset((void *) socket_address, 0, sizeof(*socket_address));

    socket_address->sin_family = family;
    /*porta in network order*/
    socket_address->sin_port = htons(port);

    /* costruzione indirizzo IP tramite inet_pton (presentation to network)*/
    if ( (inet_pton(family, ip_address, &(socket_address->sin_addr) ) ) <= 0)
    {
      perror("Address creation error");
      exit(EXIT_FAILURE);
    }
  }


bool sha_auth(int server_fd, char *pwd)
{
  unsigned char *hash = (unsigned char *) obj_malloc(SHA256_DIGEST_LENGTH);
  printf("Created hash code with SHA256...\n");
  calculate_hash(pwd, hash);

  printf("sending hash code to server...\n");
  full_write(server_fd, hash, SHA256_DIGEST_LENGTH);
  printf("Waiting for response...\n" );

  short response;
  recv_short(server_fd, &response);
  free(hash);

  if(response)
    return true;
  else
    return false;
}


/**************NET TRAFFIC**************************/
int send_char(int fd, char n)
{
  if( full_write(fd, &n, 1) != 0 )
  {
    perror("send_char()");
    return -1;
  }
  return 0;
}

int recv_char(int fd, char *n)
{
  if(full_read(fd, n, 1) != 0)
  {
    perror("rec_char()");
    return -1;
  }
  return 0;
}



int send_short(int fd, short n)
{
  char buf[8];
  memset((void*) buf, 0, 8);
  sprintf(buf, "%hd", n);
  if( full_write(fd, buf, 8) != 0 )
  {
    perror("send_short()");
    return -1;
  }
  return 0;
}


int recv_short(int fd, short *n)
{
  char buf[8];
  if(full_read(fd, buf, 8) != 0)
  {
    perror("rec_short()");
    return -1;
  }
  sscanf(buf, "%hd", n);
  return 0;
}



int send_int(int fd, int n)
{
  char buf[16];
  memset((void*) buf, 0, 16);
  sprintf(buf, "%d", n);
  if( full_write(fd, buf, 16) != 0 )
  {
    perror("send_int()");
    return -1;
  }
  return 0;
}


int recv_int(int fd, int *n)
{
  char buf[16];
  if(full_read(fd, buf, 16) != 0)
  {
    perror("rec_int()");
    return -1;
  }
  sscanf(buf, "%d", n);
  return 0;
}



int send_net_ent(int fd, Net_ent n)
{
  if ( full_write(fd, n, NET_ENT) != 0 )
  {
    perror("send_net_ent()");
    return -1;
  }
  return 0;
}


int recv_net_ent(int fd, Net_ent n)
{
  if (full_read(fd, n, NET_ENT) != 0)
  {
    perror("rec_net_ent()");
    return -1;
  }
  return 0;
}
