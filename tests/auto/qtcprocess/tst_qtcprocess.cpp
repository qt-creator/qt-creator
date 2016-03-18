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

#include <utils/qtcprocess.h>
#include <utils/hostosinfo.h>
#include <utils/stringutils.h>
#include <utils/environment.h>

#include <QtTest>

using namespace Utils;

class MacroMapExpander : public AbstractMacroExpander {
public:
    virtual bool resolveMacro(const QString &name, QString *ret)
    {
        QHash<QString, QString>::const_iterator it = m_map.constFind(name);
        if (it != m_map.constEnd()) {
            *ret = it.value();
            return true;
        }
        return false;
    }
    void insert(const QString &key, const QString &value) { m_map.insert(key, value); }
private:
    QHash<QString, QString> m_map;
};

class tst_QtcProcess : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void splitArgs_data();
    void splitArgs();
    void prepareArgs_data();
    void prepareArgs();
    void prepareArgsEnv_data();
    void prepareArgsEnv();
    void expandMacros_data();
    void expandMacros();
    void iterations_data();
    void iterations();
    void iteratorEditsWindows();
    void iteratorEditsLinux();

private:
    void iteratorEditsHelper(OsType osType);

    Environment envWindows;
    Environment envLinux;

    MacroMapExpander mxWin;
    MacroMapExpander mxUnix;
    QString homeStr;
    QString home;
};

void tst_QtcProcess::initTestCase()
{
    homeStr = QLatin1String("@HOME@");
    home = QDir::homePath();

    QStringList env;
    env << "empty=" << "word=hi" << "words=hi ho" << "spacedwords= hi   ho sucker ";
    envWindows = Environment(env, OsTypeWindows);
    envLinux = Environment(env, OsTypeLinux);

    mxWin.insert("a", "hi");
    mxWin.insert("aa", "hi ho");

    mxWin.insert("b", "h\\i");
    mxWin.insert("c", "\\hi");
    mxWin.insert("d", "hi\\");
    mxWin.insert("ba", "h\\i ho");
    mxWin.insert("ca", "\\hi ho");
    mxWin.insert("da", "hi ho\\");

    mxWin.insert("e", "h\"i");
    mxWin.insert("f", "\"hi");
    mxWin.insert("g", "hi\"");

    mxWin.insert("h", "h\\\"i");
    mxWin.insert("i", "\\\"hi");
    mxWin.insert("j", "hi\\\"");

    mxWin.insert("k", "&special;");

    mxWin.insert("x", "\\");
    mxWin.insert("y", "\"");
    mxWin.insert("z", "");

    mxUnix.insert("a", "hi");
    mxUnix.insert("b", "hi ho");
    mxUnix.insert("c", "&special;");
    mxUnix.insert("d", "h\\i");
    mxUnix.insert("e", "h\"i");
    mxUnix.insert("f", "h'i");
    mxUnix.insert("z", "");
}


Q_DECLARE_METATYPE(QtcProcess::SplitError)
Q_DECLARE_METATYPE(Utils::OsType)

void tst_QtcProcess::splitArgs_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<QtcProcess::SplitError>("err");
    QTest::addColumn<Utils::OsType>("os");

    static const struct {
        const char * const in;
        const char * const out;
        const QtcProcess::SplitError err;
        const OsType os;
    } vals[] = {
        { "", "", QtcProcess::SplitOk, OsTypeWindows },
        { " ", "", QtcProcess::SplitOk, OsTypeWindows },
        { "hi", "hi", QtcProcess::SplitOk, OsTypeWindows },
        { "hi ho", "hi ho", QtcProcess::SplitOk, OsTypeWindows },
        { " hi ho ", "hi ho", QtcProcess::SplitOk, OsTypeWindows },
        { "\"hi ho\" \"hi\" ho  ", "\"hi ho\" hi ho", QtcProcess::SplitOk, OsTypeWindows },
        { "\\", "\\", QtcProcess::SplitOk, OsTypeWindows },
        { "\\\"", "\"\"\\^\"\"\"", QtcProcess::SplitOk, OsTypeWindows },
        { "\"hi\"\"\"ho\"", "\"hi\"\\^\"\"ho\"", QtcProcess::SplitOk, OsTypeWindows },
        { "\\\\\\\"", "\"\"\\\\\\^\"\"\"", QtcProcess::SplitOk, OsTypeWindows },
        { " ^^ ", "\"^^\"", QtcProcess::SplitOk, OsTypeWindows },
        { "hi\"", "", QtcProcess::BadQuoting, OsTypeWindows },
        { "hi\"dood", "", QtcProcess::BadQuoting, OsTypeWindows },
        { "%var%", "%var%", QtcProcess::SplitOk, OsTypeWindows },

        { "", "", QtcProcess::SplitOk, OsTypeLinux },
        { " ", "", QtcProcess::SplitOk, OsTypeLinux },
        { "hi", "hi", QtcProcess::SplitOk, OsTypeLinux },
        { "hi ho", "hi ho", QtcProcess::SplitOk, OsTypeLinux },
        { " hi ho ", "hi ho", QtcProcess::SplitOk, OsTypeLinux },
        { "'hi ho' \"hi\" ho  ", "'hi ho' hi ho", QtcProcess::SplitOk, OsTypeLinux },
        { " \\ ", "' '", QtcProcess::SplitOk, OsTypeLinux },
        { " \\\" ", "'\"'", QtcProcess::SplitOk, OsTypeLinux },
        { " '\"' ", "'\"'", QtcProcess::SplitOk, OsTypeLinux },
        { " \"\\\"\" ", "'\"'", QtcProcess::SplitOk, OsTypeLinux },
        { "hi'", "", QtcProcess::BadQuoting, OsTypeLinux },
        { "hi\"dood", "", QtcProcess::BadQuoting, OsTypeLinux },
        { "$var", "'$var'", QtcProcess::SplitOk, OsTypeLinux },
        { "~", "@HOME@", QtcProcess::SplitOk, OsTypeLinux },
        { "~ foo", "@HOME@ foo", QtcProcess::SplitOk, OsTypeLinux },
        { "foo ~", "foo @HOME@", QtcProcess::SplitOk, OsTypeLinux },
        { "~/foo", "@HOME@/foo", QtcProcess::SplitOk, OsTypeLinux },
        { "~foo", "'~foo'", QtcProcess::SplitOk, OsTypeLinux }
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
        QString out = QString::fromLatin1(vals[i].out);
        if (vals[i].os == OsTypeLinux)
            out.replace(homeStr, home);
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << out << vals[i].err << vals[i].os;
    }
}

