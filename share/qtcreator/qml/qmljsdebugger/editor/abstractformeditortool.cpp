/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "abstractformeditortool.h"
#include "qdeclarativeviewobserver.h"
#include "../qdeclarativeviewobserver_p.h"

#include <QDeclarativeEngine>

#include <QtDebug>
#include <QGraphicsItem>
#include <QDeclarativeItem>

namespace QmlJSDebugger {

AbstractFormEditorTool::AbstractFormEditorTool(QDeclarativeViewObserver *editorView)
    : QObject(editorView), m_observer(editorView)
{
}


AbstractFormEditorTool::~AbstractFormEditorTool()
{

}

QDeclarativeViewObserver *AbstractFormEditorTool::observer() const
{
    return m_observer;
}

QDeclarativeView *AbstractFormEditorTool::view() const
{
    return m_observer->declarativeView();
}

QGraphicsScene* AbstractFormEditorTool::scene() const
{
    return view()->scene();
}

void AbstractFormEditorTool::updateSelectedItems()
{
    selectedItemsChanged(items());
}

QList<QGraphicsItem*> AbstractFormEditorTool::items() const
{
    return observer()->selectedItems();
}

void AbstractFormEditorTool::enterContext(QGraphicsItem *itemToEnter)
{
    observer()->data->enterContext(itemToEnter);
}

bool AbstractFormEditorTool::topItemIsMovable(const QList<QGraphicsItem*> & itemList)
{
    QGraphicsItem *firstSelectableItem = topMovableGraphicsItem(itemList);
    if (firstSelectableItem == 0)
        return false;

    QDeclarativeItem *declarativeItem
            = dynamic_cast<QDeclarativeItem*>(firstSelectableItem->toGraphicsObject());

    if (declarativeItem != 0)
        return true;

    return false;

}

bool AbstractFormEditorTool::topSelectedItemIsMovable(const QList<QGraphicsItem*> &itemList)
{
    QList<QGraphicsItem*> selectedItems = observer()->selectedItems();

    foreach (QGraphicsItem *item, itemList) {
        QDeclarativeItem *declarativeItem = toQDeclarativeItem(item);
        if (declarativeItem
                && selectedItems.contains(declarativeItem)
                /*&& (declarativeItem->qmlItemNode().hasShowContent() || selectNonContentItems)*/)
            return true;
    }

    return false;

}

bool AbstractFormEditorTool::topItemIsResizeHandle(const QList<QGraphicsItem*> &/*itemList*/)
{
    return false;
}

QDeclarativeItem *AbstractFormEditorTool::toQDeclarativeItem(QGraphicsItem *item)
{
    return dynamic_cast<QDeclarativeItem*>(item->toGraphicsObject());
}

QGraphicsItem *AbstractFormEditorTool::topMovableGraphicsItem(const QList<QGraphicsItem*> &itemList)
{
    foreach (QGraphicsItem *item, itemList) {
        if (item->flags().testFlag(QGraphicsItem::ItemIsMovable))
            return item;
    }
    return 0;
}

QDeclarativeItem *AbstractFormEditorTool::topMovableDeclarativeItem(const QList<QGraphicsItem*>
                                                                    &itemList)
{
    foreach (QGraphicsItem *item, itemList) {
        QDeclarativeItem *declarativeItem = toQDeclarativeItem(item);
        if (declarativeItem /*&& (declarativeItem->qmlItemNode().hasShowContent())*/)
            return declarativeItem;
    }

    return 0;
}

QList<QGraphicsObject*> AbstractFormEditorTool::toGraphicsObjectList(const QList<QGraphicsItem*>
                                                                     &itemList)
{
    QList<QGraphicsObject*> gfxObjects;
    foreach(QGraphicsItem *item, itemList) {
        QGraphicsObject *obj = item->toGraphicsObject();
        if (obj)
            gfxObjects << obj;
    }

    return gfxObjects;
}

QString AbstractFormEditorTool::titleForItem(QGraphicsItem *item)
{
    QString className("QGraphicsItem");
    QString objectStringId;

    QString constructedName;

    QGraphicsObject *gfxObject = item->toGraphicsObject();
    if (gfxObject) {
        className = gfxObject->metaObject()->className();

        className.replace(QRegExp("_QMLTYPE_\\d+"), "");
        className.replace(QRegExp("_QML_\\d+"), "");
        if (className.startsWith(QLatin1String("QDeclarative")))
            className = className.replace(QLatin1String("QDeclarative"), "");

        QDeclarativeItem *declarativeItem = qobject_cast<QDeclarativeItem*>(gfxObject);
        if (declarativeItem) {
            objectStringId = QDeclarativeViewObserver::idStringForObject(declarativeItem);
        }

        if (!objectStringId.isEmpty()) {
            constructedName = objectStringId + " (" + className + ")";
        } else {
            if (!gfxObject->objectName().isEmpty()) {
                constructedName = gfxObject->objectName() + " (" + className + ")";
            } else {
                constructedName = className;
            }
        }
    }

    return constructedName;
}


}
