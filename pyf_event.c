/*

The MIT License (MIT)

Copyright (c) 2016 Litrin Jiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/


#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <Python.h>
#include "structmember.h"

typedef struct {
    PyObject_HEAD

    int pid;
    int cpu;
    int counter_id;

    long fd;

} Cmonitor;

static PyObject *CmonitorError;

static long init_Cmonitor(pid_t pid, int cpu, int counter_id) {
    struct perf_event_attr pe;

    memset(&pe, 0, sizeof(struct perf_event_attr));

    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = counter_id;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    int fd;
    fd = syscall(__NR_perf_event_open, &pe, pid, cpu,
                 -1, 0);
    if (fd == -1) {
        PyErr_SetString(CmonitorError, "Permission deny!");
        return -1;
    }

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    return fd;
}

long long get_counter(long fd) {

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    long long count;

    if( read(fd, &count, sizeof(long long)))
        return count;

    return -1;
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

static int perf_event_init(Cmonitor *self, PyObject *args, PyObject *kwds) {

    int pid, cpu, counter_id;
    static char *kwlist[] = {"pid", "cpu_core", "counter_id", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i|i|i", kwlist,
                                     &pid, &cpu, &counter_id)){
        pid = 0;
        cpu = -1;
        counter_id = 0;
    }

    self->cpu = cpu;
    self->pid = pid;
    self->counter_id = counter_id;
    // self->fd = init_Cmonitor((pid_t) pid, cpu, counter_id);

    return 0;

}

static int start_counter(Cmonitor *self, PyObject *args) {

   int counter;

   if (PyArg_ParseTuple(args, "i", &counter) && (counter < 9 && counter > 0)){
       self->counter_id = counter;
   }

   self->fd = init_Cmonitor((pid_t) self->pid, self->cpu, self->counter_id);

   if (-1 == self->fd) {
        PyErr_SetString(CmonitorError, "Couldn't get ipc!");
   }

   Py_RETURN_NONE;

}


//
static PyMemberDef perf_event_members[] = {
        {"pid", T_INT, offsetof(Cmonitor, pid), 0,  ""},
        {"cpu_core", T_INT, offsetof(Cmonitor, cpu), 0,  ""},
        {"counter", T_INT, offsetof(Cmonitor, counter_id), 0,  ""},

        {NULL}  /* Sentinel */
};

static PyObject *perf_event_value(Cmonitor *self, PyObject *args, PyObject *kwds)
{
    long long ipc = get_counter(self->fd);

    return Py_BuildValue("l", ipc);

}


static PyMethodDef perf_event_methods[] = {

        {"start_counter", (PyCFunction)start_counter, METH_VARARGS,
        "CPU_CYCLES = 0, INSTRUCTIONS = 1,CACHE_REFERENCES = 2,CACHE_MISSES = 3,BRANCH_INSTRUCTIONS	= 4,BRANCH_MISSES = 5,BUS_CYCLES = 6,STALLED_CYCLES_FRONTEND	= 7,STALLED_CYCLES_BACKEND	= 8,REF_CPU_CYCLES = 9"

        },

        {"get_counter", (PyCFunction)perf_event_value, METH_VARARGS,
                "Get ipc "
        },
        {NULL}  /* Sentinel */
};

static PyTypeObject perf_event_Type = {
        PyObject_HEAD_INIT(NULL)
        0,                         /*ob_size*/
        "pyf_event.HWCounter",             /*tp_name*/
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

    if (PyType_Ready(&perf_event_Type) < 0)
        return;

    m = Py_InitModule3("pyf_event", module_methods,
                       "Example module that creates an extension type.");

    if (m == NULL)
        return;


    Py_INCREF(&perf_event_Type);
    PyModule_AddObject(m, "HWCounter", (PyObject *)&perf_event_Type);

    CmonitorError = PyErr_NewException("pyf_event.HWCounterError", NULL, NULL);
    Py_INCREF(CmonitorError);
    PyModule_AddObject(m, "HWCounterError", CmonitorError);

}
