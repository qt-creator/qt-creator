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

#include "curveeditorstyle.h"
#include "handleitem.h"
#include "keyframe.h"
#include "selectableitem.h"

#include <QGraphicsObject>

namespace DesignTools {

class HandleItem;

class KeyframeItem : public SelectableItem
{
    Q_OBJECT

signals:
    void redrawCurve();

    void keyframeMoved(KeyframeItem *item, const QPointF &direction);

    void handleMoved(KeyframeItem *frame, HandleItem::Slot slot, double angle, double deltaLength);

public:
    KeyframeItem(QGraphicsItem *parent = nullptr);

    KeyframeItem(const Keyframe &keyframe, QGraphicsItem *parent = nullptr);

    ~KeyframeItem() override;

    enum { Type = ItemTypeKeyframe };

    int type() const override;

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void lockedCallback() override;

    Keyframe keyframe() const;

    bool isUnified() const;

    bool hasLeftHandle() const;

    bool hasRightHandle() const;

    bool hasActiveHandle() const;

    HandleItem *leftHandle() const;

    HandleItem *rightHandle() const;

    QTransform transform() const;

    void setHandleVisibility(bool visible);

    void setComponentTransform(const QTransform &transform);

    void setStyle(const CurveEditorStyle &style);

    void setKeyframe(const Keyframe &keyframe);

    void toggleUnified();

    void setActivated(bool active, HandleItem::Slot slot);

    void setInterpolation(Keyframe::Interpolation interpolation);

    void setLeftHandle(const QPointF &pos);

    void setRightHandle(const QPointF &pos);

    void moveKeyframe(const QPointF &direction);

    void moveHandle(HandleItem::Slot slot, double deltaAngle, double deltaLength);

protected:
    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    void selectionCallback() override;

private:
    void updatePosition(bool emit = true);

    void updateHandle(HandleItem *handle, bool emit = true);

private:
    QTransform m_transform;

    KeyframeItemStyleOption m_style;

    Keyframe m_frame;

    HandleItem *m_left;

    HandleItem *m_right;

    bool m_visibleOverride = true;
};

} // End namespace DesignTools.
