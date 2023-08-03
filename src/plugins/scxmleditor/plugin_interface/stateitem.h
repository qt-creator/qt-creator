// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "connectableitem.h"
#include "textitem.h"
#include <QPen>
#include <QStyleOptionGraphicsItem>

QT_FORWARD_DECLARE_CLASS(QGraphicsSceneMouseEvent)

namespace ScxmlEditor {

namespace PluginInterface {

class TransitionItem;
class TextItem;
class IdWarningItem;
class StateWarningItem;
class OnEntryExitItem;

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
    void doLayout(int d) override;
    void shrink();
    void setInitial(bool initial);
    bool isInitial() const;
    void checkWarnings() override;
    void checkInitial(bool parent = false) override;
    QRectF childItemsBoundingRect() const;
    void connectToParent(BaseItem *parentItem) override;

    void addChild(ScxmlTag *child) override;

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
    void positionOnExitItems();
    void positionOnEntryItems();

    TextItem *m_stateNameItem;
    StateWarningItem *m_stateWarningItem = nullptr;
    IdWarningItem *m_idWarningItem = nullptr;
    QPen m_pen;
    bool m_initial = false;
    bool m_parallelState = false;
    QPointer<OnEntryExitItem> m_onEntryItem;
    QPointer<OnEntryExitItem> m_onExitItem;
    QImage m_backgroundImage;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
