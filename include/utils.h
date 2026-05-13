#ifndef NET_H
#define NET_H

#include <stdint.h>

int sendall(int sockfd, const void *buf, size_t *len);
int bind_to_first_success(struct addrinfo *ai);
int connect_to_first_success(struct addrinfo *ai);
void *get_in_addr(struct sockaddr *sa);
size_t gethttpline(const int sockfd, char *s, size_t n);

#endif