#include "../include/io.h"

ssize_t Read(int fd, void *buff, size_t count)
{
  size_t nleft;
  ssize_t nread;

  nleft = count;
  while (nleft > 0)
  {
    if( (nread = read(fd, buff, nleft)) < 0)
    {
      if (errno == EINTR)
        continue;
      /*if errno is not EINTR, print errno and return negative nread */
      perror("full read");
      return nread;
    }
    else
      if(nread == 0) /* if EOF */
        break;

    /*if you are here, you have actually read bytes*/
    buff = (char *)buff + nread;/* set left to read */
    nleft = nleft - nread;/* set pointer */
  }
  return nleft;
}



ssize_t Write(int fd, const void *buff, size_t count)
{
  size_t nleft;
  ssize_t nwritten;

  nleft = count;
  while (nleft > 0)
  {
    if( (nwritten = write(fd, buff, nleft)) < 0)
    {
      if(errno == EINTR)
        continue;
      /*if errno is not EINTR, print errno and return negative nwritten */
      perror("full write");
      return nwritten;
    }

    /*if you are here, you have actually write bytes*/
    buff = (char*)buff + nwritten; /* set pointer */
    nleft = nleft - nwritten;/* set left to write */
  }
  return nleft;
}


int sendInt(int fd, int n)
{
  char buf[16];
  memset((void*) buf, 0, 16);
  sprintf(buf, "%d", n);
  if( Write(fd, buf, 16) != 0 )
  {
    perror("sendInt");
    return -1;
  }
  return 0;
}


int recvInt(int fd, int *n)
{
  char buff[16];
  if( Read(fd, buff, 16) != 0)
  {
    perror("recInt");
    return -1;
  }
  sscanf(buff, "%d", n);
  return 0;
}

int sendChar(int fd, char n)
{
  if( Write(fd, (void *)&n, 1) != 0 )
  {
    perror("sendChar");
    return -1;
  }
  return 0;
}

int recvChar(int fd, char *n)
{
  if( Read(fd, n, 1) != 0)
  {
    perror("recChar");
    return -1;
  }
  return 0;
}

int sendBlock(int fd, Block b)
{
  if(Write(fd, b, BLOCK_SIZE) != 0)
  {
    perror("sendBlock");
    return -1;
  }
  return 0;
}

int recvBlock(int fd, Block b)
{
  if(Read(fd, b, BLOCK_SIZE) != 0)
  {
    perror("sendBlock");
    return -1;
  }
  return 0;
}
