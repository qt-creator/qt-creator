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

#include "qt5informationnodeinstanceserver.h"

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
#include "changeselectioncommand.h"

#include "dummycontextobject.h"
#include "../editor3d/cameracontrolhelper.h"
#include "../editor3d/mousearea3d.h"

#include <designersupportdelegate.h>

#include <QVector3D>
#include <QQmlProperty>
#include <QOpenGLContext>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>

namespace QmlDesigner {

static QVariant objectToVariant(QObject *object)
{
    return QVariant::fromValue(object);
}

QObject *Qt5InformationNodeInstanceServer::createEditView3D(QQmlEngine *engine)
{
    auto helper = new QmlDesigner::Internal::CameraControlHelper();
    engine->rootContext()->setContextProperty("designStudioNativeCameraControlHelper", helper);

#ifdef QUICK3D_MODULE
    qmlRegisterType<QmlDesigner::Internal::MouseArea3D>("MouseArea3D", 1, 0, "MouseArea3D");
#endif

    QQmlComponent component(engine, QUrl("qrc:/qtquickplugin/mockfiles/EditView3D.qml"));

    QWindow *window = qobject_cast<QWindow *>(component.create());

    if (!window) {
        qWarning() << "Could not create edit view" << component.errors();
        return nullptr;
    }

    QObject::connect(window, SIGNAL(objectClicked(QVariant)), this, SLOT(objectClicked(QVariant)));
    QObject::connect(window, SIGNAL(commitObjectPosition(QVariant)),
                     this, SLOT(handleObjectPositionCommit(QVariant)));
    QObject::connect(window, SIGNAL(moveObjectPosition(QVariant)),
                     this, SLOT(handleObjectPositionMove(QVariant)));
    QObject::connect(&m_moveTimer, &QTimer::timeout,
                     this, &Qt5InformationNodeInstanceServer::handleObjectPositionMoveTimeout);

    //For macOS we have to use the 4.1 core profile
    QSurfaceFormat surfaceFormat = window->requestedFormat();
    surfaceFormat.setVersion(4, 1);
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    window->setFormat(surfaceFormat);
    helper->setParent(window);

    return window;
}

// an object is clicked in the 3D edit view. Null object indicates selection clearing.
void Qt5InformationNodeInstanceServer::objectClicked(const QVariant &object)
{
    auto obj = object.value<QObject *>();
    ServerNodeInstance instance;
    if (obj)
        instance = instanceForObject(obj);
    selectInstance(instance);
}

QVector<Qt5InformationNodeInstanceServer::InstancePropertyValueTriple>
Qt5InformationNodeInstanceServer::vectorToPropertyValue(
    const ServerNodeInstance &instance,
    const PropertyName &propertyName,
    const QVariant &variant)
{
    QVector<InstancePropertyValueTriple> result;

    auto vector3d = variant.value<QVector3D>();

    if (vector3d.isNull())
        return result;

    const PropertyName dot = propertyName.isEmpty() ? "" : ".";

    InstancePropertyValueTriple propTriple;
    propTriple.instance = instance;
    propTriple.propertyName = propertyName + dot + PropertyName("x");
    propTriple.propertyValue = vector3d.x();
    result.append(propTriple);
    propTriple.propertyName = propertyName + dot + PropertyName("y");
    propTriple.propertyValue = vector3d.y();
    result.append(propTriple);
    propTriple.propertyName = propertyName + dot + PropertyName("z");
    propTriple.propertyValue = vector3d.z();
    result.append(propTriple);

    return result;
}

void Qt5InformationNodeInstanceServer::modifyVariantValue(
    const QVariant &node,
    const PropertyName &propertyName,
    ValuesModifiedCommand::TransactionOption option)
{
    PropertyName targetPopertyName;

    // Position is a special case, because the position can be 'position.x 'or simply 'x'.
    // We prefer 'x'.
    if (propertyName != "position")
        targetPopertyName = propertyName;

    auto *obj = node.value<QObject *>();
    if (obj) {
        // We do have to split position into position.x, position.y, position.z
        ValuesModifiedCommand command = createValuesModifiedCommand(vectorToPropertyValue(
            instanceForObject(obj),
            targetPopertyName,
            obj->property(propertyName)));

        command.transactionOption = option;

        nodeInstanceClient()->valuesModified(command);
    }
}

void Qt5InformationNodeInstanceServer::handleObjectPositionCommit(const QVariant &object)
{
    modifyVariantValue(object, "position", ValuesModifiedCommand::TransactionOption::End);
    m_movedNode = {};
    m_moveTimer.stop();
}

void Qt5InformationNodeInstanceServer::handleObjectPositionMove(const QVariant &object)
{
    if (m_movedNode.isNull()) {
        modifyVariantValue(object, "position", ValuesModifiedCommand::TransactionOption::Start);
    } else {
        if (!m_moveTimer.isActive())
            m_moveTimer.start();
    }
    m_movedNode = object;
}

Qt5InformationNodeInstanceServer::Qt5InformationNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    Qt5NodeInstanceServer(nodeInstanceClient)
{
    m_moveTimer.setInterval(100);
}

void Qt5InformationNodeInstanceServer::sendTokenBack()
{
    foreach (const TokenCommand &command, m_tokenList)
        nodeInstanceClient()->token(command);

    m_tokenList.clear();
}

void Qt5InformationNodeInstanceServer::token(const TokenCommand &command)
{
    m_tokenList.append(command);
    startRenderTimer();
}

bool Qt5InformationNodeInstanceServer::isDirtyRecursiveForNonInstanceItems(QQuickItem *item) const
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

bool Qt5InformationNodeInstanceServer::isDirtyRecursiveForParentInstances(QQuickItem *item) const
{
    if (DesignerSupport::isDirty(item,  DesignerSupport::TransformUpdateMask))
        return true;

    QQuickItem *parentItem = item->parentItem();

    if (parentItem) {
        if (hasInstanceForObject(parentItem))
            return false;

        return isDirtyRecursiveForParentInstances(parentItem);

    }

    return false;
}

/* This method allows changing the selection from the puppet */
void Qt5InformationNodeInstanceServer::selectInstance(const ServerNodeInstance &instance)
{
    nodeInstanceClient()->selectionChanged(createChangeSelectionCommand({instance}));
}

/* This method allows changing property values from the puppet
 * For performance reasons (and the undo stack) properties should always be modifed in 'bulks'.
 */
void Qt5InformationNodeInstanceServer::modifyProperties(
    const QVector<NodeInstanceServer::InstancePropertyValueTriple> &properties)
{
    nodeInstanceClient()->valuesModified(createValuesModifiedCommand(properties));
}

void Qt5InformationNodeInstanceServer::handleObjectPositionMoveTimeout()
{
    modifyVariantValue(m_movedNode, "position", ValuesModifiedCommand::TransactionOption::None);
}

QObject *Qt5InformationNodeInstanceServer::findRootNodeOf3DViewport(
        const QList<ServerNodeInstance> &instanceList) const
{
    for (const ServerNodeInstance &instance : instanceList) {
        if (instance.isSubclassOf("QQuick3DViewport")) {
            for (const ServerNodeInstance &child : instanceList) { /* Look for scene node */
                /* The QQuick3DViewport always creates a root node.
                 * This root node contains the complete scene. */
                if (child.isSubclassOf("QQuick3DNode") && child.parent() == instance)
                    return child.internalObject()->property("parent").value<QObject *>();
            }
        }
    }
    return nullptr;
}

void Qt5InformationNodeInstanceServer::findCamerasAndLights(
        const QList<ServerNodeInstance> &instanceList,
        QObjectList &cameras, QObjectList &lights) const
{
    QObjectList objList;
    for (const ServerNodeInstance &instance : instanceList) {
        if (instance.isSubclassOf("QQuick3DCamera"))
            cameras << instance.internalObject();
        else if (instance.isSubclassOf("QQuick3DAbstractLight"))
            lights << instance.internalObject();
    }
}

void Qt5InformationNodeInstanceServer::setup3DEditView(const QList<ServerNodeInstance> &instanceList)
{
    ServerNodeInstance root = rootNodeInstance();

    QObject *node = nullptr;
    bool showCustomLight = false;

    if (root.isSubclassOf("QQuick3DNode")) {
        node = root.internalObject();
        showCustomLight = true; // Pure node scene we should add a custom light
    } else {
        node = findRootNodeOf3DViewport(instanceList);
    }

    if (node) { // If we found a scene we create the edit view
        m_editView3D = createEditView3D(engine());

        if (!m_editView3D)
            return;

        QQmlProperty sceneProperty(m_editView3D, "scene", context());
        node->setParent(m_editView3D);
        sceneProperty.write(objectToVariant(node));
        QQmlProperty parentProperty(node, "parent", context());
        parentProperty.write(objectToVariant(m_editView3D));
        QQmlProperty completeSceneProperty(m_editView3D, "showLight", context());
        completeSceneProperty.write(showCustomLight);

        // Create camera and light gizmos
        QObjectList cameras;
        QObjectList lights;
        findCamerasAndLights(instanceList, cameras, lights);
        for (auto &obj : qAsConst(cameras)) {
            QMetaObject::invokeMethod(m_editView3D, "addCameraGizmo",
                    Q_ARG(QVariant, objectToVariant(obj)));
        }
        for (auto &obj : qAsConst(lights)) {
            QMetaObject::invokeMethod(m_editView3D, "addLightGizmo",
                    Q_ARG(QVariant, objectToVariant(obj)));
        }
    }
}

void Qt5InformationNodeInstanceServer::collectItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;
    if (!inFunction) {
        inFunction = true;

        DesignerSupport::polishItems(quickView());

        QSet<ServerNodeInstance> informationChangedInstanceSet;
        QVector<InstancePropertyPair> propertyChangedList;

        if (quickView()) {
            foreach (QQuickItem *item, allItems()) {
                if (item && hasInstanceForObject(item)) {
                    ServerNodeInstance instance = instanceForObject(item);

                    if (isDirtyRecursiveForNonInstanceItems(item))
                        informationChangedInstanceSet.insert(instance);
                    else if (isDirtyRecursiveForParentInstances(item))
                        informationChangedInstanceSet.insert(instance);

                    if (DesignerSupport::isDirty(item, DesignerSupport::ParentChanged)) {
                        m_parentChangedSet.insert(instance);
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

            sendTokenBack();

            if (!informationChangedInstanceSet.isEmpty())
                nodeInstanceClient()->informationChanged(
                    createAllInformationChangedCommand(QtHelpers::toList(informationChangedInstanceSet)));

            if (!propertyChangedList.isEmpty())
                nodeInstanceClient()->valuesChanged(createValuesChangedCommand(propertyChangedList));

            if (!m_parentChangedSet.isEmpty()) {
                sendChildrenChangedCommand(QtHelpers::toList(m_parentChangedSet));
                m_parentChangedSet.clear();
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

void Qt5InformationNodeInstanceServer::reparentInstances(const ReparentInstancesCommand &command)
{
    foreach (const ReparentContainer &container, command.reparentInstances()) {
        if (hasInstanceForId(container.instanceId())) {
            ServerNodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid()) {
                m_parentChangedSet.insert(instance);
            }
        }
    }

    Qt5NodeInstanceServer::reparentInstances(command);
}

void Qt5InformationNodeInstanceServer::clearScene(const ClearSceneCommand &command)
{
    Qt5NodeInstanceServer::clearScene(command);

    m_parentChangedSet.clear();
    m_completedComponentList.clear();
}

void Qt5InformationNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    Qt5NodeInstanceServer::createScene(command);

    QList<ServerNodeInstance> instanceList;
    foreach (const InstanceContainer &container, command.instances()) {
        if (hasInstanceForId(container.instanceId())) {
            ServerNodeInstance instance = instanceForId(container.instanceId());
            if (instance.isValid()) {
                instanceList.append(instance);
            }
        }
    }

    nodeInstanceClient()->informationChanged(createAllInformationChangedCommand(instanceList, true));
    nodeInstanceClient()->valuesChanged(createValuesChangedCommand(instanceList));
    sendChildrenChangedCommand(instanceList);
    nodeInstanceClient()->componentCompleted(createComponentCompletedCommand(instanceList));

    if (qEnvironmentVariableIsSet("QMLDESIGNER_QUICK3D_MODE"))
        setup3DEditView(instanceList);
}

void Qt5InformationNodeInstanceServer::sendChildrenChangedCommand(const QList<ServerNodeInstance> &childList)
{
    QSet<ServerNodeInstance> parentSet;
    QList<ServerNodeInstance> noParentList;

    foreach (const ServerNodeInstance &child, childList) {
        if (child.isValid()) {
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
    }

    foreach (const ServerNodeInstance &parent, parentSet)
        nodeInstanceClient()->childrenChanged(createChildrenChangedCommand(parent, parent.childItems()));

    if (!noParentList.isEmpty())
        nodeInstanceClient()->childrenChanged(createChildrenChangedCommand(ServerNodeInstance(), noParentList));

}

void Qt5InformationNodeInstanceServer::completeComponent(const CompleteComponentCommand &command)
{
    Qt5NodeInstanceServer::completeComponent(command);

    QList<ServerNodeInstance> instanceList;
    foreach (qint32 instanceId, command.instances()) {
        if (hasInstanceForId(instanceId)) {
            ServerNodeInstance instance = instanceForId(instanceId);
            if (instance.isValid()) {
                instanceList.append(instance);
            }
        }
    }

    m_completedComponentList.append(instanceList);

    nodeInstanceClient()->valuesChanged(createValuesChangedCommand(instanceList));
    nodeInstanceClient()->informationChanged(createAllInformationChangedCommand(instanceList, true));
}

void QmlDesigner::Qt5InformationNodeInstanceServer::removeSharedMemory(const QmlDesigner::RemoveSharedMemoryCommand &command)
{
    if (command.typeName() == "Values")
        ValuesChangedCommand::removeSharedMemorys(command.keyNumbers());
}

void Qt5InformationNodeInstanceServer::changeSelection(const ChangeSelectionCommand &command)
{
    if (!m_editView3D)
        return;

    const QVector<qint32> instanceIds = command.instanceIds();
    for (qint32 id : instanceIds) {
        if (hasInstanceForId(id)) {
            ServerNodeInstance instance = instanceForId(id);
            QObject *object = nullptr;
            if (instance.isSubclassOf("QQuick3DModel") || instance.isSubclassOf("QQuick3DCamera")
                || instance.isSubclassOf("QQuick3DAbstractLight")) {
                object = instance.internalObject();
            }
            QMetaObject::invokeMethod(m_editView3D, "selectObject", Q_ARG(QVariant,
                                                                          objectToVariant(object)));
            return; // TODO: support multi-selection
        }
    }
}

} // namespace QmlDesigner
