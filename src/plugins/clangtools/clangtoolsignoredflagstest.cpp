// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolsignoredflagstest.h"

#include "clangtoolsutils.h"

#include <cppeditor/compileroptionsbuilder.h>

#include <QTest>

namespace ClangTools::Internal {

void IgnoredFlagsTest::testFindUnknownCompilerFlags()
{
    QFETCH(QString, output);
    QFETCH(QStringList, expected);

    QCOMPARE(findUnknownCompilerFlags(output), expected);
}

void IgnoredFlagsTest::testFindUnknownCompilerFlags_data()
{
    QTest::addColumn<QString>("output");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("unknown-argument")
        << "error: unknown argument: '-fipa-pta' [clang-diagnostic-error]\n"
        << QStringList{"-fipa-pta"};
    QTest::newRow("unsupported-argument")
        << "error: unsupported argument: '-fcoroutines'\n" << QStringList{"-fcoroutines"};
    QTest::newRow("unsupported-option-value")
        << "error: unsupported argument 'auto' to option '-flto='\n"
        << QStringList{"-flto=*auto*"};
    QTest::newRow("unsupported-option-for-target")
        << "error: unsupported option '-mfoo' for target 'x86_64-pc-linux-gnu'\n"
        << QStringList{"-mfoo"};
    QTest::newRow("unsupported-option-with-suggestion")
        << "clang: error: unsupported option '-mfpu=neon'; did you mean '-mfpu=neon-vfpv4'?\n"
        << QStringList{"-mfpu=neon"};
    QTest::newRow("option-name-without-dash")
        << "clang: error: unsupported argument 'auto' to option 'flto='\n"
        << QStringList{"-flto=*auto*"};
    QTest::newRow("option-value-in-a-list")
        << "error: unsupported argument 'bounds-strict' to option '-fsanitize='\n"
        << QStringList{"-fsanitize=*bounds-strict*"};
    QTest::newRow("wildcards-are-escaped")
        << "error: unknown argument: '-fno-foo*'\n"
           "error: unsupported argument 'a?' to option '-fx[='\n"
        << QStringList{"-fno-foo[*]", "-fx[[]=*a[?]*"};
    QTest::newRow("quoted-source-line")
        << "/src/foo.cpp:12:9: warning: something [some-check]\n"
           "   12 |     << \"error: unknown argument: '-fipa-pta' [clang-diagnostic-error]\\n\"\n"
           "      |        ^\n"
        << QStringList();
    QTest::newRow("program-prefix-and-quoted-source-line")
        << "clazy-standalone: error: unknown argument: '-fno-ipa-pta'\n"
           "   12 |     // error: unsupported argument 'auto' to option '-flto='\n"
        << QStringList{"-fno-ipa-pta"};
    QTest::newRow("flag-with-whitespace")
        << "error: unknown argument: '-Xfoo bar'\n"
           "error: unsupported argument 'a b' to option '-fx='\n"
        << QStringList();
    QTest::newRow("flag-with-markup")
        << "error: unknown argument: '-D<b>x</b>'\n" << QStringList();
    QTest::newRow("duplicates")
        << "error: unknown argument: '-fipa-pta'\n"
           "error: unknown argument: '-fipa-pta'\n"
        << QStringList{"-fipa-pta"};
    QTest::newRow("sorted")
        << "error: unknown argument: '-mno-direct-extern-access'\n"
           "error: unknown argument: '-fipa-pta'\n"
           "error: unsupported argument 'auto' to option '-flto='\n"
        << QStringList{"-fipa-pta", "-flto=*auto*", "-mno-direct-extern-access"};
    QTest::newRow("other-errors") << "main.cpp:1:10: error: 'missing.h' file not found\n"
                                  << QStringList();
    QTest::newRow("empty") << QString() << QStringList();
}

void IgnoredFlagsTest::testFoundFlagsMatchThemselves()
{
    QFETCH(QString, output);
    QFETCH(QString, flag);
    QFETCH(QString, other);

    const QStringList found = findUnknownCompilerFlags(output);
    QVERIFY(!found.isEmpty());
    const QList<QRegularExpression> patterns
        = CppEditor::CompilerOptionsBuilder::wildcardsToRegularExpressions(found);
    QCOMPARE(patterns.size(), found.size());
    QVERIFY2(patterns.constFirst().match(flag).hasMatch(), qPrintable(found.constFirst()));
    QVERIFY2(!patterns.constFirst().match(other).hasMatch(), qPrintable(found.constFirst()));
}

void IgnoredFlagsTest::testFoundFlagsMatchThemselves_data()
{
    QTest::addColumn<QString>("output");
    QTest::addColumn<QString>("flag");
    QTest::addColumn<QString>("other");

    QTest::newRow("plain") << "error: unknown argument: '-fipa-pta'\n" << "-fipa-pta"
                           << "-fipa-ptax";
    QTest::newRow("star") << "error: unknown argument: '-fno-foo*'\n" << "-fno-foo*"
                          << "-fno-foobar";
    QTest::newRow("question-mark") << "error: unknown argument: '-fa?'\n" << "-fa?" << "-fab";
    QTest::newRow("bracket") << "error: unknown argument: '-fx[a]'\n" << "-fx[a]" << "-fxa";
    QTest::newRow("backslash") << "error: unknown argument: '-fplugin=C:\\x\\y.dll'\n"
                               << "-fplugin=C:\\x\\y.dll" << "-fplugin=C:/x/y.dll";
    QTest::newRow("option-value") << "error: unsupported argument 'auto' to option '-flto='\n"
                                  << "-flto=auto" << "-flto=none";
}

} // namespace ClangTools::Internal
