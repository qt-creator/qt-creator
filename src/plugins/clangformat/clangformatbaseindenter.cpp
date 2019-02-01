/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "clangformatbaseindenter.h"

#include <clang/Tooling/Core/Replacement.h>

#include <utils/fileutils.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>

#include <QTextDocument>

namespace ClangFormat {

static void adjustFormatStyleForLineBreak(clang::format::FormatStyle &style)
{
    style.DisableFormat = false;
    style.ColumnLimit = 0;
#ifdef KEEP_LINE_BREAKS_FOR_NON_EMPTY_LINES_BACKPORTED
    style.KeepLineBreaksForNonEmptyLines = true;
#endif
    style.MaxEmptyLinesToKeep = 2;
    style.SortIncludes = false;
    style.SortUsingDeclarations = false;
}

static llvm::StringRef clearExtraNewline(llvm::StringRef text)
{
    while (text.startswith("\n\n"))
        text = text.drop_front();
    return text;
}

static clang::tooling::Replacements filteredReplacements(
    const clang::tooling::Replacements &replacements,
    int offset,
    int extraOffsetToAdd,
    bool onlyIndention)
{
    clang::tooling::Replacements filtered;
    for (const clang::tooling::Replacement &replacement : replacements) {
        int replacementOffset = static_cast<int>(replacement.getOffset());
        if (onlyIndention && replacementOffset != offset - 1)
            continue;

        if (replacementOffset + 1 >= offset)
            replacementOffset += extraOffsetToAdd;

        llvm::StringRef text = onlyIndention ? clearExtraNewline(replacement.getReplacementText())
                                             : replacement.getReplacementText();

        llvm::Error error = filtered.add(
            clang::tooling::Replacement(replacement.getFilePath(),
                                        static_cast<unsigned int>(replacementOffset),
                                        replacement.getLength(),
                                        text));
        // Throws if error is not checked.
        if (error)
            break;
    }
    return filtered;
}

static void trimFirstNonEmptyBlock(const QTextBlock &currentBlock)
{
    QTextBlock prevBlock = currentBlock.previous();
    while (prevBlock.position() > 0 && prevBlock.text().trimmed().isEmpty())
        prevBlock = prevBlock.previous();

    if (prevBlock.text().trimmed().isEmpty())
        return;

    const QString initialText = prevBlock.text();
    if (!initialText.at(initialText.size() - 1).isSpace())
        return;

    auto lastNonSpace = std::find_if_not(initialText.rbegin(),
                                         initialText.rend(),
                                         [](const QChar &letter) { return letter.isSpace(); });
    const int extraSpaceCount = static_cast<int>(std::distance(initialText.rbegin(), lastNonSpace));

    QTextCursor cursor(prevBlock);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Right,
                        QTextCursor::MoveAnchor,
                        initialText.size() - extraSpaceCount);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, extraSpaceCount);
    cursor.removeSelectedText();
    cursor.endEditBlock();
}

static void trimCurrentBlock(const QTextBlock &currentBlock)
{
    if (currentBlock.text().trimmed().isEmpty()) {
        // Clear the block containing only spaces
        QTextCursor cursor(currentBlock);
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.endEditBlock();
    }
}

// Returns the total langth of previous lines with pure whitespace.
static int previousEmptyLinesLength(const QTextBlock &currentBlock)
{
    int length{0};
    QTextBlock prevBlock = currentBlock.previous();
    while (prevBlock.position() > 0 && prevBlock.text().trimmed().isEmpty()) {
        length += prevBlock.text().length() + 1;
        prevBlock = prevBlock.previous();
    }

    return length;
}

static void modifyToIndentEmptyLines(
    QByteArray &buffer, int offset, int &length, const QTextBlock &block, bool secondTry)
{
    const QString blockText = block.text().trimmed();
    const bool closingParenBlock = blockText.startsWith(')');
    if (blockText.isEmpty() || closingParenBlock) {
        //This extra text works for the most cases.
        QByteArray dummyText("a;");

        // Search for previous character
        QTextBlock prevBlock = block.previous();
        bool prevBlockIsEmpty = prevBlock.position() > 0 && prevBlock.text().trimmed().isEmpty();
        while (prevBlockIsEmpty) {
            prevBlock = prevBlock.previous();
            prevBlockIsEmpty = prevBlock.position() > 0 && prevBlock.text().trimmed().isEmpty();
        }
        if (prevBlock.text().endsWith(','))
            dummyText = "int a";

        if (closingParenBlock) {
            if (prevBlock.text().endsWith(','))
                dummyText = "int a";
            else
                dummyText = "&& a";
        }

        length += dummyText.length();
        buffer.insert(offset, dummyText);
    }

    if (secondTry) {
        int nextLinePos = buffer.indexOf('\n', offset);
        if (nextLinePos > 0) {
            // If first try was not successful try to put ')' in the end of the line to close possibly
            // unclosed parentheses.
            // TODO: Does it help to add different endings depending on the context?
            buffer.insert(nextLinePos, ')');
            length += 1;
        }
    }
}

