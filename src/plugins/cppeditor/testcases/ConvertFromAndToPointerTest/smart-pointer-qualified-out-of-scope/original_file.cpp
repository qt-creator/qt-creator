class Foo
{
public:
    void bar();
};

void func()
{
    const std::unique_ptr<Foo> @f(new Foo);
    f->bar();
}
