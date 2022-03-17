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

TextEditorOverlay::TextEditorOverlay(TextEditorWidget *editor) :
    QObject(editor),
    m_visible(false),
    m_alpha(true),
    m_borderWidth(1),
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
        || m_editor->blockBoundingGeometry(end.block()).translated(offset).bottom() < clip.top() - 10
        )
        return QPainterPath(); // nothing of the selection is visible


    QTextBlock block = begin.block();

    if (block.blockNumber() < m_editor->firstVisibleBlock().blockNumber() - 4)
        block = m_editor->document()->findBlockByNumber(m_editor->firstVisibleBlock().blockNumber() - 4);

    bool inSelection = false;

    QVector<QRectF> selection;

    if (begin.position() == end.position()) {
        // special case empty selections
        const QRectF blockGeometry = m_editor->blockBoundingGeometry(block);
        QTextLayout *blockLayout = block.layout();
        int pos = begin.position() - begin.block().position();
        QTextLine line = blockLayout->lineForTextPosition(pos);
        QRectF lineRect = line.naturalTextRect();
        int x = line.cursorToX(pos);
        lineRect.setLeft(x - m_borderWidth);
        lineRect.setRight(x + m_borderWidth);
        selection += lineRect.translated(blockGeometry.topLeft());
    } else {
        for (; block.isValid() && block.blockNumber() <= end.blockNumber(); block = block.next()) {
            if (! block.isVisible())
                continue;

            const QRectF blockGeometry = m_editor->blockBoundingGeometry(block);
            QTextLayout *blockLayout = block.layout();

            QTextLine line = blockLayout->lineAt(0);
            bool firstOrLastBlock = false;

            int beginChar = 0;
            if (!inSelection) {
                if (block == begin.block()) {
                    beginChar = begin.positionInBlock();
                    line = blockLayout->lineForTextPosition(beginChar);
                    firstOrLastBlock = true;
                }
                inSelection = true;
            } else {
//                while (beginChar < block.length() && document->characterAt(block.position() + beginChar).isSpace())
//                    ++beginChar;
//                if (beginChar == block.length())
//                    beginChar = 0;
            }

            int lastLine = blockLayout->lineCount()-1;
            int endChar = -1;
            if (block == end.block()) {
                endChar = end.positionInBlock();
                lastLine = blockLayout->lineForTextPosition(endChar).lineNumber();
                inSelection = false;
                firstOrLastBlock = true;
            } else {
                endChar = block.length();
                while (endChar > beginChar && document->characterAt(block.position() + endChar - 1).isSpace())
                    --endChar;
            }

            QRectF lineRect = line.naturalTextRect();
            if (beginChar < endChar) {
                lineRect.setLeft(line.cursorToX(beginChar));
                if (line.lineNumber() == lastLine)
                    lineRect.setRight(line.cursorToX(endChar));
                selection += lineRect.translated(blockGeometry.topLeft());

                for (int lineIndex = line.lineNumber()+1; lineIndex <= lastLine; ++lineIndex) {
                    line = blockLayout->lineAt(lineIndex);
                    lineRect = line.naturalTextRect();
                    if (lineIndex == lastLine)
                        lineRect.setRight(line.cursorToX(endChar));
                    selection += lineRect.translated(blockGeometry.topLeft());
                }
            } else { // empty lines
                const int emptyLineSelectionSize = 16;
                if (!firstOrLastBlock && !selection.isEmpty()) { // middle
                    lineRect.setLeft(selection.last().left());
                } else if (inSelection) { // first line
                    lineRect.setLeft(line.cursorToX(beginChar));
                } else { // last line
                    if (endChar == 0)
                        break;
                    lineRect.setLeft(line.cursorToX(endChar) - emptyLineSelectionSize);
                }
                lineRect.setRight(lineRect.left() + emptyLineSelectionSize);
                selection += lineRect.translated(blockGeometry.topLeft());
            }

            if (!inSelection)
                break;

            if (blockGeometry.translated(offset).y() > 2*viewportRect.height())
                break;
        }
    }


    if (selection.isEmpty())
        return QPainterPath();

    QVector<QPointF> points;

    const int margin = m_borderWidth/2;
    const int extra = 0;

    const QRectF &firstSelection = selection.at(0);
    points += (firstSelection.topLeft() + firstSelection.topRight()) / 2 + QPointF(0, -margin);
    points += firstSelection.topRight() + QPointF(margin+1, -margin);
    points += firstSelection.bottomRight() + QPointF(margin+1, 0);

    const int count = selection.count();
    for (int i = 1; i < count-1; ++i) {
        qreal x = std::max({selection.at(i - 1).right(),
                            selection.at(i).right(),
                            selection.at(i + 1).right()})
                  + margin;

        points += QPointF(x+1, selection.at(i).top());
        points += QPointF(x+1, selection.at(i).bottom());
    }

    const QRectF &lastSelection = selection.at(count-1);
    points += lastSelection.topRight() + QPointF(margin+1, 0);
    points += lastSelection.bottomRight() + QPointF(margin+1, margin+extra);
    points += lastSelection.bottomLeft() + QPointF(-margin, margin+extra);
    points += lastSelection.topLeft() + QPointF(-margin, 0);

    for (int i = count-2; i > 0; --i) {
        qreal x = std::min({selection.at(i - 1).left(),
                            selection.at(i).left(),
                            selection.at(i + 1).left()})
                  - margin;

        points += QPointF(x, selection.at(i).bottom()+extra);
        points += QPointF(x, selection.at(i).top());
    }

    points += firstSelection.bottomLeft() + QPointF(-margin, extra);
    points += firstSelection.topLeft() + QPointF(-margin, -margin);


    QPainterPath path;
    const int corner = 4;
    path.moveTo(points.at(0));
    points += points.at(0);
    QPointF previous = points.at(0);
    for (int i = 1; i < points.size(); ++i) {
        QPointF point = points.at(i);
        if (point.y() == previous.y() && qAbs(point.x() - previous.x()) > 2*corner) {
            QPointF tmp = QPointF(previous.x() + corner * ((point.x() > previous.x())?1:-1), previous.y());
            path.quadTo(previous, tmp);
            previous = tmp;
            i--;
            continue;
        } else if (point.x() == previous.x() && qAbs(point.y() - previous.y()) > 2*corner) {
            QPointF tmp = QPointF(previous.x(), previous.y() + corner * ((point.y() > previous.y())?1:-1));
            path.quadTo(previous, tmp);
            previous = tmp;
            i--;
            continue;
        }


        QPointF target = (previous + point) / 2;
        path.quadTo(previous, target);
        previous = points.at(i);
    }
    path.closeSubpath();
    path.translate(offset);
    return path.simplified();
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

    painter->save();
    QColor penColor = fg;
    if (m_alpha)
        penColor.setAlpha(220);
    QPen pen(penColor, m_borderWidth);
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

    painter->setRenderHint(QPainter::Antialiasing);

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

    pen.setJoinStyle(Qt::RoundJoin);
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

    painter->save();
    painter->translate(-.5, -.5);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->fillPath(path, color);
    painter->restore();
}

void TextEditorOverlay::paint(QPainter *painter, const QRect &clip)
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

        paintSelection(painter, selection, clip);
    }
    for (int i = m_selections.size()-1; i >= 0; --i) {
        const OverlaySelection &selection = m_selections.at(i);
        if (!selection.m_dropShadow)
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
