// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatbaseindenter.h"
#include "clangformatutils.h"
#include "llvmfilesystem.h"

#include <coreplugin/icore.h>

#include <cppeditor/cppcodestylepreferences.h>
#include <cppeditor/cpptoolssettings.h>

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/textutils.h>

#include <QDebug>
#include <QTextDocument>

#include <clang/Tooling/Core/Replacement.h>

namespace ClangFormat {

Internal::LlvmFileSystemAdapter llvmFileSystemAdapter = {};

namespace {
void adjustFormatStyleForLineBreak(clang::format::FormatStyle &style,
                                   ReplacementsToKeep replacementsToKeep)
{
    style.MaxEmptyLinesToKeep = 100;
#if LLVM_VERSION_MAJOR >= 13
    style.SortIncludes = clang::format::FormatStyle::SI_Never;
#else
    style.SortIncludes = false;
#endif
#if LLVM_VERSION_MAJOR >= 16
    style.SortUsingDeclarations = clang::format::FormatStyle::SUD_Never;
#else
    style.SortUsingDeclarations = false;
#endif

    // This is a separate pass, don't do it unless it's the full formatting.
    style.FixNamespaceComments = false;
#if LLVM_VERSION_MAJOR >= 16
    style.AlignTrailingComments = {clang::format::FormatStyle::TCAS_Never, 0};
#else
    style.AlignTrailingComments = false;
#endif

    if (replacementsToKeep == ReplacementsToKeep::IndentAndBefore)
        return;

    style.ColumnLimit = 0;
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
        if (replacementsToKeep == ReplacementsToKeep::OnlyIndent && int(text.count('\n'))
                != buffer.mid(replacementOffset, replacement.getLength()).count('\n')) {
            continue;
        }


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
    cursor.movePosition(QTextCursor::Right,
                        QTextCursor::MoveAnchor,
                        initialText.size() - extraSpaceCount);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, extraSpaceCount);
    cursor.removeSelectedText();
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

QTextBlock reverseFindLastBlockWithSymbol(QTextBlock start, QChar ch)
{
    if (start.position() > 0) {
        start = start.previous();
        while (start.position() > 0 && !start.text().contains(ch))
            start = start.previous();
    }
    return start;
}

enum class CharacterContext {
    AfterComma,
    LastAfterComma,
    NewStatementOrContinuation,
    IfOrElseWithoutScope,
    Unknown
};

QChar findFirstNonWhitespaceCharacter(const QTextBlock &currentBlock)
{
    const QTextDocument *doc = currentBlock.document();
    int currentPos = currentBlock.position();
    while (currentPos < doc->characterCount() && doc->characterAt(currentPos).isSpace())
        ++currentPos;
    return currentPos < doc->characterCount() ? doc->characterAt(currentPos) : QChar::Null;
}

int findMatchingOpeningParen(const QTextBlock &blockEndingWithClosingParen)
{
    const QTextDocument *doc = blockEndingWithClosingParen.document();
    int currentPos = blockEndingWithClosingParen.position()
                     + blockEndingWithClosingParen.text().lastIndexOf(')');
    int parenBalance = 1;

    while (currentPos > 0 && parenBalance > 0) {
        --currentPos;
        if (doc->characterAt(currentPos) == ')')
            ++parenBalance;
        if (doc->characterAt(currentPos) == '(')
            --parenBalance;
    }

    if (parenBalance == 0)
        return currentPos;

    return -1;
}

bool comesDirectlyAfterIf(const QTextDocument *doc, int pos)
{
    --pos;
    while (pos > 0 && doc->characterAt(pos).isSpace())
        --pos;
    return pos > 0 && doc->characterAt(pos) == 'f' && doc->characterAt(pos - 1) == 'i';
}

CharacterContext characterContext(const QTextBlock &currentBlock)
{
    QTextBlock previousNonEmptyBlock = reverseFindLastEmptyBlock(currentBlock);
    if (previousNonEmptyBlock.position() > 0)
        previousNonEmptyBlock = previousNonEmptyBlock.previous();

    const QString prevLineText = previousNonEmptyBlock.text().trimmed();
    if (prevLineText.isEmpty())
        return CharacterContext::NewStatementOrContinuation;

    const QChar firstNonWhitespaceChar = findFirstNonWhitespaceCharacter(currentBlock);
    if (prevLineText.endsWith(',')) {
        if (firstNonWhitespaceChar == '}') {
            if (reverseFindLastBlockWithSymbol(currentBlock, '{').text().trimmed().last(1) == '{')
                return CharacterContext::NewStatementOrContinuation;
            return CharacterContext::LastAfterComma;
        }

        if (firstNonWhitespaceChar == ')') {
            if (reverseFindLastBlockWithSymbol(currentBlock, '(').text().trimmed().last(1) == '(')
                return CharacterContext::NewStatementOrContinuation;
            return CharacterContext::LastAfterComma;
        }

        return CharacterContext::AfterComma;
    }

    if (prevLineText.endsWith("else"))
        return CharacterContext::IfOrElseWithoutScope;
    if (prevLineText.endsWith(')')) {
        const int pos = findMatchingOpeningParen(previousNonEmptyBlock);
        if (pos >= 0 && comesDirectlyAfterIf(previousNonEmptyBlock.document(), pos))
            return CharacterContext::IfOrElseWithoutScope;
    }

    return CharacterContext::NewStatementOrContinuation;
}

bool nextBlockExistsAndEmpty(const QTextBlock &currentBlock)
{
    QTextBlock nextBlock = currentBlock.next();
    if (!nextBlock.isValid() || nextBlock.position() == currentBlock.position())
        return false;

    return nextBlock.text().trimmed().isEmpty();
}

QByteArray dummyTextForContext(CharacterContext context, bool closingBraceBlock)
{
    if (closingBraceBlock && context == CharacterContext::NewStatementOrContinuation)
        return QByteArray();

    switch (context) {
    case CharacterContext::AfterComma:
        return "a,";
    case CharacterContext::LastAfterComma:
        return "a";
    case CharacterContext::IfOrElseWithoutScope:
        return ";";
    case CharacterContext::NewStatementOrContinuation:
        return "/*//*/";
    case CharacterContext::Unknown:
    default:
        QTC_ASSERT(false, return "";);
    }
}

// Add extra text in case of the empty line or the line starting with ')'.
// Track such extra pieces of text in isInsideDummyTextInLine().
int forceIndentWithExtraText(QByteArray &buffer,
                             CharacterContext &charContext,
                             const QTextBlock &block,
                             bool secondTry)
{
    if (!block.isValid())
        return 0;

    auto tmpcharContext = characterContext(block);
    if (charContext == CharacterContext::LastAfterComma
        && tmpcharContext == CharacterContext::LastAfterComma) {
        charContext = CharacterContext::AfterComma;
    } else {
        charContext = tmpcharContext;
    }

    const QString blockText = block.text();
    int firstNonWhitespace = Utils::indexOf(blockText,
                                            [](const QChar &ch) { return !ch.isSpace(); });
    int utf8Offset = Utils::Text::utf8NthLineOffset(block.document(),
                                                    buffer,
                                                    block.blockNumber() + 1);
    if (firstNonWhitespace >= 0)
        utf8Offset += firstNonWhitespace;
    else
        utf8Offset += blockText.length();

    const bool closingParenBlock = firstNonWhitespace >= 0
                                   && blockText.at(firstNonWhitespace) == ')';
    const bool closingBraceBlock = firstNonWhitespace >= 0
                                   && blockText.at(firstNonWhitespace) == '}';

    int extraLength = 0;
    QByteArray dummyText;
    if (firstNonWhitespace < 0 && charContext != CharacterContext::Unknown
        && nextBlockExistsAndEmpty(block)) {
        // If the next line is also empty it's safer to use a comment line.
        dummyText = "//";
    } else if (firstNonWhitespace < 0 || closingParenBlock || closingBraceBlock) {
        dummyText = dummyTextForContext(charContext, closingBraceBlock);
    }

    // A comment at the end of the line appears to prevent clang-format from removing line breaks.
    if (dummyText == "/*//*/" || dummyText.isEmpty()) {
        if (block.previous().isValid()) {
            const int prevEndOffset = Utils::Text::utf8NthLineOffset(block.document(),
                                                                     buffer,
                                                                     block.blockNumber())
                                      + block.previous().text().toUtf8().length();
            buffer.insert(prevEndOffset, " //");
            extraLength += 3;
        }
    }
    buffer.insert(utf8Offset + extraLength, dummyText);
    extraLength += dummyText.length();

    if (secondTry) {
        int nextLinePos = buffer.indexOf('\n', utf8Offset);
        if (nextLinePos < 0)
            nextLinePos = buffer.size() - 1;

        if (nextLinePos > 0) {
            // If first try was not successful try to put ')' in the end of the line to close possibly
            // unclosed parenthesis.
            // TODO: Does it help to add different endings depending on the context?
            buffer.insert(nextLinePos, ')');
            extraLength += 1;
        }
    }

    return extraLength;
}

bool isInsideDummyTextInLine(const QString &originalLine, const QString &modifiedLine, int column)
{
    // Detect the cases when we have inserted extra text into the line to get the indentation.
    return originalLine.length() < modifiedLine.length() && column != modifiedLine.length() + 1
           && (column > originalLine.length() || originalLine.trimmed().isEmpty()
               || !modifiedLine.startsWith(originalLine));
}

static Utils::Text::Position utf16LineColumn(const QByteArray &utf8Buffer, int utf8Offset)
{
    Utils::Text::Position position;
    position.line = static_cast<int>(std::count(utf8Buffer.begin(),
                                                utf8Buffer.begin() + utf8Offset, '\n')) + 1;
    const int startOfLineOffset = utf8Offset ? (utf8Buffer.lastIndexOf('\n', utf8Offset - 1) + 1)
                                             : 0;
    position.column = QString::fromUtf8(utf8Buffer.mid(startOfLineOffset,
                                                       utf8Offset - startOfLineOffset)).length();
    return position;
}
Utils::ChangeSet convertReplacements(const QTextDocument *doc,
                                     const QByteArray &utf8Buffer,
                                     const clang::tooling::Replacements &replacements)
{
    Utils::ChangeSet convertedReplacements;

    for (const clang::tooling::Replacement &replacement : replacements) {
        Utils::Text::Position lineColUtf16 = utf16LineColumn(
            utf8Buffer, static_cast<int>(replacement.getOffset()));
        if (!lineColUtf16.isValid())
            continue;

        const QString lineText = doc->findBlockByNumber(lineColUtf16.line - 1).text();
        const QString bufferLineText
            = Utils::Text::utf16LineTextInUtf8Buffer(utf8Buffer,
                                                     static_cast<int>(replacement.getOffset()));
        if (isInsideDummyTextInLine(lineText, bufferLineText, lineColUtf16.column + 1))
            continue;

        lineColUtf16.column = std::min(lineColUtf16.column, int(lineText.length()));
        int utf16Offset = Utils::Text::positionInText(doc,
                                                      lineColUtf16.line,
                                                      lineColUtf16.column + 1);
        int utf16Length = QString::fromUtf8(
                              utf8Buffer.mid(static_cast<int>(replacement.getOffset()),
                                             static_cast<int>(replacement.getLength())))
                              .size();

        QString replacementText = QString::fromStdString(replacement.getReplacementText().str());
        replacementText.replace("\r", "");
        auto sameCharAt = [&](int replacementOffset) {
            if (replacementText.size() <= replacementOffset || replacementOffset < 0)
                return false;
            const QChar docChar = doc->characterAt(utf16Offset + replacementOffset);
            const QChar replacementChar = replacementText.at(replacementOffset);
            return docChar == replacementChar
                   || (docChar == QChar::ParagraphSeparator && replacementChar == '\n');
        };
        // remove identical prefix from replacement text
        while (sameCharAt(0)) {
            ++utf16Offset;
            --utf16Length;
            replacementText = replacementText.mid(1);
        }
        // remove identical suffix from replacement text
        while (sameCharAt(utf16Length - 1)) {
            --utf16Length;
            replacementText.chop(1);
        }

        if (!replacementText.isEmpty() || utf16Length > 0)
            convertedReplacements.replace(utf16Offset, utf16Offset + utf16Length, replacementText);
    }

    return convertedReplacements;
}

QString selectedLines(QTextDocument *doc, const QTextBlock &startBlock, const QTextBlock &endBlock)
{
    return Utils::Text::textAt(QTextCursor(doc),
                               startBlock.position(),
                               std::max(0,
                                        endBlock.position() + endBlock.length()
                                            - startBlock.position() - 1));
}

int indentationForBlock(const Utils::ChangeSet &toReplace,
                        const QByteArray &buffer,
                        const QTextBlock &currentBlock)
{
    const int utf8Offset = Utils::Text::utf8NthLineOffset(currentBlock.document(),
                                                          buffer,
                                                          currentBlock.blockNumber() + 1);
    auto ops = toReplace.operationList();

    auto replacementIt
        = std::find_if(ops.begin(), ops.end(), [utf8Offset](const Utils::ChangeSet::EditOp &op) {
              QTC_ASSERT(op.type == Utils::ChangeSet::EditOp::Replace, return false);
              return op.pos1 == utf8Offset - 1;
          });
    if (replacementIt == ops.end())
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

Utils::ChangeSet ClangFormatBaseIndenter::replacements(QByteArray buffer,
                                                       const QTextBlock &startBlock,
                                                       const QTextBlock &endBlock,
                                                       int cursorPositionInEditor,
                                                       ReplacementsToKeep replacementsToKeep,
                                                       const QChar &typedChar,
                                                       bool secondTry) const
{
    QTC_ASSERT(replacementsToKeep != ReplacementsToKeep::All, return Utils::ChangeSet());
    QTC_ASSERT(!m_fileName.isEmpty(), return {});

    QByteArray originalBuffer = buffer;
    int utf8Offset = Utils::Text::utf8NthLineOffset(m_doc, buffer, startBlock.blockNumber() + 1);
    QTC_ASSERT(utf8Offset >= 0, return Utils::ChangeSet(););
    int utf8Length = selectedLines(m_doc, startBlock, endBlock).toUtf8().size();

    int rangeStart = 0;
    if (replacementsToKeep == ReplacementsToKeep::IndentAndBefore)
        rangeStart = formattingRangeStart(startBlock, buffer, lastSaveRevision());

    clang::format::FormatStyle style = styleForFile();
    adjustFormatStyleForLineBreak(style, replacementsToKeep);
    if (replacementsToKeep == ReplacementsToKeep::OnlyIndent) {
        CharacterContext currentCharContext = CharacterContext::Unknown;
        // Iterate backwards to reuse the same dummy text for all empty lines.
        for (int index = endBlock.blockNumber(); index >= startBlock.blockNumber(); --index) {
            utf8Length += forceIndentWithExtraText(buffer,
                                                   currentCharContext,
                                                   m_doc->findBlockByNumber(index),
                                                   secondTry);
        }
    }

    if (replacementsToKeep != ReplacementsToKeep::IndentAndBefore || utf8Offset < rangeStart)
        rangeStart = utf8Offset;

    unsigned int rangeLength = static_cast<unsigned int>(utf8Offset + utf8Length - rangeStart);
    std::vector<clang::tooling::Range> ranges{{static_cast<unsigned int>(rangeStart), rangeLength}};

    clang::format::FormattingAttemptStatus status;
    clang::tooling::Replacements clangReplacements = clang::format::reformat(
        style, buffer.data(), ranges, m_fileName.toFSPathString().toStdString(), &status);

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
                            cursorPositionInEditor,
                            replacementsToKeep,
                            typedChar,
                            true);
    }

