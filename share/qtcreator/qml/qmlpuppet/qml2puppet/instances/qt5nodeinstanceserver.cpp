/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "qt5nodeinstanceserver.h"


#include <QQuickItem>
#include <QQuickView>

#include <designersupport.h>
#include <addimportcontainer.h>
#include <createscenecommand.h>

namespace QmlDesigner {

Qt5NodeInstanceServer::Qt5NodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient)
    : NodeInstanceServer(nodeInstanceClient),
      m_designerSupport(new DesignerSupport)
{
    addImportString("import QtQuick 2.0\n");
}

Qt5NodeInstanceServer::~Qt5NodeInstanceServer()
{
    delete quickView();
    delete m_designerSupport;
    m_designerSupport = 0;
}

QQuickView *Qt5NodeInstanceServer::quickView() const
{
    return m_quickView.data();
}

void Qt5NodeInstanceServer::initializeView(const QVector<AddImportContainer> &/*importVector*/)
{
    Q_ASSERT(!quickView());

    m_quickView = new QQuickView;
}

QQmlView *Qt5NodeInstanceServer::declarativeView() const
{
    return 0;
}

QQmlEngine *Qt5NodeInstanceServer::engine() const
{
    if (quickView())
        return quickView()->engine();

    return 0;
}

void Qt5NodeInstanceServer::resizeCanvasSizeToRootItemSize()
{
}

void Qt5NodeInstanceServer::resetAllItems()
{
    foreach (QQuickItem *item, allItems())
        DesignerSupport::resetDirty(item);
}

QList<ServerNodeInstance> Qt5NodeInstanceServer::setupScene(const CreateSceneCommand &command)
{
    setupFileUrl(command.fileUrl());
    setupImports(command.imports());
    setupDummyData(command.fileUrl());

    QList<ServerNodeInstance> instanceList = setupInstances(command);

    quickView()->resize(rootNodeInstance().boundingRect().size().toSize());

    return instanceList;
}

QList<QQuickItem*> subItems(QQuickItem *parentItem)
{
    QList<QQuickItem*> itemList;
    itemList.append(parentItem->childItems());

    foreach (QQuickItem *childItem, parentItem->childItems())
        itemList.append(subItems(childItem));

    return itemList;
}

QList<QQuickItem*> Qt5NodeInstanceServer::allItems() const
{
    QList<QQuickItem*> itemList;

    if (quickView()) {
        itemList.append(quickView()->rootItem());
        itemList.append(subItems(quickView()->rootItem()));
    }

    return itemList;
}

void Qt5NodeInstanceServer::refreshBindings()
{
    DesignerSupport::refreshExpressions(context());
}

DesignerSupport *Qt5NodeInstanceServer::designerSupport() const
{
    return m_designerSupport;
}

void Qt5NodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    m_designerSupport = new DesignerSupport;
    NodeInstanceServer::createScene(command);
}

void Qt5NodeInstanceServer::clearScene(const ClearSceneCommand &command)
{
    NodeInstanceServer::clearScene(command);
    delete m_designerSupport;
    m_designerSupport = 0;
}

} // QmlDesigner
