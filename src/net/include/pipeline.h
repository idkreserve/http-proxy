#ifndef PIPELINE_H
#define PIPELINE_H

void tls_pipeline(const int clientfd, const int remotefd);
void http_pipeline(const int clientfd, const int remotefd);

#endif
