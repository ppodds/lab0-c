#include "tiny.h"
#include <arpa/inet.h> /* inet_ntoa */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define LISTENQ 1024 /* second argument to listen() */
#define MAXLINE 1024 /* max length of a line */

#ifndef RIO_BUFSIZE
#define RIO_BUFSIZE 1024
#endif

#ifndef NO_LOG_ACCESS
#define LOG_ACCESS
#endif

typedef struct {
    int rio_fd;                /* descriptor for this buf */
    int rio_cnt;               /* unread byte in this buf */
    char *rio_bufptr;          /* next unread byte in this buf */
    char rio_buf[RIO_BUFSIZE]; /* internal buffer */
} rio_t;

typedef struct {
    char function_name[512];
} http_request;

char *default_mime_type = "text/plain";

int listening = -1;
int noise = 1;

void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

/*
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;
    while (rp->rio_cnt <= 0) { /* refill if buf is empty */

        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0) {
            if (errno != EINTR) { /* interrupted by sig handler return */
                return -1;
            }
        } else if (rp->rio_cnt == 0) { /* EOF */
            return 0;
        } else
            rp->rio_bufptr = rp->rio_buf; /* reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;
    if (rp->rio_cnt < n) {
        cnt = rp->rio_cnt;
    }
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

/*
 * rio_readlineb - robustly read a text line (buffered)
 */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        int rc;
        if ((rc = rio_read(rp, &c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n') {
                break;
            }
        } else if (rc == 0) {
            if (n == 1) {
                return 0; /* EOF, no data read */
            } else {
                break; /* EOF, some data was read */
            }
        } else {
            return -1; /* error */
        }
    }
    *bufp = 0;
    return n;
}

int open_listenfd(int port)
{
    int listenfd, optval = 1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval,
                   sizeof(int)) < 0) {
        return -1;
    }

    // 6 is TCP's protocol number
    // enable this, much faster : 4000 req/s -> 17000 req/s
    if (setsockopt(listenfd, 6, TCP_CORK, (const void *) &optval, sizeof(int)) <
        0) {
        return -1;
    }

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short) port);
    if (bind(listenfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0) {
        return -1;
    }

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0) {
        return -1;
    }
    return listenfd;
}

void parse_request(int fd, http_request *req)
{
    rio_t rio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], format[64];

    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    snprintf(format, 64, "%%%ds %%%ds", MAXLINE - 1, MAXLINE - 1);
    sscanf(buf, format, method, uri); /* version is not cared */
    /* read all */
    while (buf[0] != '\n' && buf[1] != '\n') { /* \n || \r\n */
        rio_readlineb(&rio, buf, MAXLINE);
    }
    char *function_name = uri;
    if (uri[0] == '/') {
        function_name = uri + 1;
        int length = strlen(function_name);
        // don't care url query
        for (int i = 0; i < length; i++) {
            if (function_name[i] == '?') {
                function_name[i] = '\0';
                break;
            }
        }
    }
    strncpy(req->function_name, function_name, 512);
}

#ifdef LOG_ACCESS
void log_access(int status, struct sockaddr_in *c_addr, http_request *req)
{
    printf("%s:%d %d - '%s'\n", inet_ntoa(c_addr->sin_addr),
           ntohs(c_addr->sin_port), status, req->function_name);
}
#endif

/* replace '/' with ' ' */
void handle_request(int fd, char *func_name)
{
    while ((*func_name) != '\0') {
        if (*func_name == '/')
            *func_name = ' ';
        func_name++;
    }
}

char *process(int fd, struct sockaddr_in *clientaddr)
{
#ifdef LOG_ACCESS
    printf("accept request, fd is %d, pid is %d\n", fd, getpid());
#endif
    http_request req;
    parse_request(fd, &req);

    handle_request(fd, req.function_name);

    int status = 200;

#ifdef LOG_ACCESS
    log_access(status, clientaddr, &req);
#endif
    char *ret = malloc(strlen(req.function_name) + 1);
    strncpy(ret, req.function_name, strlen(req.function_name) + 1);

    return ret;
}
