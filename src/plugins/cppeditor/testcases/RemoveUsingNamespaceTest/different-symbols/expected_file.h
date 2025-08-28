namespace test{
struct foo{
    foo();
    void bar();
};
void func();
enum E {E1, E2};
int bar;
}
test::foo::foo(){}
void test::foo::bar(){}
void test(){
    int i = test::bar * 4;
    test::E val = test::E1;
    auto p = &test::foo::bar;
    test::func()
}
