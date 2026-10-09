class Foo
{
public:
    void bar();
};

std::unique_ptr<Foo> custom_make_unique();

void func()
{
    std::unique_ptr<Foo> @f = custom_make_unique();
    f->bar();
}
