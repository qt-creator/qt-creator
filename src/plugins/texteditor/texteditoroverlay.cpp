// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditoroverlay.h"
#include "texteditor.h"

#include <QDebug>
#include <QMap>
#include <QPainter>
#include <QPainterPath>
#include <QTextBlock>

#include <algorithm>
#include <utils/qtcassert.h>

using namespace TextEditor;
using namespace TextEditor::Internal;

constexpr int borderWidth = 1;

TextEditorOverlay::TextEditorOverlay(TextEditorWidget *editor) :
    QObject(editor),
    m_visible(false),
    m_alpha(true),
    m_dropShadowWidth(2),
    m_firstSelectionOriginalBegin(-1),
    m_editor(editor),
    m_viewport(editor->viewport())
{
}

void TextEditorOverlay::update()
{
    if (m_visible)
        m_viewport->update();
}


void TextEditorOverlay::setVisible(bool b)
{
    if (m_visible == b)
        return;
    m_visible = b;
    if (!m_selections.isEmpty())
        m_viewport->update();
}

void TextEditorOverlay::clear()
{
    if (m_selections.isEmpty())
        return;
    m_selections.clear();
    m_firstSelectionOriginalBegin = -1;
    update();
}

void TextEditorOverlay::addOverlaySelection(int begin, int end,
                                            const QColor &fg, const QColor &bg,
                                            uint overlaySelectionFlags)
{
    if (end < begin)
        return;

    QTextDocument *document = m_editor->document();

    OverlaySelection selection;
    selection.m_fg = fg;
    selection.m_bg = bg;

    selection.m_cursor_begin = QTextCursor(document);
    selection.m_cursor_begin.setPosition(begin);
    selection.m_cursor_end = QTextCursor(document);
    selection.m_cursor_end.setPosition(end);

    if (overlaySelectionFlags & ExpandBegin)
        selection.m_cursor_begin.setKeepPositionOnInsert(true);

    if (overlaySelectionFlags & LockSize)
        selection.m_fixedLength = (end - begin);

    selection.m_dropShadow = (overlaySelectionFlags & DropShadow);

    if (m_selections.isEmpty())
        m_firstSelectionOriginalBegin = begin;
    else if (begin < m_firstSelectionOriginalBegin)
        qWarning() << "overlay selections not in order";

    m_selections.append(selection);
    update();
}


void TextEditorOverlay::addOverlaySelection(const QTextCursor &cursor,
                                            const QColor &fg, const QColor &bg,
                                            uint overlaySelectionFlags)
{
    addOverlaySelection(cursor.selectionStart(), cursor.selectionEnd(), fg, bg, overlaySelectionFlags);
}

QRect TextEditorOverlay::rect() const
{
    return m_viewport->rect();
}

