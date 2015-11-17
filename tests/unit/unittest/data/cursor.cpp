#include "cursor.h"

void function(int x)
{

}

namespace Namespace
{
SuperClass::SuperClass(int x) noexcept
  : y(x)
{
    int LocalVariable;
}

int SuperClass::Method()
{
    Method();
    AbstractVirtualMethod(y);
    int LocalVariable;
    return y;
}

int SuperClass::VirtualMethod(int z)
{
    AbstractVirtualMethod(z);

    return y;
}

bool SuperClass::ConstMethod() const
{
    return y;
}

void SuperClass::StaticMethod()
{
    using longint = long long int;
    using lint = longint;

    lint foo;

    foo = 30;

    const lint bar = 20;
}
}

template <class T>
void TemplateFunction(T LocalVariableParameter)
{
    T LocalVariable;
}

Namespace::SuperClass::operator int() const
{
    int LocalVariable;
}

int Namespace::SuperClass::operator ++() const
{
    int LocalVariable;

    return LocalVariable;
}

Namespace::SuperClass::~SuperClass()
{
    int LocalVariable;
}

void Struct::FinalVirtualMethod()
{

}

void f1(Struct *FindFunctionCaller)
{
    FindFunctionCaller->FinalVirtualMethod();
}

void f2(){
    Struct *s = new Struct;

    f1(s);
}

void f3()
{
    auto FindFunctionCaller = Struct();

    FindFunctionCaller.FinalVirtualMethod();
}


void f4()
{
    Struct s;

    auto *sPointer = &s;
    auto sValue = s;
}

void NonFinalStruct::function()
{
    FinalVirtualMethod();
}

void OutputFunction(int &out, int in = 1, const int &in2=2, int *out2=nullptr);
void InputFunction(const int &value);

void f5()
{
    int OutputValue;
    int InputValue = 20;

    OutputFunction(OutputValue);
    InputFunction(InputValue);
}

void ArgumentCountZero();
void ArgumentCountTwo(int one, const int &two);
void IntegerValue(int);
void LValueReference(int &);
void ConstLValueReference(const int &);
void PointerToConst(const int *);
void Pointer(int *);
void ConstantPointer(int *const);
void ConstIntegerValue(const int);
