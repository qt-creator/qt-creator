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
#include <QDropEvent>
#include <QMimeData>

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
#include "update3dviewstatecommand.h"
#include "enable3dviewcommand.h"
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
#include "objectnodeinstance.h"
#include "drop3dlibraryitemcommand.h"
#include "puppettocreatorcommand.h"
#include "view3dclosedcommand.h"

#include "dummycontextobject.h"
#include "../editor3d/generalhelper.h"
#include "../editor3d/mousearea3d.h"
#include "../editor3d/camerageometry.h"
#include "../editor3d/gridgeometry.h"
#include "../editor3d/selectionboxgeometry.h"
#include "../editor3d/linegeometry.h"

#include <designersupportdelegate.h>

#include <QVector3D>
#include <QQmlProperty>
#include <QOpenGLContext>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>

#ifdef QUICK3D_MODULE
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3dscenerootnode_p.h>
#endif

namespace QmlDesigner {

static QVariant objectToVariant(QObject *object)
{
    return QVariant::fromValue(object);
}

bool Qt5InformationNodeInstanceServer::eventFilter(QObject *, QEvent *event)
{
    switch (event->type()) {
    case QEvent::DragMove: {
        if (!dropAcceptable(static_cast<QDragMoveEvent *>(event))) {
            event->ignore();
            return true;
        }
    } break;

    case QEvent::Drop: {
        QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
        QByteArray data = dropEvent->mimeData()->data(QStringLiteral("application/vnd.bauhaus.itemlibraryinfo"));
        if (!data.isEmpty())
            nodeInstanceClient()->library3DItemDropped(createDrop3DLibraryItemCommand(data));

    } break;

    case QEvent::Close: {
        nodeInstanceClient()->view3DClosed(View3DClosedCommand());
    } break;

    case QEvent::KeyPress: {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        QPair<int, int> data = {keyEvent->key(), int(keyEvent->modifiers())};
        nodeInstanceClient()->handlePuppetToCreatorCommand({PuppetToCreatorCommand::KeyPressed,
                                                            QVariant::fromValue(data)});
    } break;

    default:
        break;
    }

    return false;
}

bool Qt5InformationNodeInstanceServer::dropAcceptable(QDragMoveEvent *event) const
{
    // Note: this method parses data out of the QDataStream. This should be in sync with how the
    // data is written to the stream (check QDataStream overloaded operators << and >> in
    // itemlibraryentry.cpp). If the write order changes, this logic may break.
    QDataStream stream(event->mimeData()->data(QStringLiteral("application/vnd.bauhaus.itemlibraryinfo")));

    QString name;
    TypeName typeName;
    int majorVersion;
    int minorVersion;
    QIcon typeIcon;
    QString libraryEntryIconPath;
    QString category;
    QString requiredImport;
    QHash<QString, QString> hints;

    stream >> name;
    stream >> typeName;
    stream >> majorVersion;
    stream >> minorVersion;
    stream >> typeIcon;
    stream >> libraryEntryIconPath;
    stream >> category;
    stream >> requiredImport;
    stream >> hints;

    QString canBeDropped = hints.value("canBeDroppedInView3D");
    return canBeDropped == "true" || canBeDropped == "True";
}

QObject *Qt5InformationNodeInstanceServer::createEditView3D(QQmlEngine *engine)
{
#ifdef QUICK3D_MODULE
    auto helper = new QmlDesigner::Internal::GeneralHelper();
    QObject::connect(helper, &QmlDesigner::Internal::GeneralHelper::toolStateChanged,
                     this, &Qt5InformationNodeInstanceServer::handleToolStateChanged);
    engine->rootContext()->setContextProperty("_generalHelper", helper);
    qmlRegisterType<QmlDesigner::Internal::MouseArea3D>("MouseArea3D", 1, 0, "MouseArea3D");
    qmlRegisterType<QmlDesigner::Internal::CameraGeometry>("CameraGeometry", 1, 0, "CameraGeometry");
    qmlRegisterType<QmlDesigner::Internal::GridGeometry>("GridGeometry", 1, 0, "GridGeometry");
    qmlRegisterType<QmlDesigner::Internal::SelectionBoxGeometry>("SelectionBoxGeometry", 1, 0, "SelectionBoxGeometry");
    qmlRegisterType<QmlDesigner::Internal::LineGeometry>("LineGeometry", 1, 0, "LineGeometry");
#endif

    QQmlComponent component(engine, QUrl("qrc:/qtquickplugin/mockfiles/EditView3D.qml"));

    QWindow *window = qobject_cast<QWindow *>(component.create());

    if (!window) {
        qWarning() << "Could not create edit view 3D: " << component.errors();
        return nullptr;
    }

    window->installEventFilter(this);
    QObject::connect(window, SIGNAL(selectionChanged(QVariant)),
                     this, SLOT(handleSelectionChanged(QVariant)));
    QObject::connect(window, SIGNAL(commitObjectProperty(QVariant, QVariant)),
                     this, SLOT(handleObjectPropertyCommit(QVariant, QVariant)));
    QObject::connect(window, SIGNAL(changeObjectProperty(QVariant, QVariant)),
                     this, SLOT(handleObjectPropertyChange(QVariant, QVariant)));
    QObject::connect(&m_propertyChangeTimer, &QTimer::timeout,
                     this, &Qt5InformationNodeInstanceServer::handleObjectPropertyChangeTimeout);
    QObject::connect(&m_selectionChangeTimer, &QTimer::timeout,
                     this, &Qt5InformationNodeInstanceServer::handleSelectionChangeTimeout);

    //For macOS we have to use the 4.1 core profile
    QSurfaceFormat surfaceFormat = window->requestedFormat();
    surfaceFormat.setVersion(4, 1);
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    window->setFormat(surfaceFormat);

#ifdef QUICK3D_MODULE
    helper->setParent(window);
#endif

    return window;
}

// The selection has changed in the edit view 3D. Empty list indicates selection is cleared.
void Qt5InformationNodeInstanceServer::handleSelectionChanged(const QVariant &objs)
{
    QList<ServerNodeInstance> instanceList;
    const QVariantList varObjs = objs.value<QVariantList>();
    for (const auto &object : varObjs) {
        auto obj = object.value<QObject *>();
        if (obj) {
            ServerNodeInstance instance = instanceForObject(obj);
            instanceList << instance;
        }
    }
    selectInstances(instanceList);
    // Hold selection changes reflected back from designer for a bit
    m_selectionChangeTimer.start(500);
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
    PropertyName targetPropertyName;

    // Position is a special case, because the position can be 'position.x 'or simply 'x'.
    // We prefer 'x'.
    if (propertyName != "position")
        targetPropertyName = propertyName;

    auto *obj = node.value<QObject *>();

    if (obj) {
        ServerNodeInstance instance = instanceForObject(obj);

        if (option == ValuesModifiedCommand::TransactionOption::Start)
            instance.setModifiedFlag(true);
        else if (option == ValuesModifiedCommand::TransactionOption::End)
            instance.setModifiedFlag(false);

        // We do have to split position into position.x, position.y, position.z
        ValuesModifiedCommand command = createValuesModifiedCommand(vectorToPropertyValue(
            instance,
            targetPropertyName,
            obj->property(propertyName)));

        command.transactionOption = option;

        nodeInstanceClient()->valuesModified(command);
    }
}

void Qt5InformationNodeInstanceServer::handleObjectPropertyCommit(const QVariant &object,
                                                                  const QVariant &propName)
{
    modifyVariantValue(object, propName.toByteArray(),
                       ValuesModifiedCommand::TransactionOption::End);
    m_changedNode = {};
    m_changedProperty = {};
    m_propertyChangeTimer.stop();
}

void Qt5InformationNodeInstanceServer::handleObjectPropertyChange(const QVariant &object,
                                                                  const QVariant &propName)
{
    PropertyName propertyName(propName.toByteArray());
    if (m_changedProperty != propertyName || m_changedNode != object) {
        if (!m_changedNode.isNull())
            handleObjectPropertyCommit(m_changedNode, m_changedProperty);
        modifyVariantValue(object, propertyName,
                           ValuesModifiedCommand::TransactionOption::Start);
    } else if (!m_propertyChangeTimer.isActive()) {
        m_propertyChangeTimer.start();
    }
    m_changedNode = object;
    m_changedProperty = propertyName;
}

void Qt5InformationNodeInstanceServer::handleToolStateChanged(const QString &tool,
                                                              const QVariant &toolState)
{
    QPair<QString, QVariant> data = {tool, toolState};
    nodeInstanceClient()->handlePuppetToCreatorCommand({PuppetToCreatorCommand::Edit3DToolState,
                                                        QVariant::fromValue(data)});
}

void Qt5InformationNodeInstanceServer::handleView3DSizeChange()
{
    QObject *view3D = sender();
    if (view3D == m_active3DView.internalObject())
        updateView3DRect(view3D);
}

void Qt5InformationNodeInstanceServer::updateView3DRect(QObject *view3D)
{
    QRectF viewPortrect(0., 0., 1000., 1000.);
    if (view3D) {
        viewPortrect = QRectF(0., 0., view3D->property("width").toDouble(),
                              view3D->property("height").toDouble());
    }
    QQmlProperty viewPortProperty(m_editView3D, "viewPortRect", context());
    viewPortProperty.write(viewPortrect);
}

void Qt5InformationNodeInstanceServer::updateActiveSceneToEditView3D()
{
    QQmlProperty sceneProperty(m_editView3D, "activeScene", context());
    sceneProperty.write(objectToVariant(m_active3DScene));
    if (m_active3DView.isValid())
        updateView3DRect(m_active3DView.internalObject());
}

Qt5InformationNodeInstanceServer::Qt5InformationNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    Qt5NodeInstanceServer(nodeInstanceClient)
{
    m_propertyChangeTimer.setInterval(100);
    m_selectionChangeTimer.setSingleShot(true);
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
void Qt5InformationNodeInstanceServer::selectInstances(const QList<ServerNodeInstance> &instanceList)
{
    nodeInstanceClient()->selectionChanged(createChangeSelectionCommand(instanceList));
}

/* This method allows changing property values from the puppet
 * For performance reasons (and the undo stack) properties should always be modifed in 'bulks'.
 */
void Qt5InformationNodeInstanceServer::modifyProperties(
    const QVector<NodeInstanceServer::InstancePropertyValueTriple> &properties)
{
    nodeInstanceClient()->valuesModified(createValuesModifiedCommand(properties));
}

QList<ServerNodeInstance> Qt5InformationNodeInstanceServer::createInstances(
        const QVector<InstanceContainer> &container)
{
    const auto createdInstances = NodeInstanceServer::createInstances(container);

    if (m_editView3D)
        createCameraAndLightGizmos(createdInstances);

    return createdInstances;
}

void Qt5InformationNodeInstanceServer::handleObjectPropertyChangeTimeout()
{
    modifyVariantValue(m_changedNode, m_changedProperty,
                       ValuesModifiedCommand::TransactionOption::None);
}

void Qt5InformationNodeInstanceServer::handleSelectionChangeTimeout()
{
    changeSelection(m_pendingSelectionChangeCommand);
}

void Qt5InformationNodeInstanceServer::createCameraAndLightGizmos(
        const QList<ServerNodeInstance> &instanceList) const
{
    QHash<QObject *, QObjectList> cameras;
    QHash<QObject *, QObjectList> lights;

    for (const ServerNodeInstance &instance : instanceList) {
        if (instance.isSubclassOf("QQuick3DCamera"))
            cameras[find3DSceneRoot(instance)] << instance.internalObject();
        else if (instance.isSubclassOf("QQuick3DAbstractLight"))
            lights[find3DSceneRoot(instance)] << instance.internalObject();
    }

    auto cameraIt = cameras.constBegin();
    while (cameraIt != cameras.constEnd()) {
        const auto cameraObjs = cameraIt.value();
        for (auto &obj : cameraObjs) {
            QMetaObject::invokeMethod(m_editView3D, "addCameraGizmo",
                                      Q_ARG(QVariant, objectToVariant(cameraIt.key())),
                                      Q_ARG(QVariant, objectToVariant(obj)));
        }
        ++cameraIt;
    }
    auto lightIt = lights.constBegin();
    while (lightIt != lights.constEnd()) {
        const auto lightObjs = lightIt.value();
        for (auto &obj : lightObjs) {
            QMetaObject::invokeMethod(m_editView3D, "addLightGizmo",
                                      Q_ARG(QVariant, objectToVariant(lightIt.key())),
                                      Q_ARG(QVariant, objectToVariant(obj)));
        }
        ++lightIt;
    }
}

void Qt5InformationNodeInstanceServer::addViewPorts(const QList<ServerNodeInstance> &instanceList)
{
    for (const ServerNodeInstance &instance : instanceList) {
        if (instance.isSubclassOf("QQuick3DViewport")) {
            m_view3Ds << instance;
            QObject *obj = instance.internalObject();
            QObject::connect(obj, SIGNAL(widthChanged()), this, SLOT(handleView3DSizeChange()));
            QObject::connect(obj, SIGNAL(heightChanged()), this, SLOT(handleView3DSizeChange()));
        }
    }
}

ServerNodeInstance Qt5InformationNodeInstanceServer::findView3DForInstance(const ServerNodeInstance &instance) const
{
#ifdef QUICK3D_MODULE
    if (!instance.isValid())
        return {};

    // View3D of an instance is one of the following, in order of priority:
    // - Any direct ancestor View3D of the instance
    // - Any View3D that specifies the instance's scene as importScene
    ServerNodeInstance checkInstance = instance;
    while (checkInstance.isValid()) {
        if (checkInstance.isSubclassOf("QQuick3DViewport"))
            return checkInstance;
        else
            checkInstance = checkInstance.parent();
    }

    // If no ancestor View3D was found, check if the scene root is specified as importScene in
    // some View3D.
    QObject *sceneRoot = find3DSceneRoot(instance);
    for (const auto &view3D : qAsConst(m_view3Ds)) {
        auto view = qobject_cast<QQuick3DViewport *>(view3D.internalObject());
        if (sceneRoot == view->importScene())
            return view3D;
    }
#endif
    return {};
}

QObject *Qt5InformationNodeInstanceServer::find3DSceneRoot(const ServerNodeInstance &instance) const
{
#ifdef QUICK3D_MODULE
    // The root of a 3D scene is any QQuick3DNode that doesn't have QQuick3DNode as parent.
    // One exception is QQuick3DSceneRootNode that has only a single child QQuick3DNode (not
    // a subclass of one, but exactly QQuick3DNode). In that case we consider the single child node
    // to be the scene root (as QQuick3DSceneRootNode is not visible in the navigator scene graph).

    if (!instance.isValid())
        return nullptr;

    QQuick3DNode *childNode = nullptr;
    auto countChildNodes = [&childNode](QQuick3DViewport *view) -> int {
        QQuick3DNode *sceneNode = view->scene();
        QList<QQuick3DObject *> children = sceneNode->childItems();
        int nodeCount = 0;
        for (const auto &child : children) {
            auto nodeChild = qobject_cast<QQuick3DNode *>(child);
            if (nodeChild) {
                ++nodeCount;
                childNode = nodeChild;
            }
        }
        return nodeCount;
    };

    // In case of View3D is selected, the root scene is whatever is contained in View3D, or
    // importScene, in case there is no content in View3D
    QObject *obj = instance.internalObject();
    auto view = qobject_cast<QQuick3DViewport *>(obj);
    if (view) {
        int nodeCount = countChildNodes(view);
        if (nodeCount == 0)
            return view->importScene();
        else if (nodeCount == 1)
            return childNode;
        else
            return view->scene();
    }

    ServerNodeInstance checkInstance = instance;
    bool foundNode = checkInstance.isSubclassOf("QQuick3DNode");
    while (checkInstance.isValid()) {
        ServerNodeInstance parentInstance = checkInstance.parent();
        if (parentInstance.isSubclassOf("QQuick3DViewport")) {
            view = qobject_cast<QQuick3DViewport *>(parentInstance.internalObject());
            int nodeCount = countChildNodes(view);
            if (nodeCount == 1)
                return checkInstance.internalObject();
            else
                return view->scene();
        } else if (parentInstance.isSubclassOf("QQuick3DNode")) {
            foundNode = true;
            checkInstance = parentInstance;
        } else {
            if (!foundNode) {
                // We haven't found any node yet, continue the search
                checkInstance = parentInstance;
            } else {
                return checkInstance.internalObject();
            }
        }
    }
#endif
    return nullptr;
}

void Qt5InformationNodeInstanceServer::setup3DEditView(const QList<ServerNodeInstance> &instanceList,
                                                       const QVariantMap &toolStates)
{
    ServerNodeInstance root = rootNodeInstance();

    addViewPorts(instanceList);

    // Find any scene to show
    for (const auto &instance : instanceList) {
        if (instance.isSubclassOf("QQuick3DNode")) {
            m_active3DScene = find3DSceneRoot(instance);
            m_active3DView = findView3DForInstance(instance);
            break;
        }
    }

    if (m_active3DScene) {
        m_editView3D = createEditView3D(engine());

        if (!m_editView3D) {
            m_active3DScene = nullptr;
            m_active3DView = {};
            return;
        }

        updateActiveSceneToEditView3D();

        createCameraAndLightGizmos(instanceList);

        QMetaObject::invokeMethod(m_editView3D, "updateToolStates", Q_ARG(QVariant, toolStates));
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
        setup3DEditView(instanceList, command.edit3dToolStates());
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

    if (m_selectionChangeTimer.isActive()) {
        // If selection was recently changed by puppet, hold updating the selection for a bit to
        // avoid selection flicker, especially in multiselect cases.
        m_pendingSelectionChangeCommand = command;
        // Add additional time in case more commands are still coming through
        m_selectionChangeTimer.start(500);
        return;
    }

    // Find a scene root of the selection to update active scene shown
    const QVector<qint32> instanceIds = command.instanceIds();
    QVariantList selectedObjs;
    QObject *firstSceneRoot = nullptr;
    ServerNodeInstance firstInstance;
    for (qint32 id : instanceIds) {
        if (hasInstanceForId(id)) {
            ServerNodeInstance instance = instanceForId(id);
            QObject *sceneRoot = find3DSceneRoot(instance);
            if (!firstSceneRoot && sceneRoot) {
                firstSceneRoot = sceneRoot;
                firstInstance = instance;
            }
            QObject *object = nullptr;
            if (firstSceneRoot && sceneRoot == firstSceneRoot && instance.isSubclassOf("QQuick3DNode"))
                object = instance.internalObject();
            if (object && (firstSceneRoot != object || instance.isSubclassOf("QQuick3DModel")))
                selectedObjs << objectToVariant(object);
        }
    }

    if (firstSceneRoot && m_active3DScene != firstSceneRoot) {
        m_active3DScene = firstSceneRoot;
        m_active3DView = findView3DForInstance(firstInstance);
        updateActiveSceneToEditView3D();
    }

    // Ensure the UI has enough selection box items. If it doesn't yet have them, which can be the
    // case when the first selection processed is a multiselection, we wait a bit as
    // using the new boxes immediately leads to visual glitches.
    int boxCount = m_editView3D->property("selectionBoxes").value<QVariantList>().size();
    if (boxCount < selectedObjs.size()) {
        QMetaObject::invokeMethod(m_editView3D, "ensureSelectionBoxes",
                                  Q_ARG(QVariant, QVariant::fromValue(selectedObjs.size())));
        m_pendingSelectionChangeCommand = command;
        m_selectionChangeTimer.start(100);
    } else {
        QMetaObject::invokeMethod(m_editView3D, "selectObjects",
                                  Q_ARG(QVariant, QVariant::fromValue(selectedObjs)));
    }
}

void Qt5InformationNodeInstanceServer::changePropertyValues(const ChangeValuesCommand &command)
{
    bool hasDynamicProperties = false;
    const QVector<PropertyValueContainer> values = command.valueChanges();
    for (const PropertyValueContainer &container : values) {
        if (!container.isReflected()) {
            hasDynamicProperties |= container.isDynamic();
            setInstancePropertyVariant(container);
        }
    }

    if (hasDynamicProperties)
        refreshBindings();

    startRenderTimer();
}

// update 3D view window state when the main app window state change
void Qt5InformationNodeInstanceServer::update3DViewState(const Update3dViewStateCommand &command)
{
    auto window = qobject_cast<QWindow *>(m_editView3D);
    if (window) {
        if (command.type() == Update3dViewStateCommand::StateChange) {
            if (command.previousStates() & Qt::WindowMinimized) // main window expanded from minimize state
                window->show();
            else if (command.currentStates() & Qt::WindowMinimized) // main window minimized
                window->hide();
        } else if (command.type() == Update3dViewStateCommand::ActiveChange) {
            window->setFlag(Qt::WindowStaysOnTopHint, command.isActive());

            // main window has a popup open, lower the edit view 3D so that the pop up is visible
            if (command.hasPopup())
                window->lower();
        }
    }
}

void Qt5InformationNodeInstanceServer::enable3DView(const Enable3DViewCommand &command)
{
    // TODO: this method is not currently in use as the 3D view is currently enabled by resetting the puppet.
    //       It should however be implemented here.

    auto window = qobject_cast<QWindow *>(m_editView3D);
    if (window && !command.isEnable()) {
        // TODO: remove the 3D view
    } else if (!window && command.isEnable()) {
        // TODO: create the 3D view
    }
}

} // namespace QmlDesigner
