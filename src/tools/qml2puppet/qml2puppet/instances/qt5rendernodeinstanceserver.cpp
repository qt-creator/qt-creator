// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qt5rendernodeinstanceserver.h"

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
#include "quickitemnodeinstance.h"
#include "removesharedmemorycommand.h"

#include "dummycontextobject.h"

#include <qmlprivategate.h>

#include <private/qquickdesignersupport_p.h>

namespace QmlDesigner {

Qt5RenderNodeInstanceServer::Qt5RenderNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    Qt5NodeInstanceServer(nodeInstanceClient)
{
    Internal::QuickItemNodeInstance::createEffectItem(true);
}

void Qt5RenderNodeInstanceServer::collectItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;
    if (!inFunction) {
        inFunction = true;

        QQuickDesignerSupport::polishItems(quickWindow());

        if (quickWindow() && nodeInstanceClient()->bytesToWrite() < 10000) {
            bool windowDirty = false;
            bool hasView3D = false;
            QList<ServerNodeInstance> effectItems;
            for (QQuickItem *item : allItems()) {
                if (item) {
                    if (Internal::QuickItemNodeInstance::unifiedRenderPath()) {
                        if (QQuickDesignerSupport::isDirty(item, QQuickDesignerSupport::AllMask)) {
                            windowDirty = true;
                            break;
                        }
                    } else {
                        if (hasInstanceForObject(item)) {
                            if (QQuickDesignerSupport::isDirty(item, QQuickDesignerSupport::ContentUpdateMask)) {
                                if (!hasView3D && ServerNodeInstance::isSubclassOf(
                                            item, QByteArrayLiteral("QQuick3DViewport"))) {
                                    hasView3D = true;
                                }
                                m_dirtyInstanceSet.insert(instanceForObject(item));
                            }
                            if (hasEffect(item))
                                effectItems.append(instanceForObject(item));
                            if (QQuickItem *effectParent = parentEffectItem(item)) {
                                if ((QQuickDesignerSupport::isDirty(
                                        item,
                                        QQuickDesignerSupport::DirtyType(
                                            QQuickDesignerSupport::TransformUpdateMask
                                            | QQuickDesignerSupport::Visible
                                            | QQuickDesignerSupport::ContentUpdateMask)))
                                    && hasInstanceForObject(effectParent)) {
                                    m_dirtyInstanceSet.insert(instanceForObject(effectParent));
                                }
                            }
                        } else if (DesignerSupport::isDirty(
                                item,
                                DesignerSupport::DirtyType(
                                    DesignerSupport::AllMask
                                    | DesignerSupport::ZValue
                                    | DesignerSupport::OpacityValue
                                    | DesignerSupport::Visible))) {
                            ServerNodeInstance ancestorInstance = findNodeInstanceForItem(
                                item->parentItem());
                            if (ancestorInstance.isValid())
                                m_dirtyInstanceSet.insert(ancestorInstance);
                        }
                        Internal::QuickItemNodeInstance::updateDirtyNode(item);
                    }
                }
            }

            if (Internal::QuickItemNodeInstance::unifiedRenderPath()) {
                if (windowDirty)
                    nodeInstanceClient()->pixmapChanged(createPixmapChangedCommand({rootNodeInstance()}));
            } else {
                if (!m_dirtyInstanceSet.isEmpty()) {
                    auto renderList = QtHelpers::toList(m_dirtyInstanceSet);
                    for (auto &effectItem : std::as_const(effectItems)) {
                        // Ensure effect items are rendered last
                        if (m_dirtyInstanceSet.contains(effectItem))
                            renderList.removeOne(effectItem);
                        renderList.append(effectItem);
                    }

                    // If there is a View3D to be rendered, add all other View3Ds to be rendered
                    // as well, in case they share materials.
                    if (hasView3D) {
                        const QList<ServerNodeInstance> view3Ds = allView3DInstances();
                        for (auto &view3D : view3Ds) {
                            if (!m_dirtyInstanceSet.contains(view3D))
                                renderList.append(view3D);
                        }
                    }

                    nodeInstanceClient()->pixmapChanged(createPixmapChangedCommand(renderList));
                }
            }

            m_dirtyInstanceSet.clear();

            resetAllItems();

            slowDownRenderTimer();
            nodeInstanceClient()->flush();
            nodeInstanceClient()->synchronizeWithClientProcess();
        }

        if (rootIsRenderable3DObject() && rootNodeInstance().contentItem()
            && !changedPropertyList().isEmpty()
            && nodeInstanceClient()->bytesToWrite() < 10000) {

            QQuickItem *rootItem = rootNodeInstance().contentItem();
            QQuickDesignerSupport::addDirty(rootItem, QQuickDesignerSupport::Content);
            QQuickDesignerSupport::updateDirtyNode(rootItem);
            nodeInstanceClient()->pixmapChanged(createPixmapChangedCommand({rootNodeInstance()}));
        }

        clearChangedPropertyList();

        inFunction = false;
    }
}

ServerNodeInstance Qt5RenderNodeInstanceServer::findNodeInstanceForItem(QQuickItem *item) const
{
    if (item) {
        if (hasInstanceForObject(item))
            return instanceForObject(item);
        else if (item->parentItem())
            return findNodeInstanceForItem(item->parentItem());
    }

    return ServerNodeInstance();
}

void Qt5RenderNodeInstanceServer::resizeCanvasToRootItem()
{
    Qt5NodeInstanceServer::resizeCanvasToRootItem();
    m_dirtyInstanceSet.insert(rootNodeInstance());
}


