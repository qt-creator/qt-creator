/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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
