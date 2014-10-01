/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppdocumentationcommenthelper.h"

#include "cppautocompleter.h"

#include <cpptools/cpptoolssettings.h>
#include <cpptools/doxygengenerator.h>
#include <texteditor/basetexteditor.h>

#include <QDebug>
#include <QKeyEvent>
#include <QTextBlock>

using namespace CppTools;

namespace {

bool isStartOfDoxygenComment(const QTextCursor &cursor)
{
    const int pos = cursor.position();

    QTextDocument *document = cursor.document();
    QString comment = QString(document->characterAt(pos - 3))
            + document->characterAt(pos - 2)
            + document->characterAt(pos - 1);

    if ((comment == QLatin1String("/**"))
            || (comment == QLatin1String("/*!"))
            || (comment == QLatin1String("///"))
            || (comment == QLatin1String("//!"))) {
        return true;
    }
    return false;
}

DoxygenGenerator::DocumentationStyle doxygenStyle(const QTextCursor &cursor,
                                                  const QTextDocument *doc)
{
    const int pos = cursor.position();

    QString comment = QString(doc->characterAt(pos - 3))
            + doc->characterAt(pos - 2)
            + doc->characterAt(pos - 1);

    if (comment == QLatin1String("/**"))
        return DoxygenGenerator::JavaStyle;
    else if (comment == QLatin1String("/*!"))
        return DoxygenGenerator::QtStyle;
    else if (comment == QLatin1String("///"))
        return DoxygenGenerator::CppStyleA;
    else
        return DoxygenGenerator::CppStyleB;
}

/// Check if previous line is a CppStyle Doxygen Comment
bool isPreviousLineCppStyleComment(const QTextCursor &cursor)
{
    const QTextBlock &currentBlock = cursor.block();
    if (!currentBlock.isValid())
        return false;

    const QTextBlock &actual = currentBlock.previous();
    if (!actual.isValid())
        return false;

    const QString text = actual.text().trimmed();
    if (text.startsWith(QLatin1String("///")) || text.startsWith(QLatin1String("//!")))
        return true;

    return false;
}

/// Check if next line is a CppStyle Doxygen Comment
bool isNextLineCppStyleComment(const QTextCursor &cursor)
{
    const QTextBlock &currentBlock = cursor.block();
    if (!currentBlock.isValid())
        return false;

    const QTextBlock &actual = currentBlock.next();
    if (!actual.isValid())
        return false;

    const QString text = actual.text().trimmed();
    if (text.startsWith(QLatin1String("///")) || text.startsWith(QLatin1String("//!")))
        return true;

    return false;
}

bool isCppStyleContinuation(const QTextCursor& cursor)
{
    return (isPreviousLineCppStyleComment(cursor) || isNextLineCppStyleComment(cursor));
}

/// Check if line is a CppStyle Doxygen comment and the cursor is after the comment
bool isCursorAfterCppComment(const QTextCursor &cursor, const QTextDocument *doc)
{
    QTextCursor cursorFirstNonBlank(cursor);
    cursorFirstNonBlank.movePosition(QTextCursor::StartOfLine);
    while (doc->characterAt(cursorFirstNonBlank.position()).isSpace()
           && cursorFirstNonBlank.movePosition(QTextCursor::NextCharacter)) {
    }

    const QTextBlock& block = cursorFirstNonBlank.block();
    const QString text = block.text().trimmed();
    if (text.startsWith(QLatin1String("///")) || text.startsWith(QLatin1String("//!")))
        return (cursor.position() >= cursorFirstNonBlank.position() + 3);

    return false;
}

bool handleDoxygenCppStyleContinuation(QTextCursor &cursor, QKeyEvent *e)
{
    const int blockPos = cursor.positionInBlock();
    const QString &text = cursor.block().text();
    int offset = 0;
    for (; offset < blockPos; ++offset) {
        if (!text.at(offset).isSpace())
            break;
    }

    // If the line does not start with the comment we don't
    // consider it as a continuation. Handles situations like:
    // void d(); ///<enter>
    if (!(text.trimmed().startsWith(QLatin1String("///"))
          || text.startsWith(QLatin1String("//!")))) {
        return false;
    }

    QString newLine(QLatin1Char('\n'));
    newLine.append(QString(offset, QLatin1Char(' '))); // indent correctly

    const QString commentMarker = text.mid(offset, 3);
    newLine.append(commentMarker);
    newLine.append(QLatin1Char(' '));

    cursor.insertText(newLine);
    e->accept();
    return true;
}

bool handleDoxygenContinuation(QTextCursor &cursor,
                               QKeyEvent *e,
                               const QTextDocument *doc,
                               const bool enableDoxygen,
                               const bool leadingAsterisks)
{
    // It might be a continuation if:
    // a) current line starts with /// or //! and cursor is positioned after the comment
    // b) current line is in the middle of a multi-line Qt or Java style comment

    if (enableDoxygen && !cursor.atEnd() && isCursorAfterCppComment(cursor, doc))
        return handleDoxygenCppStyleContinuation(cursor, e);

    if (!leadingAsterisks)
        return false;

    // We continue the comment if the cursor is after a comment's line asterisk and if
    // there's no asterisk immediately after the cursor (that would already be considered
    // a leading asterisk).
    int offset = 0;
    const int blockPos = cursor.positionInBlock();
    const QString &text = cursor.block().text();
    for (; offset < blockPos; ++offset) {
        if (!text.at(offset).isSpace())
            break;
    }

    if (offset < blockPos
            && (text.at(offset) == QLatin1Char('*')
                || (offset < blockPos - 1
                    && text.at(offset) == QLatin1Char('/')
                    && text.at(offset + 1) == QLatin1Char('*')))) {
        int followinPos = blockPos;
        for (; followinPos < text.length(); ++followinPos) {
            if (!text.at(followinPos).isSpace())
                break;
        }
        if (followinPos == text.length()
                || text.at(followinPos) != QLatin1Char('*')) {
            QString newLine(QLatin1Char('\n'));
            QTextCursor c(cursor);
            c.movePosition(QTextCursor::StartOfBlock);
            c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, offset);
            newLine.append(c.selectedText());
            if (text.at(offset) == QLatin1Char('/')) {
                newLine.append(QLatin1String(" * "));
            } else {
                int start = offset;
                while (offset < blockPos && text.at(offset) == QLatin1Char('*'))
                    ++offset;
                newLine.append(QString(offset - start, QLatin1Char('*')));
                newLine.append(QLatin1Char(' '));
            }
            cursor.insertText(newLine);
            e->accept();
            return true;
        }
    }