void Qt5RenderNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    Qt5NodeInstanceServer::createScene(command);

    QList<ServerNodeInstance> instanceList;
    for (const InstanceContainer &container : std::as_const(command.instances)) {
        if (hasInstanceForId(container.instanceId())) {
            ServerNodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid()) {
                instanceList.append(instance);
            }
        }
    }

    nodeInstanceClient()->pixmapChanged(createPixmapChangedCommand(instanceList));
}

void Qt5RenderNodeInstanceServer::clearScene(const ClearSceneCommand &command)
{
    Qt5NodeInstanceServer::clearScene(command);

    m_dirtyInstanceSet.clear();
}

void Qt5RenderNodeInstanceServer::completeComponent(const CompleteComponentCommand &command)
{
    Qt5NodeInstanceServer::completeComponent(command);

    const QVector<qint32> ids = command.instances();
    for (qint32 instanceId : ids) {
        if (hasInstanceForId(instanceId)) {
            ServerNodeInstance instance = instanceForId(instanceId);
            if (instance.isValid())
                m_dirtyInstanceSet.insert(instance);
        }
    }
}

void QmlDesigner::Qt5RenderNodeInstanceServer::removeSharedMemory(const QmlDesigner::RemoveSharedMemoryCommand &command)
{
    if (command.typeName() == "Image")
        ImageContainer::removeSharedMemorys(command.keyNumbers());
}

void Qt5RenderNodeInstanceServer::changePropertyValues(const ChangeValuesCommand &command)
{
    Qt5NodeInstanceServer::changePropertyValues(command);

    const QVector<PropertyValueContainer> values = command.valueChanges();
    for (const PropertyValueContainer &container : values) {
        // In case an effect item visibility changed to false, make sure all children are rendered
        // again as they might not have valid pixmaps yet
        if (container.name() == "visible" && !container.value().toBool()
            && hasInstanceForId(container.instanceId())) {
            ServerNodeInstance instance = instanceForId(container.instanceId());
            if (instance.isSubclassOf("QtQuick/PropertyChanges")) {
                QObject *targetObject = Internal::QmlPrivateGate::PropertyChanges::targetObject(
                    instance.internalInstance()->object());
                if (hasInstanceForObject(targetObject))
                    instance = instanceForObject(targetObject);
            }

            if (instance.hasParent() && instance.isComposedEffect())
                makeDirtyRecursive(instance.parent());
        } else if (container.isDynamic() && hasInstanceForId(container.instanceId())) {
            // Changes to dynamic properties are not always noticed by normal signal spy mechanism
            addChangedProperty(InstancePropertyPair(instanceForId(container.instanceId()),
                                                    container.name()));
        }
    }
}

void Qt5RenderNodeInstanceServer::changePropertyBindings(const ChangeBindingsCommand &command)
{
    Qt5NodeInstanceServer::changePropertyBindings(command);

    const QVector<PropertyBindingContainer> changes = command.bindingChanges;
    for (const PropertyBindingContainer &container : changes) {
        if (container.isDynamic() && hasInstanceForId(container.instanceId())) {
            // Changes to dynamic properties are not always noticed by normal signal spy mechanism
            addChangedProperty(InstancePropertyPair(instanceForId(container.instanceId()),
                                                    container.name()));
        }
    }
}

void Qt5RenderNodeInstanceServer::reparentInstances(const ReparentInstancesCommand &command)
{
    ServerNodeInstance effectNode;
    ServerNodeInstance oldParent;
    const QVector<ReparentContainer> containers = command.reparentInstances();
    for (const ReparentContainer &container : containers) {
        if (hasInstanceForId(container.instanceId())) {
            ServerNodeInstance instance = instanceForId(container.instanceId());
            if (instance.isComposedEffect()) {
                oldParent = instance.parent();
                effectNode = instance;
                break;
            }
        }
    }

    Qt5NodeInstanceServer::reparentInstances(command);

    if (oldParent.isValid())
        makeDirtyRecursive(oldParent);
    if (effectNode.isValid()) {
        ServerNodeInstance newParent = effectNode.parent();
        if (newParent.isValid()) {
            // This is a hack to work around Image elements sometimes losing their textures when
            // used as children of an effect. Toggling the visibility of the affected node seems
            // to be the only way to fix this issue.
            // Note that just marking the children's visibility dirty doesn't fix this issue.
            QQuickItem *parentItem = newParent.rootQuickItem();
            if (parentItem && parentItem->isVisible()) {
                parentItem->setVisible(false);
                parentItem->setVisible(true);
            }
        }
    }
}

void Qt5RenderNodeInstanceServer::removeInstances(const RemoveInstancesCommand &command)
{
    ServerNodeInstance oldParent;
    const QVector<qint32> ids = command.instanceIds();
    for (qint32 id : ids) {
        if (hasInstanceForId(id)) {
            ServerNodeInstance instance = instanceForId(id);
            if (instance.isComposedEffect()) {
                oldParent = instance.parent();
                break;
            }
        }
    }

    Qt5NodeInstanceServer::removeInstances(command);

    if (oldParent.isValid())
        makeDirtyRecursive(oldParent);
}

void Qt5RenderNodeInstanceServer::makeDirtyRecursive(const ServerNodeInstance &instance)
{
    m_dirtyInstanceSet.insert(instance);
    const QList<ServerNodeInstance> children = instance.childItems();
    for (const auto &child : children)
        makeDirtyRecursive(child);
}

} // namespace QmlDesigner
