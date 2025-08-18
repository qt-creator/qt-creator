// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditorlayout.h"

#include "../qtcassert.h"

namespace Utils {

TextEditorLayout::TextEditorLayout(PlainTextDocumentLayout *docLayout)
    : PlainTextDocumentLayout(docLayout->document())
{
    connect(
        docLayout,
        &PlainTextDocumentLayout::documentContentsChanged,
        this,
        &TextEditorLayout::documentChanged);
    connect(
        docLayout,
        &PlainTextDocumentLayout::blockVisibilityChanged,
        this,
        &TextEditorLayout::handleBlockVisibilityChanged);
    connect(
        docLayout,
        &PlainTextDocumentLayout::documentSizeChanged,
        this,
        &TextEditorLayout::documentSizeChanged);
    connect(docLayout, &PlainTextDocumentLayout::update, this, &TextEditorLayout::update);
}

void TextEditorLayout::handleBlockVisibilityChanged(const QTextBlock &block)
{
    const int blockNumber = block.blockNumber();

    int lineCount = 0;
    LayoutData &data = layoutData(block.fragmentIndex());
    if (block.isVisible())
        lineCount = data.layout ? std::max(1, data.layout->lineCount()) : 1;
    data.lineCount = lineCount;
    resetOffsetCache(blockNumber);
}

QTextLayout *TextEditorLayout::blockLayout(const QTextBlock &block) const
{
    LayoutData &data = layoutData(block.fragmentIndex());
    QTextLayout *layout = data.layout.get();
    if (!layout) {
        layout = new QTextLayout(block);
        data.layout.reset(layout);
    }
    return layout;
}

void TextEditorLayout::clearBlockLayout(QTextBlock &block) const
{
    if (LayoutData &data = layoutData(block.fragmentIndex()); data.layout)
        data.layout.reset();
    const int blockNumber = block.blockNumber();
    resetOffsetCache(blockNumber);
}

void TextEditorLayout::clearBlockLayout(QTextBlock &start, QTextBlock &end, bool &blockVisibilityChanged) const
{
    for (QTextBlock block = start; block.isValid() && block != end; block = block.next()) {
        if (LayoutData &data = layoutData(block.fragmentIndex()); data.layout) {
            if (block.isVisible()) {
                if (blockLineCount(block) == 0) {
                    blockVisibilityChanged = true;
                    setBlockLineCount(block, 1);
                }
            } else if (blockLineCount(block) > 0) {
                blockVisibilityChanged = true;
                setBlockLineCount(block, 0);
            }
            data.layout.reset();
        }
    }
    resetOffsetCache(start.blockNumber());
}

void TextEditorLayout::relayout()
{
    for (LayoutData &data : m_layoutData)
        data.clearLayout();
    m_offsetCache.clear();
}

int TextEditorLayout::additionalBlockHeight(const QTextBlock &block) const
{
    auto documentLayout = qobject_cast<PlainTextDocumentLayout *>(document()->documentLayout());
    QTC_ASSERT(documentLayout, return 0);
    return documentLayout->additionalBlockHeight(block);
}

QRectF TextEditorLayout::replacementBlockBoundingRect(const QTextBlock &block) const
{
    auto documentLayout = qobject_cast<PlainTextDocumentLayout *>(document()->documentLayout());
    QTC_ASSERT(documentLayout, return {});
    return documentLayout->replacementBlockBoundingRect(block);
}

int TextEditorLayout::lineSpacing() const
{
    auto documentLayout = qobject_cast<PlainTextDocumentLayout *>(document()->documentLayout());
    QTC_ASSERT(documentLayout, return PlainTextDocumentLayout::lineSpacing());
    return documentLayout->lineSpacing();
}

int TextEditorLayout::relativeLineSpacing() const
{
    auto documentLayout = qobject_cast<PlainTextDocumentLayout *>(document()->documentLayout());
    QTC_ASSERT(documentLayout, return PlainTextDocumentLayout::relativeLineSpacing());
    return documentLayout->relativeLineSpacing();
}

void TextEditorLayout::documentChanged(int from, int charsRemoved, int charsAdded)
{
    if (updatingFormats)
        return;

    QTextDocument *doc = document();
    const int blockCount = doc->blockCount();
    m_offsetCache.reserve(blockCount);

    PlainTextDocumentLayout::documentChanged(from, charsRemoved, charsAdded);

    const int charsChanged = charsRemoved + charsAdded;

    QTextBlock block = doc->findBlock(from);
    QTextBlock end = doc->findBlock(qMax(0, from + charsChanged - 1)).next();

    updatingFormats = true;
    while (block.isValid() && block != end) {
        const QList<QTextLayout::FormatRange> formats = block.layout()->formats();
        if (!formats.isEmpty())
            blockLayout(block)->setFormats(formats);
        else if (LayoutData &data = layoutData(block.fragmentIndex()); data.layout)
            data.layout->setFormats(formats);
        block = block.next();
    }
    updatingFormats = false;
}

int TextEditorLayout::blockLineCount(const QTextBlock &block) const
{
    return layoutData(block.fragmentIndex()).lineCount;
}

void TextEditorLayout::setBlockLineCount(QTextBlock &block, int lineCount) const
{
    LayoutData &data = layoutData(block.fragmentIndex());
    if (data.lineCount != lineCount) {
        data.lineCount = lineCount;
        const int blockNumber = block.blockNumber();
        m_offsetCache.resize(blockNumber);
    }
}

void TextEditorLayout::setBlockLayedOut(const QTextBlock &block) const
{
    layoutData(block.fragmentIndex()).layedOut = true;
}

int TextEditorLayout::lineCount() const
{
    const QTextBlock block = document()->lastBlock();
    return firstLineNumberOf(block) + (block.isVisible() ? blockLineCount(block) : 0);
}

int TextEditorLayout::firstLineNumberOf(const QTextBlock &block) const
{
    // FIXME: The first line cache is not reset/recalculated on width change
    const int blockNumber = block.blockNumber();
    const int oldCacheSize = int(m_offsetCache.size());
    resizeOffsetCache(blockNumber);
    int line = m_offsetCache[blockNumber].firstLine;
    if (line >= 0)
        return line;

    // try to find the last entry in the cache with a valid line number
    int lastValidCacheIndex = std::min(blockNumber, oldCacheSize - 1);
    for (; lastValidCacheIndex >= 0; --lastValidCacheIndex) {
        if (m_offsetCache[lastValidCacheIndex].firstLine >= 0) {
            line = m_offsetCache[lastValidCacheIndex].firstLine;
            break;
        }
    }

    QTextBlock currentBlock;
    int currentBlockNumber = 0;
    if (lastValidCacheIndex < 0) { // no valid cache entry found, start from the first block
        currentBlock = document()->firstBlock();
        line = 0;
    } else { // found a valid cache entry add the
        currentBlock = document()->findBlockByNumber(lastValidCacheIndex);
        line += layoutData(currentBlock.fragmentIndex()).lineCount;
        currentBlockNumber = lastValidCacheIndex + 1;
        currentBlock = currentBlock.next();
    }

    // fill the cache until we reached the requested block number
    while (currentBlock.isValid()) {
        m_offsetCache[currentBlockNumber].firstLine = line;
        if (currentBlockNumber == blockNumber)
            break;
        line += layoutData(currentBlock.fragmentIndex()).lineCount;
        currentBlock = currentBlock.next();
        ++currentBlockNumber;
    }

    return line;
}

bool TextEditorLayout::blockLayoutValid(int index) const
{
    return layoutData(index).layedOut;
}

int TextEditorLayout::blockHeight(const QTextBlock &block, int lineSpacing) const
{
    if (QRectF replacement = replacementBlockBoundingRect(block); !replacement.isNull())
        return replacement.height();
    return blockLayoutValid(block.fragmentIndex()) ? blockBoundingRect(block).height()
                                                   : lineSpacing + additionalBlockHeight(block);
}

int TextEditorLayout::offsetForBlock(const QTextBlock &b) const
{
    QTC_ASSERT(b.isValid(), return 0;);
    const int blockNumber = b.blockNumber();
    int currentBlockNumber = std::max(0, std::min(int(m_offsetCache.size()) - 1, blockNumber));
    for (;currentBlockNumber > 0; --currentBlockNumber) {
        if (m_offsetCache[currentBlockNumber].offset >= 0)
            break;
    }
    resizeOffsetCache(blockNumber);
    int offset = m_offsetCache[blockNumber].offset;
    if (offset >= 0)
        return offset;

    QTextDocument *doc = document();
    QTextBlock block = doc->findBlockByNumber(currentBlockNumber);
    const int lineSpacing = this->lineSpacing();
    offset = currentBlockNumber == 0 ? 0 : m_offsetCache[currentBlockNumber].offset;
    while (block.isValid() && block != b) {
        resizeOffsetCache(currentBlockNumber); // blockHeigt potentially resizes the cache
        m_offsetCache[currentBlockNumber].offset = offset;
        if (block.isVisible())
            offset += blockHeight(block, lineSpacing);

        block = block.next();
        ++currentBlockNumber;
    }
    resizeOffsetCache(blockNumber);
    m_offsetCache[blockNumber].offset = offset;
    return offset;
}

int TextEditorLayout::offsetForLine(int line) const
{
    QTextBlock block = findBlockByLineNumber(line);
    int value = offsetForBlock(block);
    const int blockStartLine = firstLineNumberOf(block);
    if (line > blockStartLine) {
        QTextLayout *layout = blockLayout(block);
        for (int i = 0; (i < line - blockStartLine) && (i < layout->lineCount()); ++i)
            value += layout->lineAt(i).naturalTextRect().height();
    }
    return value;
}

int TextEditorLayout::lineForOffset(int offset) const
{
    QTextDocument *doc = document();
    QTextBlock block = doc->firstBlock();

    while (block.isValid()) {
        const int blockOffset = offsetForBlock(block);
        if (blockOffset == offset)
            return firstLineNumberOf(block);
        if (blockOffset > offset)
            break;
        block = block.next();
    }

    if (block.isValid()) {
        block = block.previous();
        if (!block.isValid())
            return 0;
    } else {
        block = doc->lastBlock();
    }

    if (blockLayoutValid(block.fragmentIndex())) {
        int lineOffset = offsetForBlock(block);
        QTextLayout *layout = blockLayout(block);
        for (int line = 0, end = layout->lineCount(); line < end; ++line) {
            lineOffset += layout->lineAt(line).naturalTextRect().height();
            if (lineOffset <= offset)
                return firstLineNumberOf(block) + line;
        }
    }
    return firstLineNumberOf(block);
}

int TextEditorLayout::documentPixelHeight() const
{
    const QTextBlock block = document()->lastBlock();
    return offsetForBlock(block) + blockHeight(block, lineSpacing());
}

QTextBlock TextEditorLayout::findBlockByLineNumber(int lineNumber) const
{
    int blockNumber = 0;
    if (!m_offsetCache.empty()) {
        const int cacheSize(static_cast<int>(m_offsetCache.size()));
        int i = cacheSize < lineNumber ? cacheSize - 1 : lineNumber;
        for (; i > 0; --i) {
            if (m_offsetCache[i].firstLine >= 0 && m_offsetCache[i].firstLine <= lineNumber) {
                blockNumber = i;
                break;
            }
        }
    }

    QTextBlock b = document()->findBlockByNumber(blockNumber);
    while (b.isValid() && firstLineNumberOf(b) < lineNumber)
        b = b.next();
    if (b.isValid())
        b = b.previous();
    if (!b.isValid())
        b = document()->firstBlock();
    return b;
}

bool TextEditorLayout::moveCursorImpl(QTextCursor &cursor, QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode) const
{
    QTextBlock block = cursor.block();
    QTC_ASSERT(block.isValid(), return false);
    ensureBlockLayout(block);
    QTextLayout *layout = blockLayout(block);
    QTC_ASSERT(layout, return false);
    QTextLine line = layout->lineForTextPosition(cursor.positionInBlock());
    QTC_ASSERT(line.isValid(), return false);

    switch (operation) {
    case QTextCursor::MoveOperation::StartOfLine: {
        cursor.setPosition(block.position() + line.textStart(), mode);
        return true;
    }
    case QTextCursor::MoveOperation::EndOfLine: {
        const int blockPos = block.position();
        int newPosition = blockPos + line.textStart() + line.textLength();
        if (newPosition > blockPos && line.lineNumber() < layout->lineCount() - 1) {
            if (block.text().at(newPosition - blockPos - 1).isSpace())
                --newPosition;
        }
        cursor.setPosition(newPosition, mode);
        return true;
    }
    case QTextCursor::MoveOperation::Up:
    case QTextCursor::MoveOperation::Down: {
        const bool down = operation == QTextCursor::MoveOperation::Down;
        int lineNumber = down ? line.lineNumber() + 1 : line.lineNumber() - 1;

        if (lineNumber >= layout->lineCount() || lineNumber == -1) {
            block = down ? block.next() : block.previous();

            if (!block.isValid()) {
                if (mode == QTextCursor::KeepAnchor)
                    cursor.movePosition(down ? QTextCursor::End : QTextCursor::Start, mode);
                else if (cursor.hasSelection())
                    cursor.clearSelection();

                return true;
            }
            ensureBlockLayout(block);
            layout = blockLayout(block);
            QTC_ASSERT(layout, return false);
            lineNumber = down ? 0 : layout->lineCount() - 1;
        }

        int x = cursor.verticalMovementX();
        if (x < 0)
            x = line.cursorToX(cursor.positionInBlock());
        line = layout->lineAt(lineNumber);
        QTC_ASSERT(line.isValid(), return false);
        cursor.setPosition(line.xToCursor(x) + block.position(), mode);
        cursor.setVerticalMovementX(x);
        return true;
    }
    default:
        break;
    }

    return false;
}

bool TextEditorLayout::moveCursor(QTextCursor &cursor, QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode, int steps) const
{
    if (operation == QTextCursor::MoveOperation::StartOfLine
        || operation == QTextCursor::MoveOperation::EndOfLine) {
        return moveCursorImpl(cursor, operation, mode);
    }
    if (operation == QTextCursor::MoveOperation::Up
        || operation == QTextCursor::MoveOperation::Down) {
        for (; steps > 0; --steps) {
            if (!moveCursorImpl(cursor, operation, mode))
                return false;
        }
        return true;
    }
    return PlainTextDocumentLayout::moveCursor(cursor, operation, mode, steps);
}

void TextEditorLayout::resetOffsetCache(int blockNumber) const
{
    if (m_offsetCache.size() > std::vector<OffsetData>::size_type(blockNumber))
        m_offsetCache.resize(blockNumber);
}

void TextEditorLayout::resizeOffsetCache(int blockNumber) const
{
    if (m_offsetCache.size() <= std::vector<OffsetData>::size_type(blockNumber))
        m_offsetCache.resize(blockNumber + 1);
}

TextEditorLayout::LayoutData &TextEditorLayout::layoutData(int index) const
{
    if (m_layoutData.size() <= std::vector<LayoutData>::size_type(index))
        m_layoutData.resize(index + 1);
    return m_layoutData[index];
}

} // namespace Utils
