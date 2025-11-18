#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <poll.h>

#define CONNECTION_SUCCESS 0
#define CONNECTION_FAILED 1
#define POLL_STDIN 0
#define POLL_STDOUT 3
#define BUFSIZE 16384


/* CLI options */
int lflag;

int timeout = -1;

void help();
void usage(int ret);
int remote_connect(const char * host, const char * port);
void set_common_sockopts(int s, struct sockaddr * sock_addr);
static int connect_with_timeout(int s, struct sockaddr * sock_addr, socklen_t sock_addr_len, int timeout);
static void readwrite(int net_file_descriptor);
static int fillbuf(int fd, unsigned char * buf, size_t * stdinbufpos);

int main(int argc, char *argv[])
{
    int ch, ret;
    char * host, ** port;

    ret = 1;

    while ((ch = getopt(argc, argv, "a:bch")) != -1) {
        switch (ch) {
            case 'h':
                help();
                break;
            case 'l':
                lflag = 1;
                break;

        }
    }

    argc -= optind;
    argv += optind;

    // make sure we have our options
    if (argc >= 2) {
        host = argv[0];
        port = &argv[1];
        
    } else {
        usage(1);
    }


    /* Connect to port */
    int s = -1;
    if (s != -1) { close(s); }

    s = remote_connect(host, *port);

    if (s == -1) {
        fprintf(stderr, "could not connect to the specified host and port\n");
    }

    readwrite(s);

    if (s != -1) { 
        close(s); 
        fprintf(stderr, "socket was closed...\n");
    }

    return ret;
}

/*
* Main polling loop
*/
static void readwrite(int net_file_descriptor) {
    struct pollfd pfd[4];
    int stdin_fd = STDIN_FILENO;    /* standard input file descriptor */
    int stdout_fd = STDOUT_FILENO;  /* standard output file descriptor */

    unsigned char stdinbuf[BUFSIZE];

    size_t stdinbufpos = 0;
    int numfds, n;

    int ret;
    
    /* stdin */
    pfd[POLL_STDIN].fd = stdin_fd;
    pfd[POLL_STDIN].events = POLLIN;

    /* stdout */
    pfd[POLL_STDOUT].fd = stdout_fd;
    pfd[POLL_STDOUT].events = 0;

    while (1) {
        /* standard input is gone and buffer is empty. We can stop. */ 
        if (pfd[POLL_STDIN].fd == -1 && stdinbufpos == 0) {
            return;
        }

        /* standard output is gone. We cannot continue. */
        if (pfd[POLL_STDOUT].fd == -1) {
            return;
        }

        /* poll */
        poll(pfd, 2, timeout);

        if (numfds == -1) { fprintf(stderr, "polling error"); }

        // timed out
        if (numfds == 0) { return; }

        /* treat socket error conditions */
        for (n = 0; n < 2; n++) {
            if (pfd[n].revents & (POLLERR | POLLNVAL)) {
                pfd[n].fd = -1;
            }
        }

        /* try to read from stdin */
        if (pfd[POLL_STDIN].revents && POLLIN && stdinbufpos < BUFSIZE) {
            ret = fillbuf(pfd[POLL_STDIN].fd, stdinbuf, &stdinbufpos);
        }

    }

    return;
}

static int fillbuf(int fd, unsigned char * buf, size_t * stdinbufpos) {

}

/*
* Returns a socket connected to remote host. Returns -1 on failure.
*/
int remote_connect(const char * host, const char * port) {
    fprintf(stderr, "attempting to communicate with %s %s...\n", host, port);

    struct addrinfo ai_hints = { .ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM, .ai_protocol = 0 };
    struct addrinfo * hints = &ai_hints;
    struct addrinfo * res, * cur_res;

    int error;
    int s = -1;

    error = getaddrinfo(host, port, hints, &res);

    if (error) {
        fprintf(stderr, "could not do getaddrinfo() on host \"%s\" and port %s\n", host, port);
        exit(1);
    }

    for (cur_res = res; res; res = res->ai_next) {

        /* create the socket */
        s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s == -1) {
            fprintf(stderr, "Unable to create socket...\n");
            continue;
        }

        set_common_sockopts(s, res->ai_addr);
        
        /* connect to the socket */
        error = connect_with_timeout(s, res->ai_addr, res->ai_addrlen, timeout);

        if (error == CONNECTION_SUCCESS) break; /* we can stop here */

        s = -1;
    }


    return s;
}

static int connect_with_timeout(int s, struct sockaddr * sock_addr, socklen_t sock_addr_len, int timeout) {

    int err;

    /* attempt to connect */
    err = connect(s, sock_addr, sock_addr_len);

    if (err == 0) { 
        fprintf(stderr, "successfully connected to socket...\n"); 
        return CONNECTION_SUCCESS;
    }
    else { 
        fprintf(stderr, "socket connection failed with error code %d\n", errno); 
        return CONNECTION_FAILED;
    }

    return 0;
}

/*
*
*/
void set_common_sockopts(int s, struct sockaddr * sock_addr) {
    
    return;
}

void help() {
    fprintf(stderr, "network 1.0.0 \n");
    usage(0);
}

void usage(int ret) {
    fprintf(stderr, "usage: netbear [destination] [port]\n");

    if (ret) exit(1);
}

