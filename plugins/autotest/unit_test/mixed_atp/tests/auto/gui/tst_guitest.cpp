/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/
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
