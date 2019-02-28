#include <string.h>
#include <openssl/sha.h>

#include <utils.h>

void* obj_malloc(size_t size)
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


void calculate_hash(char pwd[BUFFLEN], unsigned char *hash)
{
  SHA256_CTX context;
  SHA256_Init(&context);
  SHA256_Update(&context, pwd, strlen(pwd));
  SHA256_Final(hash, &context);

}
