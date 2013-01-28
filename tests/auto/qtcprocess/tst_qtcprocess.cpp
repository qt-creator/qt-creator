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

#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/environment.h>

#include <QtTest>

using namespace Utils;

class MacroMapExpander : public AbstractQtcMacroExpander {
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
    void iteratorEdits();

private:
    Environment env;
    MacroMapExpander mx;
#ifdef Q_OS_UNIX
    QString homeStr;
    QString home;
#endif
};

void tst_QtcProcess::initTestCase()
{
#ifdef Q_OS_UNIX
    homeStr = QLatin1String("@HOME@");
    home = QDir::homePath();
#endif

    env.set("empty", "");
    env.set("word", "hi");
    env.set("words", "hi ho");
    env.set("spacedwords", " hi   ho sucker ");

#ifdef Q_OS_WIN
    mx.insert("a", "hi");
    mx.insert("aa", "hi ho");

    mx.insert("b", "h\\i");
    mx.insert("c", "\\hi");
    mx.insert("d", "hi\\");
    mx.insert("ba", "h\\i ho");
    mx.insert("ca", "\\hi ho");
    mx.insert("da", "hi ho\\");

    mx.insert("e", "h\"i");
    mx.insert("f", "\"hi");
    mx.insert("g", "hi\"");

    mx.insert("h", "h\\\"i");
    mx.insert("i", "\\\"hi");
    mx.insert("j", "hi\\\"");

    mx.insert("k", "&special;");

    mx.insert("x", "\\");
    mx.insert("y", "\"");
#else
    mx.insert("a", "hi");
    mx.insert("b", "hi ho");
    mx.insert("c", "&special;");
    mx.insert("d", "h\\i");
    mx.insert("e", "h\"i");
    mx.insert("f", "h'i");
#endif
    mx.insert("z", "");
}

Q_DECLARE_METATYPE(QtcProcess::SplitError)

void tst_QtcProcess::splitArgs_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<QtcProcess::SplitError>("err");

    static const struct {
        const char * const in;
        const char * const out;
        const QtcProcess::SplitError err;
    } vals[] = {
#ifdef Q_OS_WIN
        { "", "", QtcProcess::SplitOk },
        { " ", "", QtcProcess::SplitOk },
        { "hi", "hi", QtcProcess::SplitOk },
        { "hi ho", "hi ho", QtcProcess::SplitOk },
        { " hi ho ", "hi ho", QtcProcess::SplitOk },
        { "\"hi ho\" \"hi\" ho  ", "\"hi ho\" hi ho", QtcProcess::SplitOk },
        { "\\", "\\", QtcProcess::SplitOk },
        { "\\\"", "\"\"\\^\"\"\"", QtcProcess::SplitOk },
        { "\"hi\"\"\"ho\"", "\"hi\"\\^\"\"ho\"", QtcProcess::SplitOk },
        { "\\\\\\\"", "\"\"\\\\\\^\"\"\"", QtcProcess::SplitOk },
        { " ^^ ", "\"^^\"", QtcProcess::SplitOk },
        { "hi\"", "", QtcProcess::BadQuoting },
        { "hi\"dood", "", QtcProcess::BadQuoting },
        { "%var%", "%var%", QtcProcess::SplitOk },
#else
        { "", "", QtcProcess::SplitOk },
        { " ", "", QtcProcess::SplitOk },
        { "hi", "hi", QtcProcess::SplitOk },
        { "hi ho", "hi ho", QtcProcess::SplitOk },
        { " hi ho ", "hi ho", QtcProcess::SplitOk },
        { "'hi ho' \"hi\" ho  ", "'hi ho' hi ho", QtcProcess::SplitOk },
        { " \\ ", "' '", QtcProcess::SplitOk },
        { " \\\" ", "'\"'", QtcProcess::SplitOk },
        { " '\"' ", "'\"'", QtcProcess::SplitOk },
        { " \"\\\"\" ", "'\"'", QtcProcess::SplitOk },
        { "hi'", "", QtcProcess::BadQuoting },
        { "hi\"dood", "", QtcProcess::BadQuoting },
        { "$var", "'$var'", QtcProcess::SplitOk },
        { "~", "@HOME@", QtcProcess::SplitOk },
        { "~ foo", "@HOME@ foo", QtcProcess::SplitOk },
        { "foo ~", "foo @HOME@", QtcProcess::SplitOk },
        { "~/foo", "@HOME@/foo", QtcProcess::SplitOk },
        { "~foo", "'~foo'", QtcProcess::SplitOk },
#endif
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++)
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << QString::fromLatin1(vals[i].out)
#ifdef Q_OS_UNIX
                                     .replace(homeStr, home)
