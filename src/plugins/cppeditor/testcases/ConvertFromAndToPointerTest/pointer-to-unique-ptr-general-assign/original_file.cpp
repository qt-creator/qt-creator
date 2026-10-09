class Foo
{
public:
    void bar();
};

Foo *getOther();

void func()
{
    Foo *@f = new Foo;
    f->bar();
    delete f;
    f = getOther();
}
