// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "transitioneditorconstants.h"

#include <timelineeditor/timelineitem.h>
#include <timelineeditor/timelinemovableabstractitem.h>

#include <modelnode.h>
#include <qmltimeline.h>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QPainter)

namespace QmlDesigner {

class TransitionEditorSectionItem;
class TransitionEditorPropertyItem;

class TransitionEditorBarItem : public TimelineMovableAbstractItem
{
    Q_DECLARE_TR_FUNCTIONS(TimelineBarItem)

    enum class Location { Undefined, Center, Left, Right };

public:
    explicit TransitionEditorBarItem(TransitionEditorSectionItem *parent);
    explicit TransitionEditorBarItem(TransitionEditorPropertyItem *parent);

    void itemMoved(const QPointF &start, const QPointF &end) override;
    void commitPosition(const QPointF &point) override;

    bool isLocked() const override;

protected:
    void scrollOffsetChanged() override;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    TransitionEditorSectionItem *sectionItem() const;
    TransitionEditorPropertyItem *propertyItem() const;

    void dragInit(const QRectF &rect, const QPointF &pos);
    void dragCenter(QRectF rect, const QPointF &pos, qreal min, qreal max);
    void dragHandle(QRectF rect, const QPointF &pos, qreal min, qreal max);
    bool handleRects(const QRectF &rect, QRectF &left, QRectF &right) const;
    bool isActiveHandle(Location location) const;

    void setOutOfBounds(Location location);
    bool validateBounds(qreal pivot);

private:
    Location m_handle = Location::Undefined;

    Location m_bounds = Location::Undefined;

    qreal m_pivot = 0.0;

    QRectF m_oldRect;

    static constexpr qreal minimumBarWidth = 2.0
                                             * static_cast<qreal>(TimelineConstants::sectionHeight);
};

class TransitionEditorSectionItem : public TimelineItem
{
    Q_OBJECT

public:
    enum { Type =  TransitionEditorConstants::transitionEditorSectionItemUserType };

    static TransitionEditorSectionItem *create(const ModelNode &animation,
                                       TimelineItem *parent);

    void invalidateBar();

    int type() const override;

    static void updateData(QGraphicsItem *item);
    static void invalidateBar(QGraphicsItem *item);
    static void updateDataForTarget(QGraphicsItem *item, const ModelNode &target, bool *b);
    static void updateHeightForTarget(QGraphicsItem *item, const ModelNode &target);

    void moveAllDurations(qreal offset);
    void scaleAllDurations(qreal scale);
    qreal firstFrame();
    AbstractView *view() const;
    bool isSelected() const;

    ModelNode targetNode() const;
    void updateData();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void resizeEvent(QGraphicsSceneResizeEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    void updateHeight();
    void invalidateHeight();
    void invalidateProperties();
    bool collapsed() const;
    qreal rulerWidth() const;
    void toggleCollapsed();
    void createPropertyItems();
    const QList<QGraphicsItem *> propertyItems() const;

    TransitionEditorSectionItem(TimelineItem *parent = nullptr);
    ModelNode m_targetNode;
    ModelNode m_animationNode;

    TransitionEditorBarItem *m_barItem = nullptr;
    TimelineItem *m_dummyItem = nullptr;
};

} // namespace QmlDesigner
