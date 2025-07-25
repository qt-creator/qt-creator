// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerprotocol.h"

#include <QTest>

//TESTED_COMPONENT=src/plugins/debugger/gdb

class tst_gdb : public QObject
{
    Q_OBJECT

public:
    tst_gdb() {}

private slots:
    void version();
    void version_data();

    void niceType();
    void niceType_data();
};

void tst_gdb::version()
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
    //qDebug() << msg << " -> " << v << bv << mac << qnx;
    QCOMPARE(v, gdbVersion);
    QCOMPARE(bv, gdbBuildVersion);
    QCOMPARE(mac, isMacGdb);
    QCOMPARE(qnx, isQnxGdb);
}

void tst_gdb::version_data()
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

    QTest::newRow("SLES")
        << "GNU gdb (GDB; SUSE Linux Enterprise 10) 7.9.1"
        << 70901 << 0 << false << false;

    QTest::newRow("Apple")
        << "GNU gdb 6.3.50-20050815 (Apple version gdb-1461.2)"
        << 60350 << 1461 << true << false;

    QTest::newRow("Apple")
        << "GNU gdb 6.3.50-20050815 (Apple version gdb-960)"
        << 60350 << 960 << true << false;

    QTest::newRow("QNX")
        << "GNU gdb (GDB) 7.3 qnx (rev. 613)"
        << 70300 << 613 << false << true;

    QTest::newRow("QNX71")
        << "GNU gdb (GDB) 8.2.1 [qnx710 r1469] (STABLE)"
        << 80201 << 710 << false << true;

    QTest::newRow("rubenvb")
        << "GNU gdb (rubenvb-4.7.2-release) 7.5.50.20120920-cvs"
        << 70550 << 20120920 << false << false;

    QTest::newRow("openSUSE 13.1")
        << "GNU gdb (GDB; openSUSE 13.1) 7.6.50.20130731-cvs"
        << 70650 << 20130731 << false << false;

    QTest::newRow("openSUSE 13.2")
        << "GNU gdb (GDB; openSUSE 13.2) 7.8"
        << 70800 << 0 << false << false;

    QTest::newRow("Fedora 26")
        << "GNU gdb (GDB) Fedora 8.0-13.fc26"
        << 80000 << 13 << false << false;

    QTest::newRow("Debian 7.12 git")
        << "GNU gdb (Debian 7.12-6) 7.12.0.20161007-git"
        << 71200 << 6 << false << false;

    QTest::newRow("Debian 10.1 git")
        << "GNU gdb (Debian 10.1-1.7) 10.1.90.20210103-git"
        << 100100 << 1 << false << false;

    QTest::newRow("Ubuntu 22.04")
        << "GNU gdb (Ubuntu 12.1-0ubuntu1~22.04) 12.1"
        << 120100 << 0 << false << false;
}

static QString chopConst(QString type)
{
   while (1) {
        if (type.startsWith("const"))
            type = type.mid(5);
        else if (type.startsWith(' '))
            type = type.mid(1);
        else if (type.endsWith("const"))
            type.chop(5);
        else if (type.endsWith(' '))
            type.chop(1);
        else
            break;
    }
    return type;
}

QString niceType(QString type)
{
    type.replace('*', '@');

    for (int i = 0; i < 10; ++i) {
        int start = type.indexOf("std::allocator<");
        if (start == -1)
            break;
        // search for matching '>'
        int pos;
        int level = 0;
        for (pos = start + 12; pos < type.size(); ++pos) {
            int c = type.at(pos).unicode();
            if (c == '<') {
                ++level;
            } else if (c == '>') {
                --level;
                if (level == 0)
                    break;
            }
        }
        QString alloc = type.mid(start, pos + 1 - start).trimmed();
        QString inner = alloc.mid(15, alloc.size() - 16).trimmed();
        //qDebug() << "MATCH: " << pos << alloc << inner;

        if (inner == QLatin1String("char"))
            // std::string
            type.replace(QLatin1String("basic_string<char, std::char_traits<char>, "
                "std::allocator<char> >"), QLatin1String("string"));
        else if (inner == QLatin1String("wchar_t"))
            // std::wstring
            type.replace(QLatin1String("basic_string<wchar_t, std::char_traits<wchar_t>, "
                "std::allocator<wchar_t> >"), QLatin1String("wstring"));

        // std::vector, std::deque, std::list
        const QRegularExpression re1(QString("(vector|list|deque)<%1, %2\\s*?>").arg(inner, alloc));
        QRegularExpressionMatch match = re1.match(type);
        if (match.hasMatch())
            type.replace(match.captured(), QString("%1<%2>").arg(match.captured(1), inner));


        // std::stack
        const QRegularExpression re6(QString("stack<%1, std::deque<%2> >").arg(inner, inner));
         match = re6.match(type);
        if (match.hasMatch())
            type.replace(match.captured(), QString("stack<%1>").arg(inner));

        // std::set
        const QRegularExpression re4(QString("set<%1, std::less<%2>, %3\\s*?>").arg(inner, inner, alloc));
        match = re4.match(type);
        if (match.hasMatch())
            type.replace(match.captured(), QString("set<%1>").arg(inner));


        // std::map
        if (inner.startsWith("std::pair<")) {
            // search for outermost ','
            int pos;
            int level = 0;
            for (pos = 10; pos < inner.size(); ++pos) {
                int c = inner.at(pos).unicode();
                if (c == '<')
                    ++level;
                else if (c == '>')
                    --level;
                else if (c == ',' && level == 0)
                    break;
            }
            QString ckey = inner.mid(10, pos - 10);
            QString key = chopConst(ckey);
            QString value = inner.mid(pos + 2, inner.size() - 3 - pos);

            const QRegularExpression re5(QString("map<%1, %2, std::less<%3>, %4\\s*?>")
                .arg(key, value, key, alloc));
            match = re5.match(type);
            if (match.hasMatch()) {
                type.replace(match.captured(), QString("map<%1, %2>").arg(key, value));
            } else {
                const QRegularExpression re7(QString("map<const %1, %2, std::less<const %3>, %4\\s*?>")
                    .arg(key, value, key, alloc));
                match = re7.match(type);
                if (match.hasMatch())
                    type.replace(match.captured(), QString("map<const %1, %2>").arg(key, value));
            }
        }
    }
    type.replace('@', '*');
    type.replace(QLatin1String(" >"), QString(QLatin1Char('>')));
    return type;
}

void tst_gdb::niceType()
{
    // cf. watchutils.cpp
    QFETCH(QString, input);
    QFETCH(QString, simplified);
    QCOMPARE(::niceType(input), simplified);
}

void tst_gdb::niceType_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("simplified");

    QTest::newRow("list")
        << "std::list<int, std::allocator<int> >"
        << "std::list<int>";

    QTest::newRow("combined")
        << "std::vector<std::list<int, std::allocator<int> >*, "
           "std::allocator<std::list<int, std::allocator<int> >*> >"
        << "std::vector<std::list<int>*>";

    QTest::newRow("stack")
        << "std::stack<int, std::deque<int, std::allocator<int> > >"
        << "std::stack<int>";

    QTest::newRow("map")
        << "std::map<myns::QString, Foo, std::less<myns::QString>, "
           "std::allocator<std::pair<const myns::QString, Foo> > >"
        << "std::map<myns::QString, Foo>";

    QTest::newRow("map2")
        << "std::map<const char*, Foo, std::less<const char*>, "
           "std::allocator<std::pair<const char* const, Foo> > >"
        << "std::map<const char*, Foo>";
}

QTEST_APPLESS_MAIN(tst_gdb);

#include "tst_gdb.moc"