#endif
                                  << vals[i].err;
}

void tst_QtcProcess::splitArgs()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(QtcProcess::SplitError, err);

    QtcProcess::SplitError outerr;
    QString outstr = QtcProcess::joinArgs(QtcProcess::splitArgs(in, false, &outerr));
    QCOMPARE(outerr, err);
    if (err == QtcProcess::SplitOk)
        QCOMPARE(outstr, out);
}

void tst_QtcProcess::prepareArgs_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<QtcProcess::SplitError>("err");

    static const struct {
        const char * const in;
        const char * const out;
        const QtcProcess::SplitError err;
    } vals[] = {
#ifdef Q_OS_WIN
        { "", "", QtcProcess::SplitOk },
        { " ", " ", QtcProcess::SplitOk },
        { "hi", "hi", QtcProcess::SplitOk },
        { "hi ho", "hi ho", QtcProcess::SplitOk },
        { " hi ho ", " hi ho ", QtcProcess::SplitOk },
        { "\"hi ho\" \"hi\" ho  ", "\"hi ho\" \"hi\" ho  ", QtcProcess::SplitOk },
        { "\\", "\\", QtcProcess::SplitOk },
        { "\\\"", "\\\"", QtcProcess::SplitOk },
        { "\"hi\"\"ho\"", "\"hi\"\"ho\"", QtcProcess::SplitOk },
        { "\\\\\\\"", "\\\\\\\"", QtcProcess::SplitOk },
        { "^^", "^", QtcProcess::SplitOk },
        { "hi\"", "hi\"", QtcProcess::SplitOk },
        { "hi\"dood", "hi\"dood", QtcProcess::SplitOk },
        { "%var%", "", QtcProcess::FoundMeta },
        { "echo hi > file", "", QtcProcess::FoundMeta },
#else
        { "", "", QtcProcess::SplitOk },
        { " ", "", QtcProcess::SplitOk },
        { "hi", "hi", QtcProcess::SplitOk },
        { "hi ho", "hi ho", QtcProcess::SplitOk },
        { " hi ho ", "hi ho", QtcProcess::SplitOk },
        { "'hi ho' \"hi\" ho  ", "'hi ho' hi ho", QtcProcess::SplitOk },
        { " \\ ", "' '", QtcProcess::SplitOk },
        { "hi'", "", QtcProcess::BadQuoting },
        { "hi\"dood", "", QtcProcess::BadQuoting },
        { "$var", "", QtcProcess::FoundMeta },
        { "~", "@HOME@", QtcProcess::SplitOk },
        { "~ foo", "@HOME@ foo", QtcProcess::SplitOk },
        { "~/foo", "@HOME@/foo", QtcProcess::SplitOk },
        { "~foo", "", QtcProcess::FoundMeta },
#endif
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++)
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << QString::fromLatin1(vals[i].out)
#ifdef Q_OS_UNIX
                                     .replace(homeStr, home)
#endif
                                  << vals[i].err;
}

void tst_QtcProcess::prepareArgs()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(QtcProcess::SplitError, err);

    QtcProcess::SplitError outerr;
    QString outstr;
#ifdef Q_OS_WIN
    outstr = QtcProcess::prepareArgs(in, &outerr);
#else
    outstr = QtcProcess::joinArgs(QtcProcess::prepareArgs(in, &outerr));
#endif
    QCOMPARE(outerr, err);
    if (err == QtcProcess::SplitOk)
        QCOMPARE(outstr, out);
}

