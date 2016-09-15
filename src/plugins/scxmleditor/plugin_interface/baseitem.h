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

#include "mytypes.h"
#include "scxmltag.h"

#include <QGraphicsItem>
#include <QPointer>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QGraphicsSceneMouseEvent)
QT_FORWARD_DECLARE_CLASS(QVariant)

namespace ScxmlEditor {

namespace PluginInterface {

const qreal WARNING_ITEM_SIZE = 25;

class GraphicsScene;
class ScxmlUiFactory;

/**
 * @brief The BaseItem class is the base class for every items in the scene which represent the one tag in the SCXML-tree.
 *
 *
 */
class BaseItem : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit BaseItem(BaseItem *parent = nullptr);
    ~BaseItem() override;

    QRectF boundingRect() const override;

    QPointF sceneCenter() const
    {
        return mapToScene(m_boundingRect.center());
    }

    QString itemId() const;

    void setItemBoundingRect(const QRectF &r);

    QPolygonF polygonShape() const
    {
        return m_polygon;
    }

    void updateDepth();
    int depth() const
    {
        return m_depth;
    }

    void setParentItem(BaseItem *item);
    virtual void checkWarnings();
    virtual void checkOverlapping();
    virtual void checkVisibility(double scaleFactor);
    virtual void setTag(ScxmlTag *tag);
    virtual void init(ScxmlTag *tag, BaseItem *parentItem = nullptr, bool initChildren = true, bool blockUpdates = false);
    virtual void updateAttributes();
    virtual void updateColors();
    virtual void updateEditorInfo(bool allChildren = false);
    virtual void moveStateBy(qreal dx, qreal dy);
    virtual ScxmlTag *tag() const;
    virtual void finalizeCreation();
    virtual void doLayout(int depth);
    virtual void setHighlight(bool hl);
    virtual void checkInitial(bool parent = false);
    virtual bool containsScenePoint(const QPointF &p) const;

    void setEditorInfo(const QString &key, const QString &value, bool block = false);
    QString editorInfo(const QString &key) const;

    void setTagValue(const QString &key, const QString &value);
    QString tagValue(const QString &key, bool useNameSpace = false) const;

    void setBlockUpdates(bool block);
    void setOverlapping(bool ol);
    bool blockUpdates() const;
    bool highlight() const;
    bool overlapping() const;

    BaseItem *parentBaseItem() const;
    bool isActiveScene() const;
    GraphicsScene *graphicsScene() const;
    ScxmlUiFactory *uiFactory() const;

    virtual void updateUIProperties();

protected:
    virtual void updatePolygon();
    void setItemSelected(bool sel, bool unselectOthers = true);

signals:
    void geometryChanged();
    void selectedStateChanged(bool sel);
    void openToDifferentView(BaseItem *item);

protected:
    void postDeleteEvent();
    void mousePressEvent(QGraphicsSceneMouseEvent *e) override;
    void showContextMenu(QGraphicsSceneMouseEvent *e);
    virtual void checkSelectionBeforeContextMenu(QGraphicsSceneMouseEvent *e);
    virtual void createContextMenu(QMenu *menu);
    virtual void selectedMenuAction(const QAction *action);
    virtual void readUISpecifiedProperties(const ScxmlTag *tag);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void checkParentBoundingRect();
    QPolygonF m_polygon;

private:
    QRectF m_boundingRect;
    QPointer<ScxmlTag> m_tag;
    QPointer<GraphicsScene> m_scene;
    bool m_blockUpdates = false;
    int m_depth = 0;
    bool m_highlight = false;
    bool m_overlapping = false;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