QPainterPath TextEditorOverlay::createSelectionPath(const QTextCursor &begin, const QTextCursor &end,
                                                    const QRect &clip)
{
    if (begin.isNull() || end.isNull() || begin.position() > end.position())
        return QPainterPath();

    QPointF offset = m_editor->contentOffset();
    QRect viewportRect = rect();
    QTextDocument *document = m_editor->document();

    if (m_editor->blockBoundingGeometry(begin.block()).translated(offset).top() > clip.bottom() + 10
        || m_editor->blockBoundingGeometry(end.block()).translated(offset).bottom()
               < clip.top() - 10) {
        return QPainterPath(); // nothing of the selection is visible
    }

    QTextBlock block = begin.block();

    if (block.blockNumber() < m_editor->firstVisibleBlock().blockNumber() - 1)
        block = document->findBlockByNumber(m_editor->firstVisibleBlock().blockNumber() - 1);

    if (begin.position() == end.position()) {
        // special case empty selections
        const QRectF blockGeometry = m_editor->blockBoundingGeometry(block);
        QTextLayout *blockLayout = block.layout();
        int pos = begin.position() - begin.block().position();
        QTextLine line = blockLayout->lineForTextPosition(pos);
        QTC_ASSERT(line.isValid(), return {});
        QRectF lineRect = line.naturalTextRect();
        lineRect = lineRect.translated(blockGeometry.topLeft());
        int x = line.cursorToX(pos);
        lineRect.setLeft(x - borderWidth);
        lineRect.setRight(x + borderWidth);
        lineRect.setBottom(lineRect.bottom() + borderWidth);
        QPainterPath path;
        path.addRect(lineRect);
        path.translate(offset);
        return path;
    }

    QPointF top;    //      *------|
    QPointF left;   //  *---|      |
    QPointF right;  //  |      |---*
    QPointF bottom; //  |------*

    for (; block.isValid() && block.blockNumber() <= end.blockNumber(); block = block.next()) {
        if (!block.isVisible())
            continue;

        const QRectF blockGeometry = m_editor->blockBoundingGeometry(block);
        QTextLayout *blockLayout = block.layout();

        int firstLine = 0;

        int beginChar = 0;
        if (block == begin.block()) {
            beginChar = begin.positionInBlock();
            const QString preeditAreaText = begin.block().layout()->preeditAreaText();
            if (!preeditAreaText.isEmpty() && beginChar >= begin.block().layout()->preeditAreaPosition())
                beginChar += preeditAreaText.length();
            QTextLine line = blockLayout->lineForTextPosition(beginChar);
            QTC_ASSERT(line.isValid(), return {});
            firstLine = line.lineNumber();
            const int lineEnd = line.textStart() + line.textLength();
            if (beginChar == lineEnd)
                continue;
        }

        int lastLine = blockLayout->lineCount() - 1;
        int endChar = -1;
        if (block == end.block()) {
            endChar = end.positionInBlock();
            const QString preeditAreaText = end.block().layout()->preeditAreaText();
            if (!preeditAreaText.isEmpty() && endChar >= end.block().layout()->preeditAreaPosition())
                endChar += preeditAreaText.length();
            QTextLine line = blockLayout->lineForTextPosition(endChar);
            QTC_ASSERT(line.isValid(), return {});
            lastLine = line.lineNumber();
            if (endChar == beginChar)
                break; // Do not expand overlay to empty selection at end
        } else {
            endChar = block.length();
            while (endChar > beginChar
                   && document->characterAt(block.position() + endChar - 1).isSpace())
                --endChar;
        }

        for (int i = firstLine; i <= lastLine; ++i) {
            QTextLine line = blockLayout->lineAt(i);
            QTC_ASSERT(line.isValid(), return {});
            QRectF lineRect = line.naturalTextRect();
            if (i == firstLine && beginChar > 0)
                lineRect.setLeft(line.cursorToX(beginChar));
            if (line.lineNumber() == lastLine)
                lineRect.setRight(line.cursorToX(endChar));
            lineRect = lineRect.translated(blockGeometry.topLeft());
            if (top.isNull())
                top = lineRect.topLeft();
            else if (left.isNull())
                left = lineRect.topLeft();
            else
                left.setX(std::min(left.x(), lineRect.left()));
            if (i == lastLine && block == end.block() && lineRect.right() <= right.x())
                bottom = lineRect.bottomRight();
            else
                right = {std::max(lineRect.right(), right.x()), lineRect.bottom()};
        }

        if (blockGeometry.translated(offset).y() > 2 * viewportRect.height())
            break;
    }

    if (top.isNull())
        return {};

    if (bottom.isNull())
        bottom = right;

    if (left.isNull())
        left = top;

    if (right.isNull())
        right = bottom;

    constexpr QPointF marginOffset = {borderWidth, borderWidth};
    right += marginOffset;
    bottom += marginOffset;

    QPainterPath path;
    path.moveTo(top);
    path.lineTo(right.x(), top.y());
    path.lineTo(right);
    path.lineTo(bottom.x(), right.y());
    path.lineTo(bottom);
    path.lineTo(left.x(), bottom.y());
    path.lineTo(left);
    path.lineTo(top.x(), left.y());
    path.lineTo(top);

    path.closeSubpath();
    path.translate(offset);
    return path;
}

void TextEditorOverlay::paintSelection(QPainter *painter,
                                       const OverlaySelection &selection,
                                       const QRect &clip)
{

    QTextCursor begin = selection.m_cursor_begin;

    const QTextCursor &end= selection.m_cursor_end;
    const QColor &fg = selection.m_fg;
    const QColor &bg = selection.m_bg;


    if (begin.isNull() || end.isNull() || begin.position() > end.position() || !bg.isValid())
        return;

    QPainterPath path = createSelectionPath(begin, end, clip);
    if (path.isEmpty())
        return;

    painter->save();
    QColor penColor = fg;
    if (m_alpha)
        penColor.setAlpha(220);
    QPen pen(penColor, borderWidth);
    painter->translate(-.5, -.5);

    QRectF pathRect = path.controlPointRect();

    if (!m_alpha || begin.blockNumber() != end.blockNumber()) {
        // gradients are too slow for larger selections :(
        QColor col = bg;
        if (m_alpha)
            col.setAlpha(50);
        painter->setBrush(col);
    } else {
        QLinearGradient linearGrad(pathRect.topLeft(), pathRect.bottomLeft());
        QColor col1 = fg.lighter(150);
        col1.setAlpha(20);
        QColor col2 = fg;
        col2.setAlpha(80);
        linearGrad.setColorAt(0, col1);
        linearGrad.setColorAt(1, col2);
        painter->setBrush(QBrush(linearGrad));
    }

    if (selection.m_dropShadow) {
        painter->save();
        QPainterPath shadow = path;
        shadow.translate(m_dropShadowWidth, m_dropShadowWidth);
        QPainterPath clip;
        clip.addRect(m_editor->viewport()->rect());
        painter->setClipPath(clip - path);
        painter->fillPath(shadow, QColor(0, 0, 0, 100));
        painter->restore();
    }

    pen.setJoinStyle(Qt::MiterJoin);
    painter->setPen(pen);
    painter->drawPath(path);
    painter->restore();
}

