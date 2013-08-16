// Copyright header

//
// Symbols in a global namespace
//

int myVariable;

int myFunction(bool yesno, int number) {}

enum MyEnum { V1, V2 };

class MyClass
{
public:
    MyClass() {}
    int function1();
    int function2(bool yesno, int number) {}
};

//
// Symbols in a named namespace
//

namespace MyNamespace {

int myVariable;

int myFunction(bool yesno, int number) {}

enum MyEnum { V1, V2 };

class MyClass
{
public:
    MyClass() {}
    int function1();
    int function2(bool yesno, int number) {}
};

} // namespace MyNamespace

//
// Symbols in an anonymous namespace
//

namespace {

int myVariable;

int myFunction(bool yesno, int number) {}

enum MyEnum { V1, V2 };

class MyClass
{
public:
    MyClass() {}
    int function1();
    int function2(bool yesno, int number) {}
};

} // anonymous namespace
