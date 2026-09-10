class Foo
{
public:
    void bar();
};

void func()
{
    Foo local;
    Foo *@f = new Foo;
    f->bar();
    f = &local + 1;
}
