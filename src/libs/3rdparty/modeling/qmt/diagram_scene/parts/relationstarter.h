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


class RelationStarter :
        public QGraphicsRectItem
{
public:
    RelationStarter(IRelationable *owner, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent = 0);

    ~RelationStarter();

public:

    QRectF boundingRect() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

public:

    void addArrow(const QString &id, ArrowItem::Shaft shaft, ArrowItem::Head end_head, ArrowItem::Head start_head = ArrowItem::HEAD_NONE);

protected:

    void mousePressEvent(QGraphicsSceneMouseEvent *event);

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    void keyPressEvent(QKeyEvent *event);

private:

    void updateCurrentPreviewArrow(const QPointF &head_point);

public:

    IRelationable *_owner;

    DiagramSceneModel *_diagram_scene_model;

    QList<ArrowItem *> _arrows;

    QHash<ArrowItem *, QString> _arrow_ids;

    ArrowItem *_current_preview_arrow;

    QString _current_preview_arrow_id;

    QList<QPointF> _current_preview_arrow_intermediate_points;
};

}

#endif // QMT_GRAPHICSRELATIONSTARTER_H
