#include <stdio.h>
#include <stddef.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

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
      if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) == -1) {
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