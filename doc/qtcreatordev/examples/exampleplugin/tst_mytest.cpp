#include "examplefunctions.h"

#include <QtTest>

class tst_MyTest : public QObject
{
    Q_OBJECT

private slots:
    void mytest();
};

void tst_MyTest::mytest()
{
    // a failing test
    QCOMPARE(Example::addOne(1), 2);
}

QTEST_GUILESS_MAIN(tst_MyTest)

#include "tst_mytest.moc"
