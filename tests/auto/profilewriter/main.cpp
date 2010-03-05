/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "profileevaluator.h"
#include "prowriter.h"

#include <QtTest/QtTest>
//#include <QtCore/QSet>

#define BASE_DIR "/some/stuff"

class tst_ProFileWriter : public QObject
{
    Q_OBJECT

private slots:
    void edit_data();
    void edit();
    void multiVar();
};

void tst_ProFileWriter::edit_data()
{
    QTest::addColumn<bool>("add");
    QTest::addColumn<QStringList>("files");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("output");

    struct Case {
        bool add;
        const char *title;
        const char * const *files;
        const char *input;
        const char *output;
    };

    static const char *f_foo[] = { "foo", 0 };
    static const char *f_foo_bar[] = { "foo", "bar", 0 };
    static const Case cases[] = {
        // Adding entries
        {
            true, "add new", f_foo,
            "# test file",
            "# test file\n"
            "\n"
            "SOURCES += \\\n"
            "    foo"
        },
        {
            true, "add new ignoring scoped", f_foo,
            "unix:SOURCES = some files",
            "unix:SOURCES = some files\n"
            "\n"
            "SOURCES += \\\n"
            "    foo"
        },
        {
            true, "add to existing", f_foo,
            "SOURCES = some files",
            "SOURCES = some files \\\n"
            "    foo"
        },
        {
            true, "add to existing after comment", f_foo,
            "SOURCES = some files   # comment",
            "SOURCES = some files \\   # comment\n"
            "    foo"
        },
        {
            true, "add to existing after comment line", f_foo,
            "SOURCES = some \\\n"
            "   # comment\n"
            "    files",
            "SOURCES = some \\\n"
            "   # comment\n"
            "    files \\\n"
            "    foo"
        },

        // Removing entries
        {
            false, "remove fail", f_foo,
            "SOURCES = bak bar",
            "SOURCES = bak bar"
        },
        {
            false, "remove one-line middle", f_foo,
            "SOURCES = bak foo bar",
            "SOURCES = bak bar"
        },
        {
            false, "remove one-line trailing", f_foo,
            "SOURCES = bak bar foo",
            "SOURCES = bak bar"
        },
        {
            false, "remove multi-line single leading", f_foo,
            "SOURCES = foo \\\n"
            "    bak \\\n"
            "    bar",
            "SOURCES = \\\n"
            "    bak \\\n"
            "    bar"
        },
        {
            false, "remove multi-line single middle", f_foo,
            "SOURCES = bak \\\n"
            "    foo \\\n"
            "    bar",
            "SOURCES = bak \\\n"
            "    bar"
        },
        {
            false, "remove multi-line single trailing", f_foo,
            "SOURCES = bak \\\n"
            "    bar \\\n"
            "    foo",
            "SOURCES = bak \\\n"
            "    bar"
        },
        {
            false, "remove multi-line single leading with comment", f_foo,
            "SOURCES = foo \\  # comment\n"
            "    bak \\\n"
            "    bar",
            "SOURCES = \\  # foo # comment\n"
            "    bak \\\n"
            "    bar"
        },
        {
            false, "remove multi-line single middle with comment", f_foo,
            "SOURCES = bak \\\n"
            "    foo \\  # comment\n"
            "    bar",
            "SOURCES = bak \\\n"
            "    \\  # foo # comment\n"
            "    bar"
        },
        {
            false, "remove multi-line single trailing with comment", f_foo,
            "SOURCES = bak \\\n"
            "    bar \\\n"
            "    foo  # comment",
            "SOURCES = bak \\\n"
            "    bar\n"
            "     # foo # comment"
        },
        {
            false, "remove multi-line single trailing after empty line", f_foo,
            "SOURCES = bak \\\n"
            "    bar \\\n"
            "    \\\n"
            "    foo",
            "SOURCES = bak \\\n"
            "    bar\n"
        },
        {
            false, "remove multi-line single trailing after comment line", f_foo,
            "SOURCES = bak \\\n"
            "    bar \\\n"
            "       # just a comment\n"
            "    foo",
            "SOURCES = bak \\\n"
            "    bar\n"
            "       # just a comment"
        },
        {
            false, "remove multi-line single trailing after empty line with comment", f_foo,
            "SOURCES = bak \\\n"
            "    bar \\\n"
            "    \\ # just a comment\n"
            "    foo",
            "SOURCES = bak \\\n"
            "    bar\n"
            "     # just a comment"
        },
        {
            false, "remove multiple one-line middle", f_foo_bar,
            "SOURCES = bak foo bar baz",
            "SOURCES = bak baz"
        },
        {
            false, "remove multiple one-line trailing", f_foo_bar,
            "SOURCES = bak baz foo bar",
            "SOURCES = bak baz"
        },
        {
            false, "remove multiple one-line interleaved", f_foo_bar,
            "SOURCES = bak foo baz bar",
            "SOURCES = bak baz"
        },
        {
            false, "remove multiple one-line middle with comment", f_foo_bar,
            "SOURCES = bak foo bar baz   # comment",
            "SOURCES = bak baz   # bar # foo # comment"
        },
        {
            false, "remove multi-line multiple trailing with empty line with comment", f_foo_bar,
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
        QStringList files;
        for (const char * const *file = _case->files; *file; file++)
            files << QString::fromLatin1(BASE_DIR "/") + QString::fromLatin1(*file);
        QTest::newRow(_case->title)
                << _case->add
                << files
                << QString::fromLatin1(_case->input)
                << QString::fromLatin1(_case->output);
    }
}

void tst_ProFileWriter::edit()
{
    QFETCH(bool, add);
    QFETCH(QStringList, files);
    QFETCH(QString, input);
    QFETCH(QString, output);

    QDir baseDir(BASE_DIR);
    QStringList lines = input.split(QLatin1String("\n"));
    QStringList vars; vars << QLatin1String("SOURCES");

    ProFileOption option;
    ProFileEvaluator reader(&option);
    ProFile *proFile = reader.parsedProFile(BASE_DIR "/test.pro", input);
    QVERIFY(proFile);
    if (add)
        Qt4ProjectManager::Internal::ProWriter::addFiles(proFile, &lines, baseDir, files, vars);
    else
        Qt4ProjectManager::Internal::ProWriter::removeFiles(proFile, &lines, baseDir, files, vars);

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

    ProFileOption option;
    ProFileEvaluator reader(&option);
    ProFile *proFile = reader.parsedProFile(BASE_DIR "/test.pro", input);
    QVERIFY(proFile);
    Qt4ProjectManager::Internal::ProWriter::removeFiles(proFile, &lines, baseDir, files, vars);

    QCOMPARE(lines.join(QLatin1String("\n")), output);
}

QTEST_MAIN(tst_ProFileWriter)
#include "main.moc"
