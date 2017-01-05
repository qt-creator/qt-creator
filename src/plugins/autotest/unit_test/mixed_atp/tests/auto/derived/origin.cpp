#include "origin.h"

#include <QtTest>

Origin::Origin()
{
}

void Origin::testSchmu()
{
    QCOMPARE(1, 1);
}

void Origin::testStr()
{
    QFETCH(bool, localAware);

    QString str1 = QLatin1String("Hello World");
    QString str2 = QLatin1String("Hallo Welt");
    if (!localAware) {
        QBENCHMARK {
            str1 == str2;
        }
    } else {
        QBENCHMARK {
            str1.localeAwareCompare(str2) == 0;
        }
    }
}

void Origin::testStr_data()
{
    QTest::addColumn<bool>("localAware");
    QTest::newRow("simple") << false;
    QTest::newRow("localAware") << true;
}

void Origin::testBase1_data()
{
    QTest::addColumn<bool>("bogus");
    QTest::newRow("foo") << false;  // we don't expect 'foo' and 'bar' to be displayed
    QTest::newRow("bar") << true;
}
