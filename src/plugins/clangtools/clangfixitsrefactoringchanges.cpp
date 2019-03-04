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

#include "clangfixitsrefactoringchanges.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppcodestylesettings.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsconstants.h>

#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QTextBlock>
#include <QTextCursor>

#include <utils/qtcassert.h>

#include <algorithm>

Q_LOGGING_CATEGORY(fixitsLog, "qtc.clangtools.fixits", QtWarningMsg);

using namespace TextEditor;
using namespace Utils;

namespace ClangTools {
namespace Internal {

int FixitsRefactoringFile::position(const QString &filePath, unsigned line, unsigned column) const
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
    qCDebug(fixitsLog) << "Applying fixits for" << m_filePath;

    if (m_replacementOperations.isEmpty())
        return false; // Error nothing to apply TODO: Is this correct to return?

    QTC_ASSERT(!m_filePath.isEmpty(), return false);

    ICodeStylePreferencesFactory *factory = TextEditorSettings::codeStyleFactory(
        CppTools::Constants::CPP_SETTINGS_ID);

    // Apply changes
    std::unique_ptr<TextEditor::Indenter> indenter;
    QString lastFilename;
    ReplacementOperations operationsForFile;

    for (int i=0; i < m_replacementOperations.size(); ++i) {
        ReplacementOperation &op = *m_replacementOperations[i];
        if (op.apply) {
            // Check for permissions
            if (!QFileInfo(op.fileName).isWritable())
                return false; // Error file not writable

            qCDebug(fixitsLog) << " " << i << "Applying" << op;

            // Shift subsequent operations that are affected
            shiftAffectedReplacements(op, i + 1);

            // Apply
            QTextDocument *doc = document(op.fileName);
            if (lastFilename != op.fileName) {
                if (indenter)
                    format(*indenter, doc, operationsForFile, i);
                operationsForFile.clear();
                indenter = std::unique_ptr<TextEditor::Indenter>(factory->createIndenter(doc));
                indenter->setFileName(Utils::FileName::fromString(op.fileName));
            }

            QTextCursor cursor(doc);
            cursor.setPosition(op.pos);
            cursor.setPosition(op.pos + op.length, QTextCursor::KeepAnchor);
            cursor.insertText(op.text);
            operationsForFile.push_back(&op);
        }
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
    const Replacements replacements = indenter.format(ranges);

    if (replacements.empty())
        return;

    shiftAffectedReplacements(operationsForFile.front()->fileName,
                              replacements,
                              firstOperationIndex + 1);
}

QTextDocument *FixitsRefactoringFile::document(const QString &filePath) const
{
    if (m_documents.find(filePath) == m_documents.end()) {
        QString fileContents;
        if (!filePath.isEmpty()) {
            QString error;
            QTextCodec *defaultCodec = Core::EditorManager::defaultTextCodec();
            TextFileFormat::ReadResult result = TextFileFormat::readFile(
                        filePath, defaultCodec,
                        &fileContents, &m_textFileFormat,
                        &error);
            if (result != TextFileFormat::ReadSuccess) {
                qCDebug(fixitsLog) << "ERROR: Could not read " << filePath << ":" << error;
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
        if (op.fileName != current.fileName)
            continue;

        ReplacementOperation before = current;

        if (op.pos <= current.pos)
            current.pos += op.text.size();
        if (op.pos < current.pos)
            current.pos -= op.length;

        qCDebug(fixitsLog) << "    shift:" << i << before << " ====> " << current;
    }
}

bool FixitsRefactoringFile::hasIntersection(const QString &fileName,
                                            const Replacements &replacements,
                                            int startIndex) const
{
    for (int i = startIndex; i < m_replacementOperations.size(); ++i) {
        const ReplacementOperation &current = *m_replacementOperations[i];
        if (fileName != current.fileName)
            continue;

        // Usually the number of replacements is from 1 to 3.
        if (std::any_of(replacements.begin(),
                        replacements.end(),
                        [&current](const Replacement &replacement) {
                            return replacement.offset + replacement.length > current.pos
                                   && replacement.offset < current.pos + current.length;
                        })) {
            return true;
        }
    }

    return false;
}

void FixitsRefactoringFile::shiftAffectedReplacements(const QString &fileName,
                                                      const Replacements &replacements,
                                                      int startIndex)
{
    for (int i = startIndex; i < m_replacementOperations.size(); ++i) {
        ReplacementOperation &current = *m_replacementOperations[i];
        if (fileName != current.fileName)
            continue;

        for (const auto &replacement : replacements) {
            if (replacement.offset > current.pos)
                break;
            current.pos += replacement.text.size() - replacement.length;
        }
    }
}

} // namespace Internal
} // namespace ClangTools
