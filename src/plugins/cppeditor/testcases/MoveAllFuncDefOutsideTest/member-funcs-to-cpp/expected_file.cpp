#include "file.h"

Foo::Foo() = default;

Foo::~Foo() = default;

Foo &Foo::operator=(const Foo &) = default;

int Foo::numberA() const
{
    return 5;
}

int Foo::numberB() const
{
    return 5;
}
