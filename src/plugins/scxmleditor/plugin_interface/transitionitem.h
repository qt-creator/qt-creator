// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "transitionwarningitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QPen>
#include <QPointF>
#include <QPolygon>
#include <QRectF>
#include <QVector>

namespace ScxmlEditor {

namespace PluginInterface {

class CornerGrabberItem;
class TagTextItem;
class ConnectableItem;

/**
 * @brief The TransitionItem class provides the item to connect two Connectable-items.
 */
class TransitionItem : public BaseItem
{
    Q_OBJECT

public:
    explicit TransitionItem(BaseItem *parent = nullptr);
    ~TransitionItem() override;

    enum TransitionTargetType {
        InternalSameTarget = 0,
        InternalNoTarget,
        ExternalNoTarget,
        ExternalTarget
    };

    enum TransitionPoint {
        Start = 0,
        End
    };

    /**
     * @brief type - return tye type of the item.
     */
    int type() const override
    {
        return TransitionType;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

    void disconnectItem(ConnectableItem *item);
    void setStartItem(ConnectableItem *item);
    void setEndItem(ConnectableItem *item, bool fixValue = true);

    void startTransitionFrom(ConnectableItem *item, const QPointF &mouseScenePos);
    void setEndPos(const QPointF &endPos, bool snap = true);
    void connectToTopItem(const QPointF &pos, TransitionPoint p, ItemType targetType);

    bool isStartItem(const ConnectableItem *item) const;
    bool isEndItem(const ConnectableItem *item) const;
    ConnectableItem *connectedItem(const ConnectableItem *other) const;

    bool hasStartItem() const;
    bool hasEndItem() const;

    void init(ScxmlTag *tag, BaseItem *parentItem = nullptr, bool initChildren = true, bool blockUpdates = false) override;
    void updateEditorInfo(bool allChilds = true) override;
    void updateAttributes() override;
    void updateTarget(bool fixValue = true);
    void finalizeCreation() override;
    void checkVisibility(double scaleFactor) override;
    bool containsScenePoint(const QPointF &p) const override;

    void updateUIProperties() override;
    void updateTargetType();
    void setTag(ScxmlTag *tag) override;
    qreal textWidth() const;
    QRectF wholeBoundingRect() const;

    TransitionTargetType targetType() const
    {
        return m_targetType;
    }

    void grabMouse(ItemType targetType);
    void storeGeometry(bool block = false);
    void storeValues(bool block = false);
    void updateComponents();
    void textItemPositionChanged();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void updateToolTip();
    void readUISpecifiedProperties(const ScxmlTag *tag) override;
    void checkSelectionBeforeContextMenu(QGraphicsSceneMouseEvent *e) override;
    void createContextMenu(QMenu *menu) override;
    void selectedMenuAction(const QAction *action) override;

private:
    void textHasChanged(const QString &text);
    void checkWarningItems();
    void storeMovePoint(bool block = false);
    void storeTargetFactors(bool block = false);
    void updateZValue();
    void removeUnnecessaryPoints();
    void findEndItem();
    void updateEventName();
    void createGrabbers();
    void removeGrabbers();
    void updateGrabberPositions();
    void removeTransition(TransitionPoint p);
    void snapToAnyPoint(int index, const QPointF &newPoint, int diff = 8);
    void snapPointToPoint(int index, const QPointF &newPoint, int diff = 8);
    QPointF loadPoint(const QString &name);
    void savePoint(const QPointF &p, const QString &name);
    QPointF calculateTargetFactor(ConnectableItem *item, const QPointF &pos);
    QPointF sceneTargetPoint(TransitionPoint p);
    QPointF findIntersectionPoint(ConnectableItem *item, const QLineF &line, const QPointF &defaultPoint);
    QVector<CornerGrabberItem*> m_cornerGrabbers;
    CornerGrabberItem *m_selectedCornerGrabber = nullptr;

    QPolygonF m_cornerPoints;

    ConnectableItem *m_startItem = nullptr;
    ConnectableItem *m_oldStartItem = nullptr;
    ConnectableItem *m_endItem = nullptr;

    QPolygonF m_arrow;
    qreal m_arrowSize = 10;
    qreal m_arrowAngle;
    QPen m_pen;
    QPen m_highlightPen;
    bool m_lineSelected = false;

    TagTextItem *m_eventTagItem;
    TransitionWarningItem *m_warningItem = nullptr;
    TransitionTargetType m_targetType = ExternalTarget;
    bool m_movingFirstPoint = false;
    bool m_movingLastPoint = false;
    bool m_mouseGrabbed = false;
    ItemType m_grabbedTargetType = UnknownType;
    int m_selectedGrabberIndex = -1;
    QPointF m_startTargetFactor;
    QPointF m_endTargetFactor;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
