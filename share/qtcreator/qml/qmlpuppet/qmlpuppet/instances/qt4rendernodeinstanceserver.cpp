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

#include "qt4rendernodeinstanceserver.h"

#include <QGraphicsItem>
#include <private/qgraphicsitem_p.h>
#include <private/qgraphicsscene_p.h>
#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QFileSystemWatcher>
#include <QUrl>
#include <QSet>
#include <QDir>
#include <QVariant>
#include <QMetaType>
#include <QDeclarativeComponent>
#include <QDeclarativeContext>
#include <private/qlistmodelinterface_p.h>
#include <QAbstractAnimation>
#include <private/qabstractanimation_p.h>

#include "servernodeinstance.h"
#include "childrenchangeeventfilter.h"
#include "propertyabstractcontainer.h"
#include "propertybindingcontainer.h"
#include "propertyvaluecontainer.h"
#include "instancecontainer.h"
#include "createinstancescommand.h"
#include "changefileurlcommand.h"
#include "clearscenecommand.h"
#include "reparentinstancescommand.h"
#include "changevaluescommand.h"
#include "changebindingscommand.h"
#include "changeidscommand.h"
#include "removeinstancescommand.h"
#include "nodeinstanceclientinterface.h"
#include "removepropertiescommand.h"
#include "valueschangedcommand.h"
#include "informationchangedcommand.h"
#include "pixmapchangedcommand.h"
#include "commondefines.h"
#include "childrenchangeeventfilter.h"
#include "changestatecommand.h"
#include "childrenchangedcommand.h"
#include "completecomponentcommand.h"
#include "componentcompletedcommand.h"
#include "createscenecommand.h"
#include "removesharedmemorycommand.h"

#include "dummycontextobject.h"

namespace QmlDesigner {

Qt4RenderNodeInstanceServer::Qt4RenderNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    Qt4NodeInstanceServer(nodeInstanceClient)
{
}

void Qt4RenderNodeInstanceServer::collectItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;
    if (!inFunction) {
        inFunction = true;

        bool adjustSceneRect = false;

        if (declarativeView() && nodeInstanceClient()->bytesToWrite() < 10000) {
            foreach (QGraphicsItem *item, declarativeView()->items()) {
                QGraphicsObject *graphicsObject = item->toGraphicsObject();
                if (graphicsObject && hasInstanceForObject(graphicsObject)) {
                    ServerNodeInstance instance = instanceForObject(graphicsObject);
                    QGraphicsItemPrivate *d = QGraphicsItemPrivate::get(item);

                    if ((d->dirty && d->notifyBoundingRectChanged) || (d->dirty && !d->dirtySceneTransform) || nonInstanceChildIsDirty(graphicsObject))
                        m_dirtyInstanceSet.insert(instance);

                    if (d->geometryChanged) {
                        if (instance.isRootNodeInstance())
                            declarativeView()->scene()->setSceneRect(item->boundingRect());
                    }

                }
            }

            foreach (QGraphicsItem *item, declarativeView()->items()) {
                QGraphicsItemPrivate *d = QGraphicsItemPrivate::get(item);
                d->ensureSceneTransform();
            }

            foreach (const InstancePropertyPair& property, changedPropertyList()) {
                const ServerNodeInstance instance = property.first;
                const QString propertyName = property.second;

                if (instance.isRootNodeInstance() && (propertyName == "width" || propertyName == "height"))
                    adjustSceneRect = true;

                if (propertyName == "width" || propertyName == "height")
                    m_dirtyInstanceSet.insert(instance);
            }

            clearChangedPropertyList();
            resetAllItems();

            if (!m_dirtyInstanceSet.isEmpty() ) {
                nodeInstanceClient()->pixmapChanged(createPixmapChangedCommand(m_dirtyInstanceSet.toList()));
                m_dirtyInstanceSet.clear();
            }

            if (adjustSceneRect) {
                QRectF boundingRect = rootNodeInstance().boundingRect();
                if (boundingRect.isValid()) {
                    declarativeView()->setSceneRect(boundingRect);
                }
            }

            slowDownRenderTimer();
            nodeInstanceClient()->flush();
            nodeInstanceClient()->synchronizeWithClientProcess();
        }

        inFunction = false;
    }
}

void Qt4RenderNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    Qt4NodeInstanceServer::createScene(command);

    QList<ServerNodeInstance> instanceList;
    foreach(const InstanceContainer &container, command.instances()) {
        ServerNodeInstance instance = instanceForId(container.instanceId());
        if (instance.isValid()) {
            instanceList.append(instance);
        }
    }

    nodeInstanceClient()->pixmapChanged(createPixmapChangedCommand(instanceList));
}

void Qt4RenderNodeInstanceServer::clearScene(const ClearSceneCommand &command)
{
    Qt4NodeInstanceServer::clearScene(command);

    m_dirtyInstanceSet.clear();
}

void Qt4RenderNodeInstanceServer::completeComponent(const CompleteComponentCommand &command)
{
    Qt4NodeInstanceServer::completeComponent(command);

    QList<ServerNodeInstance> instanceList;
    foreach(qint32 instanceId, command.instances()) {
        ServerNodeInstance instance = instanceForId(instanceId);
        if (instance.isValid()) {
            instanceList.append(instance);
        }
    }

    nodeInstanceClient()->pixmapChanged(createPixmapChangedCommand(instanceList));
}
void Qt4RenderNodeInstanceServer::changeState(const ChangeStateCommand &command)
{
    Qt4NodeInstanceServer::changeState(command);

    foreach (QGraphicsItem *item, declarativeView()->items()) {
        item->update();
        QGraphicsItemPrivate::get(item)->notifyBoundingRectChanged = 1;
    }
}

void Qt4RenderNodeInstanceServer::removeSharedMemory(const RemoveSharedMemoryCommand &command)
{
    if (command.typeName() == "Image")
        ImageContainer::removeSharedMemorys(command.keyNumbers());
}
} // namespace QmlDesigner
