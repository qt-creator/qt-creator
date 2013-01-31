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

#include <qmakeparser.h>
#include <prowriter.h>

#include <QtTest>

#define BASE_DIR "/some/stuff"

///////////// callbacks for parser/evaluator

static void print(const QString &fileName, int lineNo, const QString &msg)
{
    if (lineNo > 0)
        qWarning("%s(%d): %s", qPrintable(fileName), lineNo, qPrintable(msg));
    else if (lineNo)
        qWarning("%s: %s", qPrintable(fileName), qPrintable(msg));
    else
        qWarning("%s", qPrintable(msg));
}

class ParseHandler : public QMakeParserHandler {
public:
    virtual void message(int /* type */, const QString &msg, const QString &fileName, int lineNo)
        { print(fileName, lineNo, msg); }
};

static ParseHandler parseHandler;

//////////////// the actual autotest

typedef Qt4ProjectManager::Internal::ProWriter PW;

class tst_ProFileWriter : public QObject
{
    Q_OBJECT

private slots:
    void adds_data();
    void adds();
    void removes_data();
    void removes();
    void multiVar();
    void addFiles();
    void removeFiles();
};

static QStringList strList(const char * const *array)
{
    QStringList values;
    for (const char * const *value = array; *value; value++)
        values << QString::fromLatin1(*value);
    return values;
}

