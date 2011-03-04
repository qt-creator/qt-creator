#include <qglobal.h>

struct Foo
{
    Foo()
      : num(new qint64)
    {}

    qint64 *num;
};

int main()
{
    Foo *f = new Foo;

    return 0;
}