void tst_QtcProcess::prepareArgsEnv_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<QtcProcess::SplitError>("err");

    static const struct {
        const char * const in;
        const char * const out;
        const QtcProcess::SplitError err;
    } vals[] = {
#ifdef Q_OS_WIN
        { "", "", QtcProcess::SplitOk },
        { " ", " ", QtcProcess::SplitOk },
        { "hi", "hi", QtcProcess::SplitOk },
        { "hi ho", "hi ho", QtcProcess::SplitOk },
        { " hi ho ", " hi ho ", QtcProcess::SplitOk },
        { "\"hi ho\" \"hi\" ho  ", "\"hi ho\" \"hi\" ho  ", QtcProcess::SplitOk },
        { "\\", "\\", QtcProcess::SplitOk },
        { "\\\"", "\\\"", QtcProcess::SplitOk },
        { "\"hi\"\"ho\"", "\"hi\"\"ho\"", QtcProcess::SplitOk },
        { "\\\\\\\"", "\\\\\\\"", QtcProcess::SplitOk },
        { "^^", "^", QtcProcess::SplitOk },
        { "hi\"", "hi\"", QtcProcess::SplitOk },
        { "hi\"dood", "hi\"dood", QtcProcess::SplitOk },
        { "%empty%", "%empty%", QtcProcess::SplitOk }, // Yep, no empty variables on Windows.
        { "%word%", "hi", QtcProcess::SplitOk },
        { " %word% ", " hi ", QtcProcess::SplitOk },
        { "%words%", "hi ho", QtcProcess::SplitOk },
        { "%nonsense%words%", "%nonsensehi ho", QtcProcess::SplitOk },
        { "fail%nonsense%words%", "fail%nonsensehi ho", QtcProcess::SplitOk },
        { "%words%words%", "hi howords%", QtcProcess::SplitOk },
        { "%words%%words%", "hi hohi ho", QtcProcess::SplitOk },
        { "echo hi > file", "", QtcProcess::FoundMeta },
#else
        { "", "", QtcProcess::SplitOk },
        { " ", "", QtcProcess::SplitOk },
        { "hi", "hi", QtcProcess::SplitOk },
        { "hi ho", "hi ho", QtcProcess::SplitOk },
        { " hi ho ", "hi ho", QtcProcess::SplitOk },
        { "'hi ho' \"hi\" ho  ", "'hi ho' hi ho", QtcProcess::SplitOk },
        { " \\ ", "' '", QtcProcess::SplitOk },
        { "hi'", "", QtcProcess::BadQuoting },
        { "hi\"dood", "", QtcProcess::BadQuoting },
        { "$empty", "", QtcProcess::SplitOk },
        { "$word", "hi", QtcProcess::SplitOk },
        { " $word ", "hi", QtcProcess::SplitOk },
        { "${word}", "hi", QtcProcess::SplitOk },
        { " ${word} ", "hi", QtcProcess::SplitOk },
        { "$words", "hi ho", QtcProcess::SplitOk },
        { "$spacedwords", "hi ho sucker", QtcProcess::SplitOk },
        { "hi${empty}ho", "hiho", QtcProcess::SplitOk },
        { "hi${words}ho", "hihi hoho", QtcProcess::SplitOk },
        { "hi${spacedwords}ho", "hi hi ho sucker ho", QtcProcess::SplitOk },
        { "${", "", QtcProcess::BadQuoting },
        { "${var", "", QtcProcess::BadQuoting },
        { "${var ", "", QtcProcess::FoundMeta },
        { "\"hi${words}ho\"", "'hihi hoho'", QtcProcess::SplitOk },
        { "\"hi${spacedwords}ho\"", "'hi hi   ho sucker ho'", QtcProcess::SplitOk },
        { "\"${", "", QtcProcess::BadQuoting },
        { "\"${var", "", QtcProcess::BadQuoting },
        { "\"${var ", "", QtcProcess::FoundMeta },
#endif
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++)
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << QString::fromLatin1(vals[i].out)
                                  << vals[i].err;
}

