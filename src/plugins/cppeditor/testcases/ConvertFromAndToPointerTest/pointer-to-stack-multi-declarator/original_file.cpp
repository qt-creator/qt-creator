class Foo
{
public:
    void bar();
};

void func()
{
    Foo *@f = new Foo, *g = nullptr;
    f->bar();
    (void)g;
}
