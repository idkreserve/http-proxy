#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <sys/socket.h>
#include <stdio.h>
#include "net.h"
#include "utils.h"
#include "http.h"
#include "pipeline.h"

void pipeline(const int clientfd, const int remotefd)
{
  set_nonblock(clientfd);
  set_nonblock(remotefd);

  struct pollfd hosts[] = {
    {.fd = clientfd},
    {.fd = remotefd},
  };
  #define CLIENT hosts[0]
  #define SERVER hosts[1]

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
}