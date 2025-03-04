struct Foo {int* func();};
struct Baz {Foo* foo();};
void bar() {
    Baz *b = new Baz;
    auto localFunc = b->foo()->func();
}
