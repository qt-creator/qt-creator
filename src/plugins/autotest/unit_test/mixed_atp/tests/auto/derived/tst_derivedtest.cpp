#include "origin.h"

#include <QtTest>

class BaseTest : public Origin
{
    Q_OBJECT
private slots:
    void testBase1();
};

void BaseTest::testBase1()
{
    QVERIFY(true);
}

class DerivedTest : public BaseTest
{
    Q_OBJECT
private Q_SLOTS:
    void testCase1();
    void testBase1();
};

void DerivedTest::testCase1()
{
    QVERIFY2(true, "Failure");
}

void DerivedTest::testBase1()
{
    QVERIFY(true);
}

QTEST_APPLESS_MAIN(DerivedTest)

#include "tst_derivedtest.moc"
