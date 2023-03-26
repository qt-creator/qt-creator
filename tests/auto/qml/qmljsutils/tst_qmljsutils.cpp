// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>
#include <QDebug>

#include <qmljs/qmljsutils.h>

class tst_QmlJSUtils: public QObject
{
    Q_OBJECT

private slots:
    void moduleVersionNumbers_data();
    void moduleVersionNumbers();
};

void tst_QmlJSUtils::moduleVersionNumbers_data()
{
    QTest::addColumn<QString>("version");
    QTest::addColumn<QStringList>("result");

    QTest::newRow("empty") << "" << QStringList();
    QTest::newRow("full") << "2.15" << QStringList{"2.15", "2"};
    QTest::newRow("single") << "2" << QStringList{"2"};
    // result if "import QtQuick 2":
    QTest::newRow("major") << "2.-1" << QStringList{"2.-1", "2"};
    QTest::newRow("broken") << "2.+3" << QStringList{"2.+3", "2.+"};
}

void tst_QmlJSUtils::moduleVersionNumbers()
{
    QFETCH(QString, version);
    QFETCH(QStringList, result);
    QCOMPARE(QmlJS::splitVersion(version), result);
}

QTEST_GUILESS_MAIN(tst_QmlJSUtils)

#include "tst_qmljsutils.moc"
