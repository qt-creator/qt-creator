#include <memory>

class Foo
{
public:
    Foo(int, int) {}
    void bar();
};

void func()
{
    std::unique_ptr<Foo> f(new Foo(1, 2));
    f->bar();
    f.reset();
}
