class Foo
{
public:
    void bar();
};

void func()
{
    Foo *f = new Foo();
    f->bar();
}
