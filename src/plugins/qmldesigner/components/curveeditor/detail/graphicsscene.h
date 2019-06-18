/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "keyframeitem.h"

#include <QGraphicsScene>

namespace DesignTools {

class AnimationCurve;
class CurveItem;
class GraphicsView;

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT

signals:
    void curveChanged(unsigned int id, const AnimationCurve &curve);

public:
    GraphicsScene(QObject *parent = nullptr);

    bool empty() const;

    bool hasActiveKeyframe() const;

    bool hasActiveHandle() const;

    bool hasActiveItem() const;

    double minimumTime() const;

    double maximumTime() const;

    double minimumValue() const;

    double maximumValue() const;

    void addCurveItem(CurveItem *item);

    void setComponentTransform(const QTransform &transform);

    void keyframeMoved(KeyframeItem *item, const QPointF &direction);

    void handleMoved(KeyframeItem *frame, HandleSlot handle, double angle, double deltaLength);

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

private:
    using QGraphicsScene::addItem;

    GraphicsView *graphicsView() const;

    QRectF limits() const;

    mutable bool m_dirty;

    mutable QRectF m_limits;
};

} // End namespace DesignTools.