void tst_QtcProcess::splitArgs()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(QtcProcess::SplitError, err);
    QFETCH(Utils::OsType, os);

    QtcProcess::SplitError outerr;
    QString outstr = QtcProcess::joinArgs(QtcProcess::splitArgs(in, os, false, &outerr), os);
    QCOMPARE(outerr, err);
    if (err == QtcProcess::SplitOk)
        QCOMPARE(outstr, out);
}

void tst_QtcProcess::prepareArgs_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<QtcProcess::SplitError>("err");
    QTest::addColumn<OsType>("os");

    static const struct {
        const char * const in;
        const char * const out;
        const QtcProcess::SplitError err;
        const OsType os;
    } vals[] = {
        { " ", " ", QtcProcess::SplitOk, OsTypeWindows },
        { "", "", QtcProcess::SplitOk, OsTypeWindows },
        { "hi", "hi", QtcProcess::SplitOk, OsTypeWindows },
        { "hi ho", "hi ho", QtcProcess::SplitOk, OsTypeWindows },
        { " hi ho ", " hi ho ", QtcProcess::SplitOk, OsTypeWindows },
        { "\"hi ho\" \"hi\" ho  ", "\"hi ho\" \"hi\" ho  ", QtcProcess::SplitOk, OsTypeWindows },
        { "\\", "\\", QtcProcess::SplitOk, OsTypeWindows },
        { "\\\"", "\\\"", QtcProcess::SplitOk, OsTypeWindows },
        { "\"hi\"\"ho\"", "\"hi\"\"ho\"", QtcProcess::SplitOk, OsTypeWindows },
        { "\\\\\\\"", "\\\\\\\"", QtcProcess::SplitOk, OsTypeWindows },
        { "^^", "^", QtcProcess::SplitOk, OsTypeWindows },
        { "hi\"", "hi\"", QtcProcess::SplitOk, OsTypeWindows },
        { "hi\"dood", "hi\"dood", QtcProcess::SplitOk, OsTypeWindows },
        { "%var%", "", QtcProcess::FoundMeta, OsTypeWindows },
        { "echo hi > file", "", QtcProcess::FoundMeta, OsTypeWindows },

        { "", "", QtcProcess::SplitOk, OsTypeLinux },
        { " ", "", QtcProcess::SplitOk, OsTypeLinux },
        { "hi", "hi", QtcProcess::SplitOk, OsTypeLinux },
        { "hi ho", "hi ho", QtcProcess::SplitOk, OsTypeLinux },
        { " hi ho ", "hi ho", QtcProcess::SplitOk, OsTypeLinux },
        { "'hi ho' \"hi\" ho  ", "'hi ho' hi ho", QtcProcess::SplitOk, OsTypeLinux },
        { " \\ ", "' '", QtcProcess::SplitOk, OsTypeLinux },
        { "hi'", "", QtcProcess::BadQuoting, OsTypeLinux },
        { "hi\"dood", "", QtcProcess::BadQuoting, OsTypeLinux },
        { "$var", "", QtcProcess::FoundMeta, OsTypeLinux },
        { "~", "@HOME@", QtcProcess::SplitOk, OsTypeLinux },
        { "~ foo", "@HOME@ foo", QtcProcess::SplitOk, OsTypeLinux },
        { "~/foo", "@HOME@/foo", QtcProcess::SplitOk, OsTypeLinux },
        { "~foo", "", QtcProcess::FoundMeta, OsTypeLinux }
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
        QString out = QString::fromLatin1(vals[i].out);
        if (vals[i].os == OsTypeLinux)
            out.replace(homeStr, home);
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << out << vals[i].err << vals[i].os;
    }
}

