// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/filepath.h>
#include <utils/stringutils.h>

#include <QtTest>

//TESTED_COMPONENT=src/libs/utils

using namespace Utils;

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
        if (name == "JS:with } inside") {
            *ret = "yay";
            return true;
        }
        if (name == "JS:literal%{") {
            *ret = "hurray";
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
    void testStripAccelerator_data();
    void testStripAccelerator();
    void testParseUsedPortFromNetstatOutput_data();
    void testParseUsedPortFromNetstatOutput();
    void testJoinStrings_data();
    void testJoinStrings();
    void testTrim_data();
    void testTrim();
    void testWildcardToRegularExpression_data();
    void testWildcardToRegularExpression();
    void testSplitAtFirst_data();
    void testSplitAtFirst();

private:
    TestMacroExpander mx;
};

void tst_StringUtils::testWithTildeHomePath()
{
    const QString home = QDir::homePath();
    const FilePath homePath = FilePath::fromString(home);

#ifndef Q_OS_WIN
    // home path itself
    QCOMPARE(homePath.withTildeHomePath(), QString("~"));
    QCOMPARE(homePath.pathAppended("/").withTildeHomePath(), QString("~"));
    QCOMPARE(FilePath::fromString("/unclean/../" + home).withTildeHomePath(), QString("~"));
    // sub of home path
    QCOMPARE(homePath.pathAppended("/foo").withTildeHomePath(), QString("~/foo"));
    QCOMPARE(homePath.pathAppended("/foo/").withTildeHomePath(), QString("~/foo"));
    QCOMPARE(homePath.pathAppended("/some/path/file.txt").withTildeHomePath(),
             QString("~/some/path/file.txt"));
    QCOMPARE(homePath.pathAppended("/some/unclean/../path/file.txt").withTildeHomePath(),
             QString("~/some/path/file.txt"));
    // not sub of home path
    QCOMPARE(homePath.pathAppended("/../foo").withTildeHomePath(),
             QString(home + "/../foo"));
#else
    // windows: should return same as input
    QCOMPARE(homePath.withTildeHomePath(), home);
    QCOMPARE(homePath.pathAppended("/foo").withTildeHomePath(), home + QString("/foo"));
    QCOMPARE(homePath.pathAppended("/../foo").withTildeHomePath(),
             homePath.pathAppended("/../foo").withTildeHomePath());
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
        {"%{hihi/a/e{\\}e}", "be{}er"},   // escape closing brace
        {"%{JS:with \\} inside}", "yay"}, // escape closing brace also in JS:
        {"%{JS:literal%\\{}", "hurray"},
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

void tst_StringUtils::testStripAccelerator()
{
    QFETCH(QString, expected);

    QCOMPARE(Utils::stripAccelerator(QString::fromUtf8(QTest::currentDataTag())), expected);
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

void tst_StringUtils::testParseUsedPortFromNetstatOutput()
{
    QFETCH(QString, line);
    QFETCH(int, port);

    QCOMPARE(Utils::parseUsedPortFromNetstatOutput(line.toUtf8()), port);
}

struct JoinData
{
    QStringList input;
    QString output = {};
    QChar separator = '\n';
};

void tst_StringUtils::testJoinStrings_data()
{
    QTest::addColumn<JoinData>("data");

    QTest::newRow("0") << JoinData{};

    QTest::newRow("1") << JoinData{{"one"}, "one"};
    QTest::newRow("1_Null") << JoinData{{{}}};
    QTest::newRow("1_Empty") << JoinData{{""}};

    QTest::newRow("2") << JoinData{{"first", "second"}, "first\nsecond"};
    QTest::newRow("2_Null") << JoinData{{{}, {}}};
    QTest::newRow("2_Empty") << JoinData{{"", ""}};
    QTest::newRow("2_1stNull") << JoinData{{{}, "second"}, "second"};
    QTest::newRow("2_1stEmpty") << JoinData{{"", "second"}, "second"};
    QTest::newRow("2_2ndNull") << JoinData{{"first", {}}, "first"};
    QTest::newRow("2_2ndEmpty") << JoinData{{"first", ""}, "first"};

    QTest::newRow("3") << JoinData{{"first", "second", "third"}, "first\nsecond\nthird"};
    QTest::newRow("3_Null") << JoinData{{{}, {}, {}}};
    QTest::newRow("3_Empty") << JoinData{{"", "", ""}};
    QTest::newRow("3_1stNull") << JoinData{{{}, "second", "third"}, "second\nthird"};
    QTest::newRow("3_1stEmpty") << JoinData{{"", "second", "third"}, "second\nthird"};
    QTest::newRow("3_2ndNull") << JoinData{{"first", {}, "third"}, "first\nthird"};
    QTest::newRow("3_2ndEmpty") << JoinData{{"first", "", "third"}, "first\nthird"};
    QTest::newRow("3_3rdNull") << JoinData{{"first", "second", {}}, "first\nsecond"};
    QTest::newRow("3_3rdEmpty") << JoinData{{"first", "second", ""}, "first\nsecond"};
    QTest::newRow("3_1stNonNull") << JoinData{{"first", {}, {}}, "first"};
    QTest::newRow("3_1stNonEmpty") << JoinData{{"first", "", ""}, "first"};
    QTest::newRow("3_2ndNonNull") << JoinData{{{}, "second", {}}, "second"};
    QTest::newRow("3_2ndNonEmpty") << JoinData{{"", "second", ""}, "second"};
    QTest::newRow("3_2ndNonNull") << JoinData{{{}, {}, "third"}, "third"};
    QTest::newRow("3_3ndNonEmpty") << JoinData{{"", "", "third"}, "third"};

    QTest::newRow("DotSeparator") << JoinData{{"first", "second"}, "first.second", '.'};
}

void tst_StringUtils::testJoinStrings()
{
    QFETCH(JoinData, data);

    QCOMPARE(Utils::joinStrings(data.input, data.separator), data.output);
}

struct TrimData
{
    QString input;
    QString front = {};
    QString back = {};
    QString bothSides = {};
    QChar ch = ' ';
};

void tst_StringUtils::testTrim_data()
{
    QTest::addColumn<TrimData>("data");

    QTest::newRow("Empty") << TrimData{};
    QTest::newRow("AllToRemove") << TrimData{"   "};
    QTest::newRow("BothSides") << TrimData{" foo ", "foo ", " foo", "foo"};
    QTest::newRow("BothSidesLong") << TrimData{"  foo  ", "foo  ", "  foo", "foo"};
    QTest::newRow("CharInside") << TrimData{"  foo bar  ", "foo bar  ", "  foo bar", "foo bar"};
}

void tst_StringUtils::testTrim()
{
    QFETCH(TrimData, data);

    QCOMPARE(Utils::trimFront(data.input, data.ch), data.front);
    QCOMPARE(Utils::trimBack(data.input, data.ch), data.back);
    QCOMPARE(Utils::trim(data.input, data.ch), data.bothSides);
}

void tst_StringUtils::testWildcardToRegularExpression_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("string");
    QTest::addColumn<bool>("matches");
    auto addRow = [](const char *pattern, const char *string, bool matchesNonPathGlob) {
        QTest::addRow("%s@%s", pattern, string) << pattern << string << matchesNonPathGlob;
    };
    addRow("*.html", "test.html", true);
    addRow("*.html", "test.htm", false);
    addRow("*bar*", "foobarbaz", true);
    addRow("*", "Qt Rocks!", true);
    addRow("*.h", "test.cpp", false);
    addRow("*.???l", "test.html", true);
    addRow("*?", "test.html", true);
    addRow("*?ml", "test.html", true);
    addRow("*[*]", "test.html", false);
    addRow("*[?]", "test.html", false);
    addRow("*[?]ml", "test.h?ml", true);
    addRow("*[[]ml", "test.h[ml", true);
    addRow("*[]]ml", "test.h]ml", true);
    addRow("*.h[a-z]ml", "test.html", true);
    addRow("*.h[A-Z]ml", "test.html", false);
    addRow("*.h[A-Z]ml", "test.hTml", true);
    addRow("*.h[!A-Z]ml", "test.hTml", false);
    addRow("*.h[!A-Z]ml", "test.html", true);
    addRow("*.h[!T]ml", "test.hTml", false);
    addRow("*.h[!T]ml", "test.html", true);
    addRow("*.h[!T]m[!L]", "test.htmL", false);
    addRow("*.h[!T]m[!L]", "test.html", true);
    addRow("*.h[][!]ml", "test.h]ml", true);
    addRow("*.h[][!]ml", "test.h[ml", true);
    addRow("*.h[][!]ml", "test.h!ml", true);
    addRow("foo/*/bar", "foo/baz/bar", true);
    addRow("foo/*/bar", "foo/fie/baz/bar", true);
    addRow("foo?bar", "foo/bar", true);
    addRow("foo/(*)/bar", "foo/baz/bar", false);
    addRow("foo/(*)/bar", "foo/(baz)/bar", true);
    addRow("foo/?/bar", "foo/Q/bar", true);
    addRow("foo/?/bar", "foo/Qt/bar", false);
    addRow("foo/(?)/bar", "foo/Q/bar", false);
    addRow("foo/(?)/bar", "foo/(Q)/bar", true);
    addRow("foo\\*\\bar", "foo\\baz\\bar", true);
    addRow("foo\\*\\bar", "foo/baz/bar", false);
    addRow("foo\\*\\bar", "foo/baz\\bar", false);
    addRow("foo\\*\\bar", "foo\\fie\\baz\\bar", true);
    addRow("foo\\*\\bar", "foo/fie/baz/bar", false);
    addRow("foo/*/bar", "foo\\baz\\bar", false);
    addRow("foo/*/bar", "foo/baz/bar", true);
    addRow("foo/*/bar", "foo\\fie\\baz\\bar", false);
    addRow("foo/*/bar", "foo/fie/baz/bar", true);
    addRow("foo\\(*)\\bar", "foo\\baz\\bar", false);
    addRow("foo\\(*)\\bar", "foo\\(baz)\\bar", true);
    addRow("foo\\?\\bar", "foo\\Q\\bar", true);
    addRow("foo\\?\\bar", "foo\\Qt\\bar", false);
    addRow("foo\\(?)\\bar", "foo\\Q\\bar", false);
    addRow("foo\\(?)\\bar", "foo\\(Q)\\bar", true);

    addRow("foo*bar", "foo/fie/baz/bar", true);
    addRow("fie*bar", "foo/fie/baz/bar", false);
}

void tst_StringUtils::testWildcardToRegularExpression()
{
    QFETCH(QString, pattern);
    QFETCH(QString, string);
    QFETCH(bool, matches);

    const QRegularExpression re(Utils::wildcardToRegularExpression(pattern));
    QCOMPARE(string.contains(re), matches);
}

void tst_StringUtils::testSplitAtFirst_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QChar>("separator");
    QTest::addColumn<QString>("left");
    QTest::addColumn<QString>("right");

    QTest::newRow("Empty") << QString{} << QChar{} << QString{} << QString{};
    QTest::newRow("EmptyString") << QString{} << QChar{'a'} << QString{} << QString{};
    QTest::newRow("EmptySeparator") << QString{"abc"} << QChar{} << QString{"abc"} << QString{};
    QTest::newRow("NoSeparator") << QString{"abc"} << QChar{'d'} << QString{"abc"} << QString{};
    QTest::newRow("SeparatorAtStart") << QString{"abc"} << QChar{'a'} << QString{} << QString{"bc"};
    QTest::newRow("SeparatorAtEnd") << QString{"abc"} << QChar{'c'} << QString{"ab"} << QString{};
    QTest::newRow("SeparatorInMiddle")
        << QString{"abc"} << QChar{'b'} << QString{"a"} << QString{"c"};
    QTest::newRow("SeparatorAtStartAndEnd")
        << QString{"abca"} << QChar{'a'} << QString{} << QString{"bca"};
}

void tst_StringUtils::testSplitAtFirst()
{
    QFETCH(QString, string);
    QFETCH(QChar, separator);
    QFETCH(QString, left);
    QFETCH(QString, right);

    const auto [l, r] = Utils::splitAtFirst(string, separator);

    QCOMPARE(l, left);
    QCOMPARE(r, right);
}

QTEST_GUILESS_MAIN(tst_StringUtils)

#include "tst_stringutils.moc"
