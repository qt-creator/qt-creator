class Foo
{
public:
    Foo(int, int) {}
    void bar();
};

void func()
{
    Foo *f = new Foo(1, 2);
    f->bar();
    delete f;
    f = new Foo(3, 4);
}
