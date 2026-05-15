#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "stddef.h"
#include "net.h"

int accept_client(const int sockfd)
{
  struct sockaddr_storage their_addr;
  socklen_t their_size = sizeof their_addr;

    const int clientfd = accept(sockfd, (struct sockaddr *) &their_addr, &their_size);
    if (clientfd == -1) {
      perror("accept");
      return -1;
    }

    char ipstr[INET6_ADDRSTRLEN];
    if(inet_ntop(
      their_addr.ss_family,
      get_in_addr((struct sockaddr *) &their_addr),
      ipstr,
      sizeof ipstr
    ) == NULL) {
      perror("inet_ntop");
      close(clientfd);
      return -1;
    }

    return clientfd;
}

int set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
      return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int sendall(int sockfd, const void *buf, size_t *len)
{
  size_t total = 0;
  size_t left = *len;
  ssize_t nbytes = -1;

  while (total < left) {
      if ((nbytes = send(sockfd, buf + total, left, 0)) <= 0)
          break;
      total += nbytes;
      left -= nbytes;
  }
  *len = total;

  if (nbytes > 0)
      return 1;
  return nbytes;
}

int bind_to_first_success(struct addrinfo *ai)
{
  int sockfd;
  int yes = 1;
  struct addrinfo *p;

  for (p = ai; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        continue;
    }
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        close(sockfd);
        continue;
    }
    break;
  }

  return p == NULL ? -1 : sockfd;
}

int connect_to_first_success(struct addrinfo *ai)
{
  int sockfd;
  int yes = 1;
  struct addrinfo *p;

  for (p = ai; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
          continue;
      setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
      if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
          close(sockfd);
          continue;
      }
      break;
  }

  return p == NULL ? -1 : sockfd;
}

void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
    return (struct sockaddr_in *) sa;
  return (struct sockaddr_in6 *) sa;
}