// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QString>
#include <QtTest>
#include <QApplication>
#include <QLineEdit>

class GuiTest : public QObject
{
    Q_OBJECT

public:
    GuiTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testCase1();
    void testGui_data();
    void testGui();
};

GuiTest::GuiTest()
{
}

void GuiTest::initTestCase()
{
}

void GuiTest::cleanupTestCase()
{
}

void GuiTest::testCase1()
{
    QLatin1String str("Hello World");
    QLineEdit lineEdit;
    QTest::keyClicks(&lineEdit, str);
    QCOMPARE(lineEdit.text(), str);
}

void GuiTest::testGui()
{
    QFETCH(QTestEventList, events);
    QFETCH(QString, expected);
    QLineEdit lineEdit;
    events.simulate(&lineEdit);
    QCOMPARE(lineEdit.text(), expected);
}

void GuiTest::testGui_data()
{
    QTest::addColumn<QTestEventList>("events");
    QTest::addColumn<QString>("expected");

    QTestEventList list1;
    list1.addKeyClick('a');
    QTest::newRow("char") << list1 << "a";

    QTestEventList list2;
    list2.addKeyClick('a');
    list2.addKeyClick(Qt::Key_Backspace);
    QTest::newRow("there and back again") << list2 << "";
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    GuiTest gt;
    return QTest::qExec(&gt, argc, argv);
}

#include "tst_guitest.moc"
