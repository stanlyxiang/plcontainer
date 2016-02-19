#include <errno.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "comm_channel.h"
#include "comm_logging.h"
#include "libpq-mini.h"
#include "comm_server.h"

/*
 * Functoin binds the socket and starts listening on it
 */
int start_listener() {
    struct sockaddr_in addr;
    int                sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        lprintf(ERROR, "%s", strerror(errno));
    }

    addr = (struct sockaddr_in){
        .sin_family = AF_INET,
        .sin_port   = htons(SERVER_PORT),
        .sin_addr = {.s_addr = INADDR_ANY},
    };
    if (bind(sock, (const struct sockaddr *)&addr, sizeof(addr)) == -1) {
        lprintf(ERROR, "Cannot bind the port: %s", strerror(errno));
    }

    if (listen(sock, 10) == -1) {
        lprintf(ERROR, "Cannot listen the socket: %s", strerror(errno));
    }

    return sock;
}

/*
 * Fuction waits for the socket to accept connection for finite amount of time
 * and errors out when the timeout is reached and no client connected
 */
void connection_wait(int sock) {
    struct timeval     timeout;
    int                rv;
    fd_set             fdset;

    FD_ZERO(&fdset);    /* clear the set */
    FD_SET(sock, &fdset); /* add our file descriptor to the set */
    timeout.tv_sec  = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    rv = select(sock + 1, &fdset, NULL, NULL, &timeout);
    if (rv == -1) {
        lprintf(ERROR, "Failed to select() socket: %s", strerror(errno));
    }
    if (rv == 0) {
        lprintf(ERROR, "Socket timeout - no client connected within %d seconds", TIMEOUT_SEC);
    }
}

/*
 * Function accepts the connection and initializes libpq structure for it
 */
void connection_init(int sock) {
    socklen_t          raddr_len;
    struct sockaddr_in raddr;
    PGconn_min *       pqconn;
    int                connection;

    raddr_len  = sizeof(raddr);
    connection = accept(sock, (struct sockaddr *)&raddr, &raddr_len);
    if (connection == -1) {
        lprintf(ERROR, "failed to accept connection: %s", strerror(errno));
    }

    pqconn = pq_min_connect_fd(connection);
    //pqconn->Pfdebug = stdout;
    plcontainer_channel_initialize(pqconn);
}

/*
 * The loop of receiving commands from the Greenplum process and processing them
 */
void receive_loop( void (*handle_call)(callreq) ) {
    message msg;

    while (true) {
        msg = plcontainer_channel_receive();

        if (msg == NULL) {
            break;
        }

        switch (msg->msgtype) {
        case MT_CALLREQ:
            handle_call((callreq)msg);
            free_callreq((callreq)msg);
            break;
        default:
            lprintf(ERROR, "received unknown message: %c", msg->msgtype);
        }
    }
}
