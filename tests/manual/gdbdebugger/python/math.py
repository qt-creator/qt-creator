
def square(a):
    x = a * a
    return a

def cube(a):
    l = [1, 2, 4]
    t = (1, 2, 3)
    d = {1: 'one', 2: 'two', 'three': 3}
    s = u'unixcode'
    x = xrange(1, 10)
    b = buffer("xxx")
    x = square(a)
    x = x * a
    x = x + 1
    x = x - 1
    return x

def main():
    print cube(3)
    print cube(4)
    print cube(5)

if __name__ == '__main__':
  main()
