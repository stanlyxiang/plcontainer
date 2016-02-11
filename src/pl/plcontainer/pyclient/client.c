#include <errno.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "common/comm_channel.h"
#include "common/comm_logging.h"
#include "common/libpq-mini.h"

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

//#define _DEBUG_CLIENT

#include <Python.h>
/*
  Resources:

  1. https://docs.python.org/2/c-api/reflection.html
 */

static void init();
static void receive_loop();
static void handle_call(callreq req);

static char *progname;

static int fd, sock;

int
main(int argc, char **argv) {
    socklen_t          raddr_len;
    int                port;
    struct sockaddr_in raddr;
    struct sockaddr_in addr;
    PGconn_min *       conn;

    if (argc > 0) {
        progname = argv[0];
    }

    port = 8080;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        lprintf(ERROR, "%s", strerror(errno));
    }

    addr = (struct sockaddr_in){
        .sin_family = AF_INET,
        .sin_port   = htons(port),
        .sin_addr = {.s_addr = INADDR_ANY},
    };
    if (bind(fd, (const struct sockaddr *)&addr, sizeof(addr)) == -1) {
        lprintf(ERROR, "%s", strerror(errno));
    }

    if (listen(fd, 10) == -1) {
        lprintf(ERROR, "%s", strerror(errno));
    }

    while (true) {
        raddr_len = sizeof(raddr);
        sock      = accept(fd, (struct sockaddr *)&raddr, &raddr_len);
        if (sock == -1) {
            lprintf(ERROR, "failed to accept connection: %s", strerror(errno));
        }

        conn          = pq_min_connect_fd(sock);
        //conn->Pfdebug = stdout;
        plcontainer_channel_initialize(conn);

        receive_loop();

#ifndef _DEBUG_CLIENT
        break;
#endif

    }

    lprintf(NOTICE, "Client has finished execution");
    return 0;
}

