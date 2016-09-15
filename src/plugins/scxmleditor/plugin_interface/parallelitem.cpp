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

#include "parallelitem.h"
#include "scxmldocument.h"
#include "scxmltagutils.h"
#include <QPainter>

using namespace ScxmlEditor::PluginInterface;

ParallelItem::ParallelItem(const QPointF &pos, BaseItem *parent)
    : StateItem(pos, parent)
{
    m_pixmap = QPixmap(":/scxmleditor/images/parallel_icon.png");
    updatePolygon();
}

void ParallelItem::updatePolygon()
{
    StateItem::updatePolygon();
    int cap = m_titleRect.height() * 0.2;
    m_pixmapRect = m_titleRect.adjusted(m_titleRect.width() - m_titleRect.height(), cap, -cap, -cap).toRect();
}

void ParallelItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    StateItem::paint(painter, option, widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setOpacity(getOpacity());

    painter->drawPixmap(m_pixmapRect, m_pixmap);
    painter->restore();
}

void ParallelItem::doLayout(int d)
{
    if (depth() != d)
        return;

    // 1. Find children items
    QVector<StateItem*> children;
    foreach (QGraphicsItem *it, childItems()) {
        if (it->type() >= StateType) {
            auto itt = qgraphicsitem_cast<StateItem*>(it);
            if (itt)
                children << itt;
        }
    }

    // 2. Adjust sizes
    foreach (StateItem *itt, children) {
        itt->shrink();
    }

    qreal maxw = 0;
    foreach (StateItem *itt, children) {
        QRectF rr = itt->boundingRect();
        maxw = qMax(rr.width(), maxw);
    }

    foreach (StateItem *itt, children) {
        QRectF rr = itt->boundingRect();
        if (!qFuzzyCompare(rr.width(), maxw))
            rr.setWidth(maxw);
        itt->setItemBoundingRect(rr);
    }

    // 3. Relocate children-states
    // a) sort list
    QVector<StateItem*> sortedList;
    while (children.count() > 0) {
        qreal minTop = children.first()->boundingRect().top();
        int minTopIndex = 0;
        for (int i = 1; i < children.count(); ++i) {
            qreal top = children[i]->boundingRect().top();
            if (top < minTop) {
                minTop = top;
                minTopIndex = i;
            }
        }
        sortedList << children.takeAt(minTopIndex);
    }

    // b) relocate items
    for (int i = 1; i < sortedList.count(); ++i) {
        QRectF br1 = sortedList[i - 1]->sceneBoundingRect();
        QRectF br2 = sortedList[i]->sceneBoundingRect();
        sortedList[i]->moveStateBy(br1.left() - br2.left(), br1.bottom() + 10 - br2.top());
    }

    // 4. Shrink parallel-state
    shrink();
}
