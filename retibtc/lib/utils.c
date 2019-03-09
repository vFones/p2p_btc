#include <string.h>

#include "../include/utils.h"

void* Malloc(size_t size)
{
  void* block;
  block  = malloc(size);
  memset(block, 0, size);
  if(block == NULL)
  {
    perror("Failed to allocate object");
    exit(EXIT_FAILURE);
  }
  return block;
}


void usage(char *msg)
{
  fprintf(stderr,"%s\n",msg);
  exit(EXIT_FAILURE);
}
