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

#include <designersupportdelegate.h>

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

        DesignerSupport::polishItems(quickWindow());

        if (quickWindow() && nodeInstanceClient()->bytesToWrite() < 10000) {
            bool windowDirty = false;
            foreach (QQuickItem *item, allItems()) {
                if (item) {
                    if (Internal::QuickItemNodeInstance::unifiedRenderPath()) {
                        if (DesignerSupport::isDirty(item, DesignerSupport::AllMask)) {
                            windowDirty = true;
                            break;
                        }
                    } else {
                        if (hasInstanceForObject(item)) {
                            if (DesignerSupport::isDirty(item, DesignerSupport::ContentUpdateMask))
                                m_dirtyInstanceSet.insert(instanceForObject(item));
                            if (QQuickItem *effectParent = parentEffectItem(item)) {
                                if ((DesignerSupport::isDirty(
                                        item,
                                        DesignerSupport::DirtyType(
                                            DesignerSupport::TransformUpdateMask
                                            | DesignerSupport::Visible
                                            | DesignerSupport::ContentUpdateMask)))
                                    && hasInstanceForObject(effectParent)) {
                                    m_dirtyInstanceSet.insert(instanceForObject(effectParent));
                                }
                            }
                        } else if (DesignerSupport::isDirty(item, DesignerSupport::AllMask)) {
                            ServerNodeInstance ancestorInstance = findNodeInstanceForItem(
                                item->parentItem());
                            if (ancestorInstance.isValid())
                                m_dirtyInstanceSet.insert(ancestorInstance);
                        }
                        Internal::QuickItemNodeInstance::updateDirtyNode(item);
                    }
                }
            }

            clearChangedPropertyList();

            if (Internal::QuickItemNodeInstance::unifiedRenderPath()) {
                if (windowDirty)
                    nodeInstanceClient()->pixmapChanged(createPixmapChangedCommand({rootNodeInstance()}));
            } else {
                if (!m_dirtyInstanceSet.isEmpty()) {
                    nodeInstanceClient()->pixmapChanged(
                        createPixmapChangedCommand(QtHelpers::toList(m_dirtyInstanceSet)));
                    m_dirtyInstanceSet.clear();
                }
            }

            m_dirtyInstanceSet.clear();

            resetAllItems();

            slowDownRenderTimer();
            nodeInstanceClient()->flush();
            nodeInstanceClient()->synchronizeWithClientProcess();
        }

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
