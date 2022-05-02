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

#include "clangfixittest.h"

#include "../clangfixitoperation.h"

#include <utils/changeset.h>

#include <QFile>
#include <QtTest>
#include <QVector>

namespace ClangCodeModel::Internal::Tests {

static QString qrcPath(const QString &relativeFilePath)
{
    return QLatin1String(":/unittests/ClangCodeModel/") + relativeFilePath;
}

static QString diagnosticText() { return QString("expected ';' at end of declaration"); }

void ClangFixItTest::testDescription()
{
    ClangFixItOperation operation(diagnosticText(), {semicolonFixIt()});
    QCOMPARE(operation.description(),
             QLatin1String("Apply Fix: expected ';' at end of declaration"));
}

Utils::FilePath ClangFixItTest::semicolonFilePath() const
{
    return Utils::FilePath::fromString(m_dataDir->absolutePath("diagnostic_semicolon_fixit.cpp"));
}

Utils::FilePath ClangFixItTest::compareFilePath() const
{
    return Utils::FilePath::fromString(m_dataDir->absolutePath("diagnostic_comparison_fixit.cpp"));
}

QString ClangFixItTest::fileContent(const QByteArray &relFilePath) const
{
    QFile file(m_dataDir->absolutePath(relFilePath));
    const bool isOpen = file.open(QFile::ReadOnly | QFile::Text);
    if (!isOpen)
        qDebug() << "File with the unsaved content cannot be opened!";
    return QString::fromUtf8(file.readAll());
}

ClangFixIt ClangFixItTest::semicolonFixIt() const
{
    return {";", {{semicolonFilePath(), 3u, 12u}, {semicolonFilePath(), 3u, 12u}}};
}

void ClangFixItTest::init()
{
    m_dataDir.reset(new CppEditor::Tests::TemporaryCopiedDir(qrcPath("fixits")));
}

void ClangFixItTest::testAppendSemicolon()
{
    ClangFixItOperation operation(diagnosticText(), {semicolonFixIt()});
    operation.perform();
    QCOMPARE(operation.firstRefactoringFileContent_forTestOnly(),
             fileContent("diagnostic_semicolon_fixit_expected.cpp"));
}

void ClangFixItTest::testComparisonVersusAssignmentChooseComparison()
{
    const ClangFixIt compareFixIt{"==", {{compareFilePath(), 4u, 10u},
            {compareFilePath(), 4u, 11u}}};
    ClangFixItOperation operation(diagnosticText(), {compareFixIt});
    operation.perform();
    QCOMPARE(operation.firstRefactoringFileContent_forTestOnly(),
             fileContent("diagnostic_comparison_fixit_expected1.cpp"));
}

void ClangFixItTest::testComparisonVersusAssignmentChooseParentheses()
{
    const ClangFixIt assignmentFixItParenLeft{"(", {{compareFilePath(), 4u, 8u},
            {compareFilePath(), 4u, 8u}}};
    const ClangFixIt assignmentFixItParenRight{")", {{compareFilePath(), 4u, 13u},
            {compareFilePath(), 4u, 13u}}};
    ClangFixItOperation operation(diagnosticText(), {assignmentFixItParenLeft,
                                                     assignmentFixItParenRight});
    operation.perform();
    QCOMPARE(operation.firstRefactoringFileContent_forTestOnly(),
             fileContent("diagnostic_comparison_fixit_expected2.cpp"));
}

} // namespace ClangCodeModel::Internal::Tests