static const int kMaxLinesFromCurrentBlock = 200;

static Utils::LineColumn utf16LineColumn(const QTextBlock &block,
                                         int blockOffsetUtf8,
                                         const QByteArray &utf8Buffer,
                                         int utf8Offset)
{
    // If lastIndexOf('\n') returns -1 then we are fine to add 1 and get 0 offset.
    const int lineStartUtf8Offset = utf8Buffer.lastIndexOf('\n', utf8Offset - 1) + 1;
    int line = block.blockNumber() + 1; // Init with the line corresponding the block.

    if (utf8Offset < blockOffsetUtf8) {
        line -= static_cast<int>(std::count(utf8Buffer.begin() + lineStartUtf8Offset,
                                            utf8Buffer.begin() + blockOffsetUtf8,
                                            '\n'));
    } else {
        line += static_cast<int>(std::count(utf8Buffer.begin() + blockOffsetUtf8,
                                            utf8Buffer.begin() + lineStartUtf8Offset,
                                            '\n'));
    }

    const QByteArray lineText = utf8Buffer.mid(lineStartUtf8Offset,
                                               utf8Offset - lineStartUtf8Offset);
    return Utils::LineColumn(line, QString::fromUtf8(lineText).size() + 1);
}

static TextEditor::Replacements utf16Replacements(const QTextBlock &block,
                                                  int blockOffsetUtf8,
                                                  const QByteArray &utf8Buffer,
                                                  const clang::tooling::Replacements &replacements)
{
    TextEditor::Replacements convertedReplacements;
    convertedReplacements.reserve(replacements.size());
    for (const clang::tooling::Replacement &replacement : replacements) {
        const Utils::LineColumn lineColUtf16 = utf16LineColumn(block,
                                                               blockOffsetUtf8,
                                                               utf8Buffer,
                                                               static_cast<int>(
                                                                   replacement.getOffset()));
        if (!lineColUtf16.isValid())
            continue;
        const int utf16Offset = Utils::Text::positionInText(block.document(),
                                                            lineColUtf16.line,
                                                            lineColUtf16.column);
        const int utf16Length = QString::fromUtf8(
                                    utf8Buffer.mid(static_cast<int>(replacement.getOffset()),
                                                   static_cast<int>(replacement.getLength())))
                                    .size();
        convertedReplacements.emplace_back(utf16Offset,
                                           utf16Length,
                                           QString::fromStdString(replacement.getReplacementText()));
    }

    return convertedReplacements;
}

static void applyReplacements(const QTextBlock &block, const TextEditor::Replacements &replacements)
{
    if (replacements.empty())
        return;

    int fullOffsetShift = 0;
    QTextCursor editCursor(block);
    for (const TextEditor::Replacement &replacement : replacements) {
        editCursor.beginEditBlock();
        editCursor.setPosition(replacement.offset + fullOffsetShift);
        editCursor.movePosition(QTextCursor::NextCharacter,
                                QTextCursor::KeepAnchor,
                                replacement.length);
        editCursor.removeSelectedText();
        editCursor.insertText(replacement.text);
        editCursor.endEditBlock();
        fullOffsetShift += replacement.text.length() - replacement.length;
    }
}

static QString selectedLines(QTextDocument *doc,
                             const QTextBlock &startBlock,
                             const QTextBlock &endBlock)
{
    QString text = Utils::Text::textAt(QTextCursor(doc),
                                       startBlock.position(),
                                       std::max(0,
                                                endBlock.position() + endBlock.length()
                                                    - startBlock.position() - 1));
    while (!text.isEmpty() && text.rbegin()->isSpace())
        text.chop(1);
    return text;
}

ClangFormatBaseIndenter::ClangFormatBaseIndenter(QTextDocument *doc)
    : TextEditor::Indenter(doc)
{}

TextEditor::IndentationForBlock ClangFormatBaseIndenter::indentationForBlocks(
    const QVector<QTextBlock> &blocks,
    const TextEditor::TabSettings & /*tabSettings*/,
    int cursorPositionInEditor)
{
    TextEditor::IndentationForBlock ret;
    for (QTextBlock block : blocks)
        ret.insert(block.blockNumber(), indentFor(block, cursorPositionInEditor));
    return ret;
}

