/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMT_GRAPHICSRELATIONSTARTER_H
#define QMT_GRAPHICSRELATIONSTARTER_H

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
                    QGraphicsItem *parent = 0);
    ~RelationStarter() override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = 0) override;

    void addArrow(const QString &id, ArrowItem::Shaft shaft, ArrowItem::Head endHead,
                  ArrowItem::Head startHead = ArrowItem::HeadNone);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void updateCurrentPreviewArrow(const QPointF &headPoint);

    IRelationable *m_owner;
    DiagramSceneModel *m_diagramSceneModel;
    QList<ArrowItem *> m_arrows;
    QHash<ArrowItem *, QString> m_arrowIds;
    ArrowItem *m_currentPreviewArrow;
    QString m_currentPreviewArrowId;
    QList<QPointF> m_currentPreviewArrowIntermediatePoints;
};

} // namespace qmt

#endif // QMT_GRAPHICSRELATIONSTARTER_H