void tst_QtcProcess::prepareArgs()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(QtcProcess::SplitError, err);
    QFETCH(OsType, os);

    QtcProcess::SplitError outerr;
    QtcProcess::Arguments args = QtcProcess::prepareArgs(in, &outerr, os);
    QString outstr = args.toString();

    QCOMPARE(outerr, err);
    if (err == QtcProcess::SplitOk)
        QCOMPARE(outstr, out);
}

void tst_QtcProcess::prepareArgsEnv_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<QtcProcess::SplitError>("err");
    QTest::addColumn<OsType>("os");

    static const struct {
        const char * const in;
        const char * const out;
        const QtcProcess::SplitError err;
        const OsType os;
    } vals[] = {
        { " ", " ", QtcProcess::SplitOk, OsTypeWindows },
        { "", "", QtcProcess::SplitOk, OsTypeWindows },
        { "hi", "hi", QtcProcess::SplitOk, OsTypeWindows },
        { "hi ho", "hi ho", QtcProcess::SplitOk, OsTypeWindows },
        { " hi ho ", " hi ho ", QtcProcess::SplitOk, OsTypeWindows },
        { "\"hi ho\" \"hi\" ho  ", "\"hi ho\" \"hi\" ho  ", QtcProcess::SplitOk, OsTypeWindows },
        { "\\", "\\", QtcProcess::SplitOk, OsTypeWindows },
        { "\\\"", "\\\"", QtcProcess::SplitOk, OsTypeWindows },
        { "\"hi\"\"ho\"", "\"hi\"\"ho\"", QtcProcess::SplitOk, OsTypeWindows },
        { "\\\\\\\"", "\\\\\\\"", QtcProcess::SplitOk, OsTypeWindows },
        { "^^", "^", QtcProcess::SplitOk, OsTypeWindows },
        { "hi\"", "hi\"", QtcProcess::SplitOk, OsTypeWindows },
        { "hi\"dood", "hi\"dood", QtcProcess::SplitOk, OsTypeWindows },
        { "%empty%", "%empty%", QtcProcess::SplitOk, OsTypeWindows }, // Yep, no empty variables on Windows.
        { "%word%", "hi", QtcProcess::SplitOk, OsTypeWindows },
        { " %word% ", " hi ", QtcProcess::SplitOk, OsTypeWindows },
        { "%words%", "hi ho", QtcProcess::SplitOk, OsTypeWindows },
        { "%nonsense%words%", "%nonsensehi ho", QtcProcess::SplitOk, OsTypeWindows },
        { "fail%nonsense%words%", "fail%nonsensehi ho", QtcProcess::SplitOk, OsTypeWindows },
        { "%words%words%", "hi howords%", QtcProcess::SplitOk, OsTypeWindows },
        { "%words%%words%", "hi hohi ho", QtcProcess::SplitOk, OsTypeWindows },
        { "echo hi > file", "", QtcProcess::FoundMeta, OsTypeWindows },

        { "", "", QtcProcess::SplitOk, OsTypeLinux },
        { " ", "", QtcProcess::SplitOk, OsTypeLinux },
        { "hi", "hi", QtcProcess::SplitOk, OsTypeLinux },
        { "hi ho", "hi ho", QtcProcess::SplitOk, OsTypeLinux },
        { " hi ho ", "hi ho", QtcProcess::SplitOk, OsTypeLinux },
        { "'hi ho' \"hi\" ho  ", "'hi ho' hi ho", QtcProcess::SplitOk, OsTypeLinux },
        { " \\ ", "' '", QtcProcess::SplitOk, OsTypeLinux },
        { "hi'", "", QtcProcess::BadQuoting, OsTypeLinux },
        { "hi\"dood", "", QtcProcess::BadQuoting, OsTypeLinux },
        { "$empty", "", QtcProcess::SplitOk, OsTypeLinux },
        { "$word", "hi", QtcProcess::SplitOk, OsTypeLinux },
        { " $word ", "hi", QtcProcess::SplitOk, OsTypeLinux },
        { "${word}", "hi", QtcProcess::SplitOk, OsTypeLinux },
        { " ${word} ", "hi", QtcProcess::SplitOk, OsTypeLinux },
        { "$words", "hi ho", QtcProcess::SplitOk, OsTypeLinux },
        { "$spacedwords", "hi ho sucker", QtcProcess::SplitOk, OsTypeLinux },
        { "hi${empty}ho", "hiho", QtcProcess::SplitOk, OsTypeLinux },
        { "hi${words}ho", "hihi hoho", QtcProcess::SplitOk, OsTypeLinux },
        { "hi${spacedwords}ho", "hi hi ho sucker ho", QtcProcess::SplitOk, OsTypeLinux },
        { "${", "", QtcProcess::BadQuoting, OsTypeLinux },
        { "${var", "", QtcProcess::BadQuoting, OsTypeLinux },
        { "${var ", "", QtcProcess::FoundMeta, OsTypeLinux },
        { "\"hi${words}ho\"", "'hihi hoho'", QtcProcess::SplitOk, OsTypeLinux },
        { "\"hi${spacedwords}ho\"", "'hi hi   ho sucker ho'", QtcProcess::SplitOk, OsTypeLinux },
        { "\"${", "", QtcProcess::BadQuoting, OsTypeLinux },
        { "\"${var", "", QtcProcess::BadQuoting, OsTypeLinux },
        { "\"${var ", "", QtcProcess::FoundMeta, OsTypeLinux },
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
        QString out = QString::fromLatin1(vals[i].out);
        if (vals[i].os == OsTypeLinux)
            out.replace(homeStr, home);
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << out << vals[i].err << vals[i].os;
    }
}

