class Foo {public: void fooFunc();}
void bar() {
    Foo *f = new Foo;
    @f->fooFunc();
}