void ClangFormatBaseIndenter::indent(const QTextCursor &cursor,
                                     const QChar &typedChar,
                                     int cursorPositionInEditor)
{
    if (cursor.hasSelection()) {
        // Calling currentBlock.next() might be unsafe because we change the document.
        // Let's operate with block numbers instead.
        const int startNumber = m_doc->findBlock(cursor.selectionStart()).blockNumber();
        const int endNumber = m_doc->findBlock(cursor.selectionEnd()).blockNumber();
        for (int currentBlockNumber = startNumber; currentBlockNumber <= endNumber;
             ++currentBlockNumber) {
            const QTextBlock currentBlock = m_doc->findBlockByNumber(currentBlockNumber);
            if (currentBlock.isValid()) {
                const int blocksAmount = m_doc->blockCount();
                indentBlock(currentBlock, typedChar, cursorPositionInEditor);
                QTC_CHECK(blocksAmount == m_doc->blockCount()
                          && "ClangFormat plugin indentation changed the amount of blocks.");
            }
        }
    } else {
        indentBlock(cursor.block(), typedChar, cursorPositionInEditor);
    }
}

void ClangFormatBaseIndenter::indent(const QTextCursor &cursor,
                                     const QChar &typedChar,
                                     const TextEditor::TabSettings & /*tabSettings*/,
                                     int cursorPositionInEditor)
{
    indent(cursor, typedChar, cursorPositionInEditor);
}

void ClangFormatBaseIndenter::reindent(const QTextCursor &cursor,
                                       const TextEditor::TabSettings & /*tabSettings*/,
                                       int cursorPositionInEditor)
{
    indent(cursor, QChar::Null, cursorPositionInEditor);
}

TextEditor::Replacements ClangFormatBaseIndenter::format(const QTextCursor &cursor,
                                                         int cursorPositionInEditor)
{
    int utf8Offset;
    int utf8Length;
    const QByteArray buffer = m_doc->toPlainText().toUtf8();
    QTextBlock block = cursor.block();
    if (cursor.hasSelection()) {
        block = m_doc->findBlock(cursor.selectionStart());
        const QTextBlock end = m_doc->findBlock(cursor.selectionEnd());
        utf8Offset = Utils::Text::utf8NthLineOffset(m_doc, buffer, block.blockNumber() + 1);
        QTC_ASSERT(utf8Offset >= 0, return TextEditor::Replacements(););
        utf8Length = selectedLines(m_doc, block, end).toUtf8().size();

    } else {
        const QTextBlock block = cursor.block();
        utf8Offset = Utils::Text::utf8NthLineOffset(m_doc, buffer, block.blockNumber() + 1);
        QTC_ASSERT(utf8Offset >= 0, return TextEditor::Replacements(););
        utf8Length = block.text().toUtf8().size();
    }

    const TextEditor::Replacements toReplace
        = replacements(buffer, utf8Offset, utf8Length, block, QChar::Null, false);
    applyReplacements(block, toReplace);

    return toReplace;
}

TextEditor::Replacements ClangFormatBaseIndenter::format(
    const QTextCursor &cursor,
    const TextEditor::TabSettings & /*tabSettings*/,
    int cursorPositionInEditor)
{
    return format(cursor, cursorPositionInEditor);
}

void ClangFormatBaseIndenter::indentBlock(const QTextBlock &block,
                                          const QChar &typedChar,
                                          int /*cursorPositionInEditor*/)
{
    trimFirstNonEmptyBlock(block);
    trimCurrentBlock(block);
    const QByteArray buffer = m_doc->toPlainText().toUtf8();
    const int utf8Offset = Utils::Text::utf8NthLineOffset(m_doc, buffer, block.blockNumber() + 1);
    QTC_ASSERT(utf8Offset >= 0, return;);

    applyReplacements(block, replacements(buffer, utf8Offset, 0, block, typedChar));
}

void ClangFormatBaseIndenter::indentBlock(const QTextBlock &block,
                                          const QChar &typedChar,
                                          const TextEditor::TabSettings & /*tabSettings*/,
                                          int cursorPositionInEditor)
{
    indentBlock(block, typedChar, cursorPositionInEditor);
}

