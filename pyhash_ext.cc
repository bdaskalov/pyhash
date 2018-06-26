#include <Python.h>
#include <map>
#include <cstdio>
// #include <sparsehash/dense_hash_map>
#include <sparsehash/sparse_hash_map>
#include <tr1/unordered_map>

class MapIterator {
public:
    virtual ~MapIterator() {};
    virtual bool hasNext() const = 0;
    virtual PyObject* next() = 0;
};

class Map {
public:
    virtual ~Map() {};
    virtual ssize_t size() = 0;
    virtual PyObject* get(PyObject* key) = 0;
    virtual bool put(PyObject *key, PyObject *val) = 0;
    virtual MapIterator* makeIterator() = 0;
};

template <typename T>
T from_pyobj(PyObject* val);

template <>
long long from_pyobj(PyObject* val) {
    assert(val != NULL);
    return PyLong_AsLongLong(val);
}

template <>
int from_pyobj(PyObject* val) {
    assert(val != NULL);
    return PyLong_AsLong(val);
}

template <typename T>
PyObject* to_pyobj(T val);

template <>
PyObject* to_pyobj(long long val) {
    return PyLong_FromLongLong(val);
}

template <>
PyObject* to_pyobj(int val) {
    return PyLong_FromLong(val);
}


template<typename MapType, typename K>
class MapIteratorImpl : public MapIterator {
public:
    typedef typename MapType::iterator IteratorType;

    MapIteratorImpl(const IteratorType& iter,
                    const IteratorType& end)
        : _iter(iter), _end(end) {}

    virtual bool hasNext() const {
        return _iter != _end;
    }

    virtual PyObject* next() {
        PyObject *key = to_pyobj<K>(_iter->first);
        // maybe incref here
        ++ _iter;
        return key;
    }

private:
    IteratorType _iter;
    IteratorType _end;
};

template<typename K, typename V>
class MapImpl : public Map {
public:
    // typedef std::map<K, V> MapType;
    // typedef google::dense_hash_map<K, V> MapType;
    typedef google::sparse_hash_map<K, V> MapType;
    // typedef std::tr1::unordered_map<K, V> MapType;

    typedef typename MapType::iterator IteratorType;

    MapImpl() {
        // For dense_hash_map
        // _map.set_empty_key((K)-1);
        // For sparse_hash_map
        _map.set_deleted_key(-1);
    }

    virtual ssize_t size() {
        return _map.size();
    }

    virtual PyObject* get(PyObject* key) {
        K intkey = from_pyobj<K>(key);
        IteratorType it = _map.find(intkey);
        if (it == _map.end()) {
            return NULL;
        }
        return to_pyobj<V>(it->second);
    }

    virtual bool put(PyObject *key, PyObject *val) {
        K intkey = from_pyobj<K>(key);
        if (val) {
            V intval = from_pyobj<V>(val);
            _map[intkey] = intval;
            return true;
        } else {
            bool exists = (1 == _map.erase(intkey));
            return exists;
        }
    }

