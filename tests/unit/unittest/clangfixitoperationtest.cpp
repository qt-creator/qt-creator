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

#include "gtest/gtest.h"
#include "gmock/gmock-matchers.h"
#include "gmock/gmock.h"
#include "gtest-qt-printing.h"

#include <clangfixitoperation.h>
#include <fixitcontainer.h>

#include <utils/changeset.h>

#include <QFile>
#include <QVector>

using ClangBackEnd::FixItContainer;
using ClangCodeModel::ClangFixItOperation;

using ::testing::PrintToString;

namespace {

QString unsavedFileContent(const QString &unsavedFilePath)
{
    QFile unsavedFileContentFile(unsavedFilePath);
    const bool isOpen = unsavedFileContentFile.open(QFile::ReadOnly | QFile::Text);
    if (!isOpen)
        ADD_FAILURE() << "File with the unsaved content cannot be opened!";

    return QString::fromUtf8(unsavedFileContentFile.readAll());
}

MATCHER_P2(MatchText, errorText, expectedText,
           std::string(negation ? "hasn't" : "has") + " error text:\n" + PrintToString(errorText) +
           " and expected text:\n" + PrintToString(expectedText))
{
    QString resultText = errorText;
    Utils::ChangeSet changeSet = arg.changeSet();
    changeSet.apply(&resultText);

    if (resultText != expectedText) {
        *result_listener << "\n" << resultText.toUtf8().constData();
        return false;
    }

    return true;
}

class ClangFixItOperation : public ::testing::Test
{
protected:
    Utf8String filePath;
    Utf8String diagnosticText{Utf8StringLiteral("expected ';' at end of declaration")};
    FixItContainer semicolonFixItContainer{Utf8StringLiteral(";"),
                                  {{filePath, 3u, 29u},
                                   {filePath, 3u, 29u}}};
    QString semicolonErrorFile{QStringLiteral(TESTDATA_DIR"/diagnostic_semicolon_fixit.cpp")};
    QString semicolonExpectedFile{QStringLiteral(TESTDATA_DIR"/diagnostic_semicolon_fixit_expected.cpp")};
    QString compareWarningFile{QStringLiteral(TESTDATA_DIR"/diagnostic_comparison_fixit.cpp")};
    QString compareExpected1File{QStringLiteral(TESTDATA_DIR"/diagnostic_comparison_fixit_expected1.cpp")};
    QString compareExpected2File{QStringLiteral(TESTDATA_DIR"/diagnostic_comparison_fixit_expected2.cpp")};
    FixItContainer compareFixItContainer{Utf8StringLiteral("=="),
                                  {{filePath, 4u, 43u},
                                   {filePath, 4u, 44u}}};
    FixItContainer assignmentFixItContainerParenLeft{Utf8StringLiteral("("),
                                  {{filePath, 4u, 41u},
                                   {filePath, 4u, 41u}}};
    FixItContainer assignmentFixItContainerParenRight{Utf8StringLiteral(")"),
                                  {{filePath, 4u, 46u},
                                   {filePath, 4u, 46u}}};
};

TEST_F(ClangFixItOperation, Description)
{
    ::ClangFixItOperation operation(filePath, diagnosticText, {semicolonFixItContainer});

    ASSERT_THAT(operation.description(),
                QStringLiteral("Apply Fix: expected ';' at end of declaration"));
}

TEST_F(ClangFixItOperation, AppendSemicolon)
{
    ::ClangFixItOperation operation(filePath, diagnosticText, {semicolonFixItContainer});

    ASSERT_THAT(operation, MatchText(unsavedFileContent(semicolonErrorFile),
                                     unsavedFileContent(semicolonExpectedFile)));
}

TEST_F(ClangFixItOperation, ComparisonVersusAssignmentChooseComparison)
{
    ::ClangFixItOperation operation(filePath, diagnosticText, {compareFixItContainer});

    ASSERT_THAT(operation, MatchText(unsavedFileContent(compareWarningFile),
                                     unsavedFileContent(compareExpected1File)));
}

TEST_F(ClangFixItOperation, ComparisonVersusAssignmentChooseParentheses)
{
    ::ClangFixItOperation operation(filePath,
                                    diagnosticText,
                                    {assignmentFixItContainerParenLeft,
                                     assignmentFixItContainerParenRight});

    ASSERT_THAT(operation, MatchText(unsavedFileContent(compareWarningFile),
                                     unsavedFileContent(compareExpected2File)));
}

}
