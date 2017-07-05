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
#include <assert.h>

#include "common/comm_channel.h"
#include "common/comm_utils.h"
#include "common/comm_connectivity.h"
#include "common/comm_server.h"
#include "pycall.h"
#include "pyerror.h"

extern unsigned long long delay_time = 0;
extern unsigned long long handle_call_time =0;
extern unsigned long long serialize_time = 0;
extern unsigned long long receive_time=0;

extern unsigned long long gettime_nanosec(void);


unsigned long long gettime_nanosec(void)
{

	unsigned long long t = 0;

        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);


        t = ((unsigned long long)ts.tv_sec) * 1000000000 + ts.tv_nsec;
        return t;
}

int main(int argc UNUSED, char **argv UNUSED) {
    int      sock;
    plcConn* conn;
    int      status;

    assert(sizeof(char) == 1);
    assert(sizeof(short) == 2);
    assert(sizeof(int) == 4);
    assert(sizeof(long long) == 8);
    assert(sizeof(float) == 4);
    assert(sizeof(double) == 8);

    // Bind the socket and start listening the port
    sock = start_listener();

    // Initialize Python
    status = python_init();
    lprintf(LOG, "plcontainerstatstart" );
    #ifdef _DEBUG_CLIENT
        // In debug mode we have a cycle of connections with infinite wait time
        while (true) {
            conn = connection_init(sock);
            if (status == 0) {
                receive_loop(handle_call, conn);
            } else {
                plc_raise_delayed_error();
                return -1;
            }
        }
    #else
        // In release mode we wait for incoming connection for limited time
        // and the client works for a single connection only
        connection_wait(sock);
        conn = connection_init(sock);
        if (status == 0) {
            receive_loop(handle_call, conn);
        } else {
            plc_raise_delayed_error();
        }
    #endif

    lprintf(NOTICE, "Client has finished execution");
    return 0;
}
