// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/cpptoolstestcase.h>

#include <QObject>
#include <QScopedPointer>
#include <QString>

namespace Utils { class FilePath; }

namespace ClangCodeModel::Internal {
class ClangFixIt;

namespace Tests {

class ClangFixItTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void testAppendSemicolon();
    void testComparisonVersusAssignmentChooseComparison();
    void testComparisonVersusAssignmentChooseParentheses();
    void testDescription();

private:
    Utils::FilePath semicolonFilePath() const;
    Utils::FilePath compareFilePath() const;
    QString fileContent(const QString &relFilePath) const;

    ClangFixIt semicolonFixIt() const;

private:
    QScopedPointer<CppEditor::Tests::TemporaryCopiedDir> m_dataDir;
};

} //namespace Tests
} // namespace ClangCodeModel::Internal
