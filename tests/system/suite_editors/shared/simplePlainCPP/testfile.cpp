#include "testfile.h"

class SomeClass
{
public:
    SomeClass() {}
    void function1(int a);
};

bool function1(int a) {
  SOME_MACRO_NAME(a)
  return a;
}

