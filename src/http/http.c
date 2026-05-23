#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/socket.h>
#include "http.h"

int get_connection(struct http *tr, const int sockfd)
{
  struct metainf *m = &tr->meta;

  for (;;) {
    const ssize_t nbytes = recv(sockfd, m->buf, sizeof m->buf, 0);
    if (nbytes <= 0)
      return tr->type = CONN_UNDEFINED;

    m->off = nbytes;
    
    const char *end = memmem(m->buf, nbytes, "\r\n", 2);
    if (end == NULL)
      continue;

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
      if (sscanf(m->buf, "%*s %256s:%5s %15[^\r]", tr->host, tr->port, tr->version) != 3) {
        if (sscanf(m->buf, "%*s %256s %15[^\r]", tr->host, tr->version) != 2)
          return CONN_UNDEFINED;
        strcpy(tr->port, "443");
      }
    } else {
      tr->type = CONN_HTTP;
      
      char *bufp = m->buf;
      if (sscanf(bufp, "%*s http://%256s:%5s", tr->host, tr->port) == 2)
        bufp += 7 + strlen(tr->host) + 1 + strlen(tr->port);
      else if (sscanf(bufp, "%*s http://%256s", tr->host) == 1) {
        bufp += 7 + strlen(tr->host);
        strcpy(tr->port, "80");
      } else
        return CONN_UNDEFINED;
      // if (sscanf(bufp, "%*s http://%256s:%5s", tr->host, tr->port) != 2) {
      //   if (sscanf(bufp, "%*s http://%256s", tr->host) != 1)
      //     return CONN_UNDEFINED;
      //   strcpy(tr->port, "80");
      // }

      // const size_t len1 = strlen(tr->host);
      // const size_t len2 = strlen(tr->path);
      // if (len1 > SIZE_MAX - len2)
      //   return CONN_UNDEFINED;
      // size_t len = len1 + len2;

      // bufp += len;

      // if (sscanf(m->buf, "%*s http://%256s:%5s/%2048s %15[^\r]", tr->host, tr->port, tr->path, tr->version) != 4) {
      //   if (sscanf(m->buf, "%*s http://%256[^/]%2048s %15[^\r]", tr->host, tr->path, tr->version) != 3)
      //     return CONN_UNDEFINED;
      //   strcpy(tr->port, "80");
      // }
    }
    m->buf[len] = tmp;
    
    return tr->type;
  }
}

// int get_connection(struct http *tr, const int sockfd)
// {
//   #define HTTPS_FLAG "CONNECT"
//   #define HTTPS_FLAG_LEN 7

//   char buf[BUFSIZE > sizeof (tr->host) ? BUFSIZE : sizeof (tr->host)];
//   const size_t len = gethttpline(sockfd, buf, sizeof buf);

//   if (len == 0)
//     return CONN_UNDEFINED;
  
//   size_t i = 0, j = 0;
//   /* HTTPS */
//   if (strstr(buf + i, HTTPS_FLAG) == buf && len > HTTPS_FLAG_LEN) {
//     i += HTTPS_FLAG_LEN;
    
//     while (i < sizeof buf && isspace(buf[i]))
//       i++;
    
//     /* Find hostname */
//     while (i < sizeof buf && j < sizeof tr->host - 1) {
//       if (buf[i] == ':')
//         break;
//       if (buf[i] == '\0')
//         return CONN_UNDEFINED;
//       tr->host[j++] = buf[i++];
//     }
//     tr->host[j] = '\0';
    
//     /* Find port */
//     if (buf[i] != ':')
//       return CONN_UNDEFINED;
//     i++;
//     for (j = 0; i < sizeof buf && j < sizeof tr->port - 1 && !isspace(buf[i]); i++, j++)
//       tr->port[j] = buf[i];
//     tr->port[j] = '\0';
//     if (*tr->port == '\0') 
//       return CONN_UNDEFINED;
//     tr->type = CONN_HTTPS;
//     *tr->method = '\0';

//     if (!isspace(buf[i]))
//       return CONN_UNDEFINED;
//     i++;

//     /* Find version */
//     for (j = 0; i < sizeof buf && j < sizeof tr->version - 1; i++, j++)
//       tr->version[j] = buf[i];
//     tr->version[j] = '\0';

//     if (j == 0)
//       return CONN_UNDEFINED;

//     /* Skip remaining lines */
//     char end[] = {'\0', '\0'};
//     size_t len;

//     while ((len = gethttpline(sockfd, buf, sizeof buf)) > 2) {
//       if (buf[len-2] == '\r' && buf[len-1] == '\n' && end[0] == '\r' && end[1] == '\n')
//         break;
//       end[0] = buf[len-2];
//       end[1] = buf[len-1];
//     }
//     if (len != 2) {
//       return CONN_UNDEFINED;
//     }
//     return CONN_HTTPS;
//   }

//   /* HTTP */
//   i = 0, j = 0;

//   while (isspace(buf[i]))
//     i++;
  
//   /* Find HTTP method */
//   while (i < sizeof buf && j < sizeof tr->method - 1 && !isspace(buf[i]))
//     tr->method[j++] = buf[i++];
//   tr->method[j] = '\0';
  
//   if (!isspace(buf[i]))
//     return CONN_UNDEFINED;
//   i++;
  
//   /* Skipping protocol */
//   for (; i < sizeof tr->host - 1; i++) {
//     if (buf[i] == '/' && buf[i+1] == '/') {
//       i += 2;
//       break;
//     }
//   }
  
//   /* Find hostname */
//   for (j = 0; i < sizeof tr->host - 1 && !isspace(buf[i]) && buf[i] != '/'; i++, j++)
//     tr->host[j] = buf[i];
//   tr->host[j] = '\0';

//   if (!isspace(buf[i]) && buf[i] != '/')
//     return CONN_UNDEFINED;

//   tr->type = CONN_HTTP;
//   strcpy(tr->port, "http");

//   return CONN_HTTP;

//   #undef HTTPS_FLAG
//   #undef HTTPS_FLAG_LEN
// }

// size_t gethttpline(const int sockfd, char *s, size_t n)
// {
//   if (n <= 4)
//     return 0;

//   size_t i;
//   for (i = 0; i < n - 1 && recv(sockfd, s, 1, 0) > 0 && *s != '\r'; i++)
//     s++;
//   if (*s == '\r')
//     i++;

//   char next;
//   if (n - 1 - i > 1 && recv(sockfd, &next, 1, 0) == 1 && next == '\n') {
//     *s++ = '\n';
//     i++;
//   }
//   *s = '\0';

//   return i;
// }
