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

#include "cppdocumentationcommenthelper.h"

#include "cppautocompleter.h"

#include <cpptools/cpptoolssettings.h>
#include <cpptools/doxygengenerator.h>
#include <texteditor/commentssettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textdocument.h>
#include <cplusplus/MatchingText.h>

#include <QDebug>
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

    return comment == QLatin1String("/**")
           || comment == QLatin1String("/*!")
           || comment == QLatin1String("///")
           || comment == QLatin1String("//!");
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
    return isPreviousLineCppStyleComment(cursor) || isNextLineCppStyleComment(cursor);
}

bool lineStartsWithCppDoxygenCommentAndCursorIsAfter(const QTextCursor &cursor,
                                                     const QTextDocument *doc)
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

bool isCursorAfterNonNestedCppStyleComment(const QTextCursor &cursor,
                                           TextEditor::TextEditorWidget *editorWidget)
{
    QTextDocument *document = editorWidget->document();
    QTextCursor cursorBeforeCppComment(cursor);
    while (document->characterAt(cursorBeforeCppComment.position()) != QLatin1Char('/')
           && cursorBeforeCppComment.movePosition(QTextCursor::PreviousCharacter)) {
    }

    if (!cursorBeforeCppComment.movePosition(QTextCursor::PreviousCharacter))
        return false;

    if (document->characterAt(cursorBeforeCppComment.position()) != QLatin1Char('/'))
        return false;

    if (!cursorBeforeCppComment.movePosition(QTextCursor::PreviousCharacter))
        return false;

    return !CPlusPlus::MatchingText::isInCommentHelper(cursorBeforeCppComment);
}

bool handleDoxygenCppStyleContinuation(QTextCursor &cursor)
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
    const QStringRef commentMarker = text.midRef(offset, 3);
    if (commentMarker != QLatin1String("///") && commentMarker != QLatin1String("//!"))
        return false;

    QString newLine(QLatin1Char('\n'));
    newLine.append(text.left(offset)); // indent correctly
    newLine.append(commentMarker);
    newLine.append(QLatin1Char(' '));

    cursor.insertText(newLine);
    return true;
}

bool handleDoxygenContinuation(QTextCursor &cursor,
                               TextEditor::TextEditorWidget *editorWidget,
                               const bool enableDoxygen,
                               const bool leadingAsterisks)
{
    const QTextDocument *doc = editorWidget->document();

    // It might be a continuation if:
    // a) current line starts with /// or //! and cursor is positioned after the comment
    // b) current line is in the middle of a multi-line Qt or Java style comment

    if (!cursor.atEnd()) {
        if (enableDoxygen && lineStartsWithCppDoxygenCommentAndCursorIsAfter(cursor, doc))
            return handleDoxygenCppStyleContinuation(cursor);

        if (isCursorAfterNonNestedCppStyleComment(cursor, editorWidget))
            return false;
    }

    // We continue the comment if the cursor is after a comment's line asterisk and if
    // there's no asterisk immediately after the cursor (that would already be considered
    // a leading asterisk).
    int offset = 0;
    const int blockPos = cursor.positionInBlock();
    const QString &currentLine = cursor.block().text();
    for (; offset < blockPos; ++offset) {
        if (!currentLine.at(offset).isSpace())
            break;
    }

    // In case we don't need to insert leading asteriskses, this code will be run once (right after
    // hitting enter on the line containing '/*'). It will insert a continuation without an
    // asterisk, but with an extra space. After that, the normal indenting will take over and do the
    // Right Thing <TM>.
    if (offset < blockPos
            && (currentLine.at(offset) == QLatin1Char('*')
                || (offset < blockPos - 1
                    && currentLine.at(offset) == QLatin1Char('/')
                    && currentLine.at(offset + 1) == QLatin1Char('*')))) {
        // Ok, so the line started with an '*' or '/*'
        int followinPos = blockPos;
        // Now search for the first non-whitespace character to align to:
        for (; followinPos < currentLine.length(); ++followinPos) {
            if (!currentLine.at(followinPos).isSpace())
                break;
        }
        if (followinPos == currentLine.length() // a)
                || currentLine.at(followinPos) != QLatin1Char('*')) { // b)
            // So either a) the line ended after a '*' and we need to insert a continuation, or
            // b) we found the start of some text and we want to align the continuation to that.
            QString newLine(QLatin1Char('\n'));
            QTextCursor c(cursor);
            c.movePosition(QTextCursor::StartOfBlock);
            c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, offset);
            newLine.append(c.selectedText());
            if (currentLine.at(offset) == QLatin1Char('/')) {
                if (leadingAsterisks)
                    newLine.append(QLatin1String(" * "));
                else
                    newLine.append(QLatin1String("   "));
            } else {
                // If '*' is not within a comment, skip.
                QTextCursor cursorOnFirstNonWhiteSpace(cursor);
                const int positionOnFirstNonWhiteSpace = cursor.position() - blockPos + offset;
                cursorOnFirstNonWhiteSpace.setPosition(positionOnFirstNonWhiteSpace);
                if (!CPlusPlus::MatchingText::isInCommentHelper(cursorOnFirstNonWhiteSpace))
                    return false;

                // ...otherwise do the continuation
                int start = offset;
                while (offset < blockPos && currentLine.at(offset) == QLatin1Char('*'))
                    ++offset;
                const QChar ch = leadingAsterisks ? QLatin1Char('*') : QLatin1Char(' ');
                newLine.append(QString(offset - start, ch));
                newLine.append(QLatin1Char(' '));
            }
            cursor.insertText(newLine);
            return true;
        }
    }

    return false;
}

} // anonymous namespace

