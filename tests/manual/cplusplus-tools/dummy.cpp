#include "dummy.h"
#include "detail/header.h"

using namespace test;

extern int xi;

Dummy::Dummy()
{}

Dummy::Dummy(int)
{
    xi = 0;
    freefunc2(1.0);
}

void Dummy::bla(int)
{}

void Dummy::bla(const QString &)
{}

void Dummy::bla(const QString &) const
{}

void Dummy::bla(int, const QString &) const
{}

void Dummy::sfunc()
{}

const double Dummy::PI = 3.14;