void tst_QtcProcess::prepareArgsEnv()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(QtcProcess::SplitError, err);

    QtcProcess::SplitError outerr;
    QString outstr;
#ifdef Q_OS_WIN
    outstr = QtcProcess::prepareArgs(in, &outerr, &env);
#else
    outstr = QtcProcess::joinArgs(QtcProcess::prepareArgs(in, &outerr, &env));
#endif
    QCOMPARE(outerr, err);
    if (err == QtcProcess::SplitOk)
        QCOMPARE(outstr, out);
}

void tst_QtcProcess::expandMacros_data()

{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QChar sp(QLatin1Char(' '));

    static const struct {
        const char * const in;
        const char * const out;
    } vals[] = {
#ifdef Q_OS_WIN
        { "plain", 0 },
        { "%{a}", "hi" },
        { "%{aa}", "\"hi ho\"" },
        { "%{b}", "h\\i" },
        { "%{c}", "\\hi" },
        { "%{d}", "hi\\" },
        { "%{ba}", "\"h\\i ho\"" },
        { "%{ca}", "\"\\hi ho\"" },
        { "%{da}", "\"hi ho\\\\\"" }, // or "\"hi ho\"\\"
        { "%{e}", "\"h\"\\^\"\"i\"" },
        { "%{f}", "\"\"\\^\"\"hi\"" },
        { "%{g}", "\"hi\"\\^\"\"\"" },
        { "%{h}", "\"h\\\\\"\\^\"\"i\"" },
        { "%{i}", "\"\\\\\"\\^\"\"hi\"" },
        { "%{j}", "\"hi\\\\\"\\^\"\"\"" },
        { "%{k}", "\"&special;\"" },
        { "%{x}", "\\" },
        { "%{y}", "\"\"\\^\"\"\"" },
        { "%{z}", "\"\"" },
        { "^%{z}%{z}", "^%{z}%{z}" }, // stupid user check

        { "quoted", 0 },
        { "\"%{a}\"", "\"hi\"" },
        { "\"%{aa}\"", "\"hi ho\"" },
        { "\"%{b}\"", "\"h\\i\"" },
        { "\"%{c}\"", "\"\\hi\"" },
        { "\"%{d}\"", "\"hi\\\\\"" },
        { "\"%{ba}\"", "\"h\\i ho\"" },
        { "\"%{ca}\"", "\"\\hi ho\"" },
        { "\"%{da}\"", "\"hi ho\\\\\"" },
        { "\"%{e}\"", "\"h\"\\^\"\"i\"" },
        { "\"%{f}\"", "\"\"\\^\"\"hi\"" },
        { "\"%{g}\"", "\"hi\"\\^\"\"\"" },
        { "\"%{h}\"", "\"h\\\\\"\\^\"\"i\"" },
        { "\"%{i}\"", "\"\\\\\"\\^\"\"hi\"" },
        { "\"%{j}\"", "\"hi\\\\\"\\^\"\"\"" },
        { "\"%{k}\"", "\"&special;\"" },
        { "\"%{x}\"", "\"\\\\\"" },
        { "\"%{y}\"", "\"\"\\^\"\"\"" },
        { "\"%{z}\"", "\"\"" },

        { "leading bs", 0 },
        { "\\%{a}", "\\hi" },
        { "\\%{aa}", "\\\\\"hi ho\"" },
        { "\\%{b}", "\\h\\i" },
        { "\\%{c}", "\\\\hi" },
        { "\\%{d}", "\\hi\\" },
        { "\\%{ba}", "\\\\\"h\\i ho\"" },
        { "\\%{ca}", "\\\\\"\\hi ho\"" },
        { "\\%{da}", "\\\\\"hi ho\\\\\"" },
        { "\\%{e}", "\\\\\"h\"\\^\"\"i\"" },
        { "\\%{f}", "\\\\\"\"\\^\"\"hi\"" },
        { "\\%{g}", "\\\\\"hi\"\\^\"\"\"" },
        { "\\%{h}", "\\\\\"h\\\\\"\\^\"\"i\"" },
        { "\\%{i}", "\\\\\"\\\\\"\\^\"\"hi\"" },
        { "\\%{j}", "\\\\\"hi\\\\\"\\^\"\"\"" },
        { "\\%{x}", "\\\\" },
        { "\\%{y}", "\\\\\"\"\\^\"\"\"" },
        { "\\%{z}", "\\" },

        { "trailing bs", 0 },
        { "%{a}\\", "hi\\" },
        { "%{aa}\\", "\"hi ho\"\\" },
        { "%{b}\\", "h\\i\\" },
        { "%{c}\\", "\\hi\\" },
        { "%{d}\\", "hi\\\\" },
        { "%{ba}\\", "\"h\\i ho\"\\" },
        { "%{ca}\\", "\"\\hi ho\"\\" },
        { "%{da}\\", "\"hi ho\\\\\"\\" },
        { "%{e}\\", "\"h\"\\^\"\"i\"\\" },
        { "%{f}\\", "\"\"\\^\"\"hi\"\\" },
        { "%{g}\\", "\"hi\"\\^\"\"\"\\" },
        { "%{h}\\", "\"h\\\\\"\\^\"\"i\"\\" },
        { "%{i}\\", "\"\\\\\"\\^\"\"hi\"\\" },
        { "%{j}\\", "\"hi\\\\\"\\^\"\"\"\\" },
        { "%{x}\\", "\\\\" },
        { "%{y}\\", "\"\"\\^\"\"\"\\" },
        { "%{z}\\", "\\" },

        { "bs-enclosed", 0 },
        { "\\%{a}\\", "\\hi\\" },
        { "\\%{aa}\\", "\\\\\"hi ho\"\\" },
        { "\\%{b}\\", "\\h\\i\\" },
        { "\\%{c}\\", "\\\\hi\\" },
        { "\\%{d}\\", "\\hi\\\\" },
        { "\\%{ba}\\", "\\\\\"h\\i ho\"\\" },
        { "\\%{ca}\\", "\\\\\"\\hi ho\"\\" },
        { "\\%{da}\\", "\\\\\"hi ho\\\\\"\\" },
        { "\\%{e}\\", "\\\\\"h\"\\^\"\"i\"\\" },
        { "\\%{f}\\", "\\\\\"\"\\^\"\"hi\"\\" },
        { "\\%{g}\\", "\\\\\"hi\"\\^\"\"\"\\" },
        { "\\%{h}\\", "\\\\\"h\\\\\"\\^\"\"i\"\\" },
        { "\\%{i}\\", "\\\\\"\\\\\"\\^\"\"hi\"\\" },
        { "\\%{j}\\", "\\\\\"hi\\\\\"\\^\"\"\"\\" },
        { "\\%{x}\\", "\\\\\\" },
        { "\\%{y}\\", "\\\\\"\"\\^\"\"\"\\" },
        { "\\%{z}\\", "\\\\" },

        { "bs-enclosed and trailing literal quote", 0 },
        { "\\%{a}\\\\\\^\"", "\\hi\\\\\\^\"" },
        { "\\%{aa}\\\\\\^\"", "\\\\\"hi ho\"\\\\\\^\"" },
        { "\\%{b}\\\\\\^\"", "\\h\\i\\\\\\^\"" },
        { "\\%{c}\\\\\\^\"", "\\\\hi\\\\\\^\"" },
        { "\\%{d}\\\\\\^\"", "\\hi\\\\\\\\\\^\"" },
        { "\\%{ba}\\\\\\^\"", "\\\\\"h\\i ho\"\\\\\\^\"" },
        { "\\%{ca}\\\\\\^\"", "\\\\\"\\hi ho\"\\\\\\^\"" },
        { "\\%{da}\\\\\\^\"", "\\\\\"hi ho\\\\\"\\\\\\^\"" },
        { "\\%{e}\\\\\\^\"", "\\\\\"h\"\\^\"\"i\"\\\\\\^\"" },
        { "\\%{f}\\\\\\^\"", "\\\\\"\"\\^\"\"hi\"\\\\\\^\"" },
        { "\\%{g}\\\\\\^\"", "\\\\\"hi\"\\^\"\"\"\\\\\\^\"" },
        { "\\%{h}\\\\\\^\"", "\\\\\"h\\\\\"\\^\"\"i\"\\\\\\^\"" },
        { "\\%{i}\\\\\\^\"", "\\\\\"\\\\\"\\^\"\"hi\"\\\\\\^\"" },
        { "\\%{j}\\\\\\^\"", "\\\\\"hi\\\\\"\\^\"\"\"\\\\\\^\"" },
        { "\\%{x}\\\\\\^\"", "\\\\\\\\\\\\\\^\"" },
        { "\\%{y}\\\\\\^\"", "\\\\\"\"\\^\"\"\"\\\\\\^\"" },
        { "\\%{z}\\\\\\^\"", "\\\\\\\\\\^\"" },

        { "bs-enclosed and trailing unclosed quote", 0 },
        { "\\%{a}\\\\\"", "\\hi\\\\\"" },
        { "\\%{aa}\\\\\"", "\\\\\"hi ho\"\\\\\"" },
        { "\\%{b}\\\\\"", "\\h\\i\\\\\"" },
        { "\\%{c}\\\\\"", "\\\\hi\\\\\"" },
        { "\\%{d}\\\\\"", "\\hi\\\\\\\\\"" },
        { "\\%{ba}\\\\\"", "\\\\\"h\\i ho\"\\\\\"" },
        { "\\%{ca}\\\\\"", "\\\\\"\\hi ho\"\\\\\"" },
        { "\\%{da}\\\\\"", "\\\\\"hi ho\\\\\"\\\\\"" },
        { "\\%{e}\\\\\"", "\\\\\"h\"\\^\"\"i\"\\\\\"" },
        { "\\%{f}\\\\\"", "\\\\\"\"\\^\"\"hi\"\\\\\"" },
        { "\\%{g}\\\\\"", "\\\\\"hi\"\\^\"\"\"\\\\\"" },
        { "\\%{h}\\\\\"", "\\\\\"h\\\\\"\\^\"\"i\"\\\\\"" },
        { "\\%{i}\\\\\"", "\\\\\"\\\\\"\\^\"\"hi\"\\\\\"" },
        { "\\%{j}\\\\\"", "\\\\\"hi\\\\\"\\^\"\"\"\\\\\"" },
        { "\\%{x}\\\\\"", "\\\\\\\\\\\\\"" },
        { "\\%{y}\\\\\"", "\\\\\"\"\\^\"\"\"\\\\\"" },
        { "\\%{z}\\\\\"", "\\\\\\\\\"" },

        { "multi-var", 0 },
        { "%{x}%{y}%{z}", "\\\\\"\"\\^\"\"\"" },
        { "%{x}%{z}%{y}%{z}", "\\\\\"\"\\^\"\"\"" },
        { "%{x}%{z}%{y}", "\\\\\"\"\\^\"\"\"" },
        { "%{x}\\^\"%{z}", "\\\\\\^\"" },
        { "%{x}%{z}\\^\"%{z}", "\\\\\\^\"" },
        { "%{x}%{z}\\^\"", "\\\\\\^\"" },
        { "%{x}\\%{z}", "\\\\" },
        { "%{x}%{z}\\%{z}", "\\\\" },
        { "%{x}%{z}\\", "\\\\" },
        { "%{aa}%{a}", "\"hi hohi\"" },
        { "%{aa}%{aa}", "\"hi hohi ho\"" },
        { "%{aa}:%{aa}", "\"hi ho\":\"hi ho\"" },
        { "hallo ^|%{aa}^|", "hallo ^|\"hi ho\"^|" },

        { "quoted multi-var", 0 },
        { "\"%{x}%{y}%{z}\"", "\"\\\\\"\\^\"\"\"" },
        { "\"%{x}%{z}%{y}%{z}\"", "\"\\\\\"\\^\"\"\"" },
        { "\"%{x}%{z}%{y}\"", "\"\\\\\"\\^\"\"\"" },
        { "\"%{x}\"^\"\"%{z}\"", "\"\\\\\"^\"\"\"" },
        { "\"%{x}%{z}\"^\"\"%{z}\"", "\"\\\\\"^\"\"\"" },
        { "\"%{x}%{z}\"^\"\"\"", "\"\\\\\"^\"\"\"" },
        { "\"%{x}\\%{z}\"", "\"\\\\\\\\\"" },
        { "\"%{x}%{z}\\%{z}\"", "\"\\\\\\\\\"" },
        { "\"%{x}%{z}\\\\\"", "\"\\\\\\\\\"" },
        { "\"%{aa}%{a}\"", "\"hi hohi\"" },
        { "\"%{aa}%{aa}\"", "\"hi hohi ho\"" },
        { "\"%{aa}:%{aa}\"", "\"hi ho:hi ho\"" },
#else
        { "plain", 0 },
        { "%{a}", "hi" },
        { "%{b}", "'hi ho'" },
        { "%{c}", "'&special;'" },
        { "%{d}", "'h\\i'" },
        { "%{e}", "'h\"i'" },
        { "%{f}", "'h'\\''i'" },
        { "%{z}", "''" },
        { "\\%{z}%{z}", "\\%{z}%{z}" }, // stupid user check

        { "single-quoted", 0 },
        { "'%{a}'", "'hi'" },
        { "'%{b}'", "'hi ho'" },
        { "'%{c}'", "'&special;'" },
        { "'%{d}'", "'h\\i'" },
        { "'%{e}'", "'h\"i'" },
        { "'%{f}'", "'h'\\''i'" },
        { "'%{z}'", "''" },

        { "double-quoted", 0 },
        { "\"%{a}\"", "\"hi\"" },
        { "\"%{b}\"", "\"hi ho\"" },
        { "\"%{c}\"", "\"&special;\"" },
        { "\"%{d}\"", "\"h\\\\i\"" },
        { "\"%{e}\"", "\"h\\\"i\"" },
        { "\"%{f}\"", "\"h'i\"" },
        { "\"%{z}\"", "\"\"" },

        { "complex", 0 },
        { "echo \"$(echo %{a})\"", "echo \"$(echo hi)\"" },
        { "echo \"$(echo %{b})\"", "echo \"$(echo 'hi ho')\"" },
        { "echo \"$(echo \"%{a}\")\"", "echo \"$(echo \"hi\")\"" },
        // These make no sense shell-wise, but they test expando nesting
        { "echo \"%{echo %{a}}\"", "echo \"%{echo hi}\"" },
        { "echo \"%{echo %{b}}\"", "echo \"%{echo hi ho}\"" },
        { "echo \"%{echo \"%{a}\"}\"", "echo \"%{echo \"hi\"}\"" },
#endif
    };

    const char *title = 0;
    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
        if (!vals[i].out) {
            title = vals[i].in;
        } else {
            char buf[80];
            sprintf(buf, "%s: %s", title, vals[i].in);
            QTest::newRow(buf) << QString::fromLatin1(vals[i].in)
                               << QString::fromLatin1(vals[i].out);
            sprintf(buf, "padded %s: %s", title, vals[i].in);
            QTest::newRow(buf) << (sp + QString::fromLatin1(vals[i].in) + sp)
                               << (sp + QString::fromLatin1(vals[i].out) + sp);
        }
    }
}

