#ifndef NET_H
#define NET_H

#include <stddef.h>
#include <netdb.h>

int accept_client(const int sockfd);
int set_nonblock(int fd);
int sendall(int sockfd, const void *buf, size_t *len);
int bind_to_first_success(struct addrinfo *ai);
int connect_to_first_success(struct addrinfo *ai);
void *get_in_addr(struct sockaddr *sa);

#endif
