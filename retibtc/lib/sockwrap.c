#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>

#include <../include/sockwrap.h>

int Socket(int domain, int type, int protocol)
{
  int fd;

  if( ( fd = socket(domain, type, protocol) ) == -1)
  {
      perror("Socket");
      exit(EXIT_FAILURE);
  }
  return fd;
}

int Connect(int sockfd, const struct sockaddr *addr)
{
  socklen_t len = sizeof(*addr);

  if(connect(sockfd, addr, len) == -1)
  {
    perror("Connect");
    exit(EXIT_FAILURE);
  }
  return 0;
}

int Bind(int sockfd, const struct sockaddr *addr)
{
  socklen_t len = sizeof(*addr);

  if(bind(sockfd, addr, len) == -1)
  {
    perror("Bind");
    exit(EXIT_FAILURE);
  }
  return 0;
}

int Listen(int sockfd, int backlog)
{
  if(listen(sockfd, backlog) == -1)
  {
    perror("Listen");
    exit(EXIT_FAILURE);
  }
  return 0;
}

int Accept(int listfd, struct sockaddr *cli_add)
{
  int conn_fd;
  socklen_t len = sizeof(*cli_add);

  if( (conn_fd = accept(listfd, (struct sockaddr *) cli_add, &len)) < 0)
  {
    perror("Accept");
    exit(EXIT_FAILURE);
	}
  return conn_fd;
}
