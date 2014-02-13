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
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "selectabletexteditorwidget.h"

#include <QPainter>
#include <QTextBlock>

namespace DiffEditor {

SelectableTextEditorWidget::SelectableTextEditorWidget(QWidget *parent)
    : BaseTextEditorWidget(parent)
{
    setFrameStyle(QFrame::NoFrame);
}

SelectableTextEditorWidget::~SelectableTextEditorWidget()
{

}

void SelectableTextEditorWidget::paintEvent(QPaintEvent *e)
{
    paintSelections(e);
    BaseTextEditorWidget::paintEvent(e);
}

void SelectableTextEditorWidget::paintSelections(QPaintEvent *e)
{
    QPainter painter(viewport());

    QPointF offset = contentOffset();
    QTextBlock firstBlock = firstVisibleBlock();
    QTextBlock currentBlock = firstBlock;

    while (currentBlock.isValid()) {
        if (currentBlock.isVisible()) {
            qreal top = blockBoundingGeometry(currentBlock).translated(offset).top();
            qreal bottom = top + blockBoundingRect(currentBlock).height();

            if (top > e->rect().bottom())
                break;

            if (bottom >= e->rect().top()) {
                const int blockNumber = currentBlock.blockNumber();

                paintSelections(painter, m_selections.value(blockNumber),
                                currentBlock, top);
            }
        }
        currentBlock = currentBlock.next();
    }
}

void SelectableTextEditorWidget::paintSelections(QPainter &painter,
                                           const QList<DiffSelection> &selections,
                                           const QTextBlock &block,
                                           int top)
{
    QPointF offset = contentOffset();
    painter.save();

    QTextLayout *layout = block.layout();
    QTextLine textLine = layout->lineAt(0);
    QRectF lineRect = textLine.naturalTextRect().translated(offset.x(), top);
    QRect clipRect = contentsRect();
    painter.setClipRect(clipRect);
    for (int i = 0; i < selections.count(); i++) {
        const DiffSelection &selection = selections.at(i);

        if (!selection.format)
            continue;
        if (selection.start == -1 && selection.end == 0)
            continue;
        if (selection.start == selection.end && selection.start >= 0)
            continue;

        painter.save();
        const QBrush &brush = selection.format->background();
        painter.setPen(brush.color());
        painter.setBrush(brush);

        const int x1 = selection.start <= 0
                ? -1
                : textLine.cursorToX(selection.start) + offset.x();
        const int x2 = selection.end < 0
                ? clipRect.right()
                : textLine.cursorToX(selection.end) + offset.x();
        painter.drawRect(QRectF(QPointF(x1, lineRect.top()),
                                QPointF(x2, lineRect.bottom())));

        painter.restore();
    }
    painter.restore();
}


} // namespace DiffEditor

