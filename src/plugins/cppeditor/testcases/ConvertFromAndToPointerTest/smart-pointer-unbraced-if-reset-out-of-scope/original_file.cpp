class Foo
{
public:
    void bar();
};

void func(bool c)
{
    std::unique_ptr<Foo> @f(new Foo);
    f->bar();
    if (c)
        f.reset();
}