void tst_ProFileWriter::adds_data()
{
    QTest::addColumn<int>("flags");
    QTest::addColumn<QStringList>("values");
    QTest::addColumn<QString>("scope");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("output");

    struct Case {
        int flags;
        const char *title;
        const char * const *values;
        const char *scope;
        const char *input;
        const char *output;
    };

    static const char *f_foo[] = { "foo", 0 };
    static const char *f_foo_bar[] = { "foo", "bar", 0 };
    static const Case cases[] = {
        {
            PW::AppendValues|PW::AppendOperator|PW::MultiLine,
            "add new append multi", f_foo, 0,
            "",
            "SOURCES += \\\n"
            "    foo"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::MultiLine,
            "add new append multi after comment", f_foo, 0,
            "# test file",
            "# test file\n"
            "\n"
            "SOURCES += \\\n"
            "    foo"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::MultiLine,
            "add new append multi before newlines", f_foo, 0,
            "\n"
            "\n"
            "\n",
            "SOURCES += \\\n"
            "    foo\n"
            "\n"
            "\n"
            "\n"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::MultiLine,
            "add new append multi after comment before newlines", f_foo, 0,
            "# test file\n"
            "\n"
            "\n"
            "\n",
            "# test file\n"
            "\n"
            "SOURCES += \\\n"
            "    foo\n"
            "\n"
            "\n"
            "\n"
        },
        {
            PW::AppendValues|PW::AssignOperator|PW::MultiLine,
            "add new assign multi", f_foo, 0,
            "# test file",
            "# test file\n"
            "\n"
            "SOURCES = \\\n"
            "    foo"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::OneLine,
            "add new append oneline", f_foo, 0,
            "# test file",
            "# test file\n"
            "\n"
            "SOURCES += foo"
        },
        {
            PW::AppendValues|PW::AssignOperator|PW::OneLine,
            "add new assign oneline", f_foo, 0,
            "# test file",
            "# test file\n"
            "\n"
            "SOURCES = foo"
        },
        {
            PW::AppendValues|PW::AssignOperator|PW::OneLine,
            "add new assign oneline after existing", f_foo, 0,
            "# test file\n"
            "\n"
            "HEADERS = foo",
            "# test file\n"
            "\n"
            "HEADERS = foo\n"
            "\n"
            "SOURCES = foo"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::MultiLine,
            "add new ignoring scoped", f_foo, 0,
            "unix:SOURCES = some files",
            "unix:SOURCES = some files\n"
            "\n"
            "SOURCES += \\\n"
            "    foo"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::MultiLine,
            "add new after some scope", f_foo, 0,
            "unix {\n"
            "    SOMEVAR = foo\n"
            "}",
            "unix {\n"
            "    SOMEVAR = foo\n"
            "}\n"
            "\n"
            "SOURCES += \\\n"
            "    foo"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::MultiLine,
            "add to existing (wrong operator)", f_foo, 0,
            "SOURCES = some files",
            "SOURCES = some files \\\n"
            "    foo"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::MultiLine,
            "add to existing after comment (wrong operator)", f_foo, 0,
            "SOURCES = some files   # comment",
            "SOURCES = some files \\   # comment\n"
            "    foo"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::MultiLine,
            "add to existing after comment line (wrong operator)", f_foo, 0,
            "SOURCES = some \\\n"
            "   # comment\n"
            "    files",
            "SOURCES = some \\\n"
            "   # comment\n"
            "    files \\\n"
            "    foo"
        },
        {
            PW::AppendValues|PW::AssignOperator|PW::MultiLine,
            "add to existing", f_foo, 0,
            "SOURCES = some files",
            "SOURCES = some files \\\n"
            "    foo"
        },
        {
            PW::ReplaceValues|PW::AssignOperator|PW::MultiLine,
            "replace existing multi", f_foo_bar, 0,
            "SOURCES = some files",
            "SOURCES = \\\n"
            "    foo \\\n"
            "    bar"
        },
        {
            PW::ReplaceValues|PW::AssignOperator|PW::OneLine,
            "replace existing oneline", f_foo_bar, 0,
            "SOURCES = some files",
            "SOURCES = foo bar"
        },
        {
            PW::ReplaceValues|PW::AssignOperator|PW::OneLine,
            "replace existing complex last", f_foo_bar, 0,
            "SOURCES = some \\\n"
            "   # comment\n"
            "    files",
            "SOURCES = foo bar"
        },
        {
            PW::ReplaceValues|PW::AssignOperator|PW::OneLine,
            "replace existing complex middle 1", f_foo_bar, 0,
            "SOURCES = some \\\n"
            "   # comment\n"
            "    files\n"
            "HEADERS = blubb",
            "SOURCES = foo bar\n"
            "HEADERS = blubb"
        },
        {
            PW::ReplaceValues|PW::AssignOperator|PW::OneLine,
            "replace existing complex middle 2", f_foo_bar, 0,
            "SOURCES = some \\\n"
            "   # comment\n"
            "    files\n"
            "\n"
            "HEADERS = blubb",
            "SOURCES = foo bar\n"
            "\n"
            "HEADERS = blubb"
        },
        {
            PW::ReplaceValues|PW::AssignOperator|PW::OneLine,
            "replace existing complex middle 3", f_foo_bar, 0,
            "SOURCES = some \\\n"
            "   # comment\n"
            "    files \\\n"
            "\n"
            "HEADERS = blubb",
            "SOURCES = foo bar\n"
            "\n"
            "HEADERS = blubb"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::OneLine,
            "scoped new / new scope", f_foo, "dog",
            "# test file\n"
            "SOURCES = yo",
            "# test file\n"
            "SOURCES = yo\n"
            "\n"
            "dog {\n"
            "    SOURCES += foo\n"
            "}"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::OneLine,
            "scoped new / extend scope", f_foo, "dog",
            "# test file\n"
            "dog {\n"
            "    HEADERS += yo\n"
            "}",
            "# test file\n"
            "dog {\n"
            "    HEADERS += yo\n"
            "\n"
            "    SOURCES += foo\n"
            "}"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::OneLine,
            "scoped new / extend elongated scope", f_foo, "dog",
            "# test file\n"
            "dog {\n"
            "    HEADERS += \\\n"
            "        yo \\\n"
            "        blubb\n"
            "}",
            "# test file\n"
            "dog {\n"
            "    HEADERS += \\\n"
            "        yo \\\n"
            "        blubb\n"
            "\n"
            "    SOURCES += foo\n"
            "}"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::OneLine,
            "scoped new / extend empty scope", f_foo, "dog",
            "# test file\n"
            "dog {\n"
            "}",
            "# test file\n"
            "dog {\n"
            "    SOURCES += foo\n"
            "}"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::OneLine,
            "scoped new / extend oneline scope", f_foo, "dog",
            "# test file\n"
            "dog:HEADERS += yo",
            "# test file\n"
            "dog {\n"
            "    HEADERS += yo\n"
            "\n"
            "    SOURCES += foo\n"
            "}"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::OneLine,
            "scoped new / extend oneline scope with multiline body", f_foo, "dog",
            "# test file\n"
            "dog:HEADERS += yo \\\n"
            "        you\n"
            "\n"
            "blubb()",
            "# test file\n"
            "dog {\n"
            "    HEADERS += yo \\\n"
            "        you\n"
            "\n"
            "    SOURCES += foo\n"
            "}\n"
            "\n"
            "blubb()"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::MultiLine,
            "add new after some scope inside scope", f_foo, "dog",
            "dog {\n"
            "    unix {\n"
            "        SOMEVAR = foo\n"
            "    }\n"
            "}",
            "dog {\n"
            "    unix {\n"
            "        SOMEVAR = foo\n"
            "    }\n"
            "\n"
            "    SOURCES += \\\n"
            "        foo\n"
            "}"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::OneLine,
            "scoped new / pseudo-oneline-scope", f_foo, "dog",
            "# test file\n"
            "dog: {\n"
            "}",
            "# test file\n"
            "dog: {\n"
            "    SOURCES += foo\n"
            "}"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::MultiLine,
            "scoped append", f_foo, "dog",
            "# test file\n"
            "dog:SOURCES = yo",
            "# test file\n"
            "dog:SOURCES = yo \\\n"
            "        foo"
        },
        {
            PW::AppendValues|PW::AppendOperator|PW::MultiLine,
            "complex scoped append", f_foo, "dog",
            "# test file\n"
            "animal:!dog:SOURCES = yo",
            "# test file\n"
            "animal:!dog:SOURCES = yo\n"
            "\n"
            "dog {\n"
            "    SOURCES += \\\n"
            "        foo\n"
            "}"
        },
    };

    for (uint i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const Case *_case = &cases[i];
        QTest::newRow(_case->title)
                << _case->flags
                << strList(_case->values)
                << QString::fromLatin1(_case->scope)
                << QString::fromLatin1(_case->input)
                << QString::fromLatin1(_case->output);
    }
}

