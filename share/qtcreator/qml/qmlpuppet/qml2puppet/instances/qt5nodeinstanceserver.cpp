/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qt5nodeinstanceserver.h"


#include <QSGItem>
#include <QSGView>

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
    delete sgView();
    delete m_designerSupport;
    m_designerSupport = 0;
}

QSGView *Qt5NodeInstanceServer::sgView() const
{
    return m_sgView.data();
}

void Qt5NodeInstanceServer::initializeView(const QVector<AddImportContainer> &/*importVector*/)
{
    Q_ASSERT(!sgView());

    m_sgView = new QSGView;
#ifndef Q_OS_MAC
    sgView()->setAttribute(Qt::WA_DontShowOnScreen, true);
#endif
    sgView()->show();
#ifdef Q_OS_MAC
    sgView()->setAttribute(Qt::WA_DontShowOnScreen, true);
#endif
    sgView()->setUpdatesEnabled(false);
}

QDeclarativeView *Qt5NodeInstanceServer::declarativeView() const
{
    return 0;
}

QDeclarativeEngine *Qt5NodeInstanceServer::engine() const
{
    if (sgView())
        return sgView()->engine();

    return 0;
}

void Qt5NodeInstanceServer::resizeCanvasSizeToRootItemSize()
{
}

void Qt5NodeInstanceServer::resetAllItems()
{
    foreach (QSGItem *item, allItems())
        DesignerSupport::resetDirty(item);
}

QList<ServerNodeInstance> Qt5NodeInstanceServer::setupScene(const CreateSceneCommand &command)
{
    setupFileUrl(command.fileUrl());
    setupImports(command.imports());
    setupDummyData(command.fileUrl());

    QList<ServerNodeInstance> instanceList = setupInstances(command);

    sgView()->resize(rootNodeInstance().boundingRect().size().toSize());

    return instanceList;
}

QList<QSGItem*> subItems(QSGItem *parentItem)
{
    QList<QSGItem*> itemList;
    itemList.append(parentItem->childItems());

    foreach (QSGItem *childItem, parentItem->childItems())
        itemList.append(subItems(childItem));

    return itemList;
}

QList<QSGItem*> Qt5NodeInstanceServer::allItems() const
{
    QList<QSGItem*> itemList;

    if (sgView()) {
        itemList.append(sgView()->rootItem());
        itemList.append(subItems(sgView()->rootItem()));
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
