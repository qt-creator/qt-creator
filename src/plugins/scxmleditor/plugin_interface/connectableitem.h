/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "baseitem.h"
#include "transitionitem.h"

#include <QPen>

QT_FORWARD_DECLARE_CLASS(QGraphicsSceneMouseEvent)

namespace ScxmlEditor {

namespace PluginInterface {

class CornerGrabberItem;
class HighlightItem;
class QuickTransitionItem;

/**
 * @brief The ConnectableItem class is the base class for all draggable state-items.
 */
class ConnectableItem : public BaseItem
{
    Q_OBJECT

public:
    explicit ConnectableItem(const QPointF &pos, BaseItem *parent = nullptr);
    ~ConnectableItem() override;

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

    void addOutputTransition(TransitionItem *transition);
    void addInputTransition(TransitionItem *transition);
    void removeOutputTransition(TransitionItem *transition);
    void removeInputTransition(TransitionItem *transition);
    QVector<TransitionItem*> outputTransitions() const;
    QVector<TransitionItem*> inputTransitions() const;

    void setMinimumWidth(int width);
    void setMinimumHeight(int height);
    int minHeight() const
    {
        return m_minimumHeight;
    }

    int minWidth() const
    {
        return m_minimumWidth;
    }

    void finalizeCreation() override;
    void init(ScxmlTag *tag, BaseItem *parentItem = nullptr, bool initChildren = true, bool blockUpdates = false) override;

    void updateUIProperties() override;
    void updateAttributes() override;
    void updateEditorInfo(bool allChildren = false) override;
    void moveStateBy(qreal dx, qreal dy) override;
    void setHighlight(bool hl) override;

    int transitionCount() const;
    int outputTransitionCount() const;
    int inputTransitionCount() const;
    bool hasInputTransitions(const ConnectableItem *parentItem, bool checkChildren = false) const;
    bool hasOutputTransitions(const ConnectableItem *parentItem, bool checkChildren = false) const;
    QPointF getInternalPosition(const TransitionItem *transition, TransitionItem::TransitionTargetType type) const;

    void updateOutputTransitions();
    void updateInputTransitions();
    void updateTransitions(bool allChildren = false);
    void updateTransitionAttributes(bool allChildren = false);

    void addOverlappingItem(ConnectableItem *item);
    void removeOverlappingItem(ConnectableItem *item);
    void checkOverlapping() override;

    // Parent change
    virtual void releaseFromParent();
    virtual void connectToParent(BaseItem *parentItem);

    qreal getOpacity();

protected:
    void readUISpecifiedProperties(const ScxmlTag *tag) override;
    void addTransitions(const ScxmlTag *tag);
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    virtual bool canStartTransition(ItemType type) const;
    virtual void transitionCountChanged();
    virtual void transitionsChanged();

private:
    void updateCornerPositions();
    void createCorners();
    void removeCorners();
    void updateShadowClipRegion();

    QVector<TransitionItem*> m_outputTransitions;
    QVector<TransitionItem*> m_inputTransitions;
    QVector<CornerGrabberItem*> m_corners;
    QVector<QuickTransitionItem*> m_quickTransitions;
    HighlightItem *m_highlighItem = nullptr;
    TransitionItem *m_newTransition = nullptr;
    QPen m_selectedPen;
    QBrush m_releasedFromParentBrush;
    int m_minimumWidth = 120;
    int m_minimumHeight = 100;
    bool m_releasedFromParent = false;
    int m_releasedIndex = -1;
    QGraphicsItem *m_releasedParent = nullptr;
    QPointF m_newTransitionStartedPoint;
    QPainterPath m_shadowClipPath;
    QPointF m_moveStartPoint;
    bool m_moveMacroStarted = false;
    QVector<ConnectableItem*> m_overlappedItems;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
