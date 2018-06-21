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

#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QTextBlock>
#include <QTextCursor>

#include <utils/qtcassert.h>

Q_LOGGING_CATEGORY(fixitsLog, "qtc.clangtools.fixits");

using namespace Utils;

namespace ClangTools {
namespace Internal {

int FixitsRefactoringFile::position(unsigned line, unsigned column) const
{
    QTC_ASSERT(line != 0, return -1);
    QTC_ASSERT(column != 0, return -1);
    return document()->findBlockByNumber(line - 1).position() + column - 1;
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

    // Check for permissions
    if (!QFileInfo(m_filePath).isWritable())
        return false; // Error file not writable

    // Apply changes
    QTextDocument *doc = document();
    QTextCursor cursor(doc);

    for (int i=0; i < m_replacementOperations.size(); ++i) {
        ReplacementOperation &op = *m_replacementOperations[i];
        if (op.apply) {
            qCDebug(fixitsLog) << " " << i << "Applying" << op;

            // Shift subsequent operations that are affected
            shiftAffectedReplacements(op, i + 1);

            // Apply
            cursor.setPosition(op.pos);
            cursor.setPosition(op.pos + op.length, QTextCursor::KeepAnchor);
            cursor.insertText(op.text);
        }
    }

    // Write file
    if (!m_textFileFormat.codec)
        return false; // Error reading file

    QString error;
    if (!m_textFileFormat.writeFile(m_filePath, doc->toPlainText(), &error)) {
        qCDebug(fixitsLog) << "ERROR: Could not write file" << m_filePath << ":" << error;
        return false; // Error writing file
    }

    return true;
}

QTextDocument *FixitsRefactoringFile::document() const
{
    if (!m_document) {
        QString fileContents;
        if (!m_filePath.isEmpty()) {
            QString error;
            QTextCodec *defaultCodec = Core::EditorManager::defaultTextCodec();
            TextFileFormat::ReadResult result = TextFileFormat::readFile(
                        m_filePath, defaultCodec,
                        &fileContents, &m_textFileFormat,
                        &error);
            if (result != TextFileFormat::ReadSuccess) {
                qCDebug(fixitsLog) << "ERROR: Could not read " << m_filePath << ":" << error;
                m_textFileFormat.codec = nullptr;
            }
        }
        // always make a QTextDocument to avoid excessive null checks
        m_document = new QTextDocument(fileContents);
    }
    return m_document;
}

void FixitsRefactoringFile::shiftAffectedReplacements(const ReplacementOperation &op, int startIndex)
{
    for (int i = startIndex; i < m_replacementOperations.size(); ++i) {
        ReplacementOperation &current = *m_replacementOperations[i];
        ReplacementOperation before = current;

        if (op.pos <= current.pos)
            current.pos += op.text.size();
        if (op.pos < current.pos)
            current.pos -= op.length;

        qCDebug(fixitsLog) << "    shift:" << i << before << " ====> " << current;
    }
}

} // namespace Internal
} // namespace ClangTools
