// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditorlayout.h"

#include "../algorithm.h"
#include "../qtcassert.h"

#include <QPainter>

namespace Utils {

static const int DOCUMENT_LAYOUT_FORMAT_PROPERTY_ID = QTextCharFormat::UserFormat + 0x100;
const char MAIN_LAYOUT_CATEGORY[] = "TextEditorLayout.MainLayout";

struct LayoutData
{
    LayoutData() = default;

    LayoutData(const LayoutData &) = delete;
    LayoutData &operator=(const LayoutData &) = delete;

    LayoutData(LayoutData &&other) = default;
    LayoutData &operator=(LayoutData &&other) = default;

    std::unique_ptr<LayoutItems> layoutItems = std::make_unique<LayoutItems>();
    LayoutItems::iterator mainLayoutIterator = layoutItems->end();
    int lineCount = 1;
    bool layedOut = false;

    QTextLayout *mainLayout() const
    {
        if (mainLayoutIterator == layoutItems->end())
            return nullptr;
        if (auto mainLayoutItem = dynamic_cast<TextLayoutItem *>(mainLayoutIterator->get()))
            return mainLayoutItem->layout();
        return nullptr;
    }

    void clearLayout()
    {
        for (auto &item : *layoutItems)
            item->clear();
        lineCount = 1;
        layedOut = false;
    }
};

struct OffsetData
{
    int offset = -1;
    int firstLine = -1;
};

class TextEditorLayout::TextEditorLayoutPrivate
{
public:
    LayoutData &layoutData(int index)
    {
        if (m_layoutData.size() <= std::vector<LayoutData>::size_type(index))
            m_layoutData.resize(std::vector<LayoutData>::size_type(index) + 1);
        return m_layoutData[index];
    }

    void resetOffsetCache(int blockNumber)
    {
        if (m_offsetCache.size() > std::vector<OffsetData>::size_type(blockNumber))
            m_offsetCache.resize(blockNumber);
    }

    void resizeOffsetCache(int blockNumber)
    {
        if (m_offsetCache.size() <= std::vector<OffsetData>::size_type(blockNumber))
            m_offsetCache.resize(blockNumber + 1);
    }

