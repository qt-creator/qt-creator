// Copyright header

#define GENERATE_FUNC void myFunctionGenerated() {}

//
// Symbols in a global namespace
//

GENERATE_FUNC

int myVariable;

int myFunction(bool yesno, int number) {}

void pointOfService() {}
int getPosition() { return 0; }
int positiveNumber() { return 2; }

enum MyEnum { V1, V2 };

class MyClass
{
public:
    MyClass() {}
    int functionDeclaredOnly();
    int functionDefinedInClass(bool yesno, int number) {}
    int functionDefinedOutSideClass(char c);
};

int MyClass::functionDefinedOutSideClass(char c) {}

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
    int functionDeclaredOnly();
    int functionDefinedInClass(bool yesno, int number) {}
    int functionDefinedOutSideClass(char c);
    int functionDefinedOutSideClassAndNamespace(float x);
};

int MyClass::functionDefinedOutSideClass(char c) {}

} // namespace MyNamespace

int MyNamespace::MyClass::functionDefinedOutSideClassAndNamespace(float x) {}

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
    int functionDeclaredOnly();
    int functionDefinedInClass(bool yesno, int number) {}
    int functionDefinedOutSideClass(char c);
};

int MyClass::functionDefinedOutSideClass(char c) {}

} // anonymous namespace


int main() {}
