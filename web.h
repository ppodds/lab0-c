#ifndef LAB0_TINY_H
#define LAB0_TINY_H
#include <netinet/in.h>

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

extern int listening;
extern int noise;

int open_listenfd(int port);

char *process(int connfd, struct sockaddr_in *clientaddr);

#endif /* LAB0_TINY_H */