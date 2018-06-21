void variableSingleReference()
{
    int foo;
}



int variableMultipleReferences()
{
    int foo = 0;
    return foo;
}



class Foo {};
void bla()
{
    Foo foo;
}



namespace N { class Bar {}; }
namespace N { class Baz {}; }
N::Bar bar;



namespace G { class App {}; }
using G::App;



class Hoo;
void f(const Hoo &);



class Moo {};
void x()
{
    new Moo;
}



class Element {};
template<typename T> struct Wrap { T member; };
void g()
{
    Wrap<Element> con;
    con.member;
}



template<typename T>
struct Wrapper {
    T f()
    {
        int foo;
        ++foo;
        return mem;
    }

    T mem;
};



template<typename T>
void f()
{
    T mem;
    mem.foo();
}



struct Woo {
    Woo();
    ~Woo();
};



int muu();
int muu(int);



struct Doo {
    int muu();
    int muu(int);
};



template<typename T> int tuu();
int tuu(int);



struct Xoo {
    template<typename T> int tuu();
    int tuu(int);
};



enum ET { E1 };
bool e(ET e)
{
    return e == E1;
}



struct LData { int member; };
void lambda(LData foo) {
    auto l = [bar=foo] { return bar.member; };
}



template<class T> class Coo;
template<class T> class Coo<T*>;
template<> class Coo<int>;



template<typename T> typename T::foo n()
{
    typename T::bla hello;
}



int rec(int n = 100)
{
    return n == 0 ? 0 : rec(--n);
}



#define FOO 3
int objectLikeMacro()
{
    return FOO;
}



#define BAR(x) x
int functionLikeMacro(int foo)
{
    return BAR(foo);
}
