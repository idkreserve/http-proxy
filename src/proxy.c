#include <stdio.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include "proxy.h"
#include "pollfd_set.h"
#include "panic.h"
#include "utils.h"

#define PORT "8888"
#define BACKLOG 10

int main(void)
{
  const struct addrinfo hints = {
    .ai_family = AF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
    .ai_flags = AI_PASSIVE,
  };
  struct addrinfo *serverinfo;
  const int rv = getaddrinfo(NULL, PORT, &hints, &serverinfo);

  if (rv != 0)
    panic(PANIC_EXIT, 1, "getaddrinfo: %s\n", gai_strerror(rv));
  
  const int sockfd = bind_to_first_success(serverinfo);
  if (sockfd == -1)
    panic(PANIC_EXIT | PANIC_PERROR, 1, "bind_to_first_success");
  if (listen(sockfd, BACKLOG) == -1)
    panic(PANIC_EXIT | PANIC_PERROR, 1, "listen");
  
  struct pollfd_set master = fd_init;

  if (fd_add(&master, sockfd, POLLIN, 0) == NULL)
    panic(PANIC_EXIT, 1, "memory error\n");
  for (;;) {
    if (poll(master.data, master.len, -1) == -1)
      panic(PANIC_PERROR | PANIC_EXIT, 1, "poll");
    for (size_t i = 0; i < master.len; i++) {
      if (!(master.data[i].revents & POLLIN) && !(master.data[i].revents & POLLOUT))
        continue;
      if (master.data[i].fd == sockfd) {
        const int clientfd = accept_client(sockfd);
        if (fd_add(&master, clientfd, POLLIN | POLLOUT, 0) == NULL) {
          fprintf(stderr, "Failed to add socket %d. Disconnecting client.\n", clientfd);
          close(clientfd);
        }
      } else {
        char buf[1024];
        const int nbytes = recv(master.data[i].fd, buf, sizeof buf, 0);

        if (nbytes <= 0) {
          if (nbytes == 0)
            printf("Client %d hang out\n", master.data[i].fd);
          else
            perror("recv");
          close(master.data[i].fd);
          fd_clr(&master, master.data + i);
        } else
          fputs(buf, stdout);
      }
    }
    
  }

  return 0;
}

int accept_client(int sockfd)
{
  struct sockaddr_storage their_addr;
  socklen_t their_size = sizeof their_addr;

    const int clientfd = accept(sockfd, (struct sockaddr *) &their_addr, &their_size);
    if (clientfd == -1) {
      perror("listen");
      return 1;
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
      return 1;
    }
    printf("Got connection from %s with descriptor %d\n", ipstr, clientfd);

    return clientfd;
}
