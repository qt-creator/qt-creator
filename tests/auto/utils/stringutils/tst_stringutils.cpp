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

#include <utils/stringutils.h>

#include <QtTest>

//TESTED_COMPONENT=src/libs/utils

class TestMacroExpander : public Utils::AbstractMacroExpander
{
public:
    virtual bool resolveMacro(const QString &name, QString *ret, QSet<AbstractMacroExpander*> &seen)
    {
        // loop prevention
        const int count = seen.count();
        seen.insert(this);
        if (seen.count() == count)
            return false;

        if (name == QLatin1String("foo")) {
            *ret = QLatin1String("a");
            return true;
        }
        if (name == QLatin1String("a")) {
            *ret = QLatin1String("hi");
            return true;
        }
        if (name == QLatin1String("hi")) {
            *ret = QLatin1String("ho");
            return true;
        }
        if (name == QLatin1String("hihi")) {
            *ret = QLatin1String("bar");
            return true;
        }
        if (name == "slash") {
            *ret = "foo/bar";
            return true;
        }
        if (name == "sl/sh") {
            *ret = "slash";
            return true;
        }
        if (name == "JS:foo") {
            *ret = "bar";
            return true;
        }
        return false;
    }
};

class tst_StringUtils : public QObject
{
    Q_OBJECT

private slots:
    void testWithTildeHomePath();
    void testMacroExpander_data();
    void testMacroExpander();
    void testStripAccelerator();
    void testStripAccelerator_data();
    void testParseUsedPortFromNetstatOutput();
    void testParseUsedPortFromNetstatOutput_data();

private:
    TestMacroExpander mx;
};

void tst_StringUtils::testWithTildeHomePath()
{
#ifndef Q_OS_WIN
    // home path itself
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath()), QString::fromLatin1("~"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QLatin1Char('/')),
             QString::fromLatin1("~"));
    QCOMPARE(Utils::withTildeHomePath(QString::fromLatin1("/unclean/..") + QDir::homePath()),
             QString::fromLatin1("~"));
    // sub of home path
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/foo")),
             QString::fromLatin1("~/foo"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/foo/")),
             QString::fromLatin1("~/foo"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/some/path/file.txt")),
             QString::fromLatin1("~/some/path/file.txt"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/some/unclean/../path/file.txt")),
             QString::fromLatin1("~/some/path/file.txt"));
    // not sub of home path
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/../foo")),
             QString(QDir::homePath() + QString::fromLatin1("/../foo")));
#else
    // windows: should return same as input
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath()), QDir::homePath());
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/foo")),
             QDir::homePath() + QString::fromLatin1("/foo"));
    QCOMPARE(Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/../foo")),
             Utils::withTildeHomePath(QDir::homePath() + QString::fromLatin1("/../foo")));
#endif
}

void tst_StringUtils::testMacroExpander_data()

{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");

    static const struct {
        const char * const in;
        const char * const out;
    } vals[] = {
        {"text", "text"},
        {"%{a}", "hi"},
        {"%%{a}", "%hi"},
        {"%%%{a}", "%%hi"},
        {"%{b}", "%{b}"},
        {"pre%{a}", "prehi"},
        {"%{a}post", "hipost"},
        {"pre%{a}post", "prehipost"},
        {"%{a}%{a}", "hihi"},
        {"%{a}text%{a}", "hitexthi"},
        {"%{foo}%{a}text%{a}", "ahitexthi"},
        {"%{}{a}", "%{a}"},
        {"%{}", "%"},
        {"test%{}", "test%"},
        {"%{}test", "%test"},
        {"%{abc", "%{abc"},
        {"%{%{a}", "%{hi"},
        {"%{%{a}}", "ho"},
        {"%{%{a}}}post", "ho}post"},
        {"%{hi%{a}}", "bar"},
        {"%{hi%{%{foo}}}", "bar"},
        {"%{hihi/b/c}", "car"},
        {"%{hihi/a/}", "br"}, // empty replacement
        {"%{hihi/b}", "bar"}, // incomplete substitution
        {"%{hihi/./c}", "car"},
        {"%{hihi//./c}", "ccc"},
        {"%{hihi/(.)(.)r/\\2\\1c}", "abc"}, // no escape for capture groups
        {"%{hihi/b/c/d}", "c/dar"},
        {"%{hihi/a/e{\\}e}", "be{}er"}, // escape closing brace
        {"%{slash/o\\/b/ol's c}", "fool's car"},
        {"%{sl\\/sh/(.)(a)(.)/\\2\\1\\3as}", "salsash"}, // escape in variable name
        {"%{JS:foo/b/c}", "%{JS:foo/b/c}"}, // No replacement for JS (all considered varName)
        {"%{%{a}%{a}/b/c}", "car"},
        {"%{nonsense:-sense}", "sense"},
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++)
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << QString::fromLatin1(vals[i].out);
}

void tst_StringUtils::testMacroExpander()
{
    QFETCH(QString, in);
    QFETCH(QString, out);

    Utils::expandMacros(&in, &mx);
    QCOMPARE(in, out);
}

