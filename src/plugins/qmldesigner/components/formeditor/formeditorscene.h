/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef FORMEDITORSCENE_H
#define FORMEDITORSCENE_H


#include <QGraphicsScene>
#include <QWeakPointer>
#include <QHash>
#include <qmlitemnode.h>
#include "abstractformeditortool.h"


class QGraphicsSceneMouseEvent;

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
    enum PaintMode {
        NormalMode,
        AnchorMode
    };

    FormEditorScene(FormEditorWidget *widget, FormEditorView *editorView);
    ~FormEditorScene();
    FormEditorItem *addFormEditorItem(const QmlItemNode &qmlItemNode);

    FormEditorItem* itemForQmlItemNode(const QmlItemNode &qmlItemNode) const;

    QList<FormEditorItem*> itemsForQmlItemNodes(const QList<QmlItemNode> &nodeList) const;
    QList<FormEditorItem*> allFormEditorItems() const;

    void updateAllFormEditorItems();

    bool hasItemForQmlItemNode(const QmlItemNode &qmlItemNode) const;

    void synchronizeTransformation(const QmlItemNode &qmlItemNode);
    void synchronizeParent(const QmlItemNode &qmlItemNode);
    void synchronizeOtherProperty(const QmlItemNode &qmlItemNode);
    void synchronizeState(const QmlItemNode &qmlItemNode);

    FormEditorItem* calulateNewParent(FormEditorItem *widget);
    bool event(QEvent *event);
    LayerItem* manipulatorLayerItem() const;
    LayerItem* formLayerItem() const;
    FormEditorView *editorView() const;

    FormEditorItem *rootFormEditorItem() const;

    void reparentItem(const QmlItemNode &node, const QmlItemNode &newParent);

    PaintMode paintMode() const;
    void setPaintMode(PaintMode paintMode);

    void clearFormEditorItems();

public slots:
    void setShowBoundingRects(bool show);
    bool showBoundingRects() const;

protected:
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
    PaintMode m_paintMode;
    bool m_showBoundingRects;
};




}
#endif //FORMEDITORSCENE_H

