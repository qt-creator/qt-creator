#include "foo.h"

Foo::Bar Foo::bar() const
{
    return m_bar;
}

void Foo::setBar(Bar newBar)
{
    m_bar = newBar;
}
