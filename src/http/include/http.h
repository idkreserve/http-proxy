#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>
#include <stddef.h>
#include "types.h"

#define BUFSIZE 4 * 1024 + 1
#define HOSTSIZE 257

struct metainf {
  char buf[BUFSIZE];
  size_t len;
};

struct http {
  int type;
  char host[HOSTSIZE];
  char path[2049];
  char port[6];
  char method[8];
  char version[16];
  struct metainf meta;
};

int get_connection(struct http *tr, const int sockfd);
int load_blacklist(FILE *fp, size_t *nline);
char *normalize(char *s, const size_t len);

#endif
