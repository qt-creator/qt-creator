auto *Variable       = "Variable";
auto *u8Variable     = u8"Variable";
auto *rawVariable    = R"(Variable)";
auto Character       = 'c';


















auto integer         = 1;
auto numFloat        = 1.2f;




















int function(int x)
{
    return x;
}

struct Foo
{
    void memberFunction() {}
};

int functionDeclaration(int x);

struct Foo2
{
    void memberFunction();
};

void f()
{
    function(1);
}

struct ConversionFunction {
    operator Foo();
    operator int();
};

void TypeReference()
{
    Foo foo;
}

void LocalVariableDeclaration()
{
    Foo foo;

    foo.memberFunction();
}

void LocalVariableFunctionArgument(Foo &foo)
{
    foo.memberFunction();
}

struct Foo3 {
    int ClassMember;

    void ClassMemberReference()
    {
        ClassMember++;
    }
};

struct Foo4
{
    void MemberFunctionReference();

    void function()
    {
        MemberFunctionReference();
    }
};

struct Foo5
{
    void StaticMethod();

    void function()
    {
        Foo5::StaticMethod();
    }
};

enum Enumeration
{
    Enumerator
};

void f2()
{
    Enumeration enumeration;

    enumeration = Enumerator;
}

class ForwardReference;

class Class
{ public:
    Class();
    ~Class();
};

ForwardReference *f3()
{
    Class ConstructorReference;

    return 0;
}

union Union
{

};

Union UnionDeclarationReference;









namespace NameSpace {
struct StructInNameSpace {};
}

namespace NameSpaceAlias = NameSpace;
using NameSpace::StructInNameSpace;
NameSpace::StructInNameSpace foo6;

class BaseClass {
public:
    virtual void VirtualFunction();
    virtual void FinalVirtualFunction();
};


void f8()
{
    BaseClass NonVirtualFunctionCall;
    NonVirtualFunctionCall.VirtualFunction();

    BaseClass *NonVirtualFunctionCallPointer = new BaseClass();
    NonVirtualFunctionCallPointer->VirtualFunction();
}

class DerivedClass : public BaseClass
{public:
    void VirtualFunction() override;
    void FinalVirtualFunction() final;
};

void f8(BaseClass *VirtualFunctionCallPointer)
{
    VirtualFunctionCallPointer->VirtualFunction();
}

class FinalClass final : public DerivedClass
{
    void FinalClassThisCall();
};

void f8(DerivedClass *FinalVirtualFunctionCallPointer)
{
    FinalVirtualFunctionCallPointer->FinalVirtualFunction();
}

void f9(BaseClass *NonFinalVirtualFunctionCallPointer)
{
    NonFinalVirtualFunctionCallPointer->FinalVirtualFunction();
}

void f10(FinalClass *ClassFinalVirtualFunctionCallPointer)
{
    ClassFinalVirtualFunctionCallPointer->VirtualFunction();
}

class Operator {
public:
    Operator operator+=(const Operator &first);
};

Operator operator+(const Operator &first, const Operator &second);

void f10()
{
    auto PlusOperator = Operator() + Operator();
    Operator PlusAssignOperator;
    PlusAssignOperator += Operator();
}

/* Comment */

#define PreprocessorDefinition Class
#define MacroDefinition(a,b) ((a)>(b)?(a):(b))

void f11()
{
    MacroDefinition(2, 4);
}

#include "highlightingmarks.h"

void f12() {
GOTO_LABEL:

    goto GOTO_LABEL;
}

template <class T>
void TemplateFunction(T v)
{
    T XXXXX = v;
}
void TemplateReference()
{
    TemplateFunction(1);
//    std::vector<int> TemplateIntance;
}




template <class T>
class TemplateFoo {};


template <class TemplateTypeParameter = Foo, int NonTypeTemplateParameter = 1, template <class> class TemplateTemplateParameter = TemplateFoo>
void TemplateFunction(TemplateTypeParameter TemplateParameter)
{
    TemplateTypeParameter TemplateTypeParameterReference;
    auto NonTypeTemplateParameterReference = NonTypeTemplateParameter;
    TemplateTemplateParameter<TemplateTypeParameter> TemplateTemplateParameterReference;
}



void FinalClass::FinalClassThisCall()
{
    VirtualFunction();
}


void OutputArgument(int &one, const int &two, int *three=0);

void f12b()
{
    int One;
    OutputArgument(One, 2);
}

