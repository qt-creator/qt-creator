#include <memory>

class Foo
{
public:
    void bar();
};

Foo *getOther();

void func()
{
    std::unique_ptr<Foo> f(new Foo);
    f->bar();
    f.reset();
    f.reset(getOther());
}
