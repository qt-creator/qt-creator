class Foo
{
public:
    void bar();
};

Foo *makeFoo();

void func()
{
    std::unique_ptr<Foo> @f = makeFoo();
    f->bar();
}
