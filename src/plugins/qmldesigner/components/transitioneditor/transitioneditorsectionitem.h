/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
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
