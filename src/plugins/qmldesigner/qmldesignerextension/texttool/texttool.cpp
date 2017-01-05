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
#include <qmldesignerplugin.h>

#include <abstractaction.h>
#include <designeractionmanager.h>

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QDebug>
#include <QPair>

namespace QmlDesigner {

class TextToolAction : public AbstractAction
{
public:
    TextToolAction() : AbstractAction(QCoreApplication::translate("TextToolAction","Edit Text")) {}

    QByteArray category() const
    {
        return QByteArray();
    }

    QByteArray menuId() const
    {
        return "TextTool";
    }

    int priority() const
    {
        return CustomActionsPriority;
    }

    Type type() const
    {
        return ContextMenuAction;
    }

protected:
    bool isVisible(const SelectionContext &selectionContext) const
    {
        if (selectionContext.scenePosition().isNull())
            return false;

        if (selectionContext.singleNodeIsSelected())
            return selectionContext.currentSingleSelectedNode().metaInfo().hasProperty("text");

        return false;
    }

    bool isEnabled(const SelectionContext &selectionContext) const
    {
        return isVisible(selectionContext);
    }
};

TextTool::TextTool()
    : QObject() , AbstractCustomTool()
{
    TextToolAction *textToolAction = new TextToolAction;
    QmlDesignerPlugin::instance()->designerActionManager().addDesignerAction(textToolAction);
    connect(textToolAction->action(), &QAction::triggered, [=]() {
        view()->changeCurrentToolTo(this);
    });
}

TextTool::~TextTool()
{
}

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
    if (textItem() == 0)
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
        FormEditorItem *formEditorItem = itemList.first();
        m_textItem = new TextEditItem(scene());
        textItem()->setParentItem(scene()->manipulatorLayerItem());
        textItem()->setFormEditorItem(formEditorItem);
        connect(textItem(), &TextEditItem::returnPressed, [this] {
            textItem()->writeTextToProperty();
            view()->changeToSelectionTool();
        });
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
    typedef QPair<ModelNode, PropertyName> ModelNodePropertyNamePair;
    foreach (const ModelNodePropertyNamePair &propertyPair, propertyList) {
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
