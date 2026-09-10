class Foo
{
public:
    void bar();
};

void func()
{
    std::unique_ptr<Foo> @f = std::make_unique<Foo>();
    f->bar();
}
