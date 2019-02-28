#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <openssl/sha.h>

#define BUFFLEN 256

void* obj_malloc(size_t size);
void calculate_hash(char pwd[BUFFLEN], unsigned char *hash);

#endif
