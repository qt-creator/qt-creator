// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/macroexpander.h>
#include <utils/templateengine.h>

#include <QtTest>

//TESTED_COMPONENT=src/libs/utils

class tst_TemplateEngine : public QObject
{
    Q_OBJECT

private slots:
    void testTemplateEngine_data();
    void testTemplateEngine();
};

void tst_TemplateEngine::testTemplateEngine_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedOutput");
    QTest::addColumn<QString>("expectedErrorMessage");
    QTest::newRow("if")
        << QString::fromLatin1("@if 1\nline 1\n@elsif 0\nline 2\n@else\nline 3\n@endif\n")
        << QString::fromLatin1("line 1\n")
        << QString();
    QTest::newRow("elsif")
        << QString::fromLatin1("@if 0\nline 1\n@elsif 1\nline 2\n@else\nline 3\n@endif\n")
        << QString::fromLatin1("line 2\n")
        << QString();
    QTest::newRow("else")
        << QString::fromLatin1("@if 0\nline 1\n@elsif 0\nline 2\n@else\nline 3\n@endif\n")
        << QString::fromLatin1("line 3\n")
        << QString();
    QTest::newRow("nested-if")
        << QString::fromLatin1("@if 1\n"
                               "  @if 1\nline 1\n@elsif 0\nline 2\n@else\nline 3\n@endif\n"
                               "@else\n"
                               "  @if 1\nline 4\n@elsif 0\nline 5\n@else\nline 6\n@endif\n"
                               "@endif\n")
        << QString::fromLatin1("line 1\n")
        << QString();
    QTest::newRow("nested-else")
        << QString::fromLatin1("@if 0\n"
                               "  @if 1\nline 1\n@elsif 0\nline 2\n@else\nline 3\n@endif\n"
                               "@else\n"
                               "  @if 1\nline 4\n@elsif 0\nline 5\n@else\nline 6\n@endif\n"
                               "@endif\n")
        << QString::fromLatin1("line 4\n")
        << QString();
    QTest::newRow("twice-nested-if")
        << QString::fromLatin1("@if 0\n"
                               "  @if 1\n"
                               "    @if 1\nline 1\n@else\nline 2\n@endif\n"
                               "  @endif\n"
                               "@else\n"
                               "  @if 1\n"
                               "    @if 1\nline 3\n@else\nline 4\n@endif\n"
                               "  @endif\n"
                               "@endif\n")
        << QString::fromLatin1("line 3\n")
        << QString();
}

void tst_TemplateEngine::testTemplateEngine()
{
    QFETCH(QString, input);
    QFETCH(QString, expectedOutput);
    QFETCH(QString, expectedErrorMessage);

    QString errorMessage;
    QString output = Utils::TemplateEngine::processText(Utils::globalMacroExpander(), input, &errorMessage);

    QCOMPARE(output, expectedOutput);
    QCOMPARE(errorMessage, expectedErrorMessage);
}

QTEST_GUILESS_MAIN(tst_TemplateEngine)

#include "tst_templateengine.moc"
