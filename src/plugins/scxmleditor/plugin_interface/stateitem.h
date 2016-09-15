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

#include "connectableitem.h"
#include <QPen>

QT_FORWARD_DECLARE_CLASS(QGraphicsSceneMouseEvent)

namespace ScxmlEditor {

namespace PluginInterface {

class TransitionItem;
class TextItem;
class IdWarningItem;
class StateWarningItem;

/**
 * @brief The StateItem class represents the SCXML-State.
 */
class StateItem : public ConnectableItem
{
    Q_OBJECT

public:
    explicit StateItem(const QPointF &pos = QPointF(), BaseItem *parent = nullptr);

    int type() const override
    {
        return StateType;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QString itemId() const;
    void init(ScxmlTag *tag, BaseItem *parentItem = nullptr, bool initChildren = true, bool blockUpdates = false) override;
    void updateBoundingRect();
    void updateAttributes() override;
    void updateEditorInfo(bool allChildren = false) override;
    void updateColors() override;
    virtual void doLayout(int d) override;
    void shrink();
    void setInitial(bool initial);
    bool isInitial() const;
    void checkWarnings() override;
    void checkInitial(bool parent = false) override;
    QRectF childItemsBoundingRect() const;
    void connectToParent(BaseItem *parentItem) override;

protected:
    void updatePolygon() override;
    void transitionsChanged() override;
    void transitionCountChanged() override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    bool canStartTransition(ItemType type) const override;
    void createContextMenu(QMenu *menu) override;
    void selectedMenuAction(const QAction *action) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void writeStateProperties(const QString &tag, QXmlStreamWriter &xml);

    QRectF m_drawingRect;
    QRectF m_titleRect;
    QRectF m_transitionRect;

private:
    void titleHasChanged(const QString &text);
    void updateTextPositions();
    void checkParentBoundingRect();
    void checkWarningItems();

    TextItem *m_stateNameItem;
    StateWarningItem *m_stateWarningItem = nullptr;
    IdWarningItem *m_idWarningItem = nullptr;
    QPen m_pen;
    bool m_initial = false;
    bool m_parallelState = false;
    QImage m_backgroundImage;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