void tst_StringUtils::testStripAccelerator()
{
    QFETCH(QString, expected);

    QCOMPARE(Utils::stripAccelerator(QTest::currentDataTag()), expected);
}

void tst_StringUtils::testStripAccelerator_data()
{
    QTest::addColumn<QString>("expected");

    QTest::newRow("Test") << "Test";
    QTest::newRow("&Test") << "Test";
    QTest::newRow("&&Test") << "&Test";
    QTest::newRow("T&est") << "Test";
    QTest::newRow("&Te&&st") << "Te&st";
    QTest::newRow("T&e&st") << "Test";
    QTest::newRow("T&&est") << "T&est";
    QTest::newRow("T&&e&st") << "T&est";
    QTest::newRow("T&&&est") << "T&est";
    QTest::newRow("Tes&t") << "Test";
    QTest::newRow("Test&") << "Test";
}

void tst_StringUtils::testParseUsedPortFromNetstatOutput()
{
    QFETCH(QString, line);
    QFETCH(int, port);

    QCOMPARE(Utils::parseUsedPortFromNetstatOutput(line.toUtf8()), port);
}

void tst_StringUtils::testParseUsedPortFromNetstatOutput_data()
{
    QTest::addColumn<QString>("line");
    QTest::addColumn<int>("port");

    QTest::newRow("Empty") << "" << -1;

    // Windows netstat.
    QTest::newRow("Win1") << "Active Connection" <<  -1;
    QTest::newRow("Win2") << "   Proto  Local Address          Foreign Address        State"       <<       -1;
    QTest::newRow("Win3") << "   TCP    0.0.0.0:80             0.0.0.0:0              LISTENING"   <<       80;
    QTest::newRow("Win4") << "   TCP    0.0.0.0:113            0.0.0.0:0              LISTENING"   <<      113;
    QTest::newRow("Win5") << "   TCP    10.9.78.4:14714       0.0.0.0:0              LISTENING"    <<    14714;
    QTest::newRow("Win6") << "   TCP    10.9.78.4:50233       12.13.135.180:993      ESTABLISHED"  <<    50233;
    QTest::newRow("Win7") << "   TCP    [::]:445               [::]:0                 LISTENING"   <<      445;
    QTest::newRow("Win8") << " TCP    192.168.0.80:51905     169.55.74.50:443       ESTABLISHED"   <<    51905;
    QTest::newRow("Win9") << "  UDP    [fe80::840a:2942:8def:abcd%6]:1900  *:*   "                 <<     1900;

    // Linux
    QTest::newRow("Linux1") << "sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt ..." <<     -1;
    QTest::newRow("Linux2") << "0: 00000000:2805 00000000:0000 0A 00000000:00000000 00:00000000 00000000  ..." <<  10245;

    // Mac
    QTest::newRow("Mac1") << "Active Internet connections (including servers)"                                  <<    -1;
    QTest::newRow("Mac2") << "Proto Recv-Q Send-Q  Local Address          Foreign Address        (state)"       <<    -1;
    QTest::newRow("Mac3") << "tcp4       0      0  192.168.1.12.55687     88.198.14.66.443       ESTABLISHED"   << 55687;
    QTest::newRow("Mac4") << "tcp6       0      0  2a01:e34:ee42:d0.55684 2a02:26f0:ff::5c.443   ESTABLISHED"   << 55684;
    QTest::newRow("Mac5") << "tcp4       0      0  *.631                  *.*                    LISTEN"        <<   631;
    QTest::newRow("Mac6") << "tcp6       0      0  *.631                  *.*                    LISTEN"        <<   631;
    QTest::newRow("Mac7") << "udp4       0      0  192.168.79.1.123       *.*"                                  <<   123;
    QTest::newRow("Mac9") << "udp4       0      0  192.168.8.1.123        *.*"                                  <<   123;

    // QNX
    QTest::newRow("Qnx1") << "Active Internet connections (including servers)"                                  <<    -1;
    QTest::newRow("Qnx2") << "Proto Recv-Q Send-Q  Local Address          Foreign Address        State   "      <<    -1;
    QTest::newRow("Qnx3") << "tcp        0      0  10.9.7.5.22          10.9.7.4.46592       ESTABLISHED"       <<    22;
    QTest::newRow("Qnx4") << "tcp        0      0  *.8000                 *.*                    LISTEN     "   <<  8000;
    QTest::newRow("Qnx5") << "tcp        0      0  *.22                   *.*                    LISTEN     "   <<    22;
    QTest::newRow("Qnx6") << "udp        0      0  *.*                    *.*                               "   <<    -1;
    QTest::newRow("Qnx7") << "udp        0      0  *.*                    *.*                               "   <<    -1;
    QTest::newRow("Qnx8") << "Active Internet6 connections (including servers)"                                 <<    -1;
    QTest::newRow("Qnx9") << "Proto Recv-Q Send-Q  Local Address          Foreign Address        (state)    "   <<    -1;
    QTest::newRow("QnxA") << "tcp6       0      0  *.22                   *.*                    LISTEN   "     <<    22;
}

QTEST_MAIN(tst_StringUtils)

#include "tst_stringutils.moc"
