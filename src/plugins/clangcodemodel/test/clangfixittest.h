/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
    QString fileContent(const QByteArray &relFilePath) const;

    ClangFixIt semicolonFixIt() const;

private:
    QScopedPointer<CppEditor::Tests::TemporaryCopiedDir> m_dataDir;
};

} //namespace Tests
} // namespace ClangCodeModel::Internal
