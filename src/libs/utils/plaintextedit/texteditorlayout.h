// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plaintextedit.h"
#include "../id.h"

#pragma once

namespace Utils {

const int FULL_LINE_HIGHLIGHT_FORMAT_PROPERTY_ID = QTextFormat::UserProperty + 43;

using FormatRanges = QList<QTextLayout::FormatRange>;

class QTCREATOR_UTILS_EXPORT LayoutItem
{
public:
    LayoutItem() = default;
    explicit LayoutItem(const Id &category);
    virtual ~LayoutItem() = default;

    virtual qreal height() { return 0.0; }
    virtual qreal width() const { return 0.0; }
    virtual void ensureLayouted(QTextDocument *doc, const QFontMetrics & fm, qreal availableWidth);
    virtual void paintBackground(QPainter *p,const QPointF &pos,const QRectF &clip = {});
    virtual void paintItem(
        QPainter *p,
        const QPointF &pos,
        const FormatRanges &selections = {},
        const QRectF &clip = {});
    virtual void clear() {}

    Id category() const { return m_category; }

private:
    Id m_category;
};

using LayoutItems = std::list<std::unique_ptr<LayoutItem>>;

class QTCREATOR_UTILS_EXPORT TextLayoutItem : public LayoutItem
{
public:
    TextLayoutItem() = default;
    TextLayoutItem(std::unique_ptr<QTextLayout> &&layout, const Id &category);

    qreal height() override;
    qreal width() const override;
    void ensureLayouted(QTextDocument *doc, const QFontMetrics & fm, qreal availableWidth) override;
    void paintBackground(QPainter *p,const QPointF &pos,const QRectF &clip = {}) override;
    void paintItem(
        QPainter *p,
        const QPointF &pos,
        const FormatRanges &selections = {},
        const QRectF &clip = {}) override;
    void clear() override;

    QTextLayout* layout();

private:
    std::unique_ptr<QTextLayout> m_textLayout;
};

class QTCREATOR_UTILS_EXPORT EmptyLayoutItem : public LayoutItem
{
public:
    EmptyLayoutItem() = default;
    EmptyLayoutItem(qreal height, const Id &category = {});

    void setBackground(const QBrush &background) { m_background = background; }

    qreal height() override;
    void paintBackground(QPainter *p,const QPointF &pos,const QRectF &clip = {}) override;
    void paintItem(
        QPainter *p,
        const QPointF &pos,
        const FormatRanges &selections = {},
        const QRectF &clip = {}) override;

private:
    qreal m_height = 0.0;
    QBrush m_background;
};

class QTCREATOR_UTILS_EXPORT TextEditorLayout : public PlainTextDocumentLayout
{
public:
    TextEditorLayout(PlainTextDocumentLayout *docLayout);
    ~TextEditorLayout() override;

    void resetBlockSize(const QTextBlock &block);

    void paintBlock(
        const QTextBlock &block,
        QPainter *p,
        const QPointF &pos,
        const FormatRanges &selections = {},
        const QRectF &clipRect = {});
    void paintBackground(const QTextBlock &block, QPainter *p, const QPointF &pos,const QRectF &clip = {});

    QTextLayout *blockLayout(const QTextBlock &block) const override;
    void clearBlockLayout(QTextBlock &block) const override;
    void clearBlockLayout(
        QTextBlock &start, QTextBlock &end, bool &blockVisibilityChanged) const override;
    void relayout() override;
    int additionalBlockHeight(const QTextBlock &block, bool includeEmbeddedWidgetsHeight) const override;
    QRectF replacementBlockBoundingRect(const QTextBlock &block) const override;
    int lineSpacing() const override;
    int relativeLineSpacing() const override;

    int mainLayoutOffset(const QTextBlock &block) const;

    void prependLayoutItem(const QTextBlock &block, std::unique_ptr<LayoutItem> &&layoutItem);
    void appendLayoutItem(const QTextBlock &block, std::unique_ptr<LayoutItem> &&layoutItem);

    void prependAdditionalLayouts(
        const QTextBlock &block, const QList<QTextLayout *> &layouts, const Id id = {});
    void appendAdditionalLayouts(
        const QTextBlock &block, const QList<QTextLayout *> &layouts, const Id id = {});
    int removeLayoutItems(const QTextBlock &block, const Id id = {});

    const QList<LayoutItem *> layoutItems(const QTextBlock &block) const;
    const QList<LayoutItem *> layoutItemsForCategory(const QTextBlock &block, const Id id) const;

    void documentChanged(int from, int charsRemoved, int charsAdded) override;

    int blockLineCount(const QTextBlock &block) const override;
    void setBlockLineCount(QTextBlock &block, int lineCount) const override;

    int offsetForBlock(const QTextBlock &block) const;
    void setBlockLayedOut(const QTextBlock &block) const override;
    bool blockLayoutValid(int index) const;

    int lineCount() const override;
    int firstLineNumberOf(const QTextBlock &block) const override;
    int offsetForLine(int line) const;
    int lineForOffset(int offset) const;

    int documentPixelHeight() const;

    QTextBlock findBlockByLineNumber(int lineNumber) const override;

    bool moveCursorImpl(
        QTextCursor &cursor,
        QTextCursor::MoveOperation operation,
        QTextCursor::MoveMode mode = QTextCursor::MoveAnchor) const;

    bool moveCursor(
        QTextCursor &cursor,
        QTextCursor::MoveOperation operation,
        QTextCursor::MoveMode mode = QTextCursor::MoveAnchor,
        int steps = 1) const override;

    qreal blockWidth(const QTextBlock &block) override;

private:
    class TextEditorLayoutPrivate;
    TextEditorLayoutPrivate *d;
};

} // namespace Utils
