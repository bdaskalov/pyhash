
import pyhash as pyhash
import gc

import collections
class Bla(collections.MutableMapping):
    def __init__(self, *args, **kwargs):
        self.store = pyhash.IIMap('L', 'L')
        self.update(dict(*args, **kwargs))  # use the free update to set keys

    def __getitem__(self, key):
        return self.store[self.__keytransform__(key)]

    def __setitem__(self, key, value):
        self.store[self.__keytransform__(key)] = value

    def __delitem__(self, key):
        del self.store[self.__keytransform__(key)]

    def __iter__(self):
        return iter(self.store)

    def __len__(self):
        return len(self.store)

    def __keytransform__(self, key):
        return key    

def main():
    pyhash.greet('World')
    # ii = pyhash.IIMap('L', 'L')
    ii = Bla()
    # ii = dict()
    for i in range(1000000):
        ii[i] = 10000000 + i
    z = 0
    gc.collect()
    for i in range(1000000):
        z += ii[i]
    print(z)
    print(ii.get(1))
    print(ii.get(-123))
    print(ii.get(-123, "alabala"))

if __name__ == "__main__":
    print(dir(pyhash))
    main()