void tst_QtcProcess::prepareArgsEnv()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(QtcProcess::SplitError, err);
    QFETCH(OsType, os);

    QtcProcess::SplitError outerr;
    QtcProcess::Arguments args = QtcProcess::prepareArgs(in, &outerr, os, os == OsTypeLinux ? &envLinux : &envWindows);
    QString outstr = args.toString();

    QCOMPARE(outerr, err);
    if (err == QtcProcess::SplitOk)
        QCOMPARE(outstr, out);
}

void tst_QtcProcess::expandMacros_data()

{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<OsType>("os");
    QChar sp(QLatin1Char(' '));

    static const struct {
        const char * const in;
        const char * const out;
        OsType os;
    } vals[] = {
        { "plain", 0, OsTypeWindows },
        { "%{a}", "hi", OsTypeWindows },
        { "%{aa}", "\"hi ho\"", OsTypeWindows },
        { "%{b}", "h\\i", OsTypeWindows },
        { "%{c}", "\\hi", OsTypeWindows },
        { "%{d}", "hi\\", OsTypeWindows },
        { "%{ba}", "\"h\\i ho\"", OsTypeWindows },
        { "%{ca}", "\"\\hi ho\"", OsTypeWindows },
        { "%{da}", "\"hi ho\\\\\"", OsTypeWindows }, // or "\"hi ho\"\\"
        { "%{e}", "\"h\"\\^\"\"i\"", OsTypeWindows },
        { "%{f}", "\"\"\\^\"\"hi\"", OsTypeWindows },
        { "%{g}", "\"hi\"\\^\"\"\"", OsTypeWindows },
        { "%{h}", "\"h\\\\\"\\^\"\"i\"", OsTypeWindows },
        { "%{i}", "\"\\\\\"\\^\"\"hi\"", OsTypeWindows },
        { "%{j}", "\"hi\\\\\"\\^\"\"\"", OsTypeWindows },
        { "%{k}", "\"&special;\"", OsTypeWindows },
        { "%{x}", "\\", OsTypeWindows },
        { "%{y}", "\"\"\\^\"\"\"", OsTypeWindows },
        { "%{z}", "\"\"", OsTypeWindows },
        { "^%{z}%{z}", "^%{z}%{z}", OsTypeWindows }, // stupid user check

        { "quoted", 0, OsTypeWindows },
        { "\"%{a}\"", "\"hi\"", OsTypeWindows },
        { "\"%{aa}\"", "\"hi ho\"", OsTypeWindows },
        { "\"%{b}\"", "\"h\\i\"", OsTypeWindows },
        { "\"%{c}\"", "\"\\hi\"", OsTypeWindows },
        { "\"%{d}\"", "\"hi\\\\\"", OsTypeWindows },
        { "\"%{ba}\"", "\"h\\i ho\"", OsTypeWindows },
        { "\"%{ca}\"", "\"\\hi ho\"", OsTypeWindows },
        { "\"%{da}\"", "\"hi ho\\\\\"", OsTypeWindows },
        { "\"%{e}\"", "\"h\"\\^\"\"i\"", OsTypeWindows },
        { "\"%{f}\"", "\"\"\\^\"\"hi\"", OsTypeWindows },
        { "\"%{g}\"", "\"hi\"\\^\"\"\"", OsTypeWindows },
        { "\"%{h}\"", "\"h\\\\\"\\^\"\"i\"", OsTypeWindows },
        { "\"%{i}\"", "\"\\\\\"\\^\"\"hi\"", OsTypeWindows },
        { "\"%{j}\"", "\"hi\\\\\"\\^\"\"\"", OsTypeWindows },
        { "\"%{k}\"", "\"&special;\"", OsTypeWindows },
        { "\"%{x}\"", "\"\\\\\"", OsTypeWindows },
        { "\"%{y}\"", "\"\"\\^\"\"\"", OsTypeWindows },
        { "\"%{z}\"", "\"\"", OsTypeWindows },

        { "leading bs", 0, OsTypeWindows },
        { "\\%{a}", "\\hi", OsTypeWindows },
        { "\\%{aa}", "\\\\\"hi ho\"", OsTypeWindows },
        { "\\%{b}", "\\h\\i", OsTypeWindows },
        { "\\%{c}", "\\\\hi", OsTypeWindows },
        { "\\%{d}", "\\hi\\", OsTypeWindows },
        { "\\%{ba}", "\\\\\"h\\i ho\"", OsTypeWindows },
        { "\\%{ca}", "\\\\\"\\hi ho\"", OsTypeWindows },
        { "\\%{da}", "\\\\\"hi ho\\\\\"", OsTypeWindows },
        { "\\%{e}", "\\\\\"h\"\\^\"\"i\"", OsTypeWindows },
        { "\\%{f}", "\\\\\"\"\\^\"\"hi\"", OsTypeWindows },
        { "\\%{g}", "\\\\\"hi\"\\^\"\"\"", OsTypeWindows },
        { "\\%{h}", "\\\\\"h\\\\\"\\^\"\"i\"", OsTypeWindows },
        { "\\%{i}", "\\\\\"\\\\\"\\^\"\"hi\"", OsTypeWindows },
        { "\\%{j}", "\\\\\"hi\\\\\"\\^\"\"\"", OsTypeWindows },
        { "\\%{x}", "\\\\", OsTypeWindows },
        { "\\%{y}", "\\\\\"\"\\^\"\"\"", OsTypeWindows },
        { "\\%{z}", "\\", OsTypeWindows },

        { "trailing bs", 0, OsTypeWindows },
        { "%{a}\\", "hi\\", OsTypeWindows },
        { "%{aa}\\", "\"hi ho\"\\", OsTypeWindows },
        { "%{b}\\", "h\\i\\", OsTypeWindows },
        { "%{c}\\", "\\hi\\", OsTypeWindows },
        { "%{d}\\", "hi\\\\", OsTypeWindows },
        { "%{ba}\\", "\"h\\i ho\"\\", OsTypeWindows },
        { "%{ca}\\", "\"\\hi ho\"\\", OsTypeWindows },
        { "%{da}\\", "\"hi ho\\\\\"\\", OsTypeWindows },
        { "%{e}\\", "\"h\"\\^\"\"i\"\\", OsTypeWindows },
        { "%{f}\\", "\"\"\\^\"\"hi\"\\", OsTypeWindows },
        { "%{g}\\", "\"hi\"\\^\"\"\"\\", OsTypeWindows },
        { "%{h}\\", "\"h\\\\\"\\^\"\"i\"\\", OsTypeWindows },
        { "%{i}\\", "\"\\\\\"\\^\"\"hi\"\\", OsTypeWindows },
        { "%{j}\\", "\"hi\\\\\"\\^\"\"\"\\", OsTypeWindows },
        { "%{x}\\", "\\\\", OsTypeWindows },
        { "%{y}\\", "\"\"\\^\"\"\"\\", OsTypeWindows },
        { "%{z}\\", "\\", OsTypeWindows },

        { "bs-enclosed", 0, OsTypeWindows },
        { "\\%{a}\\", "\\hi\\", OsTypeWindows },
        { "\\%{aa}\\", "\\\\\"hi ho\"\\", OsTypeWindows },
        { "\\%{b}\\", "\\h\\i\\", OsTypeWindows },
        { "\\%{c}\\", "\\\\hi\\", OsTypeWindows },
        { "\\%{d}\\", "\\hi\\\\", OsTypeWindows },
        { "\\%{ba}\\", "\\\\\"h\\i ho\"\\", OsTypeWindows },
        { "\\%{ca}\\", "\\\\\"\\hi ho\"\\", OsTypeWindows },
        { "\\%{da}\\", "\\\\\"hi ho\\\\\"\\", OsTypeWindows },
        { "\\%{e}\\", "\\\\\"h\"\\^\"\"i\"\\", OsTypeWindows },
        { "\\%{f}\\", "\\\\\"\"\\^\"\"hi\"\\", OsTypeWindows },
        { "\\%{g}\\", "\\\\\"hi\"\\^\"\"\"\\", OsTypeWindows },
        { "\\%{h}\\", "\\\\\"h\\\\\"\\^\"\"i\"\\", OsTypeWindows },
        { "\\%{i}\\", "\\\\\"\\\\\"\\^\"\"hi\"\\", OsTypeWindows },
        { "\\%{j}\\", "\\\\\"hi\\\\\"\\^\"\"\"\\", OsTypeWindows },
        { "\\%{x}\\", "\\\\\\", OsTypeWindows },
        { "\\%{y}\\", "\\\\\"\"\\^\"\"\"\\", OsTypeWindows },
        { "\\%{z}\\", "\\\\", OsTypeWindows },

        { "bs-enclosed and trailing literal quote", 0, OsTypeWindows },
        { "\\%{a}\\\\\\^\"", "\\hi\\\\\\^\"", OsTypeWindows },
        { "\\%{aa}\\\\\\^\"", "\\\\\"hi ho\"\\\\\\^\"", OsTypeWindows },
        { "\\%{b}\\\\\\^\"", "\\h\\i\\\\\\^\"", OsTypeWindows },
        { "\\%{c}\\\\\\^\"", "\\\\hi\\\\\\^\"", OsTypeWindows },
        { "\\%{d}\\\\\\^\"", "\\hi\\\\\\\\\\^\"", OsTypeWindows },
        { "\\%{ba}\\\\\\^\"", "\\\\\"h\\i ho\"\\\\\\^\"", OsTypeWindows },
        { "\\%{ca}\\\\\\^\"", "\\\\\"\\hi ho\"\\\\\\^\"", OsTypeWindows },
        { "\\%{da}\\\\\\^\"", "\\\\\"hi ho\\\\\"\\\\\\^\"", OsTypeWindows },
        { "\\%{e}\\\\\\^\"", "\\\\\"h\"\\^\"\"i\"\\\\\\^\"", OsTypeWindows },
        { "\\%{f}\\\\\\^\"", "\\\\\"\"\\^\"\"hi\"\\\\\\^\"", OsTypeWindows },
        { "\\%{g}\\\\\\^\"", "\\\\\"hi\"\\^\"\"\"\\\\\\^\"", OsTypeWindows },
        { "\\%{h}\\\\\\^\"", "\\\\\"h\\\\\"\\^\"\"i\"\\\\\\^\"", OsTypeWindows },
        { "\\%{i}\\\\\\^\"", "\\\\\"\\\\\"\\^\"\"hi\"\\\\\\^\"", OsTypeWindows },
        { "\\%{j}\\\\\\^\"", "\\\\\"hi\\\\\"\\^\"\"\"\\\\\\^\"", OsTypeWindows },
        { "\\%{x}\\\\\\^\"", "\\\\\\\\\\\\\\^\"", OsTypeWindows },
        { "\\%{y}\\\\\\^\"", "\\\\\"\"\\^\"\"\"\\\\\\^\"", OsTypeWindows },
        { "\\%{z}\\\\\\^\"", "\\\\\\\\\\^\"", OsTypeWindows },

        { "bs-enclosed and trailing unclosed quote", 0, OsTypeWindows },
        { "\\%{a}\\\\\"", "\\hi\\\\\"", OsTypeWindows },
        { "\\%{aa}\\\\\"", "\\\\\"hi ho\"\\\\\"", OsTypeWindows },
        { "\\%{b}\\\\\"", "\\h\\i\\\\\"", OsTypeWindows },
        { "\\%{c}\\\\\"", "\\\\hi\\\\\"", OsTypeWindows },
        { "\\%{d}\\\\\"", "\\hi\\\\\\\\\"", OsTypeWindows },
        { "\\%{ba}\\\\\"", "\\\\\"h\\i ho\"\\\\\"", OsTypeWindows },
        { "\\%{ca}\\\\\"", "\\\\\"\\hi ho\"\\\\\"", OsTypeWindows },
        { "\\%{da}\\\\\"", "\\\\\"hi ho\\\\\"\\\\\"", OsTypeWindows },
        { "\\%{e}\\\\\"", "\\\\\"h\"\\^\"\"i\"\\\\\"", OsTypeWindows },
        { "\\%{f}\\\\\"", "\\\\\"\"\\^\"\"hi\"\\\\\"", OsTypeWindows },
        { "\\%{g}\\\\\"", "\\\\\"hi\"\\^\"\"\"\\\\\"", OsTypeWindows },
        { "\\%{h}\\\\\"", "\\\\\"h\\\\\"\\^\"\"i\"\\\\\"", OsTypeWindows },
        { "\\%{i}\\\\\"", "\\\\\"\\\\\"\\^\"\"hi\"\\\\\"", OsTypeWindows },
        { "\\%{j}\\\\\"", "\\\\\"hi\\\\\"\\^\"\"\"\\\\\"", OsTypeWindows },
        { "\\%{x}\\\\\"", "\\\\\\\\\\\\\"", OsTypeWindows },
        { "\\%{y}\\\\\"", "\\\\\"\"\\^\"\"\"\\\\\"", OsTypeWindows },
        { "\\%{z}\\\\\"", "\\\\\\\\\"", OsTypeWindows },

        { "multi-var", 0, OsTypeWindows },
        { "%{x}%{y}%{z}", "\\\\\"\"\\^\"\"\"", OsTypeWindows },
        { "%{x}%{z}%{y}%{z}", "\\\\\"\"\\^\"\"\"", OsTypeWindows },
        { "%{x}%{z}%{y}", "\\\\\"\"\\^\"\"\"", OsTypeWindows },
        { "%{x}\\^\"%{z}", "\\\\\\^\"", OsTypeWindows },
        { "%{x}%{z}\\^\"%{z}", "\\\\\\^\"", OsTypeWindows },
        { "%{x}%{z}\\^\"", "\\\\\\^\"", OsTypeWindows },
        { "%{x}\\%{z}", "\\\\", OsTypeWindows },
        { "%{x}%{z}\\%{z}", "\\\\", OsTypeWindows },
        { "%{x}%{z}\\", "\\\\", OsTypeWindows },
        { "%{aa}%{a}", "\"hi hohi\"", OsTypeWindows },
        { "%{aa}%{aa}", "\"hi hohi ho\"", OsTypeWindows },
        { "%{aa}:%{aa}", "\"hi ho\":\"hi ho\"", OsTypeWindows },
        { "hallo ^|%{aa}^|", "hallo ^|\"hi ho\"^|", OsTypeWindows },

        { "quoted multi-var", 0, OsTypeWindows },
        { "\"%{x}%{y}%{z}\"", "\"\\\\\"\\^\"\"\"", OsTypeWindows },
        { "\"%{x}%{z}%{y}%{z}\"", "\"\\\\\"\\^\"\"\"", OsTypeWindows },
        { "\"%{x}%{z}%{y}\"", "\"\\\\\"\\^\"\"\"", OsTypeWindows },
        { "\"%{x}\"^\"\"%{z}\"", "\"\\\\\"^\"\"\"", OsTypeWindows },
        { "\"%{x}%{z}\"^\"\"%{z}\"", "\"\\\\\"^\"\"\"", OsTypeWindows },
        { "\"%{x}%{z}\"^\"\"\"", "\"\\\\\"^\"\"\"", OsTypeWindows },
        { "\"%{x}\\%{z}\"", "\"\\\\\\\\\"", OsTypeWindows },
        { "\"%{x}%{z}\\%{z}\"", "\"\\\\\\\\\"", OsTypeWindows },
        { "\"%{x}%{z}\\\\\"", "\"\\\\\\\\\"", OsTypeWindows },
        { "\"%{aa}%{a}\"", "\"hi hohi\"", OsTypeWindows },
        { "\"%{aa}%{aa}\"", "\"hi hohi ho\"", OsTypeWindows },
        { "\"%{aa}:%{aa}\"", "\"hi ho:hi ho\"", OsTypeWindows },

        { "plain", 0, OsTypeLinux },
        { "%{a}", "hi", OsTypeLinux },
        { "%{b}", "'hi ho'", OsTypeLinux },
        { "%{c}", "'&special;'", OsTypeLinux },
        { "%{d}", "'h\\i'", OsTypeLinux },
        { "%{e}", "'h\"i'", OsTypeLinux },
        { "%{f}", "'h'\\''i'", OsTypeLinux },
        { "%{z}", "''", OsTypeLinux },
        { "\\%{z}%{z}", "\\%{z}%{z}", OsTypeLinux }, // stupid user check

        { "single-quoted", 0, OsTypeLinux },
        { "'%{a}'", "'hi'", OsTypeLinux },
        { "'%{b}'", "'hi ho'", OsTypeLinux },
        { "'%{c}'", "'&special;'", OsTypeLinux },
        { "'%{d}'", "'h\\i'", OsTypeLinux },
        { "'%{e}'", "'h\"i'", OsTypeLinux },
        { "'%{f}'", "'h'\\''i'", OsTypeLinux },
        { "'%{z}'", "''", OsTypeLinux },

        { "double-quoted", 0, OsTypeLinux },
        { "\"%{a}\"", "\"hi\"", OsTypeLinux },
        { "\"%{b}\"", "\"hi ho\"", OsTypeLinux },
        { "\"%{c}\"", "\"&special;\"", OsTypeLinux },
        { "\"%{d}\"", "\"h\\\\i\"", OsTypeLinux },
        { "\"%{e}\"", "\"h\\\"i\"", OsTypeLinux },
        { "\"%{f}\"", "\"h'i\"", OsTypeLinux },
        { "\"%{z}\"", "\"\"", OsTypeLinux },

        { "complex", 0, OsTypeLinux },
        { "echo \"$(echo %{a})\"", "echo \"$(echo hi)\"", OsTypeLinux },
        { "echo \"$(echo %{b})\"", "echo \"$(echo 'hi ho')\"", OsTypeLinux },
        { "echo \"$(echo \"%{a}\")\"", "echo \"$(echo \"hi\")\"", OsTypeLinux },
        // These make no sense shell-wise, but they test expando nesting
        { "echo \"%{echo %{a}}\"", "echo \"%{echo hi}\"", OsTypeLinux },
        { "echo \"%{echo %{b}}\"", "echo \"%{echo hi ho}\"", OsTypeLinux },
        { "echo \"%{echo \"%{a}\"}\"", "echo \"%{echo \"hi\"}\"", OsTypeLinux },
    };

    const char *title = 0;
    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
        if (!vals[i].out) {
            title = vals[i].in;
        } else {
            char buf[80];
            sprintf(buf, "%s: %s", title, vals[i].in);
            QTest::newRow(buf) << QString::fromLatin1(vals[i].in)
                               << QString::fromLatin1(vals[i].out)
                               << vals[i].os;
            sprintf(buf, "padded %s: %s", title, vals[i].in);
            QTest::newRow(buf) << QString(sp + QString::fromLatin1(vals[i].in) + sp)
                               << QString(sp + QString::fromLatin1(vals[i].out) + sp)
                               << vals[i].os;
        }
    }
}