void TextEditorOverlay::fillSelection(QPainter *painter,
                                      const OverlaySelection &selection,
                                      const QColor &color,
                                      const QRect &clip)
{
    const QTextCursor &begin = selection.m_cursor_begin;
    const QTextCursor &end= selection.m_cursor_end;
    if (begin.isNull() || end.isNull() || begin.position() > end.position())
        return;

    QPainterPath path = createSelectionPath(begin, end, clip);
    if (path.isEmpty())
        return;

    painter->save();
    painter->translate(-.5, -.5);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->fillPath(path, color);
    painter->restore();
}

void TextEditorOverlay::paint(QPainter *painter, const QRect &clip)
{
    Q_UNUSED(clip)

    const auto firstBlock = m_editor->blockForVerticalOffset(clip.top());
    const int firstBlockNumber = firstBlock.isValid() ? firstBlock.blockNumber() : 0;
    const auto lastBlock = m_editor->blockForVerticalOffset(clip.bottom());
    const int lastBlockNumber = lastBlock.isValid() ? lastBlock.blockNumber()
                                                    : m_editor->blockCount() - 1;

    auto overlapsClip = [&](const OverlaySelection &selection) {
        return selection.m_cursor_end.blockNumber() + 1 >= firstBlockNumber
               && selection.m_cursor_begin.blockNumber() - 1 <= lastBlockNumber;
    };

    for (int i = m_selections.size() - 1; i >= 0; --i) {
        const OverlaySelection &selection = m_selections.at(i);
        if (selection.m_dropShadow || !overlapsClip(selection))
            continue;
        if (selection.m_fixedLength >= 0
            && selection.m_cursor_end.position() - selection.m_cursor_begin.position()
            != selection.m_fixedLength)
            continue;

        paintSelection(painter, selection, clip);
    }
    for (int i = m_selections.size() - 1; i >= 0; --i) {
        const OverlaySelection &selection = m_selections.at(i);
        if (!selection.m_dropShadow || !overlapsClip(selection))
            continue;
        if (selection.m_fixedLength >= 0
            && selection.m_cursor_end.position() - selection.m_cursor_begin.position()
            != selection.m_fixedLength)
            continue;

        paintSelection(painter, selection, clip);
    }
}

QTextCursor TextEditorOverlay::cursorForSelection(const OverlaySelection &selection) const
{
    QTextCursor cursor = selection.m_cursor_begin;
    cursor.setPosition(selection.m_cursor_begin.position());
    cursor.setKeepPositionOnInsert(false);
    if (!cursor.isNull())
        cursor.setPosition(selection.m_cursor_end.position(), QTextCursor::KeepAnchor);
    return cursor;
}

QTextCursor TextEditorOverlay::cursorForIndex(int selectionIndex) const
{
    return cursorForSelection(m_selections.value(selectionIndex));
}

void TextEditorOverlay::fill(QPainter *painter, const QColor &color, const QRect &clip)
{
    Q_UNUSED(clip)
    for (int i = m_selections.size()-1; i >= 0; --i) {
        const OverlaySelection &selection = m_selections.at(i);
        if (selection.m_dropShadow)
            continue;
        if (selection.m_fixedLength >= 0
            && selection.m_cursor_end.position() - selection.m_cursor_begin.position()
            != selection.m_fixedLength)
            continue;

        fillSelection(painter, selection, color, clip);
    }
    for (int i = m_selections.size()-1; i >= 0; --i) {
        const OverlaySelection &selection = m_selections.at(i);
        if (!selection.m_dropShadow)
            continue;
        if (selection.m_fixedLength >= 0
            && selection.m_cursor_end.position() - selection.m_cursor_begin.position()
            != selection.m_fixedLength)
            continue;

        fillSelection(painter, selection, color, clip);
    }
}

bool TextEditorOverlay::hasFirstSelectionBeginMoved() const
{
    if (m_firstSelectionOriginalBegin == -1 || m_selections.isEmpty())
        return false;
    return m_selections.at(0).m_cursor_begin.position() != m_firstSelectionOriginalBegin;
}