    return false;
}

} // anonymous namespace

namespace CppEditor {
namespace Internal {

CppDocumentationCommentHelper::CppDocumentationCommentHelper(
        TextEditor::BaseTextEditorWidget *editorWidget)
    : m_editorWidget(editorWidget)
    , m_settings(CppToolsSettings::instance()->commentsSettings())
{
    connect(CppToolsSettings::instance(),
            SIGNAL(commentsSettingsChanged(CppTools::CommentsSettings)),
            this,
            SLOT(onCommentsSettingsChanged(CppTools::CommentsSettings)));
}

bool CppDocumentationCommentHelper::handleKeyPressEvent(QKeyEvent *e) const
{
    if (!m_settings.m_enableDoxygen && !m_settings.m_leadingAsterisks)
        return false;

    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        QTextCursor cursor = m_editorWidget->textCursor();
        if (!m_editorWidget->autoCompleter()->isInComment(cursor))
            return false;

        // We are interested on two particular cases:
        //   1) The cursor is right after a /**, /*!, /// or ///! and the user pressed enter.
        //      If Doxygen is enabled we need to generate an entire comment block.
        //   2) The cursor is already in the middle of a multi-line comment and the user pressed
        //      enter. If leading asterisk(s) is set we need to write a comment continuation
        //      with those.

        if (m_settings.m_enableDoxygen && cursor.positionInBlock() >= 3) {
            const int pos = cursor.position();
            if (isStartOfDoxygenComment(cursor)) {
                QTextDocument *textDocument = m_editorWidget->document();
                DoxygenGenerator::DocumentationStyle style = doxygenStyle(cursor, textDocument);

                // Check if we're already in a CppStyle Doxygen comment => continuation
                // Needs special handling since CppStyle does not have start and end markers
                if ((style == DoxygenGenerator::CppStyleA || style == DoxygenGenerator::CppStyleB)
                        && isCppStyleContinuation(cursor)) {
                    return handleDoxygenCppStyleContinuation(cursor, e);
                }

                DoxygenGenerator doxygen;
                doxygen.setStyle(style);
                doxygen.setAddLeadingAsterisks(m_settings.m_leadingAsterisks);
                doxygen.setGenerateBrief(m_settings.m_generateBrief);
                doxygen.setStartComment(false);

                // Move until we reach any possibly meaningful content.
                while (textDocument->characterAt(cursor.position()).isSpace()
                       && cursor.movePosition(QTextCursor::NextCharacter)) {
                }

                if (!cursor.atEnd()) {
                    const QString &comment = doxygen.generate(cursor);
                    if (!comment.isEmpty()) {
                        cursor.beginEditBlock();
                        cursor.setPosition(pos);
                        cursor.insertText(comment);
                        cursor.setPosition(pos - 3, QTextCursor::KeepAnchor);
                        m_editorWidget->baseTextDocument()->autoIndent(cursor);
                        cursor.endEditBlock();
                        e->accept();
                        return true;
                    }
                }

            }
        } // right after first doxygen comment

        return handleDoxygenContinuation(cursor,
                                         e,
                                         m_editorWidget->document(),
                                         m_settings.m_enableDoxygen,
                                         m_settings.m_leadingAsterisks);
    }

    return false;
}

void CppDocumentationCommentHelper::onCommentsSettingsChanged(const CommentsSettings &settings)
{
    m_settings = settings;
}

} // namespace Internal
} // namespace CppEditor