void
receive_loop() {
    message msg;

    init();

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

/* VVVVVVVVVVVVVVVV python specific code below this line VVVVVVVVVVVVVVVV */

static char *create_python_func(callreq req);

static void
handle_call(callreq req) {
    int              i;
    char *           func, *txt;
    plcontainer_result res;
    error_message    err;
    PyObject *       exc, *val, *retval, *tb, *str, *dict, *args;

    /* import __main__ to get the builtin functions */
    val = PyImport_ImportModule("__main__");
    if (val == NULL) {
        goto error;
    }
    dict = PyModule_GetDict(val);
    dict = PyDict_Copy(dict);

    /* wrap the input in a function and evaluate the result */
    func = create_python_func(req);
    val  = PyRun_String(func, Py_single_input, dict, dict);
    if (val == NULL) {
        goto error;
    }
    Py_DECREF(val);
    free(func);

    /* get the function from the global dictionary */
    val = PyDict_GetItemString(dict, req->proc.name);
    Py_INCREF(val);
    if (!PyCallable_Check(val)) {
        lprintf(FATAL, "object not callable");
    }

    /* create the argument list */
    args = PyTuple_New(req->nargs);
    for (i = 0; i < req->nargs; i++) {
        PyObject *arg;
        if (strcmp(req->args[i].type, "text") == 0) {
            arg = PyString_FromString((char *)req->args[i].value);
        } else if (strcmp(req->args[i].type, "int4") == 0) {
            arg = PyLong_FromString((char *)req->args[i].value, NULL, 0);
        } else {
            lprintf(ERROR, "unknown type %s", req->args[i].type);
        }

        if (arg == NULL) {
            goto error;
        }

        if (PyTuple_SetItem(args, i, arg) != 0) {
            goto error;
        }
    }

    /* call the function */
    retval = PyObject_Call(val, args, NULL);
    if (retval == NULL) {
        goto error;
    }

    val = PyObject_Str(retval);
    /* TODO: get the type of the return value */

    /* release all references */
    Py_DECREF(args);
    Py_DECREF(dict);

    txt = PyString_AsString(val);
    if (txt == NULL) {
        goto error;
    }

    /* allocate a result */
    res          = pmalloc(sizeof(*res));
    res->msgtype = MT_RESULT;
    res->names   = pmalloc(sizeof(*res->types));
    res->names   = pmalloc(sizeof(*res->names));
    res->types   = pmalloc(sizeof(*res->types));
    res->data    = pmalloc(sizeof(*res->data));
    res->data[0] = pmalloc(sizeof(*res->data[0]));

    res->rows = res->cols = 1;

    /* use strdup since free_result assumes values are on the heap */
    res->names[0] = pstrdup("result");
    res->types[0] = pstrdup("text");
    if (PyFloat_Check(retval)) {
        res->types[0] = pstrdup("double precision");
    }
    res->data[0]->isnull = false;
    res->data[0]->value  = txt;

    /* send the result back */
    plcontainer_channel_send((message)res);

    free_result(res);

    /* free the result object and the python value */
    Py_DECREF(retval);
    Py_DECREF(val);
    return;

error:
    PyErr_Fetch(&exc, &val, &tb);
    str = PyObject_Str(val);
    if (str == NULL) {
        lprintf(FATAL, "cannot convert error to string");
    }

    /* an exception was thrown */
    err             = pmalloc(sizeof(*err));
    err->msgtype    = MT_EXCEPTION;
    err->message    = PyString_AsString(str);
    err->stacktrace = "";

    /* send the result back */
    plcontainer_channel_send((message)err);

    /* free the objects */
    free(err);
    Py_DECREF(str);
}

static char *
create_python_func(callreq req) {
    int         i, plen;
    const char *sp;
    char *      mrc, *mp;
    size_t      mlen, namelen;
    const char *src, *name;

    name = req->proc.name;
    src  = req->proc.src;

    /*
     * room for function source, the def statement, and the function call.
     *
     * note: we need to allocate strlen(src) * 2 since we replace
     * newlines with newline followed by tab (i.e. "\n\t")
     */
    namelen = strlen(name);
    /* source */
    mlen = sizeof("def ") + namelen + sizeof("():\n\t") + (strlen(src) * 2);
    /* function delimiter*/
    mlen += sizeof("\n\n");
    /* room for n-1 commas and the n argument names */
    mlen += req->nargs - 1;
    for (i = 0; i < req->nargs; i++) {
        mlen += strlen(req->args[i].name);
    }
    mlen += 1; /* null byte */

    mrc  = pmalloc(mlen);
    plen = snprintf(mrc, mlen, "def %s(", name);
    assert(plen >= 0 && ((size_t)plen) < mlen);

    sp = src;
    mp = mrc + plen;

    for (i = 0; i < req->nargs; i++) {
        if (i > 0)
            mp += snprintf(mp, mlen - (mp - mrc), ",%s", req->args[i].name);
        else
            mp += snprintf(mp, mlen - (mp - mrc), "%s", req->args[i].name);
    }

    mp += snprintf(mp, mlen - (mp - mrc), "):\n\t");
    /* replace newlines with newline+tab */
    while (*sp != '\0') {
        if (*sp == '\r' && *(sp + 1) == '\n')
            sp++;

        if (*sp == '\n' || *sp == '\r') {
            *mp++ = '\n';
            *mp++ = '\t';
            sp++;
        } else
            *mp++ = *sp++;
    }
    /* finish the function definition with 2 newlines */
    *mp++ = '\n';
    *mp++ = '\n';
    *mp++ = '\0';

    assert(mp <= mrc + mlen);

    return mrc;
}

/* plpy methods */

static PyObject *
plpy_execute(PyObject *self __attribute__((unused)), PyObject *pyquery) {
    int               i, j;
    sql_msg_statement msg;
    plcontainer_result  result;
    message           resp;
    PyObject *        pyresult, *pydict, *pyval, *tmp;

    if (!PyString_Check(pyquery)) {
        PyErr_SetString(PyExc_TypeError, "expected the query string");
        return NULL;
    }

    msg            = pmalloc(sizeof(*msg));
    msg->msgtype   = MT_SQL;
    msg->sqltype   = SQL_TYPE_STATEMENT;
    msg->statement = PyString_AsString(pyquery);

    plcontainer_channel_send((message)msg);

    /* we don't need it anymore */
    pfree(msg);

    resp = plcontainer_channel_receive();
    if (resp->msgtype != MT_RESULT) {
        lprintf(ERROR, "didn't receive result back");
    }

    result = (plcontainer_result)resp;

    /* convert the result set into list of dictionaries */
    pyresult = PyList_New(result->rows);
    if (pyresult == NULL) {
        return NULL;
    }

    for (i = 0; i < result->rows; i++) {
        pydict = PyDict_New();

        for (j = 0; j < result->cols; j++) {
            /* just look at the first char, hacky */
            switch (result->types[j][0]) {
            case 'i':
                pyval = PyLong_FromString(result->data[i][j].value, NULL, 0);
                break;
            case 'f':
                tmp   = PyString_FromString(result->data[i][j].value);
                pyval = PyFloat_FromString(tmp, NULL);
                Py_DECREF(tmp);
                break;
            case 't':
                pyval = PyString_FromString(result->data[i][j].value);
                break;
            default:
                PyErr_Format(PyExc_TypeError, "unknown type %s",
                             result->types[i]);
                return NULL;
            }

            if (PyDict_SetItemString(pydict, result->names[j], pyval) != 0) {
                return NULL;
            }
        }

        if (PyList_SetItem(pyresult, i, pydict) != 0) {
            return NULL;
        }
    }

    free_result(result);

    return pyresult;
}

static PyMethodDef moddef[] = {
    {"execute", plpy_execute, METH_O, NULL}, {NULL},
};

void
init() {
    PyObject *plpymod, *mainmod;

    Py_SetProgramName(progname);
    Py_Initialize();

    /* create the plpy module */
    plpymod = Py_InitModule("plpy", moddef);

    mainmod = PyImport_ImportModule("__main__");
    PyModule_AddObject(mainmod, "plpy", plpymod);
    Py_DECREF(mainmod);
}