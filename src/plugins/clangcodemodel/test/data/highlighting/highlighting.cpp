auto *Variable       = "Variable";
auto *u8Variable     = u8"Variable";
auto *rawVariable    = R"(Vari
)a"ble)";
auto Character       = 'c';

namespace std {
template<typename T> class vector { public: void clear(); };
template<typename T, typename U> class pair {};
}












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
typedef int IntegerTypedef;
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

void PointerToConstArgument(const int *argument);
void ConstPointerArgument(int *const argument);
void f26()
{
    int *x;
    PointerToConstArgument(x);
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

template<class T> class Coo;
template<class T> class Coo<T*>;

namespace N { void goo(); }
using N::goo;

#if 1
#endif

#include <new>

struct OtherOperator { void operator()(int); };
void g(OtherOperator o, int var)
{
    o(var);
}

void NonConstPointerArgument(int &argument);

struct PointerGetterClass
{
    int &getter();
};

void f32()
{
    PointerGetterClass x;

    NonConstPointerArgument(x.getter());
}

namespace N { template <typename T> void SizeIs(); }
using N::SizeIs;

void BaseClass::VirtualFunction() {}

class WithVirtualFunctionDefined {
  virtual void VirtualFunctionDefinition() {};
};

namespace NFoo { namespace NBar { namespace NTest { class NamespaceTypeSpelling; } } }

Undeclared u;
#define Q_PROPERTY(arg) static_assert("Q_PROPERTY", #arg); // Keep these in sync with wrappedQtHeaders/QtCore/qobjectdefs.h
#define SIGNAL(arg) #arg
#define SLOT(arg) #arg
class Property {
    Q_PROPERTY(const volatile unsigned long long * prop READ getProp WRITE setProp NOTIFY propChanged SCRIPTABLE true REVISION 1)
    Q_PROPERTY(const QString str READ getStr REVISION(1,0))
};

struct X {
    void operator*(int) {}
};

void operator*(X, float) {}

void CallSite() {
    X x;
    int y = 10;
    float z = 10;
    x * y;
    x * z;
}

struct Dummy {
    Dummy operator<<=(int key);
    Dummy operator()(int a);
    int& operator[ ] (unsigned index);
    void* operator new(unsigned size);
    void operator delete(void* ptr);
    void* operator new[](unsigned size);
    void operator delete[](void* ptr);
};

void TryOverloadedOperators(Dummy object)
{
    object <<= 3;

    Dummy stacked;
    stacked(4);
    stacked[1];
    int *i = new int;
    Dummy* use_new = new Dummy();
    delete use_new;
    Dummy* many = new Dummy[10];
    delete [] many;
}

enum {
    Test = 0
};

namespace {
class B {
    struct {
        int a;
    };
};
}

struct Dummy2 {
    Dummy2 operator()();
    int operator*();
    Dummy2 operator=(int foo);
};

void TryOverloadedOperators2(Dummy object)
{
    Dummy2 dummy2;
    dummy2();
    *dummy2;
    dummy2 = 3;
}

int OperatorTest() {
    return 1 < 2 ? 20 : 30;
}

int signalSlotTest() {
    SIGNAL(something(QString));
    SLOT(something(QString));
    SIGNAL(something(QString (*func1)(QString)));
    1 == 2;
}

class NonConstParameterConstructor
{
    NonConstParameterConstructor() = default;
    NonConstParameterConstructor(NonConstParameterConstructor &buildDependenciesStorage);

    void Call()
    {
        NonConstParameterConstructor foo;
        NonConstParameterConstructor bar(foo);
    }
};

class StaticMembersAccess
{
protected:
    static int protectedValue;

private:
    static int privateValue;
};

template <int i, int j> struct S { };
template <int i> using spec = S<i, 1>;
spec<2> s;

class Property2 {
    Q_PROPERTY(

            const

            volatile

            unsigned

            long

            long

            *

            prop

            READ

            getProp

            WRITE

            setProp

            NOTIFY

            propChanged

            )
};

void structuredBindingTest() {
    const int a[] = {1, 2};
    const auto [x, y] = a;
}

#define ASSIGN(decl, ptr) do { decl = *ptr; } while (false)
#define ASSIGN2 ASSIGN
void f4()
{
    int *thePointer = 0;
    ASSIGN(int i, thePointer);
    ASSIGN2(int i, thePointer);
}

const int MyConstant = 8;
void f5()
{
    int arr[MyConstant][8];
}

static int GlobalVar = 0;

namespace N { [[deprecated]] void f(); }

template<typename T>
void func(T v);

void f6()
{
    GlobalVar = 5;
    func(1);  // QTCREATORBUG-21856
}

template<typename T>
void func(T v) {
    GlobalVar = 5;
}

static std::vector<std::pair<int, int>> pv;

template <class T, long S>
struct vecn
{
    T v[S];
};

template <class T, long S>
static inline constexpr vecn<T, S> operator<(vecn<T, S> a, vecn<T, S> b)
{
    vecn<T, S> x = vecn<T, S>{};
    for(long i = 0; i < S; ++i)
    {
        x[i] = a[i] < b[i];
    }
    return x;
}

const char *cyrillic = "б";

struct foo {
#define blubb
};

template<typename T> void funcTemplate(T v);
#if 0
void whatever();
#else
template<> void funcTemplate<int>(int v) {}
#endif
template<> class Coo<int> {};

std::vector<std::vector<std::vector<int>>> pv2;

std::vector <std::vector  <std::vector<
std::vector<int
>>
 >
  >
vp3;

void staticMemberFuncTest() {
    struct S { static void staticFunc(int &); };
    S s;
    int i;
    s.staticFunc(i);
}

void lambdaArgTest()
{
    const auto foo1 = [](int &) {};
    const auto foo2 = [](int) {};
    int val;
    foo1(val);
    foo2(val);
    [](int &) {}(val);
    [](int) {}(val);
}

void assignmentTest() {
    struct S {} s;
    s = {};
}

using FooPtrVector = std::vector<Foo *>;
FooPtrVector returnTest()  {
    FooPtrVector foo;
    return foo;
}

template <typename Container, typename Func> inline void useContainer(const Container &, Func) {}
void testConstRefAutoLambdaArgs()
{
    useContainer(FooPtrVector(), [](const auto &arg) {});
}

#define USE_STRING(s) (s)
void useString()
{
    const char *s = USE_STRING("TEXT");
    s = USE_STRING_FROM_HEADER("TEXT");
}

void useOperator()
{
    struct S { S& operator++(); } s;
    ++s;
}

static void takesFoo(const Foo &);
void constMemberAsFunctionArg()
{
    struct S {
        S(const Foo& f) : constMember(f) {}
        void func() { takesFoo(constMember); }
        const Foo &constMember;
    };
}

# if 0
#define FOO

#endif

// comment

#if 0
#define BAR
# endif

void lambdaCall()
{
    const auto lambda1 = [] {};
    lambda1();
    auto lambda2 = [] {};
    lambda2();
}

void callOperators()
{
    struct Callable1 {
        void operator()() {};
    };
    Callable1 c1;
    c1();
    struct Callable2 {
        void operator()() const {};
    };
    Callable2 c2;
    c2();
}

namespace std { template<typename T> struct atomic { atomic(int) {} }; }

void constructorMemberInitialization()
{
    struct S {
        S(): m_m1(1), m_m2(0) {}

        std::atomic<int> m_m1;
        int m_m2;
    };
}

void keywords()
{
    bool b1 = true;
    bool b2 = false;
    void *p = nullptr;
}

namespace std {
struct Debug {};
Debug& operator<<(Debug &dbg, int) { return dbg; }
Debug& operator>>(Debug &dbg, int&) { return dbg; }
static Debug cout;
static Debug cin;
}
void outputOperator()
{
    std::cout << 0;
    int i;
    std::cin >> i;
}

template <typename To, typename From, typename Op>
void transform(const From &from, To &&to, Op op) {}
struct WithVector { std::vector<int> v; };
void inputsAndOutputsFromObject(const WithVector &s)
{
    std::vector<int> out;
    transform(s.v, out, [] {});
}

void builtinDefines()
{
    const auto f1 = __func__;
    const auto f2 = __FUNCTION__;
    const auto f3 = __PRETTY_FUNCTION__;
}

void derefOperator()
{
    struct S { bool operator*(); };
    struct S2 { S s; };
    S2 s;
    if (*s.s)
        return;
}

struct my_struct
{
    void* method(int dummy);
};

my_struct* get_my_struct();

struct my_struct2
{
    my_struct2(void* p);
};

void nestedCall()
{
    my_struct* s = get_my_struct();
    new my_struct2(s->method(0));
}

template<typename T>
class my_class
{
private:
    struct my_int
    {
        int n;
    };

    std::vector<my_int> vec;

public:
    void foo()
    {
        auto it = vec.begin(), end = vec.end();

        T* ptr = nullptr;
        ptr->bar();
    }
};

namespace std { template<typename T> struct optional { T* operator->(); }; }
struct structWithData { int value; };
struct structWithOptional { std::optional<structWithData> opt_my_struct1; };

void foo(structWithOptional & s)
{
    s.opt_my_struct1->value = 5;
}

class BaseWithMember
{
protected:
    std::vector<unsigned char> vec;
};

template<typename T> class Derived : public BaseWithMember
{
    void foo()
    {
        auto lambda = [&] {};
        lambda();
        vec.clear();
    }
};

static bool testVal(int val);
class BaseWithMember2
{
protected:
    int value;
};
template<typename T> class Derived2 : public BaseWithMember2
{
    bool foo()
    {
        if (testVal(value))
            return true;
        return false;
    }
};

struct StructWithMisleadingMemberNames {
    int operatormember;
    void operatorMethod();
};
void useStrangeStruct(StructWithMisleadingMemberNames *s) {
    s->operatormember = 5;
    s->operatorMethod();
}
