#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

int bind_to_first_success(struct addrinfo *ai);
int connect_to_first_valid(struct addrinfo *ai);
int sendall(int sockfd, const void *buf, size_t *len);
void *get_in_addr(struct sockaddr *sa);

#endif