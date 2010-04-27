
def square(a):
    x = a * a
    return a

def cube(a):
    l = [1, 2, 4]
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
