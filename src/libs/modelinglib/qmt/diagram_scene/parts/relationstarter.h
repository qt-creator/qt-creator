// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
