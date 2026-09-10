class Foo
{
public:
    Foo(int, int) {}
    void bar();
};

void useRaw(Foo *);

void func()
{
    Foo *f = new Foo(1, 2);
    f->bar();
    useRaw(f);
    delete f;
    f = new Foo(3, 4);
    delete f;
    f = nullptr;
}