    virtual MapIterator* makeIterator() {
        return new MapIteratorImpl<MapType,K>(_map.begin(), _map.end());
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
    // typedef std::tr1::unordered_map<K, PyObject*> MapType;
    typedef typename MapType::iterator IteratorType;

    MapImpl() {
        // _map.set_empty_key((K)-1);
        _map.set_deleted_key(-1);
    }

    virtual ~MapImpl() {
        for(IteratorType it=_map.begin(); it != _map.end(); ++it) {
            Py_DECREF(it->second);
        }
    }

    virtual ssize_t size() {
        return _map.size();
    }

    virtual PyObject* get(PyObject* key) {
        K intkey = from_pyobj<K>(key);
        IteratorType it = _map.find(intkey);
        if (it == _map.end()) {
            return NULL;
        }
        // We are effectively crating a ref here so we need to increse refcnt.
        Py_INCREF(it->second);
        return it->second;
    }

    virtual bool put(PyObject *key, PyObject *val) {
        K intkey = from_pyobj<K>(key);
        IteratorType it = _map.find(intkey);
        if (it != _map.end()) {
            Py_DECREF(it->second);
            if (val) {
                Py_INCREF(val);
                it->second = val;
                return true;
            } else {
                _map.erase(intkey);
                return true;
            }
        } else {
            if (val) {
                Py_INCREF(val);
                _map[intkey] = val;
                return true;
            } else {
                return false;
            }
        }
    }

    virtual MapIterator* makeIterator() {
        return new MapIteratorImpl<MapType,K>(_map.begin(), _map.end());;
    }

private:
    MapType _map;
};

extern "C" {

typedef struct {
    int keytype;
    int valtype;
    PyObject_HEAD
    Map* map;
} pyhash_IIMapObject;

static int *
IIMap_init(pyhash_IIMapObject *self, PyObject *args, PyObject *kwds)
{
    int keytype, valtype;
    PyArg_ParseTuple(args, "CC", &keytype, &valtype);
    self->keytype = keytype;
    self->valtype = valtype;

    if (keytype == 'L' && valtype == 'L') {
        self->map = new MapImpl<long long, long long>();
    } else if (keytype == 'L' && valtype == 'O') {
        self->map = new MapImpl<long long, PyObject*>();
    } else if (keytype == 'I' && valtype == 'I') {
        self->map = new MapImpl<int, int>();
    } else if (keytype == 'I' && valtype == 'O') {
        self->map = new MapImpl<int, PyObject*>();
    }

    PyObject_GC_Track((PyObject*) self);
    return 0;
}

static void
IIMap_dealloc(pyhash_IIMapObject* self)
{
    PyObject_GC_UnTrack(self);
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
    if (self->map->put(key, val)) {
        return 0;
    } else {
        PyErr_SetObject(PyExc_KeyError, key);
        return -1;
    }
}

static PyObject *
IIMap_iter(pyhash_IIMapObject *self);

int IIMap_traverse(pyhash_IIMapObject *self, visitproc visit, void *arg) {
    if (self->keytype != 'O' && self->valtype != 'O') {
        return 0;
    }

    MapIterator* it = self->map->makeIterator();
    while (it->hasNext()) {
        PyObject* key = it->next();
        if (self->keytype == 'O') {
            Py_VISIT(key);
        }
        if (self->valtype == 'O') {
            PyObject* val = IIMap_getitem(self, key);
            Py_VISIT(val);
        }
        // If key is not an object it was just created by it->next() and we need to decref
        if (self->keytype != 'O') {
            Py_DecRef(key);
        }
    }
}
static int
IIMap_clear(pyhash_IIMapObject *self) {
    //TODO
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

static PyMappingMethods IIMapMappingMethods = {
    (lenfunc) IIMap_size,
    (binaryfunc) IIMap_getitem,
    (objobjargproc) IIMap_setitem,
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    "Int-to-int map",          /* tp_doc */
    (traverseproc) IIMap_traverse, /* tp_traverse */
    (inquiry) IIMap_clear,     /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    (getiterfunc) IIMap_iter,  /* tp_iter */
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


typedef struct {
    PyObject_HEAD
    MapIterator* map_iterator;
    pyhash_IIMapObject* map;
} pyhash_IIMapIteratorObject;


static void
IIMapIterator_dealloc(pyhash_IIMapIteratorObject* self) {
    delete self->map_iterator;
    Py_DECREF(self->map);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

PyObject *
IIMapIterator_iternext(pyhash_IIMapIteratorObject *self) {
    if (self->map_iterator->hasNext()) {
        return self->map_iterator->next();
    } else {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
}


static PyTypeObject pyhash_IIMapIteratorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyhash.IIMAPIterator",    /* tp_name */
    sizeof(pyhash_IIMapIteratorObject), /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor) IIMapIterator_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,       /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Map iterator",          /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    (iternextfunc) IIMapIterator_iternext, /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

static PyObject *
IIMap_iter(pyhash_IIMapObject *self) {
    pyhash_IIMapIteratorObject* iterator =
        PyObject_New(pyhash_IIMapIteratorObject, &pyhash_IIMapIteratorType);
    iterator->map_iterator = self->map->makeIterator();
    Py_INCREF(self);
    iterator->map = self;
    return (PyObject*)iterator;
}


static PyMethodDef PyHashMethods[] = {
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef greet =
{
    PyModuleDef_HEAD_INIT,
    "pyhash_ext",     /* name of module */
    "",          /* module documentation, may be NULL */
    -1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    PyHashMethods
};

PyMODINIT_FUNC PyInit_pyhash_ext(void)
{
    if (PyType_Ready(&pyhash_IIMapIteratorType) < 0)
        return NULL;

    pyhash_IIMapType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&pyhash_IIMapType) < 0)
        return NULL;

    PyObject *m = PyModule_Create(&greet);
    if (!m)
        return NULL;

    Py_INCREF(&pyhash_IIMapType);
    Py_INCREF(&pyhash_IIMapIteratorType);
    PyModule_AddObject(m, "IIMap", (PyObject*) &pyhash_IIMapType);
    PyModule_AddObject(m, "IIMapIterator", (PyObject*) &pyhash_IIMapIteratorType);
    return m;
}

} // extern "C"