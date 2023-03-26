// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "curveeditorstyle.h"
#include "handleitem.h"
#include "keyframe.h"
#include "selectableitem.h"

#include <QGraphicsObject>

#include <optional>

namespace QmlDesigner {

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

    Keyframe keyframe(bool remap = false) const;

    bool isUnified() const;

    bool hasLeftHandle() const;

    bool hasRightHandle() const;

    bool hasActiveHandle() const;

    HandleItem *leftHandle() const;

    HandleItem *rightHandle() const;

    CurveSegment segment(HandleItem::Slot slot) const;

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

    void remapValue(double min, double max);

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

    std::optional< QPointF > m_firstPos;
    std::optional< QPointF > m_validPos;

    bool m_visibleOverride = true;

    double m_min = 0.0;
    double m_max = 1.0;
};

} // End namespace QmlDesigner.
