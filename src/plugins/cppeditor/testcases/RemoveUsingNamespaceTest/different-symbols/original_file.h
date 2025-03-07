namespace test{
struct foo{
    foo();
    void bar();
};
void func();
enum E {E1, E2};
int bar;
}
using namespace t@est;
foo::foo(){}
void foo::bar(){}
void test(){
    int i = bar * 4;
    E val = E1;
    auto p = &foo::bar;
    func()
}
