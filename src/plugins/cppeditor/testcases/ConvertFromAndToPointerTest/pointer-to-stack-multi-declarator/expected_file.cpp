class Foo
{
public:
    void bar();
};

void func()
{
    Foo f, *g = nullptr;
    f.bar();
    (void)g;
}
