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

#include "itemcreatortool.h"
#include "formeditorscene.h"
#include "formeditorview.h"

#include <metainfo.h>
#include <itemlibrary.h>

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QtDebug>
#include <QClipboard>

namespace QmlDesigner {

ItemCreatorTool::ItemCreatorTool(FormEditorView *editorView)
    : AbstractFormEditorTool(editorView),
    m_rubberbandSelectionManipulator(editorView->scene()->manipulatorLayerItem(), editorView)

{

}


ItemCreatorTool::~ItemCreatorTool()
{
}

void ItemCreatorTool::mousePressEvent(const QList<QGraphicsItem*> &/*itemList*/,
                                    QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_rubberbandSelectionManipulator.begin(event->scenePos());
    else
        view()->deActivateItemCreator();
}

void ItemCreatorTool::mouseMoveEvent(const QList<QGraphicsItem*> &/*itemList*/,
                                   QGraphicsSceneMouseEvent *event)
{
    view()->setCursor(Qt::CrossCursor);
    if (m_rubberbandSelectionManipulator.isActive()) {
        m_rubberbandSelectionManipulator.update(event->scenePos());
    } else {

    }
}

void ItemCreatorTool::hoverMoveEvent(const QList<QGraphicsItem*> &/*itemList*/,
                        QGraphicsSceneMouseEvent * /*event*/)
{
}

void ItemCreatorTool::mouseReleaseEvent(const QList<QGraphicsItem*> &/*itemList*/,
                                      QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_rubberbandSelectionManipulator.isActive()) {
            QPointF mouseMovementVector = m_rubberbandSelectionManipulator.beginPoint() - event->scenePos();
            if (mouseMovementVector.toPoint().manhattanLength() < QApplication::startDragDistance()) {
                m_rubberbandSelectionManipulator.update(event->scenePos());
            } else {
                m_rubberbandSelectionManipulator.update(event->scenePos());
                QRectF rect(m_rubberbandSelectionManipulator.beginPoint(), event->scenePos());
                createAtItem(rect);
                m_rubberbandSelectionManipulator.end();
                view()->deActivateItemCreator();
            }
        }
    } else {
        view()->deActivateItemCreator();
    }
}

void ItemCreatorTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> &/*itemList*/,
                                          QGraphicsSceneMouseEvent * /*event*/)
{

}

void ItemCreatorTool::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()) {
        case Qt::Key_Escape:
            view()->deActivateItemCreator();
            break;
    }
}

void ItemCreatorTool::keyReleaseEvent(QKeyEvent * /*keyEvent*/)
{

}

void ItemCreatorTool::itemsAboutToRemoved(const QList<FormEditorItem*> &/*itemList*/)
{

}

void ItemCreatorTool::clear()
{
    m_rubberbandSelectionManipulator.clear();
}

void ItemCreatorTool::selectedItemsChanged(const QList<FormEditorItem*> &/*itemList*/)
{
}

void ItemCreatorTool::formEditorItemsChanged(const QList<FormEditorItem*> &/*itemList*/)
{
}

void ItemCreatorTool::setItemString(const QString &itemString)
{
    m_itemString = itemString;
}

FormEditorItem* ItemCreatorTool::calculateContainer(const QPointF &point)
{
    QList<QGraphicsItem *> list = scene()->items(point);
    foreach (QGraphicsItem *item, list) {
         FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
         if (formEditorItem && formEditorItem->isContainer())
             return formEditorItem;
    }
    return 0;
}

void ItemCreatorTool::createAtItem(const QRectF &rect)
{
    QPointF pos = rect.topLeft();

    QmlItemNode parentNode = view()->rootQmlItemNode();
    FormEditorItem *parentItem = calculateContainer(pos);
    if (parentItem) {
        parentNode = parentItem->qmlItemNode();
        pos = parentItem->mapFromScene(pos);
    }

    QStringList list = m_itemString.split(QLatin1Char('^'));
    if (list.count() != 2)
        return;
    if (list.first() == "item") {
        RewriterTransaction transaction = view()->beginRewriterTransaction();
        ItemLibraryInfo itemLibraryRepresentation = view()->model()->metaInfo().itemLibraryRepresentation(list.at(1));
        QmlItemNode newNode = view()->createQmlItemNode(itemLibraryRepresentation, pos, parentNode);
        newNode.modelNode().variantProperty("width") = rect.width();
        newNode.modelNode().variantProperty("height") = rect.height();
        QList<QmlItemNode> nodeList;
        nodeList.append(newNode);
        view()->setSelectedQmlItemNodes(nodeList);
    } else if (list.first() == "image") {
        RewriterTransaction transaction = view()->beginRewriterTransaction();
        QmlItemNode newNode = view()->createQmlItemNodeFromImage(list.at(1), pos, parentNode);
        newNode.modelNode().variantProperty("width") = rect.width();
        newNode.modelNode().variantProperty("height") = rect.height();
        QList<QmlItemNode> nodeList;
        nodeList.append(newNode);
        view()->setSelectedQmlItemNodes(nodeList);
    }
}

} //QmlDesigner