void tst_ProFileWriter::adds()
{
    QFETCH(int, flags);
    QFETCH(QStringList, values);
    QFETCH(QString, scope);
    QFETCH(QString, input);
    QFETCH(QString, output);

    QStringList lines = input.isEmpty() ? QStringList() : input.split(QLatin1String("\n"));
    QString var = QLatin1String("SOURCES");

    QMakeParser parser(0, &parseHandler);
    ProFile *proFile = parser.parsedProBlock(QLatin1String(BASE_DIR "/test.pro"), input);
    QVERIFY(proFile);
    PW::putVarValues(proFile, &lines, values, var, PW::PutFlags(flags), scope);
    proFile->deref();

    QCOMPARE(lines.join(QLatin1String("\n")), output);
}

void tst_ProFileWriter::removes_data()
{
    QTest::addColumn<QStringList>("values");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("output");

    struct Case {
        const char *title;
        const char * const *values;
        const char *input;
        const char *output;
    };

    static const char *f_foo[] = { "foo", 0 };
    static const char *f_foo_bar[] = { "foo", "bar", 0 };
    static const Case cases[] = {
        {
            "remove fail", f_foo,
            "SOURCES = bak bar",
            "SOURCES = bak bar"
        },
        {
            "remove one-line middle", f_foo,
            "SOURCES = bak foo bar",
            "SOURCES = bak bar"
        },
        {
            "remove one-line trailing", f_foo,
            "SOURCES = bak bar foo",
            "SOURCES = bak bar"
        },
        {
            "remove multi-line single leading", f_foo,
            "SOURCES = foo \\\n"
            "    bak \\\n"
            "    bar",
            "SOURCES = \\\n"
            "    bak \\\n"
            "    bar"
        },
        {
            "remove multi-line single middle", f_foo,
            "SOURCES = bak \\\n"
            "    foo \\\n"
            "    bar",
            "SOURCES = bak \\\n"
            "    bar"
        },
        {
            "remove multi-line single trailing", f_foo,
            "SOURCES = bak \\\n"
            "    bar \\\n"
            "    foo",
            "SOURCES = bak \\\n"
            "    bar"
        },
        {
            "remove multi-line single leading with comment", f_foo,
            "SOURCES = foo \\  # comment\n"
            "    bak \\\n"
            "    bar",
            "SOURCES = \\  # foo # comment\n"
            "    bak \\\n"
            "    bar"
        },
        {
            "remove multi-line single middle with comment", f_foo,
            "SOURCES = bak \\\n"
            "    foo \\  # comment\n"
            "    bar",
            "SOURCES = bak \\\n"
            "    \\  # foo # comment\n"
            "    bar"
        },
        {
            "remove multi-line single trailing with comment", f_foo,
            "SOURCES = bak \\\n"
            "    bar \\\n"
            "    foo  # comment",
            "SOURCES = bak \\\n"
            "    bar\n"
            "     # foo # comment"
        },
        {
            "remove multi-line single trailing after empty line", f_foo,
            "SOURCES = bak \\\n"
            "    bar \\\n"
            "    \\\n"
            "    foo",
            "SOURCES = bak \\\n"
            "    bar\n"
        },
        {
            "remove multi-line single trailing after comment line", f_foo,
            "SOURCES = bak \\\n"
            "    bar \\\n"
            "       # just a comment\n"
            "    foo",
            "SOURCES = bak \\\n"
            "    bar\n"
            "       # just a comment"
        },
        {
            "remove multi-line single trailing after empty line with comment", f_foo,
            "SOURCES = bak \\\n"
            "    bar \\\n"
            "    \\ # just a comment\n"
            "    foo",
            "SOURCES = bak \\\n"
            "    bar\n"
            "     # just a comment"
        },
        {
            "remove multiple one-line middle", f_foo_bar,
            "SOURCES = bak foo bar baz",
            "SOURCES = bak baz"
        },
        {
            "remove multiple one-line trailing", f_foo_bar,
            "SOURCES = bak baz foo bar",
            "SOURCES = bak baz"
        },
        {
            "remove multiple one-line interleaved", f_foo_bar,
            "SOURCES = bak foo baz bar",
            "SOURCES = bak baz"
        },
        {
            "remove multiple one-line middle with comment", f_foo_bar,
            "SOURCES = bak foo bar baz   # comment",
            "SOURCES = bak baz   # bar # foo # comment"
        },
        {
            "remove multi-line multiple trailing with empty line with comment", f_foo_bar,
            "SOURCES = bak \\\n"
            "    bar \\\n"
            "    \\ # just a comment\n"
            "    foo",
            "SOURCES = bak\n"
            "     # just a comment"
        },
    };

    for (uint i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const Case *_case = &cases[i];
        QTest::newRow(_case->title)
                << strList(_case->values)
                << QString::fromLatin1(_case->input)
                << QString::fromLatin1(_case->output);
    }
}