void tst_QtcProcess::expandMacros()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(OsType, os);

    if (os == OsTypeWindows)
        QtcProcess::expandMacros(&in, &mxWin, os);
    else
        QtcProcess::expandMacros(&in, &mxUnix, os);
    QCOMPARE(in, out);
}

void tst_QtcProcess::iterations_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<OsType>("os");

    static const struct {
        const char * const in;
        const char * const out;
        OsType os;
    } vals[] = {
        { "", "", OsTypeWindows },
        { "hi", "hi", OsTypeWindows },
        { "  hi ", "hi", OsTypeWindows },
        { "hi ho", "hi ho", OsTypeWindows },
        { "\"hi ho\" sucker", "\"hi ho\" sucker", OsTypeWindows },
        { "\"hi\"^\"\"ho\" sucker", "\"hi\"\\^\"\"ho\" sucker", OsTypeWindows },
        { "\"hi\"\\^\"\"ho\" sucker", "\"hi\"\\^\"\"ho\" sucker", OsTypeWindows },
        { "hi^|ho", "\"hi|ho\"", OsTypeWindows },
        { "c:\\", "c:\\", OsTypeWindows },
        { "\"c:\\\\\"", "c:\\", OsTypeWindows },
        { "\\hi\\ho", "\\hi\\ho", OsTypeWindows },
        { "hi null%", "hi null%", OsTypeWindows },
        { "hi null% ho", "hi null% ho", OsTypeWindows },
        { "hi null%here ho", "hi null%here ho", OsTypeWindows },
        { "hi null%here%too ho", "hi {} ho", OsTypeWindows },
        { "echo hello | more", "echo hello", OsTypeWindows },
        { "echo hello| more", "echo hello", OsTypeWindows },

        { "", "", OsTypeLinux },
        { " ", "", OsTypeLinux },
        { "hi", "hi", OsTypeLinux },
        { "  hi ", "hi", OsTypeLinux },
        { "'hi'", "hi", OsTypeLinux },
        { "hi ho", "hi ho", OsTypeLinux },
        { "\"hi ho\" sucker", "'hi ho' sucker", OsTypeLinux },
        { "\"hi\\\"ho\" sucker", "'hi\"ho' sucker", OsTypeLinux },
        { "\"hi'ho\" sucker", "'hi'\\''ho' sucker", OsTypeLinux },
        { "'hi ho' sucker", "'hi ho' sucker", OsTypeLinux },
        { "\\\\", "'\\'", OsTypeLinux },
        { "'\\'", "'\\'", OsTypeLinux },
        { "hi 'null${here}too' ho", "hi 'null${here}too' ho", OsTypeLinux },
        { "hi null${here}too ho", "hi {} ho", OsTypeLinux },
        { "hi $(echo $dollar cent) ho", "hi {} ho", OsTypeLinux },
        { "hi `echo $dollar \\`echo cent\\` | cat` ho", "hi {} ho", OsTypeLinux },
        { "echo hello | more", "echo hello", OsTypeLinux },
        { "echo hello| more", "echo hello", OsTypeLinux },
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++)
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << QString::fromLatin1(vals[i].out)
                                  << vals[i].os;
}

