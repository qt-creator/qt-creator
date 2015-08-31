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
#include <translationunit.h>
#include <unsavedfiles.h>
#include <sourcelocation.h>
#include <sourcerange.h>
#include <fixit.h>

#include <clang-c/Index.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using ClangBackEnd::DiagnosticSet;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::Diagnostic;
using ClangBackEnd::FixIt;
using testing::PrintToString;

namespace {

MATCHER_P4(IsSourceLocation, filePath, line, column, offset,
           std::string(negation ? "isn't" : "is")
           + " source location with file path "+ PrintToString(filePath)
           + ", line " + PrintToString(line)
           + ", column " + PrintToString(column)
           + " and offset " + PrintToString(offset)
           )
{
    if (!arg.filePath().endsWith(filePath)
            || arg.line() != line
            || arg.column() != column
            || arg.offset() != offset)
        return false;

    return true;
}

class FixIt : public ::testing::Test
{
protected:
    ProjectPart projectPart{Utf8StringLiteral("projectPartId")};
    UnsavedFiles unsavedFiles;
    TranslationUnit translationUnit{Utf8StringLiteral(TESTDATA_DIR"/diagnostic_fixit.cpp"),
                                    unsavedFiles,
                                    projectPart};
    DiagnosticSet diagnosticSet{translationUnit.diagnostics()};
    Diagnostic diagnostic{diagnosticSet.front()};
    ::FixIt fixIt{diagnostic.fixIts().front()};
};

TEST_F(FixIt, Size)
{
    ASSERT_THAT(diagnostic.fixIts().size(), 1);
}

TEST_F(FixIt, Text)
{
    ASSERT_THAT(fixIt.text(), Utf8StringLiteral(";"));
}


TEST_F(FixIt, Start)
{
    ASSERT_THAT(fixIt.range().start(), IsSourceLocation(Utf8StringLiteral("diagnostic_fixit.cpp"),
                                                        3u,
                                                        13u,
                                                        29u));
}

TEST_F(FixIt, End)
{
    ASSERT_THAT(fixIt.range().end(), IsSourceLocation(Utf8StringLiteral("diagnostic_fixit.cpp"),
                                                        3u,
                                                        13u,
                                                        29u));
}

}
