#ifndef IO_H
#define IO_H

#include <unistd.h> // unix standard library
#include <errno.h> // error definitions and routines */
#include <stdio.h> // perror
#include <string.h> // memset()
#include "blockchain.h"
#include "transaction.h"

ssize_t Read(int fd, void *buf, size_t count);
ssize_t Write(int fd, const void *buf, size_t count);

int sendInt(int fd, int n);
int recvInt(int fd, int *n);

int sendChar(int fd, char n);
int recvChar(int fd, char *n);

int sendBlock(int fd, Block b);
int recvBlock(int fd, Block b);


#endif