int ClangFormatBaseIndenter::indentFor(const QTextBlock &block, int /*cursorPositionInEditor*/)
{
    trimFirstNonEmptyBlock(block);
    trimCurrentBlock(block);
    const QByteArray buffer = m_doc->toPlainText().toUtf8();
    const int utf8Offset = Utils::Text::utf8NthLineOffset(m_doc, buffer, block.blockNumber() + 1);
    QTC_ASSERT(utf8Offset >= 0, return 0;);

    const TextEditor::Replacements toReplace = replacements(buffer, utf8Offset, 0, block);

    if (toReplace.empty())
        return -1;

    const TextEditor::Replacement &replacement = toReplace.front();
    int afterLineBreak = replacement.text.lastIndexOf('\n');
    afterLineBreak = (afterLineBreak < 0) ? 0 : afterLineBreak + 1;
    return static_cast<int>(replacement.text.size() - afterLineBreak);
}

int ClangFormatBaseIndenter::indentFor(const QTextBlock &block,
                                       const TextEditor::TabSettings & /*tabSettings*/,
                                       int cursorPositionInEditor)
{
    return indentFor(block, cursorPositionInEditor);
}

bool ClangFormatBaseIndenter::isElectricCharacter(const QChar &ch) const
{
    switch (ch.toLatin1()) {
    case '{':
    case '}':
    case ':':
    case '#':
    case '<':
    case '>':
    case ';':
    case '(':
    case ')':
        return true;
    }
    return false;
}

void ClangFormatBaseIndenter::formatOrIndent(const QTextCursor &cursor,
                                             const TextEditor::TabSettings & /*tabSettings*/,
                                             int cursorPositionInEditor)
{
    if (formatCodeInsteadOfIndent())
        format(cursor, cursorPositionInEditor);
    else
        indent(cursor, QChar::Null, cursorPositionInEditor);
}

clang::format::FormatStyle ClangFormatBaseIndenter::styleForFile() const
{
    llvm::Expected<clang::format::FormatStyle> style
        = clang::format::getStyle("file", m_fileName.toString().toStdString(), "none");
    if (style)
        return *style;

    handleAllErrors(style.takeError(), [](const llvm::ErrorInfoBase &) {
        // do nothing
    });

    return clang::format::getLLVMStyle();
}

TextEditor::Replacements ClangFormatBaseIndenter::replacements(QByteArray buffer,
                                                               int utf8Offset,
                                                               int utf8Length,
                                                               const QTextBlock &block,
                                                               const QChar &typedChar,
                                                               bool onlyIndention,
                                                               bool secondTry) const
{
    clang::format::FormatStyle style = styleForFile();

    int originalOffsetUtf8 = utf8Offset;
    int originalLengthUtf8 = utf8Length;
    QByteArray originalBuffer = buffer;

    int extraOffset = 0;
    if (onlyIndention) {
        if (block.blockNumber() > kMaxLinesFromCurrentBlock) {
            extraOffset = Utils::Text::utf8NthLineOffset(block.document(),
                                                         buffer,
                                                         block.blockNumber()
                                                             - kMaxLinesFromCurrentBlock);
        }
        int endOffset = Utils::Text::utf8NthLineOffset(block.document(),
                                                       buffer,
                                                       block.blockNumber()
                                                           + kMaxLinesFromCurrentBlock);
        if (endOffset == -1)
            endOffset = buffer.size();

        buffer = buffer.mid(extraOffset, endOffset - extraOffset);
        utf8Offset -= extraOffset;

        const int emptySpaceLength = previousEmptyLinesLength(block);
        utf8Offset -= emptySpaceLength;
        buffer.remove(utf8Offset, emptySpaceLength);

        extraOffset += emptySpaceLength;

        adjustFormatStyleForLineBreak(style);
        if (typedChar == QChar::Null)
            modifyToIndentEmptyLines(buffer, utf8Offset, utf8Length, block, secondTry);
    }

    std::vector<clang::tooling::Range> ranges{
        {static_cast<unsigned int>(utf8Offset), static_cast<unsigned int>(utf8Length)}};
    clang::format::FormattingAttemptStatus status;

    clang::tooling::Replacements clangReplacements = reformat(style,
                                                              buffer.data(),
                                                              ranges,
                                                              m_fileName.toString().toStdString(),
                                                              &status);

    if (!status.FormatComplete)
        return TextEditor::Replacements();

    const clang::tooling::Replacements filtered = filteredReplacements(clangReplacements,
                                                                       utf8Offset,
                                                                       extraOffset,
                                                                       onlyIndention);
    const bool canTryAgain = onlyIndention && typedChar == QChar::Null && !secondTry;
    if (canTryAgain && filtered.empty()) {
        return replacements(originalBuffer,
                            originalOffsetUtf8,
                            originalLengthUtf8,
                            block,
                            typedChar,
                            onlyIndention,
                            true);
    }

    return utf16Replacements(block, originalOffsetUtf8, originalBuffer, filtered);
}

} // namespace ClangFormat
