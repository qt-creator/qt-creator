class Foo
{
public:
    void bar();
};

void func()
{
    const Foo *@f = new Foo;
    f->bar();
}
