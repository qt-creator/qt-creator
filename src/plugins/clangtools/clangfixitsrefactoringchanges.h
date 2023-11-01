// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/indenter.h>

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
    Utils::FilePath filePath;
    bool apply = false;
};
using ReplacementOperations = QVector<ReplacementOperation *>;

/// Simplified version of TextEditor::RefactoringFile that allows
/// to operate on not owned ReplamentOperations
class FixitsRefactoringFile
{
public:
    FixitsRefactoringFile() = default;
    ~FixitsRefactoringFile() { qDeleteAll(m_documents); }

    int position(const Utils::FilePath &filePath, unsigned line, unsigned column) const;

    void setReplacements(const ReplacementOperations &ops) { m_replacementOperations = ops; }
    bool apply();

private:
    QTextDocument *document(const Utils::FilePath &filePath) const;
    void shiftAffectedReplacements(const ReplacementOperation &op, int startIndex);

    void format(TextEditor::Indenter &indenter,
                QTextDocument *doc,
                const ReplacementOperations &operationsForFile,
                int firstOperationIndex);
    void shiftAffectedReplacements(const Utils::FilePath &filePath,
                                   const Utils::EditOperations &replacements,
                                   int startIndex);

    mutable Utils::TextFileFormat m_textFileFormat;
    mutable QHash<Utils::FilePath, QTextDocument *> m_documents;
    ReplacementOperations m_replacementOperations; // Not owned.
};

} // namespace Internal
} // namespace ClangTools
