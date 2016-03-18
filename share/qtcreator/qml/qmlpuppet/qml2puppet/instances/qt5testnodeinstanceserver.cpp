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

#include "qt5testnodeinstanceserver.h"

#include <QQuickItem>
#include <QQuickView>

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
#include "changestatecommand.h"
#include "childrenchangedcommand.h"
#include "completecomponentcommand.h"
#include "componentcompletedcommand.h"
#include "createscenecommand.h"
#include "tokencommand.h"
#include "removesharedmemorycommand.h"
#include "changeauxiliarycommand.h"
#include "changenodesourcecommand.h"

#include "dummycontextobject.h"

#include <designersupportdelegate.h>

namespace QmlDesigner {

Qt5TestNodeInstanceServer::Qt5TestNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient)
    : Qt5NodeInstanceServer(nodeInstanceClient)
{
}

void Qt5TestNodeInstanceServer::createInstances(const CreateInstancesCommand &command)
{
    createInstances(command.instances());

    refreshBindings();

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::changeFileUrl(const ChangeFileUrlCommand &command)
{
    setupFileUrl(command.fileUrl());

    refreshBindings();
    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::changePropertyValues(const ChangeValuesCommand &command)
{
    bool hasDynamicProperties = false;
    foreach (const PropertyValueContainer &container, command.valueChanges()) {
        hasDynamicProperties |= container.isDynamic();
        setInstancePropertyVariant(container);
    }

    if (hasDynamicProperties)
        refreshBindings();

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::changePropertyBindings(const ChangeBindingsCommand &command)
{
    bool hasDynamicProperties = false;
    foreach (const PropertyBindingContainer &container, command.bindingChanges()) {
        hasDynamicProperties |= container.isDynamic();
        setInstancePropertyBinding(container);
    }

    if (hasDynamicProperties)
        refreshBindings();

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::changeAuxiliaryValues(const ChangeAuxiliaryCommand &command)
{
    foreach (const PropertyValueContainer &container, command.auxiliaryChanges()) {
        setInstanceAuxiliaryData(container);
    }

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::changeIds(const ChangeIdsCommand &command)
{
    foreach (const IdContainer &container, command.ids()) {
        if (hasInstanceForId(container.instanceId()))
            instanceForId(container.instanceId()).setId(container.id());
    }

    refreshBindings();

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    Qt5NodeInstanceServer::createScene(command);

    stopRenderTimer();

    refreshBindings();

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::clearScene(const ClearSceneCommand &command)
{
    Qt5NodeInstanceServer::clearScene(command);
}

void Qt5TestNodeInstanceServer::removeInstances(const RemoveInstancesCommand &command)
{
    ServerNodeInstance oldState = activeStateInstance();
    if (activeStateInstance().isValid())
        activeStateInstance().deactivateState();

    foreach (qint32 instanceId, command.instanceIds()) {
        removeInstanceRelationsip(instanceId);
    }

    if (oldState.isValid())
        oldState.activateState();

    refreshBindings();

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::removeProperties(const RemovePropertiesCommand &command)
{
    bool hasDynamicProperties = false;
    foreach (const PropertyAbstractContainer &container, command.properties()) {
        hasDynamicProperties |= container.isDynamic();
        resetInstanceProperty(container);
    }

    if (hasDynamicProperties)
        refreshBindings();

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::reparentInstances(const ReparentInstancesCommand &command)
{
    foreach (const ReparentContainer &container, command.reparentInstances()) {
        if (hasInstanceForId(container.instanceId())) {
            ServerNodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid()) {
                instance.reparent(instanceForId(container.oldParentInstanceId()), container.oldParentProperty(), instanceForId(container.newParentInstanceId()), container.newParentProperty());
            }
        }
    }

    refreshBindings();

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::changeState(const ChangeStateCommand &command)
{
    if (hasInstanceForId(command.stateInstanceId())) {
        if (activeStateInstance().isValid())
            activeStateInstance().deactivateState();
        ServerNodeInstance instance = instanceForId(command.stateInstanceId());
        instance.activateState();
    } else {
        if (activeStateInstance().isValid())
            activeStateInstance().deactivateState();
    }

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::completeComponent(const CompleteComponentCommand &command)
{
    QList<ServerNodeInstance> instanceList;

    foreach (qint32 instanceId, command.instances()) {
        if (hasInstanceForId(instanceId)) {
            ServerNodeInstance instance = instanceForId(instanceId);
            instance.doComponentComplete();
            instanceList.append(instance);
        }
    }

    refreshBindings();

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::changeNodeSource(const ChangeNodeSourceCommand &command)
{
    if (hasInstanceForId(command.instanceId())) {
        ServerNodeInstance instance = instanceForId(command.instanceId());
        if (instance.isValid())
            instance.setNodeSource(command.nodeSource());
    }

    refreshBindings();

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::removeSharedMemory(const RemoveSharedMemoryCommand &command)
{
    if (command.typeName() == "Values")
        ValuesChangedCommand::removeSharedMemorys(command.keyNumbers());
}

void QmlDesigner::Qt5TestNodeInstanceServer::collectItemChangesAndSendChangeCommands()
{
    DesignerSupport::polishItems(quickView());

    QSet<ServerNodeInstance> informationChangedInstanceSet;
    QVector<InstancePropertyPair> propertyChangedList;
    QSet<ServerNodeInstance> parentChangedSet;

    if (quickView()) {
        foreach (QQuickItem *item, allItems()) {
            if (item && hasInstanceForObject(item)) {
                ServerNodeInstance instance = instanceForObject(item);

                if (isDirtyRecursiveForNonInstanceItems(item))
                    informationChangedInstanceSet.insert(instance);


                if (DesignerSupport::isDirty(item, DesignerSupport::ParentChanged)) {
                    parentChangedSet.insert(instance);
                    informationChangedInstanceSet.insert(instance);
                }
            }
        }

        foreach (const InstancePropertyPair& property, changedPropertyList()) {
            const ServerNodeInstance instance = property.first;
            if (instance.isValid()) {
                if (property.second.contains("anchors"))
                    informationChangedInstanceSet.insert(instance);

                propertyChangedList.append(property);
            }
        }

        resetAllItems();
        clearChangedPropertyList();

        if (!informationChangedInstanceSet.isEmpty()) {
            InformationChangedCommand command = createAllInformationChangedCommand(informationChangedInstanceSet.toList());
            command.sort();
            nodeInstanceClient()->informationChanged(command);
        }
        if (!propertyChangedList.isEmpty()) {
            ValuesChangedCommand command = createValuesChangedCommand(propertyChangedList);
            command.sort();
            nodeInstanceClient()->valuesChanged(command);
        }

        if (!parentChangedSet.isEmpty())
            sendChildrenChangedCommand(parentChangedSet.toList());
    }
}

void Qt5TestNodeInstanceServer::sendChildrenChangedCommand(const QList<ServerNodeInstance> &childList)
{
    QSet<ServerNodeInstance> parentSet;
    QList<ServerNodeInstance> noParentList;

    foreach (const ServerNodeInstance &child, childList) {
        if (!child.hasParent()) {
            noParentList.append(child);
        } else {
            ServerNodeInstance parent = child.parent();
            if (parent.isValid()) {
                parentSet.insert(parent);
            } else {
                noParentList.append(child);
            }
        }
    }

    foreach (const ServerNodeInstance &parent, parentSet) {
        ChildrenChangedCommand command = createChildrenChangedCommand(parent, parent.childItems());
        command.sort();
        nodeInstanceClient()->childrenChanged(command);
    }

    if (!noParentList.isEmpty()) {
        ChildrenChangedCommand command = createChildrenChangedCommand(ServerNodeInstance(), noParentList);
        command.sort();
        nodeInstanceClient()->childrenChanged(command);
    }
}

bool Qt5TestNodeInstanceServer::isDirtyRecursiveForNonInstanceItems(QQuickItem *item) const
{
    static DesignerSupport::DirtyType informationsDirty = DesignerSupport::DirtyType(DesignerSupport::TransformUpdateMask
                                                                              | DesignerSupport::ContentUpdateMask
                                                                              | DesignerSupport::Visible
                                                                              | DesignerSupport::ZValue
                                                                              | DesignerSupport::OpacityValue);

    if (DesignerSupport::isDirty(item, informationsDirty))
        return true;

    foreach (QQuickItem *childItem, item->childItems()) {
        if (!hasInstanceForObject(childItem)) {
            if (DesignerSupport::isDirty(childItem, informationsDirty))
                return true;
            else if (isDirtyRecursiveForNonInstanceItems(childItem))
                return true;
        }
    }

    return false;
}

} // namespace QmlDesigner
