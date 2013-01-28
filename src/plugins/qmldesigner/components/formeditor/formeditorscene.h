/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FORMEDITORSCENE_H
#define FORMEDITORSCENE_H


#include <QGraphicsScene>
#include <QWeakPointer>
#include <QHash>
#include <qmlitemnode.h>
#include "abstractformeditortool.h"

QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

namespace QmlDesigner {

class FormEditorWidget;
class FormEditorItem;
class FormEditorView;
class LayerItem;

class FormEditorScene : public QGraphicsScene
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

    bool hasItemForQmlItemNode(const QmlItemNode &qmlItemNode) const;

    void synchronizeTransformation(const QmlItemNode &qmlItemNode);
    void synchronizeParent(const QmlItemNode &qmlItemNode);
    void synchronizeOtherProperty(const QmlItemNode &qmlItemNode, const QString &propertyName);
    void synchronizeState(const QmlItemNode &qmlItemNode);

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
    bool event(QEvent *event);
    void dropEvent(QGraphicsSceneDragDropEvent * event);
    void dragEnterEvent(QGraphicsSceneDragDropEvent * event);
    void dragLeaveEvent(QGraphicsSceneDragDropEvent * event);
    void dragMoveEvent(QGraphicsSceneDragDropEvent * event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

    void keyPressEvent(QKeyEvent *keyEvent);
    void keyReleaseEvent(QKeyEvent *keyEvent);

private:
    QList<QGraphicsItem *> removeLayerItems(const QList<QGraphicsItem *> &itemList);

    AbstractFormEditorTool* currentTool() const;
    void removeItemFromHash(FormEditorItem*);

    FormEditorView *m_editorView;
    AbstractFormEditorTool *m_currentTool;
    QHash<QmlItemNode, FormEditorItem*> m_qmlItemNodeItemHash;
    QWeakPointer<LayerItem> m_formLayerItem;
    QWeakPointer<LayerItem> m_manipulatorLayerItem;
    ModelNode m_dragNode;
    bool m_showBoundingRects;
};




}
#endif //FORMEDITORSCENE_H

