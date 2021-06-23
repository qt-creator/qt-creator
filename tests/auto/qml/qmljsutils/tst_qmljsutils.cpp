/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
