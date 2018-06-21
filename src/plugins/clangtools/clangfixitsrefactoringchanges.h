/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <utils/changeset.h>
#include <utils/textfileformat.h>

#include <QString>
#include <QTextDocument>
#include <QVector>

namespace ClangTools {
namespace Internal {

class ReplacementOperation
{
public:
    int pos = -1;
    int length = -1;
    QString text;
    bool apply = false;
};
using ReplacementOperations = QVector<ReplacementOperation *>;

/// Simplified version of TextEditor::RefactoringFile that allows
/// to operate on not owned ReplamentOperations
class FixitsRefactoringFile
{
public:
    FixitsRefactoringFile() = default;
    FixitsRefactoringFile(const QString &filePath) : m_filePath(filePath) {}

    bool isValid() const { return !m_filePath.isEmpty(); }
    int position(unsigned line, unsigned column) const;

    void setReplacements(const ReplacementOperations &ops) { m_replacementOperations = ops; }
    bool apply();

private:
    QTextDocument *document() const;
    void shiftAffectedReplacements(const ReplacementOperation &op, int startIndex);

    QString m_filePath;
    mutable Utils::TextFileFormat m_textFileFormat;
    mutable QTextDocument *m_document = nullptr;
    ReplacementOperations m_replacementOperations; // Not owned.
};

} // namespace Internal
} // namespace ClangTools
