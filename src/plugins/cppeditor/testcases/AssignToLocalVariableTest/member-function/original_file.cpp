class Foo {public: int* fooFunc();}
void bar() {
    Foo *f = new Foo;
    @f->fooFunc();
}
