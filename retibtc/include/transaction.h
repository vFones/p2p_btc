#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "net.h"

struct transaction{
  char src[LEN_ADDRESS];
  short srcport;
  char dst[LEN_ADDRESS];
  short dstport;
  int random;
};
#define TRNS_SIZE sizeof(struct transaction);


#endif
