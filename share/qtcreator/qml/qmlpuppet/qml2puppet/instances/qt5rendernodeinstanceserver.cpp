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
#include "childrenchangeeventfilter.h"
#include "changestatecommand.h"
#include "childrenchangedcommand.h"
#include "completecomponentcommand.h"
#include "componentcompletedcommand.h"
#include "createscenecommand.h"
#include "quickitemnodeinstance.h"
#include "removesharedmemorycommand.h"

#include "dummycontextobject.h"

#include <designersupport.h>

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

        DesignerSupport::polishItems(quickView());

        if (quickView() && nodeInstanceClient()->bytesToWrite() < 10000) {
            foreach (QQuickItem *item, allItems()) {
                if (item) {
                    if (hasInstanceForObject(item)
                            && DesignerSupport::isDirty(item, DesignerSupport::ContentUpdateMask)) {
                        m_dirtyInstanceSet.insert(instanceForObject(item));
                    } else if (DesignerSupport::isDirty(item, DesignerSupport::AllMask)) {
                        ServerNodeInstance ancestorInstance = findNodeInstanceForItem(item->parentItem());
                        if (ancestorInstance.isValid())
                            m_dirtyInstanceSet.insert(ancestorInstance);
                    }
                    DesignerSupport::updateDirtyNode(item);
                }
            }

            clearChangedPropertyList();

            if (!m_dirtyInstanceSet.isEmpty()) {
                nodeInstanceClient()->pixmapChanged(createPixmapChangedCommand(m_dirtyInstanceSet.toList()));
                m_dirtyInstanceSet.clear();
            }

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


void Qt5RenderNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    Qt5NodeInstanceServer::createScene(command);

    QList<ServerNodeInstance> instanceList;
    foreach (const InstanceContainer &container, command.instances()) {
        ServerNodeInstance instance = instanceForId(container.instanceId());
        if (instance.isValid()) {
            instanceList.append(instance);
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

    QList<ServerNodeInstance> instanceList;
    foreach (qint32 instanceId, command.instances()) {
        ServerNodeInstance instance = instanceForId(instanceId);
        if (instance.isValid()) {
            instanceList.append(instance);
            m_dirtyInstanceSet.insert(instance);
        }
    }

    nodeInstanceClient()->pixmapChanged(createPixmapChangedCommand(instanceList));
}

void QmlDesigner::Qt5RenderNodeInstanceServer::removeSharedMemory(const QmlDesigner::RemoveSharedMemoryCommand &command)
{
    if (command.typeName() == "Image")
        ImageContainer::removeSharedMemorys(command.keyNumbers());
}

} // namespace QmlDesigner
