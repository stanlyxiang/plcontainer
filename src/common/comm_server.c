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

extern unsigned long long gettime_nanosec(void);

extern unsigned long long qe_delay_time;
extern unsigned long long delay_time;
extern unsigned long long handle_call_time ;
extern unsigned long long receive_time;
extern unsigned long long py_pre_time ;
extern unsigned long long charstar_convert2py_time ;
extern unsigned long long py_exec_time ;
extern unsigned long long py_convert2charstar_time ;
extern unsigned long long client_send_time ;
extern unsigned long long free_charstar_result_time ;



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
    unsigned long long times = 0;
    while (1) {
    		times++;
    		t1 =gettime_nanosec();
        res = plcontainer_channel_receive(conn, &msg);
        	receive_time += gettime_nanosec() - t1;
        	t1 =gettime_nanosec();
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
        handle_call_time += gettime_nanosec() - t1;
        if(times % 100000 ==0){
        lprintf(LOG, "plcontainerstatpyclient receivetime:%llu(delay_time: %llu); handle_call_time: %llu("
        		"py_pre_time:%llu, charstar_convert2py_time:%llu, py_exec_time:%llu,py_convert2charstar_time:%llu"
        		" client_send_time:%llu ,free_charstar_result_time:%llu)"
                   , receive_time, delay_time, handle_call_time,py_pre_time, charstar_convert2py_time, py_exec_time,
				   py_convert2charstar_time,client_send_time,free_charstar_result_time);
        }
    }
}
