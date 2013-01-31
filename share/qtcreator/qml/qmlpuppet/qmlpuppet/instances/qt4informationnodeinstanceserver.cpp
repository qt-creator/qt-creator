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

#include "qt4informationnodeinstanceserver.h"

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
#include "tokencommand.h"
#include "removesharedmemorycommand.h"
#include "dummycontextobject.h"

namespace QmlDesigner {

Qt4InformationNodeInstanceServer::Qt4InformationNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    Qt4NodeInstanceServer(nodeInstanceClient)
{
}

void Qt4InformationNodeInstanceServer::sendTokenBack()
{
    foreach (const TokenCommand &command, m_tokenList)
        nodeInstanceClient()->token(command);

    m_tokenList.clear();
}

void Qt4InformationNodeInstanceServer::token(const TokenCommand &command)
{
    m_tokenList.append(command);
    startRenderTimer();
}

void Qt4InformationNodeInstanceServer::removeSharedMemory(const RemoveSharedMemoryCommand &command)
{
    if (command.typeName() == "Values")
        ValuesChangedCommand::removeSharedMemorys(command.keyNumbers());
}

void Qt4InformationNodeInstanceServer::collectItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;
    if (!inFunction) {
        inFunction = true;

        QSet<ServerNodeInstance> informationChangedInstanceSet;
        QVector<InstancePropertyPair> propertyChangedList;
        bool adjustSceneRect = false;

        if (declarativeView()) {
            foreach (QGraphicsItem *item, declarativeView()->items()) {
                QGraphicsObject *graphicsObject = item->toGraphicsObject();
                if (graphicsObject && hasInstanceForObject(graphicsObject)) {
                    ServerNodeInstance instance = instanceForObject(graphicsObject);
                    QGraphicsItemPrivate *d = QGraphicsItemPrivate::get(item);

                    if (d->dirtySceneTransform || d->geometryChanged || d->dirty)
                        informationChangedInstanceSet.insert(instance);

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

                if (instance.isValid()) {
                    if (instance.isRootNodeInstance() && (propertyName == "width" || propertyName == "height"))
                        adjustSceneRect = true;

                    if (propertyName.contains("anchors"))
                        informationChangedInstanceSet.insert(instance);

                    if (propertyName == "parent") {
                        informationChangedInstanceSet.insert(instance);
                        m_parentChangedSet.insert(instance);
                    }

                    propertyChangedList.append(property);
                }
            }

            informationChangedInstanceSet.subtract(m_parentChangedSet);

            clearChangedPropertyList();
            resetAllItems();

            sendTokenBack();

            if (!informationChangedInstanceSet.isEmpty())
                nodeInstanceClient()->informationChanged(createAllInformationChangedCommand(informationChangedInstanceSet.toList()));

            if (!m_parentChangedSet.isEmpty()) {
                sendChildrenChangedCommand(m_parentChangedSet.toList());
                m_parentChangedSet.clear();
            }

            if (!propertyChangedList.isEmpty())
                nodeInstanceClient()->valuesChanged(createValuesChangedCommand(propertyChangedList));

            if (adjustSceneRect) {
                QRectF boundingRect = rootNodeInstance().boundingRect();
                if (boundingRect.isValid()) {
                    declarativeView()->setSceneRect(boundingRect);
                }
            }

            if (!m_completedComponentList.isEmpty()) {
                nodeInstanceClient()->componentCompleted(createComponentCompletedCommand(m_completedComponentList));
                m_completedComponentList.clear();
            }

            slowDownRenderTimer();
            nodeInstanceClient()->flush();
            nodeInstanceClient()->synchronizeWithClientProcess();
        }

        inFunction = false;
    }

}

void Qt4InformationNodeInstanceServer::reparentInstances(const ReparentInstancesCommand &command)
{
    foreach(const ReparentContainer &container, command.reparentInstances()) {
        ServerNodeInstance instance = instanceForId(container.instanceId());
        if (instance.isValid()) {
            m_parentChangedSet.insert(instance);
        }
    }

    Qt4NodeInstanceServer::reparentInstances(command);
}

void Qt4InformationNodeInstanceServer::clearScene(const ClearSceneCommand &command)
{
    NodeInstanceServer::clearScene(command);

    m_parentChangedSet.clear();
    m_completedComponentList.clear();
}

void Qt4InformationNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    Qt4NodeInstanceServer::createScene(command);

    QList<ServerNodeInstance> instanceList;
    foreach(const InstanceContainer &container, command.instances()) {
        ServerNodeInstance instance = instanceForId(container.instanceId());
        if (instance.isValid()) {
            instanceList.append(instance);
        }
    }

    nodeInstanceClient()->informationChanged(createAllInformationChangedCommand(instanceList, true));
    nodeInstanceClient()->valuesChanged(createValuesChangedCommand(instanceList));
    sendChildrenChangedCommand(instanceList);
    nodeInstanceClient()->componentCompleted(createComponentCompletedCommand(instanceList));

}

void Qt4InformationNodeInstanceServer::sendChildrenChangedCommand(const QList<ServerNodeInstance> childList)
{
    QSet<ServerNodeInstance> parentSet;
    QList<ServerNodeInstance> noParentList;

    foreach (const ServerNodeInstance &child, childList) {
        if (!child.hasParent())
            noParentList.append(child);
        else
            parentSet.insert(child.parent());
    }


    foreach (const ServerNodeInstance &parent, parentSet)
        nodeInstanceClient()->childrenChanged(createChildrenChangedCommand(parent, parent.childItems()));

    if (!noParentList.isEmpty())
        nodeInstanceClient()->childrenChanged(createChildrenChangedCommand(ServerNodeInstance(), noParentList));

}

void Qt4InformationNodeInstanceServer::completeComponent(const CompleteComponentCommand &command)
{
    Qt4NodeInstanceServer::completeComponent(command);

    QList<ServerNodeInstance> instanceList;
    foreach(qint32 instanceId, command.instances()) {
        ServerNodeInstance instance = instanceForId(instanceId);
        if (instance.isValid()) {
            instanceList.append(instance);
        }
    }

    m_completedComponentList.append(instanceList);

    nodeInstanceClient()->valuesChanged(createValuesChangedCommand(instanceList));
    nodeInstanceClient()->informationChanged(createAllInformationChangedCommand(instanceList, true));
}

} // namespace QmlDesigner
