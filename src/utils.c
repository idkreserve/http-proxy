#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include "utils.h"

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

size_t gethttpline(const int sockfd, char *s, size_t n)
{
  if (n <= 4)
    return 0;

  size_t i;
  char *c = s;
  for (i = 0; i < n - 1 && recv(sockfd, s, 1, 0) > 0 && *s != '\r'; i++)
    s++;
  if (*s == '\r')
    i++;

  char next;
  if (n - 1 - i > 1 && recv(sockfd, &next, 1, 0) == 1 && next == '\n') {
    *s++ = '\n';
    i++;
  }
  *s = '\0';

  fputs(c, stdout);
  return i;
}
