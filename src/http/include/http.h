#ifndef HTTP_H
#define HTTP_H

#include "types.h"

struct http {
  int type;
  char host[256];
  char port[6];
  char method[METHODSIZE];
};

int get_connection(struct http *tr, const int sockfd);
size_t gethttpline(const int sockfd, char *s, size_t n);

#endif
