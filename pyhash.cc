#include <Python.h>
#include <map>
#include <cstdio>


static PyObject *
greet_name(PyObject *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "s", &name))
    {
        return NULL;
    }
    
    printf("Hello %s!\n", name);

    Py_RETURN_NONE;
}

typedef struct {
    PyObject_HEAD
    std::map<int64_t, int64_t>* map;
} pyhash_IIMapObject;

static int *
IIMap_init(pyhash_IIMapObject *self, PyObject *args, PyObject *kwds) 
{
    std::printf("init\n");
    self->map = new std::map<int64_t,int64_t>();
    std::printf("map %p\n", self->map);
}


static void
IIMap_dealloc(pyhash_IIMapObject* self)
{
    delete self->map;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

Py_ssize_t IIMap_size(pyhash_IIMapObject* self) {
    return self->map->size();
}

PyObject* IIMap_getitem(pyhash_IIMapObject *self, PyObject *key) {
    int64_t intkey = PyLong_AsLongLong(key);
    std::map<int64_t, int64_t>::const_iterator it = self->map->find(intkey);
    if (it == self->map->end()) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }
    return PyLong_FromLongLong((*(self->map))[intkey]);
}

int IIMap_setitem(pyhash_IIMapObject *self, PyObject *key, PyObject *val) {
    int64_t intkey = PyLong_AsLongLong(key);
    int64_t intval = PyLong_AsLongLong(val);
    (*self->map)[intkey] = intval;
    return 0;
}

static PyMappingMethods IIMapMappingMethods {
    .mp_length = (lenfunc) IIMap_size,
    .mp_subscript = (binaryfunc) IIMap_getitem,
    .mp_ass_subscript = (objobjargproc) IIMap_setitem,
};

static PyTypeObject pyhash_IIMapType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyhash.IIMAP",            /* tp_name */
    sizeof(pyhash_IIMapObject), /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)IIMap_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    &IIMapMappingMethods,       /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Int-to-int map",          /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    0,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc) IIMap_init,     /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
};

static PyMethodDef PyHashMethods[] = {
    {"greet", greet_name, METH_VARARGS, "Greet an entity."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef greet =
{
    PyModuleDef_HEAD_INIT,
    "pyhash",     /* name of module */
    "",          /* module documentation, may be NULL */
    -1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    PyHashMethods
};

PyMODINIT_FUNC PyInit_pyhash(void)
{
    pyhash_IIMapType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&pyhash_IIMapType) < 0)
        return NULL;

    PyObject *m = PyModule_Create(&greet);
    if (!m)
        return NULL;

    Py_INCREF(&pyhash_IIMapType);
    PyModule_AddObject(m, "IIMap", (PyObject*) &pyhash_IIMapType);
    return m;
}
