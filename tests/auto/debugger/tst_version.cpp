/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggerprotocol.h"

#include <QtTest>

//TESTED_COMPONENT=src/plugins/debugger/gdb

class tst_Version : public QObject
{
    Q_OBJECT

public:
    tst_Version() {}

private slots:
    void version();
    void version_data();
};

void tst_Version::version()
{
    QFETCH(QString, msg);
    QFETCH(int, gdbVersion);
    QFETCH(int, gdbBuildVersion);
    QFETCH(bool, isMacGdb);
    QFETCH(bool, isQnxGdb);
    int v = 0, bv = 0;
    bool mac = true;
    bool qnx = true;
    Debugger::Internal::extractGdbVersion(msg, &v, &bv, &mac, &qnx);
    qDebug() << msg << " -> " << v << bv << mac << qnx;
    QCOMPARE(v, gdbVersion);
    QCOMPARE(bv, gdbBuildVersion);
    QCOMPARE(mac, isMacGdb);
    QCOMPARE(qnx, isQnxGdb);
}

void tst_Version::version_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("gdbVersion");
    QTest::addColumn<int>("gdbBuildVersion");
    QTest::addColumn<bool>("isMacGdb");
    QTest::addColumn<bool>("isQnxGdb");

    QTest::newRow("Debian")
        << "GNU gdb (GDB) 7.0.1-debian"
        << 70001 << 0 << false << false;

    QTest::newRow("CVS 7.0.90")
        << "GNU gdb (GDB) 7.0.90.20100226-cvs"
        << 70090 << 20100226 << false << false;

    QTest::newRow("Ubuntu Lucid")
        << "GNU gdb (GDB) 7.1-ubuntu"
        << 70100 << 0 << false << false;

    QTest::newRow("Fedora 13")
        << "GNU gdb (GDB) Fedora (7.1-22.fc13)"
        << 70100 << 22 << false << false;

    QTest::newRow("Gentoo")
        << "GNU gdb (Gentoo 7.1 p1) 7.1"
        << 70100 << 1 << false << false;

    QTest::newRow("Fedora EL5")
        << "GNU gdb Fedora (6.8-37.el5)"
        << 60800 << 37 << false << false;

    QTest::newRow("SUSE")
        << "GNU gdb (GDB) SUSE (6.8.91.20090930-2.4)"
        << 60891 << 20090930 << false << false;

    QTest::newRow("Apple")
        << "GNU gdb 6.3.50-20050815 (Apple version gdb-1461.2)"
        << 60350 << 1461 << true << false;

    QTest::newRow("Apple")
        << "GNU gdb 6.3.50-20050815 (Apple version gdb-960)"
        << 60350 << 960 << true << false;

    QTest::newRow("QNX")
        << "GNU gdb (GDB) 7.3 qnx (rev. 613)"
        << 70300 << 613 << false << true;
}


int main(int argc, char *argv[])
{
    tst_Version test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_version.moc"

