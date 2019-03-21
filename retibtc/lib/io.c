#include "../include/io.h"

ssize_t Read(int fd, void *buff, size_t count)
{
  size_t nleft = count;
  ssize_t nread;

  while (nleft > 0)
  {
    if( (nread = read(fd, buff, nleft)) < 0)
    {
      if (errno == EINTR)
        continue; /*if errno is not EINTR, print errno and return negative nread */
      else
      {
        perror("wrapper read");
        return nread;
      }
    }
    else
      if(nread == 0) /* if EOF */
        break;

    /*if you are here, you have actually read bytes*/
    nleft = nleft - nread;/* set pointer */
    buff = (char *)buff + nread;/* set left to read */
  }
  buff = 0;
  return nleft;
}


ssize_t Write(int fd, const void *buff, size_t count)
{
  size_t nleft = count;
  ssize_t nwritten;

  while (nleft > 0)
  {
    if( (nwritten = write(fd, buff, nleft)) < 0)
    {
      if(errno == EINTR)
        continue; /*if errno is not EINTR, print errno and return negative nwritten */
      else
      {
        perror("wrapper write");
        return nwritten;
      }
    }
    /*if you are here, you have actually write bytes*/
    nleft = nleft - nwritten;/* set left to write */
    buff = (char *) buff + nwritten; /* set pointer */
  }
  return nleft;
}