void tst_ProFileWriter::removes()
{
    QFETCH(QStringList, values);
    QFETCH(QString, input);
    QFETCH(QString, output);

    QStringList lines = input.split(QLatin1String("\n"));
    QStringList vars; vars << QLatin1String("SOURCES");

    QMakeParser parser(0, &parseHandler);
    ProFile *proFile = parser.parsedProBlock(QLatin1String(BASE_DIR "/test.pro"), input);
    QVERIFY(proFile);
    Qt4ProjectManager::Internal::ProWriter::removeVarValues(proFile, &lines, values, vars);
    proFile->deref();

    QCOMPARE(lines.join(QLatin1String("\n")), output);
}

void tst_ProFileWriter::multiVar()
{
    QDir baseDir(BASE_DIR);
    QString input = QLatin1String(
            "SOURCES = foo bar\n"
            "# comment line\n"
            "HEADERS = baz bak"
            );
    QStringList lines = input.split(QLatin1String("\n"));
    QString output = QLatin1String(
            "SOURCES = bar\n"
            "# comment line\n"
            "HEADERS = baz"
            );
    QStringList files; files
            << QString::fromLatin1(BASE_DIR "/foo")
            << QString::fromLatin1(BASE_DIR "/bak");
    QStringList vars; vars << QLatin1String("SOURCES") << QLatin1String("HEADERS");

    QMakeParser parser(0, &parseHandler);
    ProFile *proFile = parser.parsedProBlock(QLatin1String(BASE_DIR "/test.pro"), input);
    QVERIFY(proFile);
    Qt4ProjectManager::Internal::ProWriter::removeFiles(proFile, &lines, baseDir, files, vars);
    proFile->deref();

    QCOMPARE(lines.join(QLatin1String("\n")), output);
}

void tst_ProFileWriter::addFiles()
{
    QString input = QLatin1String(
            "SOURCES = foo.cpp"
            );
    QStringList lines = input.split(QLatin1String("\n"));
    QString output = QLatin1String(
            "SOURCES = foo.cpp \\\n"
            "    sub/bar.cpp"
            );

    QMakeParser parser(0, &parseHandler);
    ProFile *proFile = parser.parsedProBlock(QLatin1String(BASE_DIR "/test.pro"), input);
    QVERIFY(proFile);
    Qt4ProjectManager::Internal::ProWriter::addFiles(proFile, &lines, QDir(BASE_DIR),
            QStringList() << QString::fromLatin1(BASE_DIR "/sub/bar.cpp"),
            QLatin1String("SOURCES"));
    proFile->deref();

    QCOMPARE(lines.join(QLatin1String("\n")), output);
}

void tst_ProFileWriter::removeFiles()
{
    QString input = QLatin1String(
            "SOURCES = foo.cpp sub/bar.cpp"
            );
    QStringList lines = input.split(QLatin1String("\n"));
    QString output = QLatin1String(
            "SOURCES = foo.cpp"
            );

    QMakeParser parser(0, &parseHandler);
    ProFile *proFile = parser.parsedProBlock(QLatin1String(BASE_DIR "/test.pro"), input);
    QVERIFY(proFile);
    Qt4ProjectManager::Internal::ProWriter::removeFiles(proFile, &lines, QDir(BASE_DIR),
            QStringList() << QString::fromLatin1(BASE_DIR "/sub/bar.cpp"),
            QStringList() << QLatin1String("SOURCES") << QLatin1String("HEADERS"));
    proFile->deref();

    QCOMPARE(lines.join(QLatin1String("\n")), output);
}


QTEST_MAIN(tst_ProFileWriter)
#include "tst_profilewriter.moc"
