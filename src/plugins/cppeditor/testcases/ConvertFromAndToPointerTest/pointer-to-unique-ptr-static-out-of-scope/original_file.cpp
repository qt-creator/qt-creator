class Foo
{
public:
    void bar();
};

void func()
{
    static Foo *@f = new Foo;
    f->bar();
}
