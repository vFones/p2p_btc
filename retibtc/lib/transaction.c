#include "../include/transaction.h"

struct transaction fillTransaction( struct connected_node src, \
                                    struct connected_node dst, float amount )
{
  struct transaction trns;
  strncpy(trns.src.address, src.address, LEN_ADDRESS);
  trns.src.port = src.port;

  strncpy(trns.dst.address, dst.address, LEN_ADDRESS);
  trns.dst.port = dst.port;

  trns.amount = amount;
  trns.random = rand()%1000;

  return trns;
}

int sendTrns(int fd, Trns trns)
{
  if(Write(fd, trns, TRNS_SIZE) != 0)
  {
    perror("sendTrns");
    return -1;
  }
  return 0;
}


int recvTrns(int fd, Trns trns)
{
  if(Read(fd, trns, TRNS_SIZE) != 0)
  {
    perror("recvTrns");
    return -1;
  }
  return 0;
}