void tst_QtcProcess::expandMacros()
{
    QFETCH(QString, in);
    QFETCH(QString, out);

    QtcProcess::expandMacros(&in, &mx);
    QCOMPARE(in, out);
}

void tst_QtcProcess::iterations_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");

    static const struct {
        const char * const in;
        const char * const out;
    } vals[] = {
#ifdef Q_OS_WIN
        { "", "" },
        { "hi", "hi" },
        { "  hi ", "hi" },
        { "hi ho", "hi ho" },
        { "\"hi ho\" sucker", "\"hi ho\" sucker" },
        { "\"hi\"^\"\"ho\" sucker", "\"hi\"\\^\"\"ho\" sucker" },
        { "\"hi\"\\^\"\"ho\" sucker", "\"hi\"\\^\"\"ho\" sucker" },
        { "hi^|ho", "\"hi|ho\"" },
        { "c:\\", "c:\\" },
        { "\"c:\\\\\"", "c:\\" },
        { "\\hi\\ho", "\\hi\\ho" },
        { "hi null%", "hi null%" },
        { "hi null% ho", "hi null% ho" },
        { "hi null%here ho", "hi null%here ho" },
        { "hi null%here%too ho", "hi {} ho" },
        { "echo hello | more", "echo hello" },
        { "echo hello| more", "echo hello" },
#else
        { "", "" },
        { " ", "" },
        { "hi", "hi" },
        { "  hi ", "hi" },
        { "'hi'", "hi" },
        { "hi ho", "hi ho" },
        { "\"hi ho\" sucker", "'hi ho' sucker" },
        { "\"hi\\\"ho\" sucker", "'hi\"ho' sucker" },
        { "\"hi'ho\" sucker", "'hi'\\''ho' sucker" },
        { "'hi ho' sucker", "'hi ho' sucker" },
        { "\\\\", "'\\'" },
        { "'\\'", "'\\'" },
        { "hi 'null${here}too' ho", "hi 'null${here}too' ho" },
        { "hi null${here}too ho", "hi {} ho" },
        { "hi $(echo $dollar cent) ho", "hi {} ho" },
        { "hi `echo $dollar \\`echo cent\\` | cat` ho", "hi {} ho" },
        { "echo hello | more", "echo hello" },
        { "echo hello| more", "echo hello" },
#endif
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++)
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << QString::fromLatin1(vals[i].out);
}

