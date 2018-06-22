#include <Python.h>
#include <map>
#include <cstdio>
#include <sparsehash/dense_hash_map>
#include <sparsehash/sparse_hash_map>

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

class Map {
public:
    virtual ~Map() {};
    virtual ssize_t size() = 0;
    virtual PyObject* get(PyObject* key) = 0;
    virtual void put(PyObject *key, PyObject *val) = 0;
};

template <typename T>
T from_pyobj(PyObject* val);

template <>
long long from_pyobj(PyObject* val) {
    return PyLong_AsLongLong(val);
}

template <typename T>
PyObject* to_pyobj(T val);

template <>
PyObject* to_pyobj(long long val) {
    return PyLong_FromLongLong(val);
}

template<typename K, typename V>
class MapImpl : public Map {
public:
    // typedef std::map<K, V> MapType;
    // typedef google::dense_hash_map<K, V> MapType;
    typedef google::sparse_hash_map<K, V> MapType;
    typedef typename MapType::iterator MapIterator;

    MapImpl() {
        // _map.set_empty_key((K)-1);
    }

    virtual ssize_t size() {
        return _map.size();
    }

    virtual PyObject* get(PyObject* key) {
        K intkey = from_pyobj<K>(key);
        MapIterator it = _map.find(intkey);
        if (it == _map.end()) {
            return NULL;
        }
        return to_pyobj<V>(it->second);
    }

    virtual void put(PyObject *key, PyObject *val) {
        K intkey = from_pyobj<K>(key);
        V intval = from_pyobj<V>(val);
        _map[intkey] = intval;
    }

private:
    MapType _map;
};

template<typename K>
class MapImpl<K, PyObject*> : public Map {
public:
    // typedef std::map<K, PyObject*> MapType;
    // typedef google::dense_hash_map<K, PyObject*> MapType;
    typedef google::sparse_hash_map<K, PyObject*> MapType;
    typedef typename MapType::iterator MapIterator;

    MapImpl() {
        // _map.set_empty_key((K)-1);
    }

    virtual ~MapImpl() {
        for(MapIterator it=_map.begin(); it != _map.end(); ++it) {
            Py_DECREF(it->second);
        }
    }

    virtual ssize_t size() {
        return _map.size();
    }

    virtual PyObject* get(PyObject* key) {
        K intkey = from_pyobj<K>(key);
        MapIterator it = _map.find(intkey);
        if (it == _map.end()) {
            return NULL;
        }
        return it->second;
    }

    virtual void put(PyObject *key, PyObject *val) {
        K intkey = from_pyobj<K>(key);
        Py_INCREF(val);
        _map[intkey] = val;
    }

private:
    MapType _map;
};

typedef struct {
    PyObject_HEAD
    Map* map;
} pyhash_IIMapObject;

static int *
IIMap_init(pyhash_IIMapObject *self, PyObject *args, PyObject *kwds) 
{
    int keytype, valtype;
    PyArg_ParseTuple(args, "CC", &keytype, &valtype);
    if (keytype == 'L' && valtype == 'L') {
        self->map = new MapImpl<long long, long long>();
    } else if (keytype == 'L' && valtype == 'O') {
        self->map = new MapImpl<long long, PyObject*>();
    }
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
    PyObject* val = self->map->get(key);
    if (!val) {
        PyErr_SetObject(PyExc_KeyError, key);
    }
    return val;
}

int IIMap_setitem(pyhash_IIMapObject *self, PyObject *key, PyObject *val) {
    self->map->put(key, val);
    return 0;
}

// Broken
// static PyObject*
// IIMap_get(pyhash_IIMapObject* self, PyObject *args) {
//     PyObject* key = NULL;
//     PyObject* alternative = NULL;
//     PyArg_ParseTuple(args, "O|O", key, alternative);
//     PyObject* val = self->map->get(key);
//     if (!val) {
//         return alternative;
//     }
//     return val;
// }

static PyMappingMethods IIMapMappingMethods {
    .mp_length = (lenfunc) IIMap_size,
    .mp_subscript = (binaryfunc) IIMap_getitem,
    .mp_ass_subscript = (objobjargproc) IIMap_setitem,
};

static struct PyMethodDef IIMap_Methods[] = {
    // {"get", (PyCFunction)IIMap_get, METH_VARARGS, "similar to dict.get()"},
    {NULL}
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
    &IIMap_Methods[0],             /* tp_methods */
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
