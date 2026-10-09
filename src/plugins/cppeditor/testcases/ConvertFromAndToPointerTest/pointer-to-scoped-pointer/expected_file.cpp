#include <QScopedPointer>

class Foo
{
public:
    Foo(int, int) {}
    void bar();
};

void func()
{
    QScopedPointer<Foo> f(new Foo(1, 2));
    f->bar();
    f.reset();
}