#include <highlightingmarks.h>

#define FOREACH(variable, container) \
    variable; \
    auto x = container;

#define foreach2 FOREACH

#include <initializer_list>

void f13()
{
    auto container = 1;
    foreach2(int index, container);
}

class SecondArgumentInMacroExpansionIsField {
    int container = 1;

    void f()
    {
        foreach2(int index, container);
    }
};

typedef unsigned uint32;

enum EnumerationType : uint32
{
    Other = 0,
};


struct TypeInCast {
    void function();
};

void f14()
{
    static_cast<void (TypeInCast::*)()>(&TypeInCast::function);
    reinterpret_cast<void (TypeInCast::*)()>(&TypeInCast::function);
}

using IntegerAlias = int;
using SecondIntegerAlias = IntegerAlias;
using IntegerTypedef = int;
using Function = void (*)();



void f15()
{
    IntegerAlias integerAlias;
    SecondIntegerAlias secondIntegerAlias;
    IntegerTypedef integerTypedef;
    Function();
}

class FriendFoo
{
public:
    friend class FooFriend;
    friend bool operator==(const FriendFoo &first, const FriendFoo &second);
};

class FieldInitialization
{
public:
    FieldInitialization() :
        member(0)
    {}

    int member;
};

template<class Type>
void TemplateFunctionCall(Type type)
{
    type + type;
}

void f16()
{
    TemplateFunctionCall(1);
}


template <typename T>
class TemplatedType
{
    T value = T();
};

void f17()
{
    TemplatedType<int> TemplatedTypeDeclaration;
}

void f18()
{
    auto value = 1 + 2;
}

class ScopeClass
{
public:
    static void ScopeOperator();
};

void f19()
{
    ScopeClass::ScopeOperator();
}

namespace TemplateClassNamespace {
template<class X>
class TemplateClass
{

};
}

void f20()
{
   TemplateClassNamespace::TemplateClass<ScopeClass> TemplateClassDefinition;
}

void f21()
{
    typedef int TypeDefDeclaration;
    TypeDefDeclaration TypeDefDeclarationUsage;
}

typedef int EnumerationTypeDef;

enum Enumeration2 : EnumerationTypeDef {

};

struct Bar {
    Bar &operator[](int &key);
};

void argumentToUserDefinedIndexOperator(Bar object, int index = 3)
{
    object[index];
}

struct LambdaTester
{
    int member = 0;
    void func() {
        const int var = 42, var2 = 84;
        auto lambda = [var, this](int input) {
            return var + input + member;
        };
        lambda(var2);
    }
};

void NonConstReferenceArgument(int &argument);

void f22()
{
    int x = 1;

    NonConstReferenceArgument(x);
}

void ConstReferenceArgument(const int &argument);

void f23()
{
    int x = 1;

    ConstReferenceArgument(x);
}

void RValueReferenceArgument(int &&argument);

void f24()
{
    int x = 1;

    RValueReferenceArgument(static_cast<int&&>(x));
}

void NonConstPointerArgument(int *argument);

void f25()
{
    int *x;

    NonConstPointerArgument(x);
}

void ConstPointerArgument(const int *argument);

void f26()
{
    int *x;

    ConstPointerArgument(x);
}

void NonConstReferenceArgumentCallInsideCall(int x, int &argument);
int GetArgument(int x);

void f27()
{
    int x = 1;

    NonConstReferenceArgumentCallInsideCall(GetArgument(x), x);
}

void f28(int &Reference)
{
    NonConstReferenceArgument(Reference);
}

void f29()
{
    int x;

    NonConstPointerArgument(&x);
}

struct NonConstPointerArgumentAsMemberOfClass
{
    int member;
};

void f30()
{
    NonConstPointerArgumentAsMemberOfClass instance;

    NonConstReferenceArgument(instance.member);
}

struct NonConstReferenceArgumentConstructor
{
    NonConstReferenceArgumentConstructor() = default;
    NonConstReferenceArgumentConstructor(NonConstReferenceArgumentConstructor &other);

    void NonConstReferenceArgumentMember(NonConstReferenceArgumentConstructor &other);
};

void f31()
{
    NonConstReferenceArgumentConstructor instance;

    NonConstReferenceArgumentConstructor copy(instance);
}

struct NonConstReferenceMemberInitialization
{
    NonConstReferenceMemberInitialization(int &foo)
        : foo(foo)
    {}

    int &foo;
};
