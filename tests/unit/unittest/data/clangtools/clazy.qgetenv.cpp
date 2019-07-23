// clazy-standalone -export-fixes=/tmp/clazy.qgetenv.yaml -checks=qgetenv clazy.qgetenv.cpp
#include <QByteArray>
#include <QtGlobal>

void g()
{
    qgetenv("Foo").isEmpty();
}
