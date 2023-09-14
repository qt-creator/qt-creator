// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "keyframeitem.h"

#include <QElapsedTimer>
#include <QGraphicsScene>

namespace QmlDesigner {

class AnimationCurve;
class CurveItem;
class GraphicsView;

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT

signals:
    void curveMessage(const QString& msg);

    void curveChanged(unsigned int id, const AnimationCurve &curve);

public:
    GraphicsScene(QObject *parent = nullptr);

    ~GraphicsScene() override;

    bool empty() const;

    bool hasActiveKeyframe() const;

    bool hasActiveHandle() const;

    bool hasActiveItem() const;

    bool hasSelectedKeyframe() const;

    bool hasEditableSegment(double time) const;

    double minimumTime() const;

    double maximumTime() const;

    double minimumValue() const;

    double maximumValue() const;

    double animationRangeMin() const;

    double animationRangeMax() const;

    QRectF rect() const;

    QList<CurveItem *> curves() const;

    QList<CurveItem *> selectedCurves() const;

    QList<KeyframeItem *> keyframes() const;

    QList<KeyframeItem *> selectedKeyframes() const;

    QList<HandleItem *> handles() const;

    CurveItem *findCurve(unsigned int id) const;

    SelectableItem *intersect(const QPointF &pos) const;

    void setDirty(bool dirty);

    void setIsMcu(bool isMcu);

    void reset();

    void deleteSelectedKeyframes();

    void insertKeyframe(double time, bool all = false);

    void doNotMoveItems(bool tmp);

    void removeCurveItem(unsigned int id);

    void addCurveItem(CurveItem *item);

    void moveToBottom(CurveItem *item);

    void moveToTop(CurveItem *item);

    void setComponentTransform(const QTransform &transform);

    void keyframeMoved(KeyframeItem *item, const QPointF &direction);

    void handleUnderMouse(HandleItem *handle);

    void handleMoved(KeyframeItem *frame, HandleItem::Slot slot, double angle, double deltaLength);

    void setPinned(uint id, bool pinned);

    std::vector<CurveItem *> takePinnedItems();

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

    void focusOutEvent(QFocusEvent *focusEvent) override;

    void focusInEvent(QFocusEvent *focusEvent) override;

private:
    using QGraphicsScene::addItem;

    using QGraphicsScene::clear;

    using QGraphicsScene::removeItem;

    GraphicsView *graphicsView() const;

    QRectF limits() const;

    void resetZValues();

    QList<CurveItem *> m_curves;

    mutable bool m_dirty;

    mutable QRectF m_limits;

    bool m_doNotMoveItems;

    QElapsedTimer m_usageTimer;

    bool m_isMcu;
};

} // End namespace QmlDesigner.
