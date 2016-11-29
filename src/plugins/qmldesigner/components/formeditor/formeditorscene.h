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

#include <qmlitemnode.h>
#include "abstractformeditortool.h"

#include <QGraphicsScene>
#include <QPointer>
#include <QHash>

QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

namespace QmlDesigner {

class FormEditorWidget;
class FormEditorItem;
class FormEditorView;
class LayerItem;

class QMLDESIGNERCORE_EXPORT FormEditorScene : public QGraphicsScene
{
    Q_OBJECT

    friend class QmlDesigner::FormEditorItem;
    friend class QmlDesigner::FormEditorView;

public:
    FormEditorScene(FormEditorWidget *widget, FormEditorView *editorView);
    ~FormEditorScene();
    FormEditorItem *addFormEditorItem(const QmlItemNode &qmlItemNode);

    FormEditorItem* itemForQmlItemNode(const QmlItemNode &qmlItemNode) const;

    QList<FormEditorItem*> itemsForQmlItemNodes(const QList<QmlItemNode> &nodeList) const;
    QList<FormEditorItem*> allFormEditorItems() const;

    void updateAllFormEditorItems();

    void setupScene();
    void resetScene();

    double canvasWidth() const;
    double canvasHeight() const;

    void synchronizeTransformation(FormEditorItem *item);
    void synchronizeParent(const QmlItemNode &qmlItemNode);
    void synchronizeOtherProperty(FormEditorItem *item, const QByteArray &propertyName);

    FormEditorItem* calulateNewParent(FormEditorItem *widget);
    LayerItem* manipulatorLayerItem() const;
    LayerItem* formLayerItem() const;
    FormEditorView *editorView() const;

    FormEditorItem *rootFormEditorItem() const;

    void reparentItem(const QmlItemNode &node, const QmlItemNode &newParent);

    void clearFormEditorItems();

    void highlightBoundingRect(FormEditorItem *formEditorItem);

public slots:
    void setShowBoundingRects(bool show);
    bool showBoundingRects() const;

protected:
    bool event(QEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent * event) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent * event) override;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent * event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent * event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

    void keyPressEvent(QKeyEvent *keyEvent) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;

    void focusOutEvent(QFocusEvent *focusEvent) override;

private:
    QList<QGraphicsItem *> removeLayerItems(const QList<QGraphicsItem *> &itemList);
    QList<QGraphicsItem *> itemsAt(const QPointF &pos);

    AbstractFormEditorTool* currentTool() const;
    void removeItemFromHash(FormEditorItem*);

    FormEditorView *m_editorView;
    AbstractFormEditorTool *m_currentTool;
    QHash<QmlItemNode, FormEditorItem*> m_qmlItemNodeItemHash;
    QPointer<LayerItem> m_formLayerItem;
    QPointer<LayerItem> m_manipulatorLayerItem;
    ModelNode m_dragNode;
    bool m_showBoundingRects;
};

} // namespace QmlDesigner
