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

#include <clangdiagnosticfilter.h>
#include <diagnosticcontainer.h>
#include <fixitcontainer.h>

namespace {

using ::testing::Contains;
using ::testing::Not;

using ClangBackEnd::DiagnosticContainer;
using ClangBackEnd::FixItContainer;
using ClangBackEnd::SourceLocationContainer;
using ClangBackEnd::SourceRangeContainer;

DiagnosticContainer createDiagnostic(const QString &text,
                                     ClangBackEnd::DiagnosticSeverity severity,
                                     const QString &filePath,
                                     QVector<DiagnosticContainer> children = QVector<DiagnosticContainer>(),
                                     bool addFixItContainer = false)
{
    QVector<FixItContainer> fixIts;
    if (addFixItContainer)
        fixIts.append(FixItContainer(Utf8StringLiteral("("), {{filePath, 1, 1}, {filePath, 1, 2}}));

    return DiagnosticContainer(
            text,
            Utf8StringLiteral(""),
            {},
            severity,
            {filePath, 0u, 0u},
            {},
            fixIts,
            children
    );
}

const QString mainFileHeaderPath = QString::fromUtf8(TESTDATA_DIR "/someHeader.h");
const QString mainFilePath = QString::fromUtf8(TESTDATA_DIR "/diagnostic_erroneous_source.cpp");
const QString headerFilePath = QString::fromUtf8(TESTDATA_DIR "/diagnostic_erroneous_header.cpp");

DiagnosticContainer warningFromHeader()
{
    return createDiagnostic(
                QStringLiteral("warning: control reaches end of non-void function"),
                ClangBackEnd::DiagnosticSeverity::Warning,
                headerFilePath);
}

DiagnosticContainer errorFromHeader()
{
    return createDiagnostic(
                QStringLiteral("C++ requires a type specifier for all declarations"),
                ClangBackEnd::DiagnosticSeverity::Error,
                headerFilePath);
}

DiagnosticContainer warningFromMainFile()
{
    return createDiagnostic(
                QStringLiteral("warning: enumeration value 'Three' not handled in switch"),
                ClangBackEnd::DiagnosticSeverity::Warning,
                mainFilePath);
}

DiagnosticContainer pragmaOnceWarningInHeader()
{
    return createDiagnostic(
                QStringLiteral("warning: #pragma once in main file"),
                ClangBackEnd::DiagnosticSeverity::Warning,
                mainFileHeaderPath);
}

DiagnosticContainer includeNextInPrimarySourceFileWarningInHeader()
{
    return createDiagnostic(
                QStringLiteral("warning: #include_next in primary source file"),
                ClangBackEnd::DiagnosticSeverity::Warning,
                mainFileHeaderPath);
}

DiagnosticContainer errorFromMainFile()
{
    return createDiagnostic(
                QStringLiteral("error: void function 'g' should not return a value"),
                ClangBackEnd::DiagnosticSeverity::Error,
                mainFilePath);
}

DiagnosticContainer fixIt1(const QString &filePath)
{
    return createDiagnostic(
                QStringLiteral("note: place parentheses around the assignment to silence this warning"),
                ClangBackEnd::DiagnosticSeverity::Note,
                filePath);
}

DiagnosticContainer fixIt2(const QString &filePath)
{
    return createDiagnostic(
                QStringLiteral("note: use '==' to turn this assignment into an equality comparison"),
                ClangBackEnd::DiagnosticSeverity::Note,
                filePath);
}

DiagnosticContainer createDiagnosticWithFixIt(const QString &filePath)
{
    return createDiagnostic(
                QStringLiteral("warning: using the result of an assignment as a condition without parentheses"),
                ClangBackEnd::DiagnosticSeverity::Warning,
                filePath,
                {fixIt1(filePath), fixIt2(filePath)},
                true);
}

DiagnosticContainer warningFromMainFileWithFixits()
{
    return createDiagnosticWithFixIt(mainFilePath);
}

DiagnosticContainer warningFromHeaderFileWithFixits()
{
    return createDiagnosticWithFixIt(headerFilePath);
}

class ClangDiagnosticFilter : public ::testing::Test
{
protected:
    ClangCodeModel::Internal::ClangDiagnosticFilter clangDiagnosticFilter{mainFilePath};
};

TEST_F(ClangDiagnosticFilter, WarningsFromMainFileAreLetThrough)
{
    clangDiagnosticFilter.filter({warningFromMainFile()});

    ASSERT_THAT(clangDiagnosticFilter.takeWarnings(),Contains(warningFromMainFile()));
}

TEST_F(ClangDiagnosticFilter, WarningsFromHeaderAreIgnored)
{
    clangDiagnosticFilter.filter({warningFromHeader()});

    ASSERT_THAT(clangDiagnosticFilter.takeWarnings(), Not(Contains(warningFromHeader())));
}

TEST_F(ClangDiagnosticFilter, ErrorsFromMainFileAreLetThrough)
{
    clangDiagnosticFilter.filter({errorFromMainFile()});

    ASSERT_THAT(clangDiagnosticFilter.takeErrors(), Contains(errorFromMainFile()));
}

TEST_F(ClangDiagnosticFilter, ErrorsFromHeaderAreIgnored)
{
    clangDiagnosticFilter.filter({errorFromHeader()});

    ASSERT_THAT(clangDiagnosticFilter.takeErrors(), Not(Contains(errorFromHeader())));
}

TEST_F(ClangDiagnosticFilter, FixItsFromMainFileAreLetThrough)
{
    clangDiagnosticFilter.filter({warningFromMainFileWithFixits()});

    ASSERT_THAT(clangDiagnosticFilter.takeFixIts(), Contains(warningFromMainFileWithFixits()));
}

TEST_F(ClangDiagnosticFilter, FixItsFromHeaderAreIgnored)
{
    clangDiagnosticFilter.filter({warningFromHeaderFileWithFixits()});

    ASSERT_THAT(clangDiagnosticFilter.takeFixIts(),
                Not(Contains(warningFromHeaderFileWithFixits())));
}

TEST_F(ClangDiagnosticFilter, WarningsAreEmptyAfterTaking)
{
    clangDiagnosticFilter.filter({warningFromMainFile()});

    clangDiagnosticFilter.takeWarnings();

    ASSERT_TRUE(clangDiagnosticFilter.takeWarnings().isEmpty());
}

TEST_F(ClangDiagnosticFilter, IgnoreCertainWarningsInHeaderFiles)
{
    ClangCodeModel::Internal::ClangDiagnosticFilter myClangDiagnosticFilter{mainFileHeaderPath};

    myClangDiagnosticFilter.filter({pragmaOnceWarningInHeader(),
                                    includeNextInPrimarySourceFileWarningInHeader()});

    ASSERT_TRUE(myClangDiagnosticFilter.takeWarnings().isEmpty());
}

TEST_F(ClangDiagnosticFilter, ErrorsAreEmptyAfterTaking)
{
    clangDiagnosticFilter.filter({errorFromMainFile()});

    clangDiagnosticFilter.takeErrors();

    ASSERT_TRUE(clangDiagnosticFilter.takeErrors().isEmpty());
}

TEST_F(ClangDiagnosticFilter, FixItssAreEmptyAfterTaking)
{
    clangDiagnosticFilter.filter({warningFromMainFileWithFixits()});

    clangDiagnosticFilter.takeFixIts();

    ASSERT_TRUE(clangDiagnosticFilter.takeFixIts().isEmpty());
}

} // anonymous namespace
