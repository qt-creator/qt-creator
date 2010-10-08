#include "header.h"
#include "dummy.h"

#include <QtCore/QString>

int xi = 10;

int freefunc2(int a) { return a; }

int freefunc2(double)
{ return 1; }

int freefunc2(const QString &)
{ return 1; }

void here() {
    test::Dummy d;
    d;
}