void tst_QtcProcess::iterations()
{
    QFETCH(QString, in);
    QFETCH(QString, out);

    QString outstr;
    for (QtcProcess::ArgIterator ait(&in); ait.next(); )
        if (ait.isSimple())
            QtcProcess::addArg(&outstr, ait.value());
        else
            QtcProcess::addArgs(&outstr, "{}");
    QCOMPARE(outstr, out);
}

void tst_QtcProcess::iteratorEdits()
{
    QString in1 = "one two three", in2 = in1, in3 = in1, in4 = in1, in5 = in1;

    QtcProcess::ArgIterator ait1(&in1);
    QVERIFY(ait1.next());
    ait1.deleteArg();
    QVERIFY(ait1.next());
    QVERIFY(ait1.next());
    QVERIFY(!ait1.next());
    QCOMPARE(in1, QString::fromLatin1("two three"));
    ait1.appendArg("four");
    QCOMPARE(in1, QString::fromLatin1("two three four"));

    QtcProcess::ArgIterator ait2(&in2);
    QVERIFY(ait2.next());
    QVERIFY(ait2.next());
    ait2.deleteArg();
    QVERIFY(ait2.next());
    ait2.appendArg("four");
    QVERIFY(!ait2.next());
    QCOMPARE(in2, QString::fromLatin1("one three four"));

    QtcProcess::ArgIterator ait3(&in3);
    QVERIFY(ait3.next());
    ait3.appendArg("one-b");
    QVERIFY(ait3.next());
    QVERIFY(ait3.next());
    ait3.deleteArg();
    QVERIFY(!ait3.next());
    QCOMPARE(in3, QString::fromLatin1("one one-b two"));

    QtcProcess::ArgIterator ait4(&in4);
    ait4.appendArg("pre-one");
    QVERIFY(ait4.next());
    QVERIFY(ait4.next());
    QVERIFY(ait4.next());
    ait4.deleteArg();
    QVERIFY(!ait4.next());
    QCOMPARE(in4, QString::fromLatin1("pre-one one two"));

    QtcProcess::ArgIterator ait5(&in5);
    QVERIFY(ait5.next());
    QVERIFY(ait5.next());
    QVERIFY(ait5.next());
    QVERIFY(!ait5.next());
    ait5.deleteArg();
    QVERIFY(!ait5.next());
    QCOMPARE(in5, QString::fromLatin1("one two"));
}

QTEST_MAIN(tst_QtcProcess)

#include "tst_qtcprocess.moc"
