class Foo
{
public:
    void bar();
};

void func()
{
    Foo local;
    Foo *@f = &local + 1;
    f->bar();
}
