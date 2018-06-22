
import pyhash as pyhash

def main():
    pyhash.greet('World')
    ii = pyhash.IIMap()
    print(ii)
    print(len(ii))
    ii[1] = 2
    ii[2] = 3
    ii[3] = 4
    print(len(ii))
    print(ii[1])
    print(ii[2])
    print(ii[3])
    try:
        print(ii[4])
    except KeyError as ke:
        print("got KeyError")
    del ii

if __name__ == "__main__":
    print(dir(pyhash))
    main()
