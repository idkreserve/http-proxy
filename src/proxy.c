#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include "proxy.h"
#include "panic.h"
#include "utils.h"

#define PORT "8888"
#define BACKLOG 10

static int set_nonblock(int fd);
static void sigchild_handler(int s);

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

int get_connection(struct transfer *tr, const int sockfd)
{
  #define HTTPS_FLAG "CONNECT"
  #define HTTPS_FLAG_LEN 7

  char buf[CONNSIZE];
  const size_t len = gethttpline(sockfd, buf, sizeof buf);

  if (len == 0)
    return CONN_UNDEFINED;
  
  /* HTTPS */
  if (strstr(buf, HTTPS_FLAG) == buf && len > HTTPS_FLAG_LEN) {
    char *https_data = buf + HTTPS_FLAG_LEN;
    while (isspace(*https_data))
      https_data++;

    size_t i;

    /* Find hostname */
    for (i = 0; i < sizeof tr->host - 1; i++) {
      if (*https_data == ':')
        break;
      if (*https_data == '\0')
        goto undefined;
      tr->host[i] = *https_data++;
    }
    tr->host[i] = '\0';
    
    /* Find port */
    if (*https_data != ':')
      goto undefined;
    https_data++;
    for (i = 0; i < sizeof tr->port - 1 && !isspace(*https_data); i++)
      tr->port[i] = *https_data++;
    tr->port[i] = '\0';
    if (*tr->port == '\0') 
      goto undefined;
    tr->type = CONN_HTTPS;

    char end[2] = {'\0', '\0'};
    size_t len;

    /* Skip remaining lines */
    while ((len = gethttpline(sockfd, buf, sizeof buf)) > 2) {
      if (buf[len-2] == '\r' && buf[len-1] == '\n' && end[0] == '\r' && end[1] == '\n')
        break;
      end[0] = buf[len-2];
      end[1] = buf[len-1];
    }
    if (len != 2) {
      goto undefined;
    }
    return CONN_HTTPS;
  }

  undefined:
  return CONN_UNDEFINED;

  #undef HTTPS_FLAG
  #undef HTTPS_FLAG_LEN
}

void handle_new_client(const int sockfd)
{
  struct transfer tr;

  const int conn_type = get_connection(&tr, sockfd);

  if (conn_type == CONN_UNDEFINED)
    return;

  const struct addrinfo hints = {
    .ai_family = AF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
  };
  struct addrinfo *remote_info;
  const int rv = getaddrinfo(tr.host, tr.port, &hints, &remote_info);
  if (rv == -1) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return;
  }

  const int remotefd = connect_to_first_success(remote_info);
  freeaddrinfo(remote_info);
  if (remotefd == -1)
    panic(PANIC_EXIT | PANIC_PERROR, 1, "connect_to_first_success");
  
  switch (conn_type) {
    case CONN_HTTPS:
      printf("HTTPS %s %s\n", tr.host, tr.port);
      const char *response = "HTTP/1.1 200 Connection Established\r\n\r\n";
      size_t len = strlen(response);
      sendall(sockfd, response, &len);
      tls_pipeline(sockfd, remotefd);
      break;
    case CONN_HTTP:
      puts("HTTP");
      break;
    case CONN_UNDEFINED:
      puts("UNDEFINED");
      break;
  }
}

static int set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
      return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void sigchild_handler(int)
{
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

void tls_pipeline(const int clientfd, const int remotefd)
{
  set_nonblock(clientfd);
  set_nonblock(remotefd);

  struct pollfd hosts[] = {
    {.fd = clientfd},
    {.fd = remotefd},
  };
  #define CLIENT hosts[0]
  #define SERVER hosts[1]
  #define BUFSIZE CONNSIZE

  char c2s[BUFSIZE], s2c[BUFSIZE];
  size_t c2s_len = 0, c2s_off = 0;
  size_t s2c_len = 0, s2c_off = 0;
  bool client_closed = false, remote_closed = false;

  struct sigaction sa;
  sa.sa_handler = sigchild_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    return;
  }
  
  for (;;) {
    CLIENT.events = 0;
    SERVER.events = 0;

    if (!client_closed && c2s_len < sizeof c2s)
      CLIENT.events |= POLLIN;
    if (!remote_closed && s2c_len < sizeof s2c)
      SERVER.events |= POLLIN;

    if (c2s_off < c2s_len)
      SERVER.events |= POLLOUT;
    if (s2c_off < s2c_len)
      CLIENT.events |= POLLOUT;

    if (poll(hosts, sizeof hosts / sizeof *hosts, -1) <= 0)
      break;
    
    if (CLIENT.revents & POLLIN) {
      const ssize_t nbytes = recv(clientfd, c2s + c2s_len, sizeof c2s - c2s_len, 0);
      if (nbytes == 0) {
        client_closed = true;
        shutdown(remotefd, SHUT_WR);
      } else if (nbytes > 0)
        c2s_len += nbytes;
      else if (errno != EAGAIN && errno != EWOULDBLOCK)
        break;
    }
    if (!remote_closed && (SERVER.revents & POLLOUT)) {
      const ssize_t nbytes = send(remotefd, c2s + c2s_off, c2s_len - c2s_off, MSG_NOSIGNAL);
      if (nbytes > 0) {
        c2s_off += nbytes;
        if (c2s_off == c2s_len)
          c2s_off = c2s_len = 0;
      } else if (nbytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        break;
    }
    if (SERVER.revents & POLLIN) {
      const ssize_t nbytes = recv(remotefd, s2c + s2c_len, sizeof s2c - s2c_len, 0);
      if (nbytes == 0) {
        remote_closed = true;
        shutdown(clientfd, SHUT_WR);
      } else if (nbytes > 0)
        s2c_len += nbytes;
      else if (errno != EAGAIN && errno != EWOULDBLOCK)
        break;
    }
    if (!client_closed && (CLIENT.revents & POLLOUT)) {
      const ssize_t nbytes = send(clientfd, s2c + s2c_off, s2c_len - s2c_off, MSG_NOSIGNAL);
      if (nbytes > 0) {
        s2c_off += nbytes;
        if (s2c_off == s2c_len)
          s2c_off = s2c_len = 0;
      } else if (nbytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        break;
    }

    if (client_closed && remote_closed && c2s_len == 0 && s2c_len == 0)
      break;
    if (CLIENT.revents & POLLERR)
      break;
    if (SERVER.revents & POLLERR)
      break;
  }

  #undef CLIENT
  #undef SERVER
  #undef BUFSIZE
}

int accept_client(const int sockfd)
{
  struct sockaddr_storage their_addr;
  socklen_t their_size = sizeof their_addr;

    const int clientfd = accept(sockfd, (struct sockaddr *) &their_addr, &their_size);
    if (clientfd == -1) {
      perror("listen");
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
