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

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>

#include <QTextDocument>

namespace ClangFormat {

namespace {
void adjustFormatStyleForLineBreak(clang::format::FormatStyle &style,
                                   ReplacementsToKeep replacementsToKeep)
{
    style.MaxEmptyLinesToKeep = 100;
    style.SortIncludes = false;
    style.SortUsingDeclarations = false;

    // This is a separate pass, don't do it unless it's the full formatting.
    style.FixNamespaceComments = false;

    if (replacementsToKeep == ReplacementsToKeep::IndentAndBefore)
        return;

    style.DisableFormat = false;
    style.ColumnLimit = 0;
#ifdef KEEP_LINE_BREAKS_FOR_NON_EMPTY_LINES_BACKPORTED
    style.KeepLineBreaksForNonEmptyLines = true;
#endif
}

llvm::StringRef clearExtraNewline(llvm::StringRef text)
{
    while (text.startswith("\n\n"))
        text = text.drop_front();
    return text;
}

clang::tooling::Replacements filteredReplacements(const QByteArray &buffer,
                                                  const clang::tooling::Replacements &replacements,
                                                  int utf8Offset,
                                                  int utf8Length,
                                                  ReplacementsToKeep replacementsToKeep)
{
    clang::tooling::Replacements filtered;
    for (const clang::tooling::Replacement &replacement : replacements) {
        int replacementOffset = static_cast<int>(replacement.getOffset());

        // Skip everything after.
        if (replacementOffset >= utf8Offset + utf8Length)
            return filtered;

        const bool isNotIndentOrInRange = replacementOffset < utf8Offset - 1
                                          || buffer.at(replacementOffset) != '\n';
        if (isNotIndentOrInRange && replacementsToKeep == ReplacementsToKeep::OnlyIndent)
            continue;

        llvm::StringRef text = replacementsToKeep == ReplacementsToKeep::OnlyIndent
                                   ? clearExtraNewline(replacement.getReplacementText())
                                   : replacement.getReplacementText();

        llvm::Error error = filtered.add(
            clang::tooling::Replacement(replacement.getFilePath(),
                                        static_cast<unsigned int>(replacementOffset),
                                        replacement.getLength(),
                                        text));
        // Throws if error is not checked.
        if (error) {
            error = llvm::handleErrors(std::move(error),
                                       [](const llvm::ErrorInfoBase &) -> llvm::Error {
                                           return llvm::Error::success();
                                       });
            QTC_CHECK(!error && "Error must be a \"success\" at this point");
            break;
        }
    }
    return filtered;
}

void trimRHSWhitespace(const QTextBlock &block)
{
    const QString initialText = block.text();
    if (!initialText.rbegin()->isSpace())
        return;

    auto lastNonSpace = std::find_if_not(initialText.rbegin(),
                                         initialText.rend(),
                                         [](const QChar &letter) { return letter.isSpace(); });
    const int extraSpaceCount = static_cast<int>(std::distance(initialText.rbegin(), lastNonSpace));

    QTextCursor cursor(block);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Right,
                        QTextCursor::MoveAnchor,
                        initialText.size() - extraSpaceCount);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, extraSpaceCount);
    cursor.removeSelectedText();
    cursor.endEditBlock();
}

// Add extra text in case of the empty line or the line starting with ')'.
// Track such extra pieces of text in isInsideModifiedLine().
int forceIndentWithExtraText(QByteArray &buffer, const QTextBlock &block, bool secondTry)
{
    const QString blockText = block.text();
    int firstNonWhitespace = Utils::indexOf(blockText,
                                            [](const QChar &ch) { return !ch.isSpace(); });
    int utf8Offset = Utils::Text::utf8NthLineOffset(block.document(),
                                                    buffer,
                                                    block.blockNumber() + 1);
    if (firstNonWhitespace > 0)
        utf8Offset += firstNonWhitespace;
    else
        utf8Offset += blockText.length();

    const bool closingParenBlock = firstNonWhitespace >= 0
                                   && blockText.at(firstNonWhitespace) == ')';

    int extraLength = 0;
    if (firstNonWhitespace < 0 || closingParenBlock) {
        //This extra text works for the most cases.
        QByteArray dummyText("a;a;");

        // Search for previous character
        QTextBlock prevBlock = block.previous();
        bool prevBlockIsEmpty = prevBlock.position() > 0 && prevBlock.text().trimmed().isEmpty();
        while (prevBlockIsEmpty) {
            prevBlock = prevBlock.previous();
            prevBlockIsEmpty = prevBlock.position() > 0 && prevBlock.text().trimmed().isEmpty();
        }
        if (closingParenBlock || prevBlock.text().endsWith(','))
            dummyText = "&& a";

        buffer.insert(utf8Offset, dummyText);
        extraLength += dummyText.length();
    }

    if (secondTry) {
        int nextLinePos = buffer.indexOf('\n', utf8Offset);
        if (nextLinePos < 0)
            nextLinePos = buffer.size() - 1;

        if (nextLinePos > 0) {
            // If first try was not successful try to put ')' in the end of the line to close possibly
            // unclosed parentheses.
            // TODO: Does it help to add different endings depending on the context?
            buffer.insert(nextLinePos, ')');
            extraLength += 1;
        }
    }

    return extraLength;
}

