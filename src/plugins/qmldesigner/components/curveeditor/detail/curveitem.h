// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "curveeditorstyle.h"
#include "curvesegment.h"
#include "handleitem.h"
#include "keyframe.h"
#include "selectableitem.h"
#include "treeitem.h"

#include <string>
#include <QGraphicsObject>

namespace QmlDesigner {

class AnimationCurve;
class KeyframeItem;
class GraphicsScene;

class CurveItem : public CurveEditorItem
{
    Q_OBJECT

signals:
    void curveMessage(const QString& msg);

    void curveChanged(unsigned int id, const AnimationCurve &curve);

    void keyframeMoved(KeyframeItem *item, const QPointF &direction);

    void handleMoved(KeyframeItem *frame, HandleItem::Slot slot, double angle, double deltaLength);

public:
    CurveItem(unsigned int id, const AnimationCurve &curve, QGraphicsItem *parent = nullptr);

    ~CurveItem() override;

    enum { Type = ItemTypeCurve };

    int type() const override;

    QRectF boundingRect() const override;

    bool contains(const QPointF &point) const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void lockedCallback() override;

    bool isDirty() const;

    bool hasActiveKeyframe() const;

    bool hasActiveHandle() const;

    bool hasSelectedKeyframe() const;

    bool hasEditableSegment(double time) const;

    bool isFirst(const KeyframeItem *key) const;

    bool isLast(const KeyframeItem *key) const;

    int indexOf(const KeyframeItem *key) const;

    unsigned int id() const;

    PropertyTreeItem::ValueType valueType() const;

    PropertyTreeItem::Component component() const;

    AnimationCurve curve(bool remap = false) const;

    AnimationCurve resolvedCurve() const;

    std::vector<AnimationCurve> curves() const;

    QList<KeyframeItem *> keyframes() const;

    QList<KeyframeItem *> selectedKeyframes() const;

    QList<HandleItem *> handles() const;

    CurveSegment segment(const KeyframeItem *keyframe, HandleItem::Slot slot) const;

    void restore();

    void setDirty(bool dirty);

    void setIsMcu(bool isMcu);

    void setHandleVisibility(bool visible);

    void setValueType(PropertyTreeItem::ValueType type);

    void setComponent(PropertyTreeItem::Component comp);

    void setCurve(const AnimationCurve &curve);

    QRectF setComponentTransform(const QTransform &transform);

    void setStyle(const CurveEditorStyle &style);

    void setInterpolation(Keyframe::Interpolation interpolation);

    void toggleUnified();

    void connect(GraphicsScene *scene);

    void insertKeyframeByTime(double time);

    void deleteSelectedKeyframes();

    void remapValue(double min, double max);

private:
    void markDirty();

    unsigned int m_id;

    CurveItemStyleOption m_style;

    PropertyTreeItem::ValueType m_type;

    PropertyTreeItem::Component m_component;

    QTransform m_transform;

    QList<KeyframeItem *> m_keyframes;

    bool m_mcu;

    bool m_itemDirty;
};

} // End namespace QmlDesigner.
