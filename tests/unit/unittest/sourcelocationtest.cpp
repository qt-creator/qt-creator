/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <diagnostic.h>
#include <diagnosticset.h>
#include <projectpart.h>
#include <projects.h>
#include <translationunit.h>
#include <translationunits.h>
#include <unsavedfiles.h>
#include <sourcelocation.h>

#include <clang-c/Index.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using ClangBackEnd::Diagnostic;
using ClangBackEnd::DiagnosticSet;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::SourceLocation;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using testing::EndsWith;

namespace {

class SourceLocation : public ::testing::Test
{
protected:
    ProjectPart projectPart{Utf8StringLiteral("projectPartId")};
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::TranslationUnits translationUnits{projects, unsavedFiles};
    TranslationUnit translationUnit{Utf8StringLiteral(TESTDATA_DIR"/diagnostic_source_location.cpp"),
                                    projectPart,
                                    Utf8StringVector(),
                                    translationUnits};
    DiagnosticSet diagnosticSet{translationUnit.diagnostics()};
    Diagnostic diagnostic{diagnosticSet.front()};
    ::SourceLocation sourceLocation{diagnostic.location()};

};

TEST_F(SourceLocation, FilePath)
{
    ASSERT_THAT(sourceLocation.filePath().constData(), EndsWith("diagnostic_source_location.cpp"));
}

TEST_F(SourceLocation, Line)
{
    ASSERT_THAT(sourceLocation.line(), 4);
}

TEST_F(SourceLocation, Column)
{
    ASSERT_THAT(sourceLocation.column(), 1);
}

TEST_F(SourceLocation, Offset)
{
    ASSERT_THAT(sourceLocation.offset(), 18);
}

}