bool isInsideModifiedLine(const QString &originalLine, const QString &modifiedLine, int column)
{
    // Detect the cases when we have inserted extra text into the line to get the indentation.
    return originalLine.length() < modifiedLine.length() && column != modifiedLine.length() + 1
           && (column > originalLine.length() || originalLine.trimmed().isEmpty()
               || !modifiedLine.startsWith(originalLine));
}

TextEditor::Replacements utf16Replacements(const QTextDocument *doc,
                                           const QByteArray &utf8Buffer,
                                           const clang::tooling::Replacements &replacements)
{
    TextEditor::Replacements convertedReplacements;
    convertedReplacements.reserve(replacements.size());

    for (const clang::tooling::Replacement &replacement : replacements) {
        Utils::LineColumn lineColUtf16 = Utils::Text::utf16LineColumn(utf8Buffer,
                                                                      static_cast<int>(
                                                                          replacement.getOffset()));
        if (!lineColUtf16.isValid())
            continue;

        const QString lineText = doc->findBlockByNumber(lineColUtf16.line - 1).text();
        const QString bufferLineText
            = Utils::Text::utf16LineTextInUtf8Buffer(utf8Buffer,
                                                     static_cast<int>(replacement.getOffset()));
        if (isInsideModifiedLine(lineText, bufferLineText, lineColUtf16.column))
            continue;

        lineColUtf16.column = std::min(lineColUtf16.column, lineText.length() + 1);

        const int utf16Offset = Utils::Text::positionInText(doc,
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

void applyReplacements(QTextDocument *doc, const TextEditor::Replacements &replacements)
{
    if (replacements.empty())
        return;

    int fullOffsetShift = 0;
    QTextCursor editCursor(doc);
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

QString selectedLines(QTextDocument *doc, const QTextBlock &startBlock, const QTextBlock &endBlock)
{
    return Utils::Text::textAt(QTextCursor(doc),
                               startBlock.position(),
                               std::max(0,
                                        endBlock.position() + endBlock.length()
                                            - startBlock.position() - 1));
}

int indentationForBlock(const TextEditor::Replacements &toReplace,
                        const QByteArray &buffer,
                        const QTextBlock &currentBlock)
{
    const int utf8Offset = Utils::Text::utf8NthLineOffset(currentBlock.document(),
                                                          buffer,
                                                          currentBlock.blockNumber() + 1);
    auto replacementIt = std::find_if(toReplace.begin(),
                                      toReplace.end(),
                                      [utf8Offset](const TextEditor::Replacement &replacement) {
                                          return replacement.offset == utf8Offset - 1;
                                      });
    if (replacementIt == toReplace.end())
        return -1;

    int afterLineBreak = replacementIt->text.lastIndexOf('\n');
    afterLineBreak = (afterLineBreak < 0) ? 0 : afterLineBreak + 1;
    return static_cast<int>(replacementIt->text.size() - afterLineBreak);
}

bool doNotIndentInContext(QTextDocument *doc, int pos)
{
    const QChar character = doc->characterAt(pos);
    const QTextBlock currentBlock = doc->findBlock(pos);
    const QString text = currentBlock.text().left(pos - currentBlock.position());
    // NOTE: check if "<<" and ">>" always work correctly.
    switch (character.toLatin1()) {
    default:
        break;
    case ':':
        // Do not indent when it's the first ':' and it's not the 'case' line.
        if (text.contains(QLatin1String("case")) || text.contains(QLatin1String("default"))
            || text.contains(QLatin1String("public")) || text.contains(QLatin1String("private"))
            || text.contains(QLatin1String("protected")) || text.contains(QLatin1String("signals"))
            || text.contains(QLatin1String("Q_SIGNALS"))) {
            return false;
        }
        if (pos > 0 && doc->characterAt(pos - 1) != ':')
            return true;
        break;
    }

    return false;
}

QTextBlock reverseFindLastEmptyBlock(QTextBlock start)
{
    if (start.position() > 0) {
        start = start.previous();
        while (start.position() > 0 && start.text().trimmed().isEmpty())
            start = start.previous();
        if (!start.text().trimmed().isEmpty())
            start = start.next();
    }
    return start;
}

int formattingRangeStart(const QTextBlock &currentBlock,
                         const QByteArray &buffer,
                         int documentRevision)
{
    QTextBlock prevBlock = currentBlock.previous();
    while ((prevBlock.position() > 0 || prevBlock.length() > 0)
           && prevBlock.revision() != documentRevision) {
        // Find the first block with not matching revision.
        prevBlock = prevBlock.previous();
    }
    if (prevBlock.revision() == documentRevision)
        prevBlock = prevBlock.next();

    return Utils::Text::utf8NthLineOffset(prevBlock.document(), buffer, prevBlock.blockNumber() + 1);
}
} // namespace

ClangFormatBaseIndenter::ClangFormatBaseIndenter(QTextDocument *doc)
    : TextEditor::Indenter(doc)
{}

TextEditor::Replacements ClangFormatBaseIndenter::replacements(QByteArray buffer,
                                                               const QTextBlock &startBlock,
                                                               const QTextBlock &endBlock,
                                                               ReplacementsToKeep replacementsToKeep,
                                                               const QChar &typedChar,
                                                               bool secondTry) const
{
    QTC_ASSERT(replacementsToKeep != ReplacementsToKeep::All, return TextEditor::Replacements());

    clang::format::FormatStyle style = styleForFile();
    QByteArray originalBuffer = buffer;

    int utf8Offset = Utils::Text::utf8NthLineOffset(m_doc, buffer, startBlock.blockNumber() + 1);
    QTC_ASSERT(utf8Offset >= 0, return TextEditor::Replacements(););
    int utf8Length = selectedLines(m_doc, startBlock, endBlock).toUtf8().size();

    int rangeStart = 0;
    if (replacementsToKeep == ReplacementsToKeep::IndentAndBefore)
        rangeStart = formattingRangeStart(startBlock, buffer, lastSaveRevision());

    if (replacementsToKeep == ReplacementsToKeep::IndentAndBefore) {
        buffer.insert(utf8Offset - 1, " //");
        utf8Offset += 3;
    }

    adjustFormatStyleForLineBreak(style, replacementsToKeep);
    if (typedChar == QChar::Null) {
        for (int index = startBlock.blockNumber(); index <= endBlock.blockNumber(); ++index) {
            utf8Length += forceIndentWithExtraText(buffer,
                                                   m_doc->findBlockByNumber(index),
                                                   secondTry);
        }
    }

    if (replacementsToKeep != ReplacementsToKeep::IndentAndBefore || utf8Offset < rangeStart)
        rangeStart = utf8Offset;

    unsigned int rangeLength = static_cast<unsigned int>(utf8Offset + utf8Length - rangeStart);
    std::vector<clang::tooling::Range> ranges{{static_cast<unsigned int>(rangeStart), rangeLength}};

    clang::format::FormattingAttemptStatus status;
    clang::tooling::Replacements clangReplacements = reformat(style,
                                                              buffer.data(),
                                                              ranges,
                                                              m_fileName.toString().toStdString(),
                                                              &status);

    clang::tooling::Replacements filtered;
    if (status.FormatComplete) {
        filtered = filteredReplacements(buffer,
                                        clangReplacements,
                                        utf8Offset,
                                        utf8Length,
                                        replacementsToKeep);
    }
    const bool canTryAgain = replacementsToKeep == ReplacementsToKeep::OnlyIndent
                             && typedChar == QChar::Null && !secondTry;
    if (canTryAgain && filtered.empty()) {
        return replacements(originalBuffer,
                            startBlock,
                            endBlock,
                            replacementsToKeep,
                            typedChar,
                            true);
    }

    return utf16Replacements(m_doc, buffer, filtered);
}

TextEditor::Replacements ClangFormatBaseIndenter::format(
    const TextEditor::RangesInLines &rangesInLines)
{
    if (rangesInLines.empty())
        return TextEditor::Replacements();

    const QByteArray buffer = m_doc->toPlainText().toUtf8();
    std::vector<clang::tooling::Range> ranges;
    ranges.reserve(rangesInLines.size());

    for (auto &range : rangesInLines) {
        const int utf8StartOffset = Utils::Text::utf8NthLineOffset(m_doc, buffer, range.startLine);
        int utf8RangeLength = m_doc->findBlockByNumber(range.endLine - 1).text().toUtf8().size();
        if (range.endLine > range.startLine) {
            utf8RangeLength += Utils::Text::utf8NthLineOffset(m_doc, buffer, range.endLine)
                               - utf8StartOffset;
        }
        ranges.emplace_back(static_cast<unsigned int>(utf8StartOffset),
                            static_cast<unsigned int>(utf8RangeLength));
    }

    clang::format::FormattingAttemptStatus status;
    const clang::tooling::Replacements clangReplacements
        = reformat(styleForFile(), buffer.data(), ranges, m_fileName.toString().toStdString(), &status);
    const TextEditor::Replacements toReplace = utf16Replacements(m_doc, buffer, clangReplacements);
    applyReplacements(m_doc, toReplace);

    return toReplace;
}

TextEditor::Replacements ClangFormatBaseIndenter::indentsFor(QTextBlock startBlock,
                                                             const QTextBlock &endBlock,
                                                             const QByteArray &buffer,
                                                             const QChar &typedChar,
                                                             int cursorPositionInEditor)
{
    if (typedChar != QChar::Null && cursorPositionInEditor > 0
        && m_doc->characterAt(cursorPositionInEditor - 1) == typedChar
        && doNotIndentInContext(m_doc, cursorPositionInEditor - 1)) {
        return TextEditor::Replacements();
    }

    startBlock = reverseFindLastEmptyBlock(startBlock);
    const int startBlockPosition = startBlock.position();
    if (startBlock.position() > 0) {
        trimRHSWhitespace(startBlock.previous());
        if (cursorPositionInEditor >= 0)
            cursorPositionInEditor += startBlock.position() - startBlockPosition;
    }

    ReplacementsToKeep replacementsToKeep = ReplacementsToKeep::OnlyIndent;
    if (formatWhileTyping()
        && (cursorPositionInEditor == -1 || cursorPositionInEditor >= startBlockPosition)
        && (typedChar == QChar::Null || typedChar == ';' || typedChar == '}')) {
        // Format before current position only in case the cursor is inside the indented block.
        // So if cursor position is less then the block position then the current line is before
        // the indented block - don't trigger extra formatting in this case.
        // cursorPositionInEditor == -1 means the consition matches automatically.

        // Format only before newline or complete statement not to break code.
        replacementsToKeep = ReplacementsToKeep::IndentAndBefore;
    }

    return replacements(buffer,
                        startBlock,
                        endBlock,
                        replacementsToKeep,
                        typedChar);
}

void ClangFormatBaseIndenter::indentBlocks(const QTextBlock &startBlock,
                                           const QTextBlock &endBlock,
                                           const QChar &typedChar,
                                           int cursorPositionInEditor)
{
    const QByteArray buffer = m_doc->toPlainText().toUtf8();
    applyReplacements(m_doc,
                      indentsFor(startBlock, endBlock, buffer, typedChar, cursorPositionInEditor));
}

void ClangFormatBaseIndenter::indent(const QTextCursor &cursor,
                                     const QChar &typedChar,
                                     int cursorPositionInEditor)
{
    if (cursor.hasSelection()) {
        indentBlocks(m_doc->findBlock(cursor.selectionStart()),
                     m_doc->findBlock(cursor.selectionEnd()),
                     typedChar,
                     cursorPositionInEditor);
    } else {
        indentBlocks(cursor.block(), cursor.block(), typedChar, cursorPositionInEditor);
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

void ClangFormatBaseIndenter::indentBlock(const QTextBlock &block,
                                          const QChar &typedChar,
                                          const TextEditor::TabSettings & /*tabSettings*/,
                                          int cursorPositionInEditor)
{
    indentBlocks(block, block, typedChar, cursorPositionInEditor);
}

int ClangFormatBaseIndenter::indentFor(const QTextBlock &block,
                                       const TextEditor::TabSettings & /*tabSettings*/,
                                       int cursorPositionInEditor)
{
    const QByteArray buffer = m_doc->toPlainText().toUtf8();
    TextEditor::Replacements toReplace = indentsFor(block,
                                                    block,
                                                    buffer,
                                                    QChar::Null,
                                                    cursorPositionInEditor);
    if (toReplace.empty())
        return -1;

    return indentationForBlock(toReplace, buffer, block);
}

TextEditor::IndentationForBlock ClangFormatBaseIndenter::indentationForBlocks(
    const QVector<QTextBlock> &blocks,
    const TextEditor::TabSettings & /*tabSettings*/,
    int cursorPositionInEditor)
{
    TextEditor::IndentationForBlock ret;
    if (blocks.isEmpty())
        return ret;
    const QByteArray buffer = m_doc->toPlainText().toUtf8();
    TextEditor::Replacements toReplace = indentsFor(blocks.front(),
                                                    blocks.back(),
                                                    buffer,
                                                    QChar::Null,
                                                    cursorPositionInEditor);

    for (const QTextBlock &block : blocks)
        ret.insert(block.blockNumber(), indentationForBlock(toReplace, buffer, block));
    return ret;
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
    if (formatCodeInsteadOfIndent()) {
        QTextBlock start;
        QTextBlock end;
        if (cursor.hasSelection()) {
            start = m_doc->findBlock(cursor.selectionStart());
            end = m_doc->findBlock(cursor.selectionEnd());
        } else {
            start = end = cursor.block();
        }
        format({{start.blockNumber() + 1, end.blockNumber() + 1}});
    } else {
        indent(cursor, QChar::Null, cursorPositionInEditor);
    }
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

} // namespace ClangFormat
