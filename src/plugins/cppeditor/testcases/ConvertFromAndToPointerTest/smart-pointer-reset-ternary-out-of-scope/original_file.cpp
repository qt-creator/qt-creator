class Foo
{
public:
    void bar();
};

void g();

void func(bool c)
{
    std::unique_ptr<Foo> @f(new Foo);
    f->bar();
    c ? f.reset() : g();
}
