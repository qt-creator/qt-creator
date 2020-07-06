void f(int foo, const int *cfoo)
{
    foo++;
    cfoo++;
}



struct Foo { int member = 0; };
int g(const Foo &foo)
{
    return foo.member;
    const Foo bar;
    bar;
}

struct Bar { virtual ~Bar(); int mem(){} virtual int virtualConstMem() const; };
void h(const Foo &foo, Bar &bar)
{
    g(foo);
    bar.mem();
    bar.virtualConstMem();
}


template <typename T>
void t(int foo) { (void)foo; }
void c()
{
    t<Foo>(3);
}



/**
 * \brief This is a crazy function.
 */
void documentedFunction();
void d()
{
    documentedFunction();
}



enum EnumType { V1, V2, Custom = V2 + 5 };
EnumType e()
{
    return EnumType::Custom;
}



template <typename T> struct Baz { T member; };
void t2(const Baz<int> &b) {
    Baz<int> baz; baz = b;
}

#include "tooltipinfo.h"



#define MACRO_FROM_MAINFILE(x) x + 3
void foo()
{
    MACRO_FROM_MAINFILE(7);
    MACRO_FROM_HEADER(7);
}



namespace N { struct Muu{}; }
namespace G = N;
void o()
{
    using namespace N;
    Muu muu; (void)muu;
}
void n()
{
    using namespace G;
    Muu muu; (void)muu;
}
void q()
{
    using N::Muu;
    Muu muu; (void)muu;
}



struct Sizes
{
    char memberChar1;
    char memberChar2;
};
enum class FancyEnumType { V1, V2 };
union Union
{
    char memberChar1;
    char memberChar2;
};



namespace X {
namespace Y {
}
}



template<typename T> struct Ptr {};
struct Nuu {};

typedef Ptr<Nuu> PtrFromTypeDef;
using PtrFromTypeAlias = Ptr<Nuu>;
template<typename T> using PtrFromTemplateTypeAlias = Ptr<T>;

void y()
{
    PtrFromTypeDef b; (void)b;
    PtrFromTypeAlias a; (void)a;
    PtrFromTemplateTypeAlias<Nuu> c; (void)c;
}



template <typename T> struct Zii {};
namespace U { template <typename T> struct Yii {}; }
void mc()
{
    using namespace U;
    Zii<int> zii; (void) zii;
    Yii<int> yii; (void) yii;
}



namespace A { struct X {}; }
namespace B = A;
void ab()
{
    B::X x; (void)x;
}



namespace N {
struct Outer
{
    template <typename T> struct Inner {};
    Inner<int> inner;
};
}



void f();
namespace R { void f(); }
void f(int param);
void z(int = 1);
void user()
{
    f();
    R::f();
    f(1);
    z();
}




void autoTypes()
{
    auto a = 3; (void)a;
    auto b = EnumType::V1; (void)b;
    auto c = Bar(); (void)c;
    auto d = Zii<int>(); (void)d;
}




struct Con {};
struct ExplicitCon {
    ExplicitCon() = default;
    ExplicitCon(int m) :member(m) {}
    int member;
};
void constructor()
{
    Con();
    ExplicitCon();
    ExplicitCon(2);
}

Nuu **pointers(Nuu **p1)
{
    return p1;
}

static constexpr int calcValue() { return 1 + 2; }
const auto val = calcValue() + sizeof(char);

const int zero = 0;

static void func()
{
    const int i = 5;
    const int j = i;
}
