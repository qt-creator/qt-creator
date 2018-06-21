/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#pragma once

#include <QGraphicsRectItem>

#include "arrowitem.h"

#include <QHash>

namespace qmt {

class DiagramSceneModel;
class IRelationable;

class RelationStarter : public QGraphicsRectItem
{
public:
    RelationStarter(IRelationable *owner, DiagramSceneModel *diagramSceneModel,
                    QGraphicsItem *parent = nullptr);
    ~RelationStarter() override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    void addArrow(const QString &id, ArrowItem::Shaft shaft, ArrowItem::Head startHead,
                  ArrowItem::Head endHead,
                  const QString &toolTip = QString());

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void updateCurrentPreviewArrow(const QPointF &headPoint);

    IRelationable *m_owner = nullptr;
    DiagramSceneModel *m_diagramSceneModel = nullptr;
    QList<ArrowItem *> m_arrows;
    QHash<ArrowItem *, QString> m_arrowIds;
    ArrowItem *m_currentPreviewArrow = nullptr;
    QString m_currentPreviewArrowId;
    QList<QPointF> m_currentPreviewArrowIntermediatePoints;
};

} // namespace qmt
