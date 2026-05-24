#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include "pipeline.h"
#include "proxy.h"
#include "http.h"
#include "types.h"
#include "net.h"
#include "panic.h"

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
  freeaddrinfo(serverinfo);

  for (;;) {
    const int clientfd = accept_client(sockfd);
    
    if (clientfd == -1)
      continue;
    if (fork() == 0) {
      handle_new_client(clientfd);
      return 0;
    }
    close(clientfd);
  }

  return 0;
}

void handle_new_client(const int sockfd)
{
  struct http tr;

  const int conn_type = get_connection(&tr, sockfd);

  if (conn_type == CONN_UNDEFINED)
    return;
  const struct addrinfo hints = {
    .ai_family = AF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
  };
  struct addrinfo *remote_info;
  const int rv = getaddrinfo(tr.host, tr.port, &hints, &remote_info);
  if (rv != 0) {
    printf("HOST: [%s] %s\n", tr.host, tr.port);
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return;
  }

  const int remotefd = connect_to_first_success(remote_info);
  freeaddrinfo(remote_info);
  if (remotefd == -1)
    panic(PANIC_EXIT | PANIC_PERROR, 1, "connect_to_first_success");
  
  switch (conn_type) {
    case CONN_HTTPS:
      dprintf(sockfd, "%s 200 Connection Established\r\n\r\n", tr.version);
      pipeline(sockfd, remotefd, NULL, 0);
      break;
    case CONN_HTTP:
      dprintf(remotefd, "%s %s %s\r\n", tr.method, tr.path, tr.version);

      char *append = NULL;
      size_t appendlen = 0;
      char *ptr = memmem(tr.meta.buf, tr.meta.len, "\r\n", 2);

      if (ptr + 2 < tr.meta.buf + tr.meta.len) {
        append = ptr + 2;
        appendlen = tr.meta.len - (ptr - tr.meta.buf + 2);
      }
      pipeline(sockfd, remotefd, append, appendlen);
      break;
    case CONN_UNDEFINED:
      puts("UNDEFINED");
      break;
  }
}

