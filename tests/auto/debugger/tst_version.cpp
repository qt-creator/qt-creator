/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "gdb/gdbmi.h"

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
    int v = 0, bv = 0;
    bool mac = true;
    Debugger::Internal::extractGdbVersion(msg, &v, &bv, &mac);
    qDebug() << msg << " -> " << v << bv << mac;
    QCOMPARE(v, gdbVersion);
    QCOMPARE(bv, gdbBuildVersion);
    QCOMPARE(mac, isMacGdb);
}

void tst_Version::version_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("gdbVersion");
    QTest::addColumn<int>("gdbBuildVersion");
    QTest::addColumn<bool>("isMacGdb");

    QTest::newRow("Debian")
        << "GNU gdb (GDB) 7.0.1-debian"
        << 70001 << 0 << false;

    QTest::newRow("CVS 7.0.90")
        << "GNU gdb (GDB) 7.0.90.20100226-cvs"
        << 70090 << 20100226 << false;

    QTest::newRow("Ubuntu Lucid")
        << "GNU gdb (GDB) 7.1-ubuntu"
        << 70100 << 0 << false;

    QTest::newRow("Fedora 13")
        << "GNU gdb (GDB) Fedora (7.1-22.fc13)"
        << 70100 << 22 << false;

    QTest::newRow("Gentoo")
        << "GNU gdb (Gentoo 7.1 p1) 7.1"
        << 70100 << 1 << false;

    QTest::newRow("Fedora EL5")
        << "GNU gdb Fedora (6.8-37.el5)"
        << 60800 << 37 << false;

    QTest::newRow("SUSE")
        << "GNU gdb (GDB) SUSE (6.8.91.20090930-2.4)"
        << 60891 << 20090930 << false;

    QTest::newRow("Apple")
        << "GNU gdb 6.3.50-20050815 (Apple version gdb-1461.2)"
        << 60350 << 1461 << true;

    QTest::newRow("Apple")
        << "GNU gdb 6.3.50-20050815 (Apple version gdb-960)"
        << 60350 << 960 << true;
}


int main(int argc, char *argv[])
{
    tst_Version test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_version.moc"

