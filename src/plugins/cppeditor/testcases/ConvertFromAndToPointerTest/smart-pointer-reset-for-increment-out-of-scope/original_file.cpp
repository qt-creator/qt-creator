class Foo
{
public:
    void bar();
};

void func()
{
    std::unique_ptr<Foo> @f(new Foo);
    f->bar();
    for (int i = 0; i < 10; f.reset()) {
    }
}
