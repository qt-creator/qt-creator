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

#include "abstractliveedittool.h"
#include "qdeclarativeviewinspector.h"
#include "../qdeclarativeviewinspector_p.h"

#include <QDeclarativeEngine>

#include <QDebug>
#include <QGraphicsItem>
#include <QDeclarativeItem>

namespace QmlJSDebugger {

AbstractLiveEditTool::AbstractLiveEditTool(QDeclarativeViewInspector *editorView)
    : QObject(editorView), m_inspector(editorView)
{
}


AbstractLiveEditTool::~AbstractLiveEditTool()
{
}

QDeclarativeViewInspector *AbstractLiveEditTool::inspector() const
{
    return m_inspector;
}

QDeclarativeView *AbstractLiveEditTool::view() const
{
    return m_inspector->declarativeView();
}

QGraphicsScene* AbstractLiveEditTool::scene() const
{
    return view()->scene();
}

void AbstractLiveEditTool::updateSelectedItems()
{
    selectedItemsChanged(items());
}

QList<QGraphicsItem*> AbstractLiveEditTool::items() const
{
    return inspector()->selectedItems();
}

bool AbstractLiveEditTool::topItemIsMovable(const QList<QGraphicsItem*> & itemList)
{
    QGraphicsItem *firstSelectableItem = topMovableGraphicsItem(itemList);
    if (firstSelectableItem == 0)
        return false;
    if (toQDeclarativeItem(firstSelectableItem) != 0)
        return true;

    return false;

}

bool AbstractLiveEditTool::topSelectedItemIsMovable(const QList<QGraphicsItem*> &itemList)
{
    QList<QGraphicsItem*> selectedItems = inspector()->selectedItems();

    foreach (QGraphicsItem *item, itemList) {
        QDeclarativeItem *declarativeItem = toQDeclarativeItem(item);
        if (declarativeItem
                && selectedItems.contains(declarativeItem)
                /*&& (declarativeItem->qmlItemNode().hasShowContent() || selectNonContentItems)*/)
            return true;
    }

    return false;

}

bool AbstractLiveEditTool::topItemIsResizeHandle(const QList<QGraphicsItem*> &/*itemList*/)
{
    return false;
}

QDeclarativeItem *AbstractLiveEditTool::toQDeclarativeItem(QGraphicsItem *item)
{
    return qobject_cast<QDeclarativeItem*>(item->toGraphicsObject());
}

QGraphicsItem *AbstractLiveEditTool::topMovableGraphicsItem(const QList<QGraphicsItem*> &itemList)
{
    foreach (QGraphicsItem *item, itemList) {
        if (item->flags().testFlag(QGraphicsItem::ItemIsMovable))
            return item;
    }
    return 0;
}

QDeclarativeItem *AbstractLiveEditTool::topMovableDeclarativeItem(const QList<QGraphicsItem*>
                                                                  &itemList)
{
    foreach (QGraphicsItem *item, itemList) {
        QDeclarativeItem *declarativeItem = toQDeclarativeItem(item);
        if (declarativeItem /*&& (declarativeItem->qmlItemNode().hasShowContent())*/)
            return declarativeItem;
    }

    return 0;
}

QList<QGraphicsObject*> AbstractLiveEditTool::toGraphicsObjectList(const QList<QGraphicsItem*>
                                                                   &itemList)
{
    QList<QGraphicsObject*> gfxObjects;
    foreach (QGraphicsItem *item, itemList) {
        QGraphicsObject *obj = item->toGraphicsObject();
        if (obj)
            gfxObjects << obj;
    }

    return gfxObjects;
}

QString AbstractLiveEditTool::titleForItem(QGraphicsItem *item)
{
    QString className("QGraphicsItem");
    QString objectStringId;

    QString constructedName;

    QGraphicsObject *gfxObject = item->toGraphicsObject();
    if (gfxObject) {
        className = gfxObject->metaObject()->className();

        className.remove(QRegExp("_QMLTYPE_\\d+"));
        className.remove(QRegExp("_QML_\\d+"));
        if (className.startsWith(QLatin1String("QDeclarative")))
            className = className.remove(QLatin1String("QDeclarative"));

        QDeclarativeItem *declarativeItem = qobject_cast<QDeclarativeItem*>(gfxObject);
        if (declarativeItem) {
            objectStringId = QDeclarativeViewInspector::idStringForObject(declarativeItem);
        }

        if (!objectStringId.isEmpty()) {
            constructedName = objectStringId + " (" + className + QLatin1Char(')');
        } else {
            if (!gfxObject->objectName().isEmpty()) {
                constructedName = gfxObject->objectName() + " (" + className + QLatin1Char(')');
            } else {
                constructedName = className;
            }
        }
    }

    return constructedName;
}


} // namespace QmlJSDebugger
