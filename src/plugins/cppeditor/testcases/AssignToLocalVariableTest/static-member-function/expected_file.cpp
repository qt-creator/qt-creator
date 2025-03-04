class Foo {public: static int* fooFunc();}
void bar() {
    auto localFooFunc = Foo::fooFunc();
}
