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

            Internal::QuickItemNodeInstance::updateDirtyNode(rootNodeInstance().contentItem());
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

} // namespace QmlDesigner
