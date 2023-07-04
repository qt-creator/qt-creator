// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangfixitsrefactoringchanges.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cppeditor/cppcodestylesettings.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppmodelmanager.h>

#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <QDebug>
#include <QLoggingCategory>
#include <QTextBlock>
#include <QTextCursor>

#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <algorithm>

static Q_LOGGING_CATEGORY(fixitsLog, "qtc.clangtools.fixits", QtWarningMsg);

using namespace TextEditor;
using namespace Utils;

namespace ClangTools {
namespace Internal {

int FixitsRefactoringFile::position(const FilePath &filePath, unsigned line, unsigned column) const
{
    QTC_ASSERT(line != 0, return -1);
    QTC_ASSERT(column != 0, return -1);
    return document(filePath)->findBlockByNumber(line - 1).position() + column - 1;
}

static QDebug operator<<(QDebug debug, const ReplacementOperation &op)
{
    debug.nospace() << "ReplacementOperation("
                    << op.pos << ", "
                    << op.length << ", "
                    << op.text << ", "
                    << op.apply
                    << ")"
                    ;

    return debug;
}

bool FixitsRefactoringFile::apply()
{
    if (m_replacementOperations.isEmpty())
        return false; // Error nothing to apply TODO: Is this correct to return?

    ICodeStylePreferencesFactory *factory = TextEditorSettings::codeStyleFactory(
        CppEditor::Constants::CPP_SETTINGS_ID);

    QHash<FilePath, QPair<ReplacementOperations, int>> operationsByFile;

    for (int i = 0; i < m_replacementOperations.size(); ++i) {
        ReplacementOperation &op = *m_replacementOperations[i];
        if (!op.apply)
            continue;

        // Check for permissions
        if (!op.filePath.isWritableFile())
            return false;

        qCDebug(fixitsLog) << " " << i << "Applying" << op;

        // Shift subsequent operations that are affected
        shiftAffectedReplacements(op, i + 1);

        // Apply
        QTextDocument * const doc = document(op.filePath);
        QTextCursor cursor(doc);
        cursor.setPosition(op.pos);
        cursor.setPosition(op.pos + op.length, QTextCursor::KeepAnchor);
        cursor.insertText(op.text);
        auto &opsForFile = operationsByFile[op.filePath];
        opsForFile.first.push_back(&op);
        opsForFile.second = i;
    }

    // Format
    for (auto it = operationsByFile.cbegin(); it != operationsByFile.cend(); ++it) {
        QTextDocument * const doc = document(it.key());
        const std::unique_ptr<TextEditor::Indenter> indenter(factory->createIndenter(doc));
        if (!indenter)
            continue;
        indenter->setFileName(it.key());
        format(*indenter, doc, it.value().first, it.value().second);
    }

    // Write file
    if (!m_textFileFormat.codec)
        return false; // Error reading file

    QString error;
    for (auto it = m_documents.begin(); it != m_documents.end(); ++it) {
        if (!m_textFileFormat.writeFile(it.key(), it.value()->toPlainText(), &error)) {
            qCDebug(fixitsLog) << "ERROR: Could not write file" << it.key() << ":" << error;
            return false; // Error writing file
        }
    }

    return true;
}

void FixitsRefactoringFile::format(TextEditor::Indenter &indenter,
                                   QTextDocument *doc,
                                   const ReplacementOperations &operationsForFile,
                                   int firstOperationIndex)
{
    if (operationsForFile.isEmpty())
        return;

    TextEditor::RangesInLines ranges;
    for (int i = 0; i < operationsForFile.size(); ++i) {
        const ReplacementOperation &op = *operationsForFile.at(i);
        const int start = doc->findBlock(op.pos).blockNumber() + 1;
        const int end = doc->findBlock(op.pos + op.length).blockNumber() + 1;
        ranges.push_back({start, end});
    }
    const EditOperations replacements = indenter.format(ranges);

    if (replacements.isEmpty())
        return;

    shiftAffectedReplacements(operationsForFile.front()->filePath,
                              replacements,
                              firstOperationIndex + 1);
}

QTextDocument *FixitsRefactoringFile::document(const FilePath &filePath) const
{
    if (m_documents.find(filePath) == m_documents.end()) {
        QString fileContents;
        if (!filePath.isEmpty()) {
            QString error;
            QTextCodec *defaultCodec = Core::EditorManager::defaultTextCodec();
            TextFileFormat::ReadResult result = TextFileFormat::readFile(filePath,
                                                                         defaultCodec,
                                                                         &fileContents,
                                                                         &m_textFileFormat,
                                                                         &error);
            if (result != TextFileFormat::ReadSuccess) {
                qCDebug(fixitsLog)
                    << "ERROR: Could not read " << filePath.toUserOutput() << ":" << error;
                m_textFileFormat.codec = nullptr;
            }
        }
        // always make a QTextDocument to avoid excessive null checks
        m_documents[filePath] = new QTextDocument(fileContents);
    }
    return m_documents[filePath];
}

void FixitsRefactoringFile::shiftAffectedReplacements(const ReplacementOperation &op, int startIndex)
{
    for (int i = startIndex; i < m_replacementOperations.size(); ++i) {
        ReplacementOperation &current = *m_replacementOperations[i];
        if (op.filePath != current.filePath)
            continue;

        ReplacementOperation before = current;

        if (op.pos <= current.pos)
            current.pos += op.text.size();
        if (op.pos < current.pos)
            current.pos -= op.length;

        qCDebug(fixitsLog) << "    shift:" << i << before << " ====> " << current;
    }
}

void FixitsRefactoringFile::shiftAffectedReplacements(const FilePath &filePath,
                                                      const EditOperations &replacements,
                                                      int startIndex)
{
    for (int i = startIndex; i < m_replacementOperations.size(); ++i) {
        ReplacementOperation &current = *m_replacementOperations[i];
        if (filePath != current.filePath)
            continue;

        for (const auto &op : replacements) {
            QTC_ASSERT(op.type == ChangeSet::EditOp::Replace, continue);
            if (op.pos1 > current.pos)
                break;
            current.pos += op.text.size() - op.length1;
        }
    }
}

} // namespace Internal
} // namespace ClangTools