    std::vector<LayoutData> m_layoutData;
    std::vector<OffsetData> m_offsetCache;
    bool updatingFormats = false;
};

TextEditorLayout::TextEditorLayout(PlainTextDocumentLayout *docLayout)
    : PlainTextDocumentLayout(docLayout->document())
    , d(new TextEditorLayoutPrivate)
{
    connect(
        docLayout,
        &PlainTextDocumentLayout::documentContentsChanged,
        this,
        &TextEditorLayout::documentChanged);
    connect(
        docLayout,
        &PlainTextDocumentLayout::blockSizeChanged,
        this,
        &TextEditorLayout::resetBlockSize);
    connect(
        this,
        &PlainTextDocumentLayout::blockSizeChanged,
        this,
        &TextEditorLayout::resetBlockSize);
    connect(
        docLayout,
        &PlainTextDocumentLayout::documentSizeChanged,
        this,
        &TextEditorLayout::documentSizeChanged);
    connect(docLayout, &PlainTextDocumentLayout::update, this, &TextEditorLayout::update);
}

TextEditorLayout::~TextEditorLayout()
{
    delete d;
}

void TextEditorLayout::resetBlockSize(const QTextBlock &block)
{
    const int blockNumber = block.blockNumber();

    int lineCount = 0;
    LayoutData &data = d->layoutData(block.fragmentIndex());
    if (block.isVisible()) {
        if (auto mainLayout = data.mainLayout())
            lineCount = mainLayout->lineCount();
        lineCount = std::max(lineCount, 1);
    }
    data.lineCount = lineCount;
    d->resetOffsetCache(blockNumber);
}

void TextEditorLayout::paintBackground(
    const QTextBlock &block, QPainter *p, const QPointF &offset, const QRectF &clip)
{
    QPointF pos = offset;
    for (auto &layoutItem : *d->layoutData(block.fragmentIndex()).layoutItems) {
        layoutItem->paintBackground(p, pos, clip);
        pos.ry() += layoutItem->height();
    }
}

void TextEditorLayout::paintBlock(
    const QTextBlock &block,
    QPainter *painter,
    const QPointF &offset,
    const QList<QTextLayout::FormatRange> &selections,
    const QRectF &clipRect)
{
    QPointF pos = offset;
    for (auto &layoutItem : *d->layoutData(block.fragmentIndex()).layoutItems) {
        layoutItem->paintItem(painter, pos, selections, clipRect);
        pos.ry() += layoutItem->height();
    }
}

QTextLayout *TextEditorLayout::blockLayout(const QTextBlock &block) const
{
    LayoutData &data = d->layoutData(block.fragmentIndex());
    QTextLayout *layout = data.mainLayout();
    if (!layout) {
        layout = new QTextLayout(block);
        data.mainLayoutIterator = data.layoutItems->emplace(
            data.layoutItems->begin(),
            std::make_unique<TextLayoutItem>(
                std::unique_ptr<QTextLayout>(layout), MAIN_LAYOUT_CATEGORY));
    }
    return layout;
}

void TextEditorLayout::clearBlockLayout(QTextBlock &block) const
{
    d->layoutData(block.fragmentIndex()).clearLayout();
    const int blockNumber = block.blockNumber();
    d->resetOffsetCache(blockNumber);
}

void TextEditorLayout::clearBlockLayout(QTextBlock &start, QTextBlock &end, bool &blockVisibilityChanged) const
{
    for (QTextBlock block = start; block.isValid(); block = block.next()) {
        LayoutData &data = d->layoutData(block.fragmentIndex());
        if (block.isVisible()) {
            if (blockLineCount(block) == 0) {
                blockVisibilityChanged = true;
                setBlockLineCount(block, 1);
            }
        } else if (blockLineCount(block) > 0) {
            blockVisibilityChanged = true;
            setBlockLineCount(block, 0);
        }
        data.clearLayout();
        if (block == end)
            break;
    }
    d->resetOffsetCache(start.blockNumber());
}

void TextEditorLayout::relayout()
{
    for (LayoutData &data : d->m_layoutData)
        data.clearLayout();
    d->m_offsetCache.clear();
}

int TextEditorLayout::additionalBlockHeight(
    const QTextBlock &block, bool includeEmbeddedWidgetsHeight) const
{
    int additionalHeight = 0;
    const QFontMetrics fm(block.charFormat().font());
    LayoutItems *layoutItems = d->layoutData(block.fragmentIndex()).layoutItems.get();
    const LayoutItems::iterator end = layoutItems->end();
    for (auto it = layoutItems->begin(); it != end; ++it) {
        if (it == d->layoutData(block.fragmentIndex()).mainLayoutIterator)
            continue; // only collect additional height from additional layoutItems
        (*it)->ensureLayouted(document(), fm, textWidth());
        additionalHeight += (*it)->height();
    }
    auto documentLayout = qobject_cast<PlainTextDocumentLayout *>(document()->documentLayout());
    if (QTC_GUARD(documentLayout))
        additionalHeight
            += documentLayout->additionalBlockHeight(block, includeEmbeddedWidgetsHeight);
    return additionalHeight;
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

int TextEditorLayout::mainLayoutOffset(const QTextBlock &block) const
{
    int offset = 0;
    const LayoutItems *layouts = d->layoutData(block.fragmentIndex()).layoutItems.get();
    auto it = layouts->begin();
    auto end = d->layoutData(block.fragmentIndex()).mainLayoutIterator;
    while (it != end) {
        offset += (*it)->height();
        ++it;
    }
    return offset;
}

void TextEditorLayout::prependLayoutItem(
    const QTextBlock &block, std::unique_ptr<LayoutItem> &&layoutItem)
{
    LayoutData &data = d->layoutData(block.fragmentIndex());
    data.layoutItems->emplace(data.layoutItems->begin(), std::move(layoutItem));
    d->resetOffsetCache(block.blockNumber());
}

void TextEditorLayout::appendLayoutItem(
    const QTextBlock &block, std::unique_ptr<LayoutItem> &&layoutItem)
{
    LayoutData &data = d->layoutData(block.fragmentIndex());
    data.layoutItems->emplace_back(std::move(layoutItem));
    d->resetOffsetCache(block.blockNumber());
}

void TextEditorLayout::prependAdditionalLayouts(
    const QTextBlock &block, const QList<QTextLayout *> &layouts, const Id id)
{
    if (layouts.isEmpty())
        return;
    LayoutData &data = d->layoutData(block.fragmentIndex());
    for (auto layout : layouts) {
        QTC_ASSERT(layout, continue);
        data.layoutItems->emplace(
            data.layoutItems->begin(),
            std::make_unique<TextLayoutItem>(std::unique_ptr<QTextLayout>(layout), id));
    }
    d->resetOffsetCache(block.blockNumber());
}

void TextEditorLayout::appendAdditionalLayouts(
    const QTextBlock &block, const QList<QTextLayout *> &layouts, const Id id)
{
    if (layouts.isEmpty())
        return;
    LayoutData &data = d->layoutData(block.fragmentIndex());
    for (auto layout : layouts) {
        QTC_ASSERT(layout, continue);
        data.layoutItems->emplace_back(
            std::make_unique<TextLayoutItem>(std::unique_ptr<QTextLayout>(layout), id));
    }
    d->resetOffsetCache(block.blockNumber());
}

int TextEditorLayout::removeLayoutItems(const QTextBlock &block, const Id id)
{
    LayoutItems *layoutItems = d->layoutData(block.fragmentIndex()).layoutItems.get();
    const size_t removeCount = layoutItems->remove_if(
        [&id](const std::unique_ptr<LayoutItem> &item) { return item->category() == id; });
    return int(removeCount);
}

const QList<LayoutItem *> TextEditorLayout::layoutItems(const QTextBlock &block) const
{
    return Utils::transform<QList>(
        *d->layoutData(block.fragmentIndex()).layoutItems,
        [](const std::unique_ptr<LayoutItem> &item) -> LayoutItem * { return item.get(); });
}

const QList<LayoutItem *> TextEditorLayout::layoutItemsForCategory(
    const QTextBlock &block, const Id id) const
{
    return Utils::filtered(layoutItems(block), Utils::equal(&LayoutItem::category, id));
}

void TextEditorLayout::documentChanged(int from, int charsRemoved, int charsAdded)
{
    if (d->updatingFormats)
        return;

    QTextDocument *doc = document();
    const int blockCount = doc->blockCount();
    d->m_offsetCache.reserve(blockCount);

    PlainTextDocumentLayout::documentChanged(from, charsRemoved, charsAdded);

    const int to = from + qMax(0, charsAdded);

    QTextBlock block = doc->findBlock(from);
    QTextBlock end = doc->findBlock(to);

    d->updatingFormats = true;
    while (block.isValid()) {
        QList<QTextLayout::FormatRange> documentFormats = block.layout()->formats();
        for (QTextLayout::FormatRange &range : documentFormats)
            range.format.setProperty(DOCUMENT_LAYOUT_FORMAT_PROPERTY_ID, true);
        if (QTextLayout *mainLayout = d->layoutData(block.fragmentIndex()).mainLayout()) {
            QList<QTextLayout::FormatRange> editorFormats = Utils::filtered(
                mainLayout->formats(), [](const QTextLayout::FormatRange &range) {
                    return !range.format.property(DOCUMENT_LAYOUT_FORMAT_PROPERTY_ID).toBool();
                });

            mainLayout->setFormats(editorFormats + documentFormats);
        } else if (!documentFormats.isEmpty()) {
            blockLayout(block)->setFormats(documentFormats);
        }
        if (block == end)
            break;
        block = block.next();
    }
    d->updatingFormats = false;
}

int TextEditorLayout::blockLineCount(const QTextBlock &block) const
{
    return d->layoutData(block.fragmentIndex()).lineCount;
}

void TextEditorLayout::setBlockLineCount(QTextBlock &block, int lineCount) const
{
    LayoutData &data = d->layoutData(block.fragmentIndex());
    if (data.lineCount != lineCount) {
        data.lineCount = lineCount;
        const int blockNumber = block.blockNumber();
        d->m_offsetCache.resize(blockNumber);
    }
}

void TextEditorLayout::setBlockLayedOut(const QTextBlock &block) const
{
    d->layoutData(block.fragmentIndex()).layedOut = true;
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
    const int oldCacheSize = int(d->m_offsetCache.size());
    d->resizeOffsetCache(blockNumber);
    int line = d->m_offsetCache[blockNumber].firstLine;
    if (line >= 0)
        return line;

    // try to find the last entry in the cache with a valid line number
    int lastValidCacheIndex = std::min(blockNumber, oldCacheSize - 1);
    for (; lastValidCacheIndex >= 0; --lastValidCacheIndex) {
        if (d->m_offsetCache[lastValidCacheIndex].firstLine >= 0) {
            line = d->m_offsetCache[lastValidCacheIndex].firstLine;
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
        line += d->layoutData(currentBlock.fragmentIndex()).lineCount;
        currentBlockNumber = lastValidCacheIndex + 1;
        currentBlock = currentBlock.next();
    }

    // fill the cache until we reached the requested block number
    while (currentBlock.isValid()) {
        d->m_offsetCache[currentBlockNumber].firstLine = line;
        if (currentBlockNumber == blockNumber)
            break;
        line += d->layoutData(currentBlock.fragmentIndex()).lineCount;
        currentBlock = currentBlock.next();
        ++currentBlockNumber;
    }

    return line;
}

bool TextEditorLayout::blockLayoutValid(int index) const
{
    return d->layoutData(index).layedOut;
}

int TextEditorLayout::offsetForBlock(const QTextBlock &block) const
{
    QTC_ASSERT(block.isValid(), return 0;);
    const int blockNumber = block.blockNumber();
    int currentBlockNumber = std::max(0, std::min(int(d->m_offsetCache.size()) - 1, blockNumber));
    for (;currentBlockNumber > 0; --currentBlockNumber) {
        if (d->m_offsetCache[currentBlockNumber].offset >= 0)
            break;
    }
    d->resizeOffsetCache(blockNumber);
    int offset = d->m_offsetCache[blockNumber].offset;
    if (offset >= 0)
        return offset;

    QTextDocument *doc = document();
    QTextBlock currrentBlock = doc->findBlockByNumber(currentBlockNumber);
    offset = currentBlockNumber == 0 ? 0 : d->m_offsetCache[currentBlockNumber].offset;
    while (block.isValid() && block != currrentBlock) {
        d->resizeOffsetCache(currentBlockNumber); // blockHeigt potentially resizes the cache
        d->m_offsetCache[currentBlockNumber].offset = offset;
        if (currrentBlock.isVisible())
            offset += blockHeight(currrentBlock);

        currrentBlock = currrentBlock.next();
        ++currentBlockNumber;
    }
    d->resizeOffsetCache(blockNumber);
    d->m_offsetCache[blockNumber].offset = offset;
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
    int currentBlockNumber = block.blockNumber();

    while (block.isValid()) {
        int blockOffset = -1;
        if (currentBlockNumber < int(d->m_offsetCache.size()))
            blockOffset = d->m_offsetCache[currentBlockNumber].offset;
        ++currentBlockNumber;
        if (blockOffset < 0)
            blockOffset = offsetForBlock(block);
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
    return offsetForBlock(block) + blockHeight(block);
}

QTextBlock TextEditorLayout::findBlockByLineNumber(int lineNumber) const
{
    int blockNumber = 0;
    if (!d->m_offsetCache.empty()) {
        const int cacheSize = int(d->m_offsetCache.size());
        int i = cacheSize <= lineNumber ? cacheSize - 1 : lineNumber;
        for (; i > 0; --i) {
            if (d->m_offsetCache[i].firstLine >= 0 && d->m_offsetCache[i].firstLine <= lineNumber) {
                blockNumber = i;
                break;
            }
        }
    }

    QTextBlock b = document()->findBlockByNumber(blockNumber);
    while (b.isValid()) {
        if (firstLineNumberOf(b) + blockLineCount(b) - 1 >= lineNumber)
            return b;
        b = b.next();
    }
    return document()->lastBlock();
}

bool TextEditorLayout::moveCursorImpl(
    QTextCursor &cursor, QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode) const
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
            do {
                block = down ? block.next() : block.previous();
            } while (block.isValid() && !block.isVisible());

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

bool TextEditorLayout::moveCursor(
    QTextCursor &cursor,
    QTextCursor::MoveOperation operation,
    QTextCursor::MoveMode mode,
    int steps) const
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

qreal TextEditorLayout::blockWidth(const QTextBlock &block)
{
    qreal width = 0.0;
    for (auto &layoutItem : *d->layoutData(block.fragmentIndex()).layoutItems)
        width = qMax(width, layoutItem->width());
    return width;
}

LayoutItem::LayoutItem(const Id &category)
    : m_category(category)
{}

void LayoutItem::ensureLayouted(QTextDocument *, const QFontMetrics &, qreal)
{
}

void LayoutItem::paintBackground(QPainter *, const QPointF &, const QRectF &)
{
}

void LayoutItem::paintItem(QPainter *, const QPointF &, const FormatRanges &, const QRectF &)
{

}

TextLayoutItem::TextLayoutItem(std::unique_ptr<QTextLayout> &&layout, const Id &category)
    : LayoutItem(category)
    , m_textLayout(std::move(layout))
{}

qreal TextLayoutItem::height()
{
    QTC_ASSERT(m_textLayout, return 0);
    return m_textLayout->boundingRect().height();
}

qreal TextLayoutItem::width() const
{
    QTC_ASSERT(m_textLayout, return 0);
    return PlainTextDocumentLayout::layoutWidth(m_textLayout.get());
}

void TextLayoutItem::ensureLayouted(QTextDocument *doc, const QFontMetrics & fm, qreal availableWidth)
{
    QTC_ASSERT(m_textLayout, return);

    if (m_textLayout->lineCount() > 0)
        return;

    qreal margin = doc->documentMargin();
    qreal height = 0;
    QTextOption option = doc->defaultTextOption();
    m_textLayout->setTextOption(option);

    int extraMargin = 0;
    if (option.flags() & QTextOption::AddSpaceForLineAndParagraphSeparators)
        extraMargin += fm.horizontalAdvance(QChar(0x21B5));

    m_textLayout->beginLayout();
    if (availableWidth <= 0)
        availableWidth = qreal(INT_MAX); // similar to text edit with pageSize.width == 0

    availableWidth -= 2 * margin + extraMargin;
    while (1) {
        QTextLine line = m_textLayout->createLine();
        if (!line.isValid())
            break;
        line.setLeadingIncluded(true);
        line.setLineWidth(availableWidth);
        line.setPosition(QPointF(margin, height));
        height += line.height();
        if (line.leading() < 0)
            height += qCeil(line.leading());
    }
    m_textLayout->endLayout();

}

void TextLayoutItem::paintBackground(QPainter *p, const QPointF &pos, const QRectF &clip)
{
    QTC_ASSERT(m_textLayout, return);
    for (auto format : m_textLayout->formats()) {
        if (format.format.boolProperty(FULL_LINE_HIGHLIGHT_FORMAT_PROPERTY_ID)) {
            QRectF blockRect = m_textLayout->boundingRect();
            blockRect = blockRect.translated(pos);
            blockRect.setRight(clip.right());
            p->fillRect(blockRect, format.format.background());
        }
    }
}

void TextLayoutItem::paintItem(
    QPainter *p, const QPointF &pos, const FormatRanges &selections, const QRectF &clip)
{
    QTC_ASSERT(m_textLayout, return);
    m_textLayout->draw(p, pos, selections, clip);
}

void TextLayoutItem::clear()
{
    QTC_ASSERT(m_textLayout, return);
    m_textLayout->clearLayout();
}

QTextLayout *TextLayoutItem::layout()
{
    return m_textLayout.get();
}

EmptyLayoutItem::EmptyLayoutItem(qreal height, const Id &category)
    : LayoutItem(category)
    , m_height(height)
{}

qreal EmptyLayoutItem::height()
{
    return m_height;
}

void EmptyLayoutItem::paintBackground(QPainter *p, const QPointF &pos, const QRectF &clip)
{
    if (m_background.style() == Qt::NoBrush || m_height <= 0)
        return;
    QRectF rect;
    rect.setTopLeft(pos);
    rect.setRight(clip.right());
    rect.setHeight(m_height);
    p->fillRect(rect, m_background);
}

void EmptyLayoutItem::paintItem(QPainter *p, const QPointF &pos, const FormatRanges &selections, const QRectF &clip)
{
    Q_UNUSED(p);
    Q_UNUSED(pos);
    Q_UNUSED(selections);
    Q_UNUSED(clip);
}

} // namespace Utils
