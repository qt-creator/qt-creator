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

#include "googletest.h"

#include <clangtranslationunit.h>
#include <clangtranslationunitupdater.h>
#include <diagnosticcontainer.h>
#include <utf8string.h>

#include <clang-c/Index.h>

using ClangBackEnd::DiagnosticContainer;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::TranslationUnitUpdateInput;
using ClangBackEnd::TranslationUnitUpdateResult;

using testing::ContainerEq;
using testing::Eq;

namespace {

class TranslationUnit : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    void parse();
    void reparse();

    DiagnosticContainer createDiagnostic(const QString &text,
                                         ClangBackEnd::DiagnosticSeverity severity,
                                         uint line,
                                         uint column,
                                         const QString &filePath) const;
    QVector<DiagnosticContainer> diagnosticsFromMainFile() const;
    QVector<DiagnosticContainer> errorDiagnosticsFromHeaders() const;

protected:
    Utf8String sourceFilePath = Utf8StringLiteral(TESTDATA_DIR"/diagnostic_erroneous_source.cpp");
    Utf8String headerFilePath = Utf8StringLiteral(TESTDATA_DIR"/diagnostic_erroneous_header.h");
    CXIndex cxIndex = nullptr;
    CXTranslationUnit cxTranslationUnit = nullptr;
    ::TranslationUnit translationUnit{Utf8String(), sourceFilePath, cxIndex, cxTranslationUnit};

    DiagnosticContainer extractedFirstHeaderErrorDiagnostic;
    QVector<DiagnosticContainer> extractedMainFileDiagnostics;
};

using TranslationUnitSlowTest = TranslationUnit;

TEST_F(TranslationUnitSlowTest, HasExpectedMainFileDiagnostics)
{
    translationUnit.extractDiagnostics(extractedFirstHeaderErrorDiagnostic,
                                       extractedMainFileDiagnostics);

    ASSERT_THAT(extractedMainFileDiagnostics, ContainerEq(diagnosticsFromMainFile()));
}

TEST_F(TranslationUnitSlowTest, HasExpectedMainFileDiagnosticsAfterReparse)
{
    reparse();

    translationUnit.extractDiagnostics(extractedFirstHeaderErrorDiagnostic,
                                       extractedMainFileDiagnostics);

    ASSERT_THAT(extractedMainFileDiagnostics, ContainerEq(diagnosticsFromMainFile()));
}

TEST_F(TranslationUnitSlowTest, HasErrorDiagnosticsInHeaders)
{
    translationUnit.extractDiagnostics(extractedFirstHeaderErrorDiagnostic,
                                       extractedMainFileDiagnostics);

    ASSERT_THAT(extractedFirstHeaderErrorDiagnostic,
                Eq(errorDiagnosticsFromHeaders().first()));
}

TEST_F(TranslationUnitSlowTest, HasErrorDiagnosticsInHeadersAfterReparse)
{
    reparse();

    translationUnit.extractDiagnostics(extractedFirstHeaderErrorDiagnostic,
                                       extractedMainFileDiagnostics);

    ASSERT_THAT(extractedFirstHeaderErrorDiagnostic,
                Eq(errorDiagnosticsFromHeaders().first()));
}

void TranslationUnit::SetUp()
{
    parse();
}

void TranslationUnit::TearDown()
{
    clang_disposeTranslationUnit(cxTranslationUnit);
    clang_disposeIndex(cxIndex);
}

void TranslationUnit::parse()
{
    TranslationUnitUpdateInput parseInput;
    parseInput.filePath = sourceFilePath;
    parseInput.parseNeeded = true;
    const TranslationUnitUpdateResult parseResult = translationUnit.update(parseInput);
    ASSERT_TRUE(parseResult.hasParsed());
}

void TranslationUnit::reparse()
{
    TranslationUnitUpdateInput parseInput;
    parseInput.filePath = sourceFilePath;
    parseInput.reparseNeeded = true;
    const TranslationUnitUpdateResult parseResult = translationUnit.update(parseInput);
    ASSERT_TRUE(parseResult.hasReparsed());
}

DiagnosticContainer TranslationUnit::createDiagnostic(const QString &text,
                                                      ClangBackEnd::DiagnosticSeverity severity,
                                                      uint line,
                                                      uint column,
                                                      const QString &filePath) const
{
    return DiagnosticContainer(
            text,
            Utf8StringLiteral(""),
            {},
            severity,
            {filePath, line, column},
            {},
            {},
            {}
    );
}

QVector<ClangBackEnd::DiagnosticContainer> TranslationUnit::diagnosticsFromMainFile() const
{
    return {
        createDiagnostic(
            QStringLiteral("warning: enumeration value 'Three' not handled in switch"),
            ClangBackEnd::DiagnosticSeverity::Warning,
            7,
            13,
            sourceFilePath),
        createDiagnostic(
            QStringLiteral("error: void function 'g' should not return a value"),
            ClangBackEnd::DiagnosticSeverity::Error,
            15,
            5,
            sourceFilePath),
        createDiagnostic(
            QStringLiteral("warning: using the result of an assignment as a condition without parentheses"),
            ClangBackEnd::DiagnosticSeverity::Warning,
            21,
            11,
            sourceFilePath),
    };
}

QVector<ClangBackEnd::DiagnosticContainer> TranslationUnit::errorDiagnosticsFromHeaders() const
{
    return {
        createDiagnostic(
            QStringLiteral("error: C++ requires a type specifier for all declarations"),
            ClangBackEnd::DiagnosticSeverity::Error,
            11,
            1,
            headerFilePath),
    };
}

} // anonymous namespace
