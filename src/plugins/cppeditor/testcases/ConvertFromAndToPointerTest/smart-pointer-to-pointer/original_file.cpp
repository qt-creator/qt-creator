class Foo
{
public:
    Foo(int, int) {}
    void bar();
};

void useRaw(Foo *);

void func()
{
    std::unique_ptr<Foo> @f(new Foo(1, 2));
    f->bar();
    useRaw(f.get());
    f.reset(new Foo(3, 4));
    f.reset();
}
