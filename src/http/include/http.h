#ifndef HTTP_H
#define HTTP_H

#include "types.h"

#define BUFSIZE 4 * 1024 + 1

struct metainf {
  char buf[BUFSIZE];
  size_t len;
};

struct http {
  int type;
  char host[257];
  char path[2049];
  char port[6];
  char method[8];
  char version[16];
  struct metainf meta;
};

int get_connection(struct http *tr, const int sockfd);

#endif
