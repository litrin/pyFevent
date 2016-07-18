#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <time.h>

#include <Python.h>
#include "structmember.h"

typedef struct {
    PyObject_HEAD
    long fd;

    int pid;
    int cpu;
} Cmonitor;

static long init_Cmonitor(pid_t pid, int cpu) {
    struct perf_event_attr pe;

    memset(&pe, 0, sizeof(struct perf_event_attr));

    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    int fd;
    fd = syscall(__NR_perf_event_open, &pe, pid, cpu,
                 -1, 0);
    if (fd == -1) {
        return -1;
    }

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    return fd;
}

long long get_ipc(long fd) {

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    long long count;
    int size = read(fd, &count, sizeof(long long));


    return count;
}

static void perf_event_dealloc(Cmonitor *self) {
    close(self->fd);
    self->ob_type->tp_free((PyObject *) self);
}

static PyObject *perf_event_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    Cmonitor *self;

    self = (Cmonitor *) type->tp_alloc(type, 0);

    self->fd = -1;

    return (PyObject *) self;
}

static PyObject *CounterErr;

void throw_counter_error(){
    CounterErr = PyErr_NewException("pyf_event.CounterError", NULL, NULL);
    Py_INCREF(CounterErr);
}

static int perf_event_init(Cmonitor *self, PyObject *args, PyObject *kwds) {

    int pid, cpu;
    if (! PyArg_ParseTuple(args, "ii", &pid, &cpu)){
        pid = 0;
        cpu = -1;
    }

    self->cpu = cpu;
    self->pid = pid;
    self->fd = init_Cmonitor((pid_t) pid, cpu);
    if (self->fd == -1){
        throw_counter_error();
    }
    return 0;

}

//
static PyMemberDef perf_event_members[] = {
        {"pid", T_INT, offsetof(Cmonitor, pid), 0,  ""},
        {"cpu", T_INT, offsetof(Cmonitor, cpu), 0,  ""},

        {NULL}  /* Sentinel */
};

static PyObject *perf_event_value(Cmonitor *self, PyObject *args, PyObject *kwds)
{
    long long ipc = get_ipc(self->fd);

    return Py_BuildValue("l", ipc);

}


static PyMethodDef perf_event_methods[] = {
        {"ipc", (PyCFunction) perf_event_value, METH_VARARGS,
                "Get ipc"
        },
        {NULL}  /* Sentinel */
};

static PyTypeObject perf_event_Type = {
        PyObject_HEAD_INIT(NULL)
        0,                         /*ob_size*/
        "pyf_event.IPC",             /*tp_name*/
        sizeof(Cmonitor),             /*tp_basicsize*/
        0,                         /*tp_itemsize*/
        (destructor)perf_event_dealloc, /*tp_dealloc*/
        0,                         /*tp_print*/
        0,                         /*tp_getattr*/
        0,                         /*tp_setattr*/
        0,                         /*tp_compare*/
        0,                         /*tp_repr*/
        0,                         /*tp_as_number*/
        0,                         /*tp_as_sequence*/
        0,                         /*tp_as_mapping*/
        0,                         /*tp_hash */
        0,                         /*tp_call*/
        0,                         /*tp_str*/
        0,                         /*tp_getattro*/
        0,                         /*tp_setattro*/
        0,                         /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
        "",           /* tp_doc */
        0,                       /* tp_traverse */
        0,                       /* tp_clear */
        0,                       /* tp_richcompare */
        0,                       /* tp_weaklistoffset */
        0,                       /* tp_iter */
        0,                       /* tp_iternext */
        perf_event_methods,             /* tp_methods */
        perf_event_members,             /* tp_members */
//        NULL,             /* tp_members */
        0,                         /* tp_getset */
        0,                         /* tp_base */
        0,                         /* tp_dict */
        0,                         /* tp_descr_get */
        0,                         /* tp_descr_set */
        0,                         /* tp_dictoffset */
        (initproc)perf_event_init,      /* tp_init */
        0,                         /* tp_alloc */
        perf_event_new,                 /* tp_new */
};

static PyMethodDef module_methods[] = {
        {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC    /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

PyMODINIT_FUNC initpyf_event(void) {
    PyObject *m;
    
    if (PyType_Ready(CounterErr) < 0)
        return ;

    if (PyType_Ready(&perf_event_Type) < 0)
        return;

    m = Py_InitModule3("pyf_event", module_methods,
                       "Example module that creates an extension type.");

    if (m == NULL)
        return;

    Py_INCREF(&perf_event_Type);
    Py_INCREF(&CounterErr);

    PyModule_AddObject(m, "IPC", (PyObject *)&perf_event_Type);
    PyModule_AddObject(m, "CountError", (PyObject *)CounterErr);
}
