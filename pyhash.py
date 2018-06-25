import pyhash_ext
import collections

class SparseHashMap(collections.MutableMapping):
    def __init__(self, key_type='L', value_type='O'):
        self.store = pyhash_ext.IIMap(key_type, value_type)

    def __getitem__(self, key):
        return self.store[key]

    def __setitem__(self, key, value):
        self.store[key] = value

    def __delitem__(self, key):
        del self.store[key]

    def __iter__(self):
        return iter(self.store)

    def __len__(self):
        return len(self.store)