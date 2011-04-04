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
    new Foo;

    return 0;
}
