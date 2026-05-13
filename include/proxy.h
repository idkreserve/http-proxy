#ifndef PROXY_H
#define PROXY_H

#include "utils.h"

#define CONNSIZE 8 * 1024

#define CONN_HTTP 0
#define CONN_HTTPS 1
#define CONN_UNDEFINED 2

struct transfer {
  int type;
  char host[256];
  char port[6];
};

int accept_client(const int sockfd);
void handle_new_client(const int sockfd);
int get_connection(struct transfer *tr, const int sockfd);
void tls_pipeline(const int clientfd, const int remotefd);

#endif
