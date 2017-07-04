/*------------------------------------------------------------------------------
 *
 *
 * Copyright (c) 2016, Pivotal.
 *
 *------------------------------------------------------------------------------
 */
#include <errno.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "comm_channel.h"
#include "comm_utils.h"
#include "comm_connectivity.h"
#include "comm_server.h"
#include "messages/messages.h"

/*
 * Functoin binds the socket and starts listening on it
 */
int start_listener() {
    struct sockaddr_un addr;
    int                sock;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        lprintf(ERROR, "%s", strerror(errno));
    }

    remove("/opt/share/server.socket");
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/opt/share/server.socket");
    
    if (bind(sock, (const struct sockaddr *)&addr, sizeof(addr)) == -1) {
        lprintf(ERROR, "Cannot bind the port: %s", strerror(errno));
    }
    chmod("/opt/share/server.socket", 0777);
#ifdef _DEBUG_CLIENT
    int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        lprintf(ERROR, "setsockopt(SO_REUSEADDR) failed");
    }
#endif
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
 * Function accepts the connection and initializes structure for it
 */
plcConn* connection_init(int sock) {
    socklen_t          raddr_len;
    struct sockaddr_in raddr;
    int                connection;

    raddr_len  = sizeof(raddr);
    connection = accept(sock, (struct sockaddr *)&raddr, &raddr_len);
    if (connection == -1) {
        lprintf(ERROR, "failed to accept connection: %s", strerror(errno));
    }

    return plcConnInit(connection);
}

unsigned long long gettime_microsec2(void)
{
	struct timeval newTime;
	int status = 1;
	unsigned long long t = 0;

	if (status != 0)
	{
		gettimeofday(&newTime, NULL);
	}
	t = ((unsigned long long)newTime.tv_sec) * 1000000 + newTime.tv_usec;
	return t;
}

/*
 * The loop of receiving commands from the Greenplum process and processing them
 */
void receive_loop( void (*handle_call)(plcMsgCallreq*, plcConn*, int), plcConn* conn) {
    plcMessage *msg;
    int res = 0;

    res = plcontainer_channel_receive(conn, &msg);
    if (res < 0) {
        lprintf(ERROR, "Error receiving data from the backend, %d", res);
        return;
    }
    if (msg->msgtype != MT_PING) {
        lprintf(ERROR, "First received message should be 'ping' message, got '%c' instead", msg->msgtype);
        return;
    } else {
        res = plcontainer_channel_send(conn, msg);
        if (res < 0) {
            lprintf(ERROR, "Cannot send 'ping' message response");
            return;
        }
    }
    pfree(msg);

    unsigned long long t1;
    unsigned long long t2;
    unsigned long long receive_time = 0;
    unsigned long long handle_call_time = 0;
    unsigned long long times = 0;
    while (1) {
    	times++;
    		t1 =gettime_microsec2();
        res = plcontainer_channel_receive(conn, &msg);
        t2 = gettime_microsec2();

        	receive_time += t2 - t1;
        if (res == -3) {
            lprintf(NOTICE, "Backend must have closed the connection");
            break;
        }
        if (res < 0) {
            lprintf(ERROR, "Error receiving data from the backend, %d", res);
            break;
        }

        switch (msg->msgtype) {
            case MT_CALLREQ:
                handle_call((plcMsgCallreq*)msg, conn, times);
                free_callreq((plcMsgCallreq*)msg, false, false);
                break;
            default:
                lprintf(ERROR, "received unknown message: %c", msg->msgtype);
        }
        handle_call_time += gettime_microsec2() - t2;
        if(times % 1000 ==0){
        lprintf(LOG, "plcontainerstat %llu : %llu"
                   , receive_time, handle_call_time);
        }
    }
}