    return convertReplacements(m_doc, buffer, filtered);
}

Utils::EditOperations ClangFormatBaseIndenter::format(const TextEditor::RangesInLines &rangesInLines,
                                                      FormattingMode mode)
{
    bool doFormatting = mode == FormattingMode::Forced || formatCodeInsteadOfIndent();
#ifdef WITH_TESTS
    doFormatting = doFormatting || CppEditor::CppToolsSettings::cppCodeStyle()
            ->codeStyleSettings().forceFormatting;
#endif
    if (!doFormatting)
        return {};

    QTC_ASSERT(!m_fileName.isEmpty(), return {});
    if (rangesInLines.empty())
        return {};

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

    clang::format::FormatStyle style = styleForFile();
    const std::string assumedFileName = m_fileName.toFSPathString().toStdString();
    clang::tooling::Replacements clangReplacements = clang::format::sortIncludes(style,
                                                                                 buffer.data(),
                                                                                 ranges,
                                                                                 assumedFileName);
    auto changedCode = clang::tooling::applyAllReplacements(buffer.data(), clangReplacements);
    QTC_ASSERT(changedCode, {
        qDebug() << QString::fromStdString(llvm::toString(changedCode.takeError()));
        return {};
    });
    ranges = clang::tooling::calculateRangesAfterReplacements(clangReplacements, ranges);

    clang::format::FormattingAttemptStatus status;
    const clang::tooling::Replacements formatReplacements = clang::format::reformat(style,
                                                                                    *changedCode,
                                                                                    ranges,
                                                                                    assumedFileName,
                                                                                    &status);
    clangReplacements = clangReplacements.merge(formatReplacements);

    Utils::ChangeSet changeSet = convertReplacements(m_doc, buffer, clangReplacements);
    const Utils::EditOperations editOperations = changeSet.operationList();
    changeSet.apply(m_doc);

    return editOperations;
}

Utils::ChangeSet ClangFormatBaseIndenter::indentsFor(QTextBlock startBlock,
                                                              const QTextBlock &endBlock,
                                                              const QChar &typedChar,
                                                              int cursorPositionInEditor,
                                                              bool trimTrailingWhitespace)
{
    if (typedChar != QChar::Null && cursorPositionInEditor > 0
        && m_doc->characterAt(cursorPositionInEditor - 1) == typedChar
        && doNotIndentInContext(m_doc, cursorPositionInEditor - 1)) {
        return Utils::ChangeSet();
    }

    startBlock = reverseFindLastEmptyBlock(startBlock);
    const int startBlockPosition = startBlock.position();
    if (trimTrailingWhitespace && startBlockPosition > 0) {
        trimRHSWhitespace(startBlock.previous());
        if (cursorPositionInEditor >= 0)
            cursorPositionInEditor += startBlock.position() - startBlockPosition;
    }

    const QByteArray buffer = m_doc->toPlainText().toUtf8();

    ReplacementsToKeep replacementsToKeep = ReplacementsToKeep::OnlyIndent;
    if (formatWhileTyping()
        && (cursorPositionInEditor == -1 || cursorPositionInEditor >= startBlockPosition)
        && (typedChar == ';' || typedChar == '}')) {
        // Format before current position only in case the cursor is inside the indented block.
        // So if cursor position is less then the block position then the current line is before
        // the indented block - don't trigger extra formatting in this case.
        // cursorPositionInEditor == -1 means the condition matches automatically.

        // Format only before complete statement not to break code.
        replacementsToKeep = ReplacementsToKeep::IndentAndBefore;
    }

    return replacements(buffer,
                        startBlock,
                        endBlock,
                        cursorPositionInEditor,
                        replacementsToKeep,
                        typedChar);
}

void ClangFormatBaseIndenter::indentBlocks(const QTextBlock &startBlock,
                                           const QTextBlock &endBlock,
                                           const QChar &typedChar,
                                           int cursorPositionInEditor)
{
    Utils::ChangeSet changeset = indentsFor(startBlock, endBlock, typedChar, cursorPositionInEditor);
    changeset.apply(m_doc);
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
    Utils::ChangeSet toReplace
        = indentsFor(block, block, QChar::Null, cursorPositionInEditor, false);
    if (toReplace.isEmpty())
        return -1;

    const QByteArray buffer = m_doc->toPlainText().toUtf8();
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
    Utils::ChangeSet toReplace = indentsFor(blocks.front(),
                                                     blocks.back(),
                                                     QChar::Null,
                                                     cursorPositionInEditor);

    const QByteArray buffer = m_doc->toPlainText().toUtf8();
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

std::optional<int> ClangFormat::ClangFormatBaseIndenter::margin() const
{
    return styleForFile().ColumnLimit;
}

void ClangFormatBaseIndenter::autoIndent(const QTextCursor &cursor,
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

clang::format::FormatStyle ClangFormatBaseIndenter::overrideStyle(
    const Utils::FilePath &fileName) const
{
    const ProjectExplorer::Project *projectForFile
        = ProjectExplorer::ProjectManager::projectForFile(fileName);

    const TextEditor::ICodeStylePreferences *preferences
        = projectForFile
              ? projectForFile->editorConfiguration()->codeStyle("Cpp")->currentPreferences()
              : TextEditor::TextEditorSettings::codeStyle("Cpp")->currentPreferences();

    if (m_overriddenPreferences)
        preferences = m_overriddenPreferences->currentPreferences();

    Utils::FilePath filePath = filePathToCurrentSettings(preferences);

    if (!filePath.exists())
        return currentQtStyle(preferences);

    clang::format::FormatStyle currentSettingsStyle;
    currentSettingsStyle.Language = clang::format::FormatStyle::LK_Cpp;
    const std::error_code error = clang::format::parseConfiguration(filePath.fileContents()
                                                                        .value_or(QByteArray())
                                                                        .toStdString(),
                                                                    &currentSettingsStyle);
    QTC_ASSERT(error.value() == static_cast<int>(clang::format::ParseError::Success),
               return currentQtStyle(preferences));

    return currentSettingsStyle;
}

static std::chrono::milliseconds getCacheTimeout()
{
    using namespace std::chrono_literals;
    bool ok = false;
    const int envCacheTimeout = qEnvironmentVariableIntValue("CLANG_FORMAT_CACHE_TIMEOUT", &ok);
    return ok ? std::chrono::milliseconds(envCacheTimeout) : 1s;
}

const clang::format::FormatStyle &ClangFormatBaseIndenter::styleForFile() const
{
    using namespace std::chrono_literals;
    static const std::chrono::milliseconds cacheTimeout = getCacheTimeout();

    QDateTime time = QDateTime::currentDateTime();
    if (m_cachedStyle.expirationTime > time && !(m_cachedStyle.style == clang::format::getNoStyle()))
        return m_cachedStyle.style;

    if (getCurrentOverriddenSettings(m_fileName)) {
        clang::format::FormatStyle style = overrideStyle(m_fileName);
        m_cachedStyle.setCache(style, cacheTimeout);
        return m_cachedStyle.style;
    }

    llvm::Expected<clang::format::FormatStyle> styleFromProjectFolder
        = clang::format::getStyle("file",
                                  m_fileName.toFSPathString().toStdString(),
                                  "none",
                                  "",
                                  &llvmFileSystemAdapter);

    if (styleFromProjectFolder && !(*styleFromProjectFolder == clang::format::getNoStyle())) {
        addQtcStatementMacros(*styleFromProjectFolder);
        m_cachedStyle.setCache(*styleFromProjectFolder, cacheTimeout);
        return m_cachedStyle.style;
    }

    handleAllErrors(styleFromProjectFolder.takeError(), [](const llvm::ErrorInfoBase &) {
        // do nothing
    });


    m_cachedStyle.setCache(qtcStyle(), 0ms);
    return m_cachedStyle.style;
}

void ClangFormatBaseIndenter::setOverriddenPreferences(TextEditor::ICodeStylePreferences *preferences)
{
    m_overriddenPreferences = preferences;
}

} // namespace ClangFormat
