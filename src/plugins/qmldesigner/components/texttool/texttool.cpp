// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texttool.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include "formeditorwidget.h"
#include "itemutilfunctions.h"
#include "formeditoritem.h"

#include "resizehandleitem.h"
#include "textedititem.h"

#include "nodemetainfo.h"
#include "qmlitemnode.h"
#include "richtexteditordialog.h"
#include <qmldesignerplugin.h>

#include <abstractaction.h>
#include <designeractionmanager.h>

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QDebug>
#include <QPair>

namespace QmlDesigner {

TextTool::TextTool()
{
}

TextTool::~TextTool() = default;

void TextTool::clear()
{
    if (textItem()) {
        textItem()->clearFocus();
        textItem()->deleteLater();
    }

    AbstractFormEditorTool::clear();
}

void TextTool::mousePressEvent(const QList<QGraphicsItem*> &itemList,
                                            QGraphicsSceneMouseEvent *event)
{
    event->setPos(textItem()->mapFromScene(event->scenePos()));
    event->setLastPos(textItem()->mapFromScene(event->lastScenePos()));
    scene()->sendEvent(textItem(), event);
    AbstractFormEditorTool::mousePressEvent(itemList, event);
}

void TextTool::mouseMoveEvent(const QList<QGraphicsItem*> & /*itemList*/,
                              QGraphicsSceneMouseEvent *event)
{    event->setPos(textItem()->mapFromScene(event->scenePos()));
     event->setLastPos(textItem()->mapFromScene(event->lastScenePos()));
     scene()->sendEvent(textItem(), event);
}

void TextTool::hoverMoveEvent(const QList<QGraphicsItem*> & /*itemList*/,
                        QGraphicsSceneMouseEvent * event)
{
    event->setPos(textItem()->mapFromScene(event->scenePos()));
    event->setLastPos(textItem()->mapFromScene(event->lastScenePos()));
    scene()->sendEvent(textItem(), event);
}

void TextTool::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->key() == Qt::Key_Escape) {
        textItem()->writeTextToProperty();
        keyEvent->accept();
    } else {
        scene()->sendEvent(textItem(), keyEvent);
    }
}

void TextTool::keyReleaseEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->key() == Qt::Key_Escape) {
        keyEvent->accept();
        view()->changeToSelectionTool();
    } else {
        scene()->sendEvent(textItem(), keyEvent);
    }
}

void  TextTool::dragLeaveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{

}

void  TextTool::dragMoveEvent(const QList<QGraphicsItem*> &/*itemList*/, QGraphicsSceneDragDropEvent * /*event*/)
{

}

void TextTool::mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                                 QGraphicsSceneMouseEvent *event)
{
    if (!itemList.contains(textItem())) {
        textItem()->writeTextToProperty();
        view()->changeToSelectionTool();
    }
    AbstractFormEditorTool::mouseReleaseEvent(itemList, event);
}


void TextTool::mouseDoubleClickEvent(const QList<QGraphicsItem*> & /*itemList*/, QGraphicsSceneMouseEvent *event)
{
    if (textItem() && !textItem()->boundingRect().contains(textItem()->mapFromScene(event->scenePos()))) {
        textItem()->writeTextToProperty();
        view()->changeToSelectionTool();
    } else {
        event->setPos(textItem()->mapFromScene(event->scenePos()));
        event->setLastPos(textItem()->mapFromScene(event->lastScenePos()));
        scene()->sendEvent(textItem(), event);
    }
}

void TextTool::itemsAboutToRemoved(const QList<FormEditorItem*> &removedItemList)
{
    if (textItem() == nullptr)
        return;

    if (removedItemList.contains(textItem()->formEditorItem()))
        view()->changeToSelectionTool();
}

void TextTool::selectedItemsChanged(const QList<FormEditorItem*> &itemList)
{
    if (textItem()) {
        textItem()->writeTextToProperty();
        view()->changeToSelectionTool();
    }
    if (!itemList.isEmpty()) {
        FormEditorItem *formEditorItem = itemList.constFirst();
        auto text = formEditorItem->qmlItemNode().instanceValue("text").toString();
        auto format = formEditorItem->qmlItemNode().instanceValue("format").value<int>();
        if (format == Qt::RichText || Qt::mightBeRichText(text)) {
            RichTextEditorDialog* editorDialog = new RichTextEditorDialog(text);
            editorDialog->setFormEditorItem(formEditorItem);
            editorDialog->show();
            view()->changeToSelectionTool();
        } else {
            m_textItem = new TextEditItem(scene());
            textItem()->setParentItem(scene()->manipulatorLayerItem());
            textItem()->setFormEditorItem(formEditorItem);
            connect(textItem(), &TextEditItem::returnPressed, [this] {
                textItem()->writeTextToProperty();
                view()->changeToSelectionTool();
            });
        }
    } else {
        view()->changeToSelectionTool();
    }
}

void TextTool::instancesCompleted(const QList<FormEditorItem*> & /*itemList*/)
{
}

void  TextTool::instancesParentChanged(const QList<FormEditorItem *> & /*itemList*/)
{
}

void TextTool::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
    using ModelNodePropertyNamePair = QPair<ModelNode, PropertyName>;
    for (const ModelNodePropertyNamePair &propertyPair : propertyList) {
        if (propertyPair.first == textItem()->formEditorItem()->qmlItemNode().modelNode()
                && propertyPair.second == "text")
            textItem()->updateText();
    }
}

void TextTool::formEditorItemsChanged(const QList<FormEditorItem*> & /*itemList*/)
{
}

int TextTool::wantHandleItem(const ModelNode &modelNode) const
{
    if (modelNode.metaInfo().hasProperty("text"))
        return 20;

    return 0;
}

QString TextTool::name() const
{
    return QCoreApplication::translate("TextTool", "Text Tool");
}

void TextTool::focusLost()
{
    if (textItem()) {
        textItem()->writeTextToProperty();
        view()->changeToSelectionTool();
    }
}

TextEditItem *TextTool::textItem() const
{
    return m_textItem.data();
}

}