namespace CppEditor {
namespace Internal {

bool trySplitComment(TextEditor::TextEditorWidget *editorWidget,
                     const CPlusPlus::Snapshot &snapshot)
{
    const TextEditor::CommentsSettings &settings = CppToolsSettings::instance()->commentsSettings();
    if (!settings.m_enableDoxygen && !settings.m_leadingAsterisks)
        return false;

    QTextCursor cursor = editorWidget->textCursor();
    if (!CPlusPlus::MatchingText::isInCommentHelper(cursor))
        return false;

    // We are interested on two particular cases:
    //   1) The cursor is right after a /**, /*!, /// or ///! and the user pressed enter.
    //      If Doxygen is enabled we need to generate an entire comment block.
    //   2) The cursor is already in the middle of a multi-line comment and the user pressed
    //      enter. If leading asterisk(s) is set we need to write a comment continuation
    //      with those.

    if (settings.m_enableDoxygen && cursor.positionInBlock() >= 3) {
        const int pos = cursor.position();
        if (isStartOfDoxygenComment(cursor)) {
            QTextDocument *textDocument = editorWidget->document();
            DoxygenGenerator::DocumentationStyle style = doxygenStyle(cursor, textDocument);

            // Check if we're already in a CppStyle Doxygen comment => continuation
            // Needs special handling since CppStyle does not have start and end markers
            if ((style == DoxygenGenerator::CppStyleA || style == DoxygenGenerator::CppStyleB)
                    && isCppStyleContinuation(cursor)) {
                return handleDoxygenCppStyleContinuation(cursor);
            }

            DoxygenGenerator doxygen;
            doxygen.setStyle(style);
            doxygen.setAddLeadingAsterisks(settings.m_leadingAsterisks);
            doxygen.setGenerateBrief(settings.m_generateBrief);
            doxygen.setStartComment(false);

            // Move until we reach any possibly meaningful content.
            while (textDocument->characterAt(cursor.position()).isSpace()
                   && cursor.movePosition(QTextCursor::NextCharacter)) {
            }

            if (!cursor.atEnd()) {
                const QString &comment = doxygen.generate(cursor,
                                                          snapshot,
                                                          editorWidget->textDocument()->filePath());
                if (!comment.isEmpty()) {
                    cursor.beginEditBlock();
                    cursor.setPosition(pos);
                    cursor.insertText(comment);
                    cursor.setPosition(pos - 3, QTextCursor::KeepAnchor);
                    editorWidget->textDocument()->autoIndent(cursor);
                    cursor.endEditBlock();
                    return true;
                }
            }
        }
    } // right after first doxygen comment

    return handleDoxygenContinuation(cursor,
                                     editorWidget,
                                     settings.m_enableDoxygen,
                                     settings.m_leadingAsterisks);
}

} // namespace Internal
} // namespace CppEditor