void tst_QtcProcess::iterations()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(OsType, os);

    QString outstr;
    for (QtcProcess::ArgIterator ait(&in, os); ait.next(); ) {
        if (ait.isSimple())
            QtcProcess::addArg(&outstr, ait.value(), os);
        else
            QtcProcess::addArgs(&outstr, "{}");
    }
    QCOMPARE(outstr, out);
}

void tst_QtcProcess::iteratorEditsHelper(OsType osType)
{
    QString in1 = "one two three", in2 = in1, in3 = in1, in4 = in1, in5 = in1;

    QtcProcess::ArgIterator ait1(&in1, osType);
    QVERIFY(ait1.next());
    ait1.deleteArg();
    QVERIFY(ait1.next());
    QVERIFY(ait1.next());
    QVERIFY(!ait1.next());
    QCOMPARE(in1, QString::fromLatin1("two three"));
    ait1.appendArg("four");
    QCOMPARE(in1, QString::fromLatin1("two three four"));

    QtcProcess::ArgIterator ait2(&in2, osType);
    QVERIFY(ait2.next());
    QVERIFY(ait2.next());
    ait2.deleteArg();
    QVERIFY(ait2.next());
    ait2.appendArg("four");
    QVERIFY(!ait2.next());
    QCOMPARE(in2, QString::fromLatin1("one three four"));

    QtcProcess::ArgIterator ait3(&in3, osType);
    QVERIFY(ait3.next());
    ait3.appendArg("one-b");
    QVERIFY(ait3.next());
    QVERIFY(ait3.next());
    ait3.deleteArg();
    QVERIFY(!ait3.next());
    QCOMPARE(in3, QString::fromLatin1("one one-b two"));

    QtcProcess::ArgIterator ait4(&in4, osType);
    ait4.appendArg("pre-one");
    QVERIFY(ait4.next());
    QVERIFY(ait4.next());
    QVERIFY(ait4.next());
    ait4.deleteArg();
    QVERIFY(!ait4.next());
    QCOMPARE(in4, QString::fromLatin1("pre-one one two"));

    QtcProcess::ArgIterator ait5(&in5, osType);
    QVERIFY(ait5.next());
    QVERIFY(ait5.next());
    QVERIFY(ait5.next());
    QVERIFY(!ait5.next());
    ait5.deleteArg();
    QVERIFY(!ait5.next());
    QCOMPARE(in5, QString::fromLatin1("one two"));
}

void tst_QtcProcess::iteratorEditsWindows()
{
    iteratorEditsHelper(OsTypeWindows);
}

void tst_QtcProcess::iteratorEditsLinux()
{
    iteratorEditsHelper(OsTypeLinux);
}

QTEST_MAIN(tst_QtcProcess)

#include "tst_qtcprocess.moc"
