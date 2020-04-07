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
#include "curvesegment.h"
#include "handleitem.h"
#include "keyframe.h"
#include "selectableitem.h"
#include "treeitem.h"

#include <string>
#include <QGraphicsObject>

namespace DesignTools {

class AnimationCurve;
class KeyframeItem;
class GraphicsScene;

class CurveItem : public CurveEditorItem
{
    Q_OBJECT

signals:
    void curveChanged(unsigned int id, const AnimationCurve &curve);

    void keyframeMoved(KeyframeItem *item, const QPointF &direction);

    void handleMoved(KeyframeItem *frame, HandleItem::Slot slot, double angle, double deltaLength);

public:
    CurveItem(QGraphicsItem *parent = nullptr);

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

    unsigned int id() const;

    ValueType valueType() const;

    PropertyTreeItem::Component component() const;

    AnimationCurve curve() const;

    AnimationCurve resolvedCurve() const;

    std::vector<AnimationCurve> curves() const;

    QVector<KeyframeItem *> keyframes() const;

    QVector<KeyframeItem *> selectedKeyframes() const;

    QVector<HandleItem *> handles() const;

    void restore();

    void setDirty(bool dirty);

    void setHandleVisibility(bool visible);

    void setValueType(ValueType type);

    void setComponent(PropertyTreeItem::Component comp);

    void setCurve(const AnimationCurve &curve);

    QRectF setComponentTransform(const QTransform &transform);

    void setStyle(const CurveEditorStyle &style);

    void setInterpolation(Keyframe::Interpolation interpolation);

    void toggleUnified();

    void connect(GraphicsScene *scene);

    void insertKeyframeByTime(double time);

    void deleteSelectedKeyframes();

private:
    void emitCurveChanged();

    unsigned int m_id;

    CurveItemStyleOption m_style;

    ValueType m_type;

    PropertyTreeItem::Component m_component;

    QTransform m_transform;

    QVector<KeyframeItem *> m_keyframes;

    bool m_itemDirty;
};

} // End namespace DesignTools.
