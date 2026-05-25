#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/socket.h>
#include "http.h"
#include "hash.h"

static int skiphttpsresponse(struct metainf *m, const int sockfd);

int get_connection(struct http *tr, const int sockfd)
{
  struct metainf *m = &tr->meta;
  char *end;

  for (;;) {
    const ssize_t nbytes = recv(sockfd, m->buf, sizeof m->buf, 0);
    if (nbytes <= 0)
      return tr->type = CONN_UNDEFINED;
    m->len = nbytes;

    end = memmem(m->buf, nbytes, "\r\n", 2);
    if (end != NULL)
      break;
  }
  
  const ptrdiff_t len = end - m->buf;
  const char tmp = m->buf[len];

  m->buf[len] = '\0';

  if (sscanf(m->buf, "%7s", tr->method) != 1) {
    m->buf[len] = tmp;
    return tr->type = CONN_UNDEFINED;
  }
  
  if (strcmp(tr->method, "CONNECT") == 0) {
    tr->type = CONN_HTTPS;
    *tr->path = '\0';
    if (sscanf(m->buf, "%*s %256[^:]:%5s %15[^\r]", tr->host, tr->port, tr->version) != 3) {
      if (sscanf(m->buf, "%*s %256s %15[^\r]", tr->host, tr->version) != 2)
        return CONN_UNDEFINED;
      strcpy(tr->port, "443");
    }
    if (skiphttpsresponse(&tr->meta, sockfd) == -1)
      return CONN_UNDEFINED;
  } else {
    tr->type = CONN_HTTP;
    
    char *bufp = m->buf;
    if (sscanf(bufp, "%*s http://%256s:%5[^/]", tr->host, tr->port) == 2)
      bufp += 8 + strlen(tr->method) + strlen(tr->host) + 1 + strlen(tr->port);
    else if (sscanf(bufp, "%*s http://%256[^/]", tr->host) == 1) {
      bufp += 8 + strlen(tr->method) + strlen(tr->host);
      strcpy(tr->port, "80");
    } else
      return CONN_UNDEFINED;
    if (sscanf(bufp, "%2048s %15s", tr->path, tr->version) != 2)
      return CONN_UNDEFINED;
  }
  m->buf[len] = tmp;
  
  return tr->type;
}

int load_blacklist(FILE *fp, size_t *nline)
{
  char buf[HOSTSIZE];
  int n;

  *nline = 0;

  while ((n = fscanf(fp, "%256s", buf)) != EOF && n == 1) {
    char *s = normalize(buf, strlen(buf));
    if (lookup(s) == NULL && install(s) == NULL)
      return -1;
    nline++;
  }
  if (n != EOF)
    return -1;
  return 0;
}

char *normalize(char *s, const size_t len)
{
  if (strncmp(s, "https://", 8) == 0 && len > 8)
    s += 8;
  else if (strncmp(s, "http://", 7) == 0 && len > 7)
    s += 7;
  if (strncmp(s, "www.", 4) == 0 && len > 4)
    s += 4;
  return s;
}

static int skiphttpsresponse(struct metainf *m, const int sockfd)
{
  int state = 0;

  if (memmem(m->buf, sizeof m->buf, "\r\n\r\n", 4) != NULL)
    return 0;

  for (;;) {
    const ssize_t nbytes = recv(sockfd, m->buf, sizeof m->buf, 0);

    if (nbytes <= 0)
      return -1;
    for (ssize_t i = 0; i < nbytes; i++) {
      const char c = m->buf[i];

      switch (state) {
        case 0:
          if (c == '\r')
            state = 1;
          break;
        case 1:
          if (c == '\n')
            state = 2;
          else if (c == '\r')
            state = 1;
          else
            state = 0;
          break;
        case 2:
          state = (c == '\r') ? 3 : 0;
          break;
        case 3:
          if (c == '\n')
            return 0;
          state = 0;
          break;
      }
    }
  }
}
