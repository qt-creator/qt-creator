class Foo
{
public:
    void bar();
};

std::unique_ptr<Foo> getFoo();

void func()
{
    std::unique_ptr<Foo> @f(new Foo);
    f->bar();
    f = getFoo();
}
