// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qt5testnodeinstanceserver.h"

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

#include <QQuickItem>
#include <QQuickView>

#include <private/qquickdesignersupport_p.h>

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
    setupFileUrl(command.fileUrl);

    refreshBindings();
    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::changePropertyValues(const ChangeValuesCommand &command)
{
    bool hasDynamicProperties = false;
    for (const PropertyValueContainer &container : command.valueChanges()) {
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
    for (const PropertyBindingContainer &container : command.bindingChanges) {
        hasDynamicProperties |= container.isDynamic();
        setInstancePropertyBinding(container);
    }

    if (hasDynamicProperties)
        refreshBindings();

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::changeAuxiliaryValues(const ChangeAuxiliaryCommand &command)
{
    for (const PropertyValueContainer &container : command.auxiliaryChanges) {
        setInstanceAuxiliaryData(container);
    }

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::changeIds(const ChangeIdsCommand &command)
{
    for (const IdContainer &container : command.ids) {
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

void Qt5TestNodeInstanceServer::changeSelection(const ChangeSelectionCommand &)
{

}

void Qt5TestNodeInstanceServer::removeInstances(const RemoveInstancesCommand &command)
{
    ServerNodeInstance oldState = activeStateInstance();
    if (activeStateInstance().isValid())
        activeStateInstance().deactivateState();

    for (qint32 instanceId : command.instanceIds()) {
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
    for (const PropertyAbstractContainer &container : command.properties()) {
        hasDynamicProperties |= container.isDynamic();
        resetInstanceProperty(container);
    }

    if (hasDynamicProperties)
        refreshBindings();

    collectItemChangesAndSendChangeCommands();
}

void Qt5TestNodeInstanceServer::reparentInstances(const ReparentInstancesCommand &command)
{
    for (const ReparentContainer &container : command.reparentInstances()) {
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

    for (qint32 instanceId : command.instances()) {
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
    QQuickDesignerSupport::polishItems(quickWindow());

    QSet<ServerNodeInstance> informationChangedInstanceSet;
    QVector<InstancePropertyPair> propertyChangedList;
    QSet<ServerNodeInstance> parentChangedSet;

    if (quickWindow()) {
        for (QQuickItem *item : allItems()) {
            if (item && hasInstanceForObject(item)) {
                ServerNodeInstance instance = instanceForObject(item);

                if (isDirtyRecursiveForNonInstanceItems(item))
                    informationChangedInstanceSet.insert(instance);


                if (QQuickDesignerSupport::isDirty(item, QQuickDesignerSupport::ParentChanged)) {
                    parentChangedSet.insert(instance);
                    informationChangedInstanceSet.insert(instance);
                }
            }
        }

        for (const InstancePropertyPair& property : changedPropertyList()) {
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
            InformationChangedCommand command
                    = createAllInformationChangedCommand(QtHelpers::toList(informationChangedInstanceSet));
            command.sort();
            nodeInstanceClient()->informationChanged(command);
        }
        if (!propertyChangedList.isEmpty()) {
            ValuesChangedCommand command = createValuesChangedCommand(propertyChangedList);
            command.sort();
            nodeInstanceClient()->valuesChanged(command);
        }

        if (!parentChangedSet.isEmpty())
            sendChildrenChangedCommand(QtHelpers::toList(parentChangedSet));
    }
}

void Qt5TestNodeInstanceServer::sendChildrenChangedCommand(const QList<ServerNodeInstance> &childList)
{
    QSet<ServerNodeInstance> parentSet;
    QList<ServerNodeInstance> noParentList;

    for (const ServerNodeInstance &child : childList) {
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

    for (const ServerNodeInstance &parent : std::as_const(parentSet)) {
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
    static QQuickDesignerSupport::DirtyType informationsDirty = QQuickDesignerSupport::DirtyType(QQuickDesignerSupport::TransformUpdateMask
                                                                              | QQuickDesignerSupport::ContentUpdateMask
                                                                              | QQuickDesignerSupport::Visible
                                                                              | QQuickDesignerSupport::ZValue
                                                                              | QQuickDesignerSupport::OpacityValue);

    if (QQuickDesignerSupport::isDirty(item, informationsDirty))
        return true;

    const QList<QQuickItem *> childItems = item->childItems();
    for (QQuickItem *childItem : childItems) {
        if (!hasInstanceForObject(childItem)) {
            if (QQuickDesignerSupport::isDirty(childItem, informationsDirty))
                return true;
            else if (isDirtyRecursiveForNonInstanceItems(childItem))
                return true;
        }
    }

    return false;
}

} // namespace QmlDesigner
