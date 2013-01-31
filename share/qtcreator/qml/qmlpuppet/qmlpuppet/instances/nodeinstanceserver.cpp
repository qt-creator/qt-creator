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

#include "nodeinstanceserver.h"

#include <QDeclarativeEngine>
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
#include <QMutableVectorIterator>

#include "servernodeinstance.h"
#include "objectnodeinstance.h"
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
#include "changeauxiliarycommand.h"
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
#include "changenodesourcecommand.h"
#include "tokencommand.h"
#include "removesharedmemorycommand.h"

#include "dummycontextobject.h"


namespace QmlDesigner {

NodeInstanceServer::NodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    NodeInstanceServerInterface(),
    m_childrenChangeEventFilter(new Internal::ChildrenChangeEventFilter(this)),
    m_nodeInstanceClient(nodeInstanceClient),
    m_timer(0),
    m_renderTimerInterval(16),
    m_slowRenderTimer(false),
    m_slowRenderTimerInterval(200)
{
    qmlRegisterType<DummyContextObject>("QmlDesigner", 1, 0, "DummyContextObject");

    connect(m_childrenChangeEventFilter.data(), SIGNAL(childrenChanged(QObject*)), this, SLOT(emitParentChanged(QObject*)));
}

NodeInstanceServer::~NodeInstanceServer()
{
}

QList<ServerNodeInstance>  NodeInstanceServer::createInstances(const QVector<InstanceContainer> &containerVector)
{
    Q_ASSERT(declarativeView() || sgView());
    QList<ServerNodeInstance> instanceList;
    foreach (const InstanceContainer &instanceContainer, containerVector) {
        ServerNodeInstance instance;
        if (instanceContainer.nodeSourceType() == InstanceContainer::ComponentSource) {
            instance = ServerNodeInstance::create(this, instanceContainer, ServerNodeInstance::WrapAsComponent);
        } else {
            instance = ServerNodeInstance::create(this, instanceContainer, ServerNodeInstance::DoNotWrapAsComponent);
        }
        insertInstanceRelationship(instance);
        instanceList.append(instance);
        instance.internalObject()->installEventFilter(childrenChangeEventFilter());
        if (instanceContainer.instanceId() == 0) {
            m_rootNodeInstance = instance;
            resizeCanvasSizeToRootItemSize();
        }

        foreach (QDeclarativeContext* context, allSubContextsForObject(instance.internalObject()))
            setupDummysForContext(context);
    }

    return instanceList;
}

void NodeInstanceServer::createInstances(const CreateInstancesCommand &command)
{
    createInstances(command.instances());
    refreshBindings();
    startRenderTimer();
}

ServerNodeInstance NodeInstanceServer::instanceForId(qint32 id) const
{
    if (id < 0)
        return ServerNodeInstance();

    Q_ASSERT(m_idInstanceHash.contains(id));
    return m_idInstanceHash.value(id);
}

bool NodeInstanceServer::hasInstanceForId(qint32 id) const
{
    if (id < 0)
        return false;

    return m_idInstanceHash.contains(id);
}

ServerNodeInstance NodeInstanceServer::instanceForObject(QObject *object) const
{
    Q_ASSERT(m_objectInstanceHash.contains(object));
    return m_objectInstanceHash.value(object);
}

bool NodeInstanceServer::hasInstanceForObject(QObject *object) const
{
    if (object == 0)
        return false;

    return m_objectInstanceHash.contains(object);
}

void NodeInstanceServer::setRenderTimerInterval(int timerInterval)
{
    m_renderTimerInterval = timerInterval;
}

void NodeInstanceServer::setSlowRenderTimerInterval(int timerInterval)
{
    m_slowRenderTimerInterval = timerInterval;
}

void NodeInstanceServer::setTimerId(int timerId)
{
    m_timer = timerId;
}

int NodeInstanceServer::timerId() const
{
    return m_timer;
}

int NodeInstanceServer::renderTimerInterval() const
{
    return m_renderTimerInterval;
}

void NodeInstanceServer::startRenderTimer()
{
    if (m_slowRenderTimer)
        stopRenderTimer();

    if (m_timer == 0)
        m_timer = startTimer(m_renderTimerInterval);

    m_slowRenderTimer = false;
}

void NodeInstanceServer::slowDownRenderTimer()
{
    if (!m_slowRenderTimer)
        stopRenderTimer();

    if (m_timer != 0) {
        killTimer(m_timer);
        m_timer = 0;
    }

    if (m_timer == 0)
        m_timer = startTimer(m_slowRenderTimerInterval);

    m_slowRenderTimer = true;
}

void NodeInstanceServer::stopRenderTimer()
{
    if (m_timer) {
        killTimer(m_timer);
        m_timer = 0;
    }
}

void NodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    initializeView(command.imports());
    QUnifiedTimer::instance()->setSlowdownFactor(0.00001);
    QUnifiedTimer::instance()->setSlowModeEnabled(true);

    QList<ServerNodeInstance> instanceList = setupScene(command);

    refreshBindings();

    startRenderTimer();
}

void NodeInstanceServer::clearScene(const ClearSceneCommand &/*command*/)
{
    stopRenderTimer();

    removeAllInstanceRelationships();
    m_fileSystemWatcherHash.clear();
    m_rootNodeInstance.makeInvalid();
    m_changedPropertyList.clear();
    m_fileUrl.clear();
}

void NodeInstanceServer::removeInstances(const RemoveInstancesCommand &command)
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
    startRenderTimer();
}

void NodeInstanceServer::removeProperties(const RemovePropertiesCommand &command)
{
    bool hasDynamicProperties = false;
    foreach (const PropertyAbstractContainer &container, command.properties()) {
        hasDynamicProperties |= container.isDynamic();
        resetInstanceProperty(container);
    }

    if (hasDynamicProperties)
        refreshBindings();

    startRenderTimer();
}

void NodeInstanceServer::reparentInstances(const QVector<ReparentContainer> &containerVector)
{
    foreach (const ReparentContainer &container, containerVector) {
        ServerNodeInstance instance = instanceForId(container.instanceId());
        if (instance.isValid()) {
            instance.reparent(instanceForId(container.oldParentInstanceId()), container.oldParentProperty(), instanceForId(container.newParentInstanceId()), container.newParentProperty());
        }
    }

}

void NodeInstanceServer::reparentInstances(const ReparentInstancesCommand &command)
{
    reparentInstances(command.reparentInstances());
    refreshBindings();
    startRenderTimer();
}

void NodeInstanceServer::changeState(const ChangeStateCommand &command)
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

    startRenderTimer();
}

void NodeInstanceServer::completeComponent(const CompleteComponentCommand &command)
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

    startRenderTimer();
}

void NodeInstanceServer::changeNodeSource(const ChangeNodeSourceCommand &command)
{
    if (hasInstanceForId(command.instanceId())) {
        ServerNodeInstance instance = instanceForId(command.instanceId());
        if (instance.isValid())
            instance.setNodeSource(command.nodeSource());
    }

    startRenderTimer();
}

void NodeInstanceServer::token(const TokenCommand &/*command*/)
{

}

void NodeInstanceServer::removeSharedMemory(const RemoveSharedMemoryCommand &/*command*/)
{
}

void NodeInstanceServer::setupImports(const QVector<AddImportContainer> &containerVector)
{
    foreach (const AddImportContainer &container, containerVector) {
        QString importStatement = QString("import ");

        if (!container.fileName().isEmpty())
            importStatement += '"' + container.fileName() + '"';
        else if (!container.url().isEmpty())
            importStatement += container.url().toString();

        if (!container.version().isEmpty())
            importStatement += ' ' + container.version();

        if (!container.alias().isEmpty())
            importStatement += " as " + container.alias();

        importStatement.append('\n');

        if (!m_importList.contains(importStatement))
            m_importList.append(importStatement);
    }

    delete m_importComponent.data();
    delete m_importComponentObject.data();

    m_importComponent = new QDeclarativeComponent(engine(), 0);
    QString componentString;
    foreach (const QString &importStatement, m_importList)
        componentString += QString("%1").arg(importStatement);

    componentString += QString("Item {}\n");

    m_importComponent->setData(componentString.toUtf8(), fileUrl());
    m_importComponentObject = m_importComponent->create();

    if (!m_importComponent->errorString().isEmpty())
        qDebug() << "QmlDesigner.NodeInstances: import wrong: " << m_importComponent->errorString();
}

void NodeInstanceServer::setupFileUrl(const QUrl &fileUrl)
{
    if (!fileUrl.isEmpty()) {
        engine()->setBaseUrl(fileUrl);
        m_fileUrl = fileUrl;
    }
}

void NodeInstanceServer::setupDummyData(const QUrl &fileUrl)
{
    if (!fileUrl.isEmpty()) {
        QStringList dummyDataDirectoryList = dummyDataDirectories(QFileInfo(fileUrl.toLocalFile()).path());
        foreach (const QString &dummyDataDirectory, dummyDataDirectoryList) {
            loadDummyDataFiles(dummyDataDirectory);
            loadDummyDataContext(dummyDataDirectory);
        }
    }

    if (m_dummyContextObject.isNull())
        setupDefaultDummyData();
}

void NodeInstanceServer::setupDefaultDummyData()
{
    QDeclarativeComponent component(engine());
    QByteArray defaultContextObjectArray("import QtQuick 1.0\n"
                                         "import QmlDesigner 1.0\n"
                                         "DummyContextObject {\n"
                                         "    parent: QtObject {\n"
                                         "        property real width: 360\n"
                                         "        property real height: 640\n"
                                         "    }\n"
                                         "}\n");

    component.setData(defaultContextObjectArray, fileUrl());
    m_dummyContextObject = component.create();

    if (component.isError()) {
        QList<QDeclarativeError> errors = component.errors();
        foreach (const QDeclarativeError &error, errors) {
            qWarning() << error;
        }
    }

    if (m_dummyContextObject) {
        qWarning() << "Loaded default dummy context object.";
        m_dummyContextObject->setParent(this);
    }

    refreshBindings();
}

QList<ServerNodeInstance>  NodeInstanceServer::setupInstances(const CreateSceneCommand &command)
{
    QList<ServerNodeInstance> instanceList = createInstances(command.instances());

    foreach (const IdContainer &container, command.ids()) {
        if (hasInstanceForId(container.instanceId()))
            instanceForId(container.instanceId()).setId(container.id());
    }

    foreach (const PropertyValueContainer &container, command.valueChanges()) {
        if (container.isDynamic())
            setInstancePropertyVariant(container);
    }

    foreach (const PropertyValueContainer &container, command.valueChanges()) {
        if (!container.isDynamic())
            setInstancePropertyVariant(container);
    }

    reparentInstances(command.reparentInstances());

    foreach (const PropertyBindingContainer &container, command.bindingChanges()) {
        if (container.isDynamic())
            setInstancePropertyBinding(container);
    }

    foreach (const PropertyBindingContainer &container, command.bindingChanges()) {
        if (!container.isDynamic())
            setInstancePropertyBinding(container);
    }

    foreach (const PropertyValueContainer &container, command.auxiliaryChanges()) {
        setInstanceAuxiliaryData(container);
    }

    foreach (ServerNodeInstance instance, instanceList)
        instance.doComponentComplete();

    return instanceList;
}

void NodeInstanceServer::changeFileUrl(const ChangeFileUrlCommand &command)
{
    m_fileUrl = command.fileUrl();

    if (engine())
        engine()->setBaseUrl(m_fileUrl);

    refreshBindings();
    startRenderTimer();
}

void NodeInstanceServer::changePropertyValues(const ChangeValuesCommand &command)
{
    bool hasDynamicProperties = false;
    foreach (const PropertyValueContainer &container, command.valueChanges()) {
        hasDynamicProperties |= container.isDynamic();
        setInstancePropertyVariant(container);
    }

    if (hasDynamicProperties)
        refreshBindings();

    startRenderTimer();
}

void NodeInstanceServer::changeAuxiliaryValues(const ChangeAuxiliaryCommand &command)
{
    foreach (const PropertyValueContainer &container, command.auxiliaryChanges()) {
        setInstanceAuxiliaryData(container);
    }

    startRenderTimer();
}

void NodeInstanceServer::changePropertyBindings(const ChangeBindingsCommand &command)
{
    bool hasDynamicProperties = false;
    foreach (const PropertyBindingContainer &container, command.bindingChanges()) {
        hasDynamicProperties |= container.isDynamic();
        setInstancePropertyBinding(container);
    }

    if (hasDynamicProperties)
        refreshBindings();

    startRenderTimer();
}

void NodeInstanceServer::changeIds(const ChangeIdsCommand &command)
{
    foreach (const IdContainer &container, command.ids()) {
        if (hasInstanceForId(container.instanceId()))
            instanceForId(container.instanceId()).setId(container.id());
    }

    refreshBindings();
    startRenderTimer();
}

QDeclarativeContext *NodeInstanceServer::context() const
{
    if (m_importComponentObject) {
        QDeclarativeContext *importComponentContext = QDeclarativeEngine::contextForObject(m_importComponentObject.data());
        if (importComponentContext) // this should be the default
            return importComponentContext;
    }

    if (engine())
        return rootContext();

    return 0;
}

QDeclarativeContext *NodeInstanceServer::rootContext() const
{
    return engine()->rootContext();
}

const QVector<NodeInstanceServer::InstancePropertyPair> NodeInstanceServer::changedPropertyList() const
{
    return m_changedPropertyList;
}

void NodeInstanceServer::clearChangedPropertyList()
{
    m_changedPropertyList.clear();
}

void NodeInstanceServer::setupDummysForContext(QDeclarativeContext *context)
{
    foreach (const DummyPair& dummyPair, m_dummyObjectList) {
        if (dummyPair.second) {
            context->setContextProperty(dummyPair.first, dummyPair.second.data());
        }
    }
}


QList<QDeclarativeContext*> NodeInstanceServer::allSubContextsForObject(QObject *object)
{
    QList<QDeclarativeContext*> contextList;

    if (object) {
        foreach (QObject *subObject, allSubObjectsForObject(object)) {
            QDeclarativeContext *contextOfObject = QDeclarativeEngine::contextForObject(subObject);
            if (contextOfObject) {
                if (contextOfObject != context() && !contextList.contains(contextOfObject))
                    contextList.append(contextOfObject);
            }
        }
    }

    return contextList;
}

QList<QObject*> NodeInstanceServer::allSubObjectsForObject(QObject *object)
{
    QList<QObject*> subChildren;
    if (object) {
        subChildren = object->findChildren<QObject*>();
    }

    return subChildren;
}

void NodeInstanceServer::removeAllInstanceRelationships()
{
    // prevent destroyed() signals calling back

    foreach (ServerNodeInstance instance, m_objectInstanceHash) {
        if (instance.isValid())
            instance.setId(QString());
    }

    //first  the root object
    if (rootNodeInstance().internalObject())
        rootNodeInstance().internalObject()->disconnect();

    rootNodeInstance().makeInvalid();


    foreach (ServerNodeInstance instance, m_objectInstanceHash) {
        if (instance.internalObject())
            instance.internalObject()->disconnect();
        instance.makeInvalid();
    }

    m_idInstanceHash.clear();
    m_objectInstanceHash.clear();
}

QFileSystemWatcher *NodeInstanceServer::dummydataFileSystemWatcher()
{
    if (m_dummdataFileSystemWatcher.isNull()) {
        m_dummdataFileSystemWatcher = new QFileSystemWatcher(this);
        connect(m_dummdataFileSystemWatcher.data(), SIGNAL(fileChanged(QString)), this, SLOT(refreshDummyData(QString)));
    }

    return m_dummdataFileSystemWatcher.data();
}

QFileSystemWatcher *NodeInstanceServer::fileSystemWatcher()
{
    if (m_fileSystemWatcher.isNull()) {
        m_fileSystemWatcher = new QFileSystemWatcher(this);
        connect(m_fileSystemWatcher.data(), SIGNAL(fileChanged(QString)), this, SLOT(refreshLocalFileProperty(QString)));
    }

    return m_fileSystemWatcher.data();
}

Internal::ChildrenChangeEventFilter *NodeInstanceServer::childrenChangeEventFilter() const
{
    return m_childrenChangeEventFilter.data();
}

void NodeInstanceServer::addFilePropertyToFileSystemWatcher(QObject *object, const QString &propertyName, const QString &path)
{
    if (!m_fileSystemWatcherHash.contains(path)) {
        m_fileSystemWatcherHash.insert(path, ObjectPropertyPair(object, propertyName));
        fileSystemWatcher()->addPath(path);
    }
}

void NodeInstanceServer::removeFilePropertyFromFileSystemWatcher(QObject *object, const QString &propertyName, const QString &path)
{
    if (m_fileSystemWatcherHash.contains(path)) {
        fileSystemWatcher()->removePath(path);
        m_fileSystemWatcherHash.remove(path, ObjectPropertyPair(object, propertyName));
    }
}

void NodeInstanceServer::refreshLocalFileProperty(const QString &path)
{
    if (m_fileSystemWatcherHash.contains(path)) {
        foreach (const ObjectPropertyPair &objectPropertyPair, m_fileSystemWatcherHash) {
            QObject *object = objectPropertyPair.first.data();
            QString propertyName = objectPropertyPair.second;

            if (hasInstanceForObject(object)) {
                instanceForObject(object).refreshProperty(propertyName);
            }
        }
    }
}

void NodeInstanceServer::refreshDummyData(const QString &path)
{
    engine()->clearComponentCache();
    QFileInfo filePath(path);
    if (filePath.completeBaseName().contains("_dummycontext")) {
        loadDummyContextObjectFile(filePath);
    } else {
        loadDummyDataFile(filePath);
    }

    refreshBindings();
    startRenderTimer();
}

void NodeInstanceServer::addChangedProperty(const InstancePropertyPair &property)
{
    if (!m_changedPropertyList.contains(property))
        m_changedPropertyList.append(property);
}

void NodeInstanceServer::emitParentChanged(QObject *child)
{
    if (hasInstanceForObject(child)) {
        addChangedProperty(InstancePropertyPair(instanceForObject(child), "parent"));
    }
}

Internal::ChildrenChangeEventFilter *NodeInstanceServer::childrenChangeEventFilter()
{
    if (m_childrenChangeEventFilter.isNull()) {
        m_childrenChangeEventFilter = new Internal::ChildrenChangeEventFilter(this);
        connect(m_childrenChangeEventFilter.data(), SIGNAL(childrenChanged(QObject*)), this, SLOT(emitParentChanged(QObject*)));
    }

    return m_childrenChangeEventFilter.data();
}

void NodeInstanceServer::resetInstanceProperty(const PropertyAbstractContainer &propertyContainer)
{
    if (hasInstanceForId(propertyContainer.instanceId())) { // TODO ugly workaround
        ServerNodeInstance instance = instanceForId(propertyContainer.instanceId());
        Q_ASSERT(instance.isValid());

        const QString name = propertyContainer.name();

        if (activeStateInstance().isValid() && !instance.isSubclassOf("QtQuick/PropertyChanges")) {
            bool statePropertyWasReseted = activeStateInstance().resetStateProperty(instance, name, instance.resetVariant(name));
            if (!statePropertyWasReseted)
                instance.resetProperty(name);
        } else {
            instance.resetProperty(name);
        }

        if (propertyContainer.isDynamic() && propertyContainer.instanceId() == 0 && engine())
            rootContext()->setContextProperty(name, QVariant());
    }
}


void NodeInstanceServer::setInstancePropertyBinding(const PropertyBindingContainer &bindingContainer)
{
    if (hasInstanceForId(bindingContainer.instanceId())) {
        ServerNodeInstance instance = instanceForId(bindingContainer.instanceId());

        const QString name = bindingContainer.name();
        const QString expression = bindingContainer.expression();


        if (activeStateInstance().isValid() && !instance.isSubclassOf("QtQuick/PropertyChanges")) {
            bool stateBindingWasUpdated = activeStateInstance().updateStateBinding(instance, name, expression);
            if (!stateBindingWasUpdated) {
                if (bindingContainer.isDynamic())
                    instance.setPropertyDynamicBinding(name, bindingContainer.dynamicTypeName(), expression);
                else
                    instance.setPropertyBinding(name, expression);
            }
        } else {
            if (bindingContainer.isDynamic())
                instance.setPropertyDynamicBinding(name, bindingContainer.dynamicTypeName(), expression);
            else
                instance.setPropertyBinding(name, expression);
        }
    }
}


void NodeInstanceServer::removeProperties(const QList<PropertyAbstractContainer> &propertyList)
{
    foreach (const PropertyAbstractContainer &property, propertyList)
        resetInstanceProperty(property);
}

void NodeInstanceServer::setInstancePropertyVariant(const PropertyValueContainer &valueContainer)
{
    if (hasInstanceForId(valueContainer.instanceId())) {
        ServerNodeInstance instance = instanceForId(valueContainer.instanceId());


        const QString name = valueContainer.name();
        const QVariant value = valueContainer.value();


        if (activeStateInstance().isValid() && !instance.isSubclassOf("QtQuick/PropertyChanges")) {
            bool stateValueWasUpdated = activeStateInstance().updateStateVariant(instance, name, value);
            if (!stateValueWasUpdated) {
                if (valueContainer.isDynamic())
                    instance.setPropertyDynamicVariant(name, valueContainer.dynamicTypeName(), value);
                else
                    instance.setPropertyVariant(name, value);
            }
        } else { //base state
            if (valueContainer.isDynamic())
                instance.setPropertyDynamicVariant(name, valueContainer.dynamicTypeName(), value);
            else
                instance.setPropertyVariant(name, value);
        }

        if (valueContainer.isDynamic() && valueContainer.instanceId() == 0 && engine())
            rootContext()->setContextProperty(name, Internal::ObjectNodeInstance::fixResourcePaths(value));
    }
}

void NodeInstanceServer::setInstanceAuxiliaryData(const PropertyValueContainer &auxiliaryContainer)
{
    //instanceId() == 0: the item is root
    if (auxiliaryContainer.instanceId() == 0 && (auxiliaryContainer.name() == QLatin1String("width") ||
                                        auxiliaryContainer.name() == QLatin1String("height"))) {

        if (!auxiliaryContainer.value().isNull()) {
            setInstancePropertyVariant(auxiliaryContainer);
        } else {
            rootNodeInstance().resetProperty(auxiliaryContainer.name());
        }
    }
    if (auxiliaryContainer.name().endsWith(QLatin1String("@NodeInstance"))) {
        QString propertyName = auxiliaryContainer.name().leftRef(auxiliaryContainer.name().count() - 12).toString();
        if (!auxiliaryContainer.value().isNull()) {
            setInstancePropertyVariant(PropertyValueContainer(auxiliaryContainer.instanceId(),
                                                              propertyName,
                                                              auxiliaryContainer.value(),
                                                              auxiliaryContainer.dynamicTypeName()));
        } else {
            rootNodeInstance().resetProperty(propertyName);
        }
    }
}


QUrl NodeInstanceServer::fileUrl() const
{
    return m_fileUrl;
}

ServerNodeInstance NodeInstanceServer::activeStateInstance() const
{
    return m_activeStateInstance;
}

ServerNodeInstance NodeInstanceServer::rootNodeInstance() const
{
    return m_rootNodeInstance;
}

void NodeInstanceServer::setStateInstance(const ServerNodeInstance &stateInstance)
{
    m_activeStateInstance = stateInstance;
}

void NodeInstanceServer::clearStateInstance()
{
    m_activeStateInstance = ServerNodeInstance();
}

void NodeInstanceServer::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_timer) {
        collectItemChangesAndSendChangeCommands();
    }

    NodeInstanceServerInterface::timerEvent(event);
}

NodeInstanceClientInterface *NodeInstanceServer::nodeInstanceClient() const
{
    return m_nodeInstanceClient;
}

static QVector<InformationContainer> createInformationVector(const QList<ServerNodeInstance> &instanceList, bool initial)
{
    QVector<InformationContainer> informationVector;

    foreach (const ServerNodeInstance &instance, instanceList) {
        informationVector.append(InformationContainer(instance.instanceId(), Position, instance.position()));
        informationVector.append(InformationContainer(instance.instanceId(), Transform, instance.transform()));
        informationVector.append(InformationContainer(instance.instanceId(), SceneTransform, instance.sceneTransform()));
        informationVector.append(InformationContainer(instance.instanceId(), Size, instance.size()));
        informationVector.append(InformationContainer(instance.instanceId(), BoundingRect, instance.boundingRect()));
        informationVector.append(InformationContainer(instance.instanceId(), Transform, instance.transform()));
        informationVector.append(InformationContainer(instance.instanceId(), HasContent, instance.hasContent()));
        informationVector.append(InformationContainer(instance.instanceId(), IsMovable, instance.isMovable()));
        informationVector.append(InformationContainer(instance.instanceId(), IsResizable, instance.isResizable()));
        informationVector.append(InformationContainer(instance.instanceId(), IsInPositioner, instance.isInPositioner()));
        informationVector.append(InformationContainer(instance.instanceId(), PenWidth, instance.penWidth()));
        informationVector.append(InformationContainer(instance.instanceId(), IsAnchoredByChildren, instance.isAnchoredByChildren()));
        informationVector.append(InformationContainer(instance.instanceId(), IsAnchoredBySibling, instance.isAnchoredBySibling()));

        informationVector.append(InformationContainer(instance.instanceId(), HasAnchor, QString("anchors.fill"), instance.hasAnchor("anchors.fill")));
        informationVector.append(InformationContainer(instance.instanceId(), HasAnchor, QString("anchors.centerIn"), instance.hasAnchor("anchors.centerIn")));
        informationVector.append(InformationContainer(instance.instanceId(), HasAnchor, QString("anchors.right"), instance.hasAnchor("anchors.right")));
        informationVector.append(InformationContainer(instance.instanceId(), HasAnchor, QString("anchors.top"), instance.hasAnchor("anchors.top")));
        informationVector.append(InformationContainer(instance.instanceId(), HasAnchor, QString("anchors.left"), instance.hasAnchor("anchors.left")));
        informationVector.append(InformationContainer(instance.instanceId(), HasAnchor, QString("anchors.bottom"), instance.hasAnchor("anchors.bottom")));
        informationVector.append(InformationContainer(instance.instanceId(), HasAnchor, QString("anchors.horizontalCenter"), instance.hasAnchor("anchors.horizontalCenter")));
        informationVector.append(InformationContainer(instance.instanceId(), HasAnchor, QString("anchors.verticalCenter"), instance.hasAnchor("anchors.verticalCenter")));
        informationVector.append(InformationContainer(instance.instanceId(), HasAnchor, QString("anchors.baseline"), instance.hasAnchor("anchors.baseline")));

        QPair<QString, ServerNodeInstance> anchorPair = instance.anchor("anchors.fill");
        informationVector.append(InformationContainer(instance.instanceId(), Anchor, QString("anchors.fill"), anchorPair.first, anchorPair.second.instanceId()));

        anchorPair = instance.anchor("anchors.centerIn");
        informationVector.append(InformationContainer(instance.instanceId(), Anchor, QString("anchors.centerIn"), anchorPair.first, anchorPair.second.instanceId()));

        anchorPair = instance.anchor("anchors.right");
        informationVector.append(InformationContainer(instance.instanceId(), Anchor, QString("anchors.right"), anchorPair.first, anchorPair.second.instanceId()));

        anchorPair = instance.anchor("anchors.top");
        informationVector.append(InformationContainer(instance.instanceId(), Anchor, QString("anchors.top"), anchorPair.first, anchorPair.second.instanceId()));

        anchorPair = instance.anchor("anchors.left");
        informationVector.append(InformationContainer(instance.instanceId(), Anchor, QString("anchors.left"), anchorPair.first, anchorPair.second.instanceId()));

        anchorPair = instance.anchor("anchors.bottom");
        informationVector.append(InformationContainer(instance.instanceId(), Anchor, QString("anchors.bottom"), anchorPair.first, anchorPair.second.instanceId()));

        anchorPair = instance.anchor("anchors.horizontalCenter");
        informationVector.append(InformationContainer(instance.instanceId(), Anchor, QString("anchors.horizontalCenter"), anchorPair.first, anchorPair.second.instanceId()));

        anchorPair = instance.anchor("anchors.verticalCenter");
        informationVector.append(InformationContainer(instance.instanceId(), Anchor, QString("anchors.verticalCenter"), anchorPair.first, anchorPair.second.instanceId()));

        anchorPair = instance.anchor("anchors.baseline");
        informationVector.append(InformationContainer(instance.instanceId(), Anchor, QString("anchors.baseline"), anchorPair.first, anchorPair.second.instanceId()));

        QStringList propertyNames = instance.propertyNames();

        if (initial) {
            foreach (const QString &propertyName,propertyNames)
                informationVector.append(InformationContainer(instance.instanceId(), InstanceTypeForProperty, propertyName, instance.instanceType(propertyName)));
        }

        foreach (const QString &propertyName,instance.propertyNames()) {
            bool hasChanged = false;
            bool hasBinding = instance.hasBindingForProperty(propertyName, &hasChanged);
            if (hasChanged)
                informationVector.append(InformationContainer(instance.instanceId(), HasBindingForProperty, propertyName, hasBinding));
        }

    }

    return informationVector;
}


ChildrenChangedCommand NodeInstanceServer::createChildrenChangedCommand(const ServerNodeInstance &parentInstance, const QList<ServerNodeInstance> &instanceList) const
{
    QVector<qint32> instanceVector;

    foreach (const ServerNodeInstance &instance, instanceList)
        instanceVector.append(instance.instanceId());

    return ChildrenChangedCommand(parentInstance.instanceId(), instanceVector, createInformationVector(instanceList, false));
}

InformationChangedCommand NodeInstanceServer::createAllInformationChangedCommand(const QList<ServerNodeInstance> &instanceList, bool initial) const
{
    return InformationChangedCommand(createInformationVector(instanceList, initial));
}

static bool supportedVariantType(QVariant::Type type)
{
    return type < int(QVariant::UserType) && type != QMetaType::QObjectStar && type != QVariant::Icon;
}

ValuesChangedCommand NodeInstanceServer::createValuesChangedCommand(const QList<ServerNodeInstance> &instanceList) const
{
    QVector<PropertyValueContainer> valueVector;

    foreach (const ServerNodeInstance &instance, instanceList) {
        foreach (const QString &propertyName, instance.propertyNames()) {
            QVariant propertyValue = instance.property(propertyName);
            if (supportedVariantType(propertyValue.type()))
                valueVector.append(PropertyValueContainer(instance.instanceId(), propertyName, propertyValue, QString()));
        }
    }

    return ValuesChangedCommand(valueVector);
}

ComponentCompletedCommand NodeInstanceServer::createComponentCompletedCommand(const QList<ServerNodeInstance> &instanceList)
{
    QVector<qint32> idVector;
    foreach (const ServerNodeInstance &instance, instanceList) {
        if (instance.instanceId() >= 0)
            idVector.append(instance.instanceId());
    }

    return ComponentCompletedCommand(idVector);
}

ValuesChangedCommand NodeInstanceServer::createValuesChangedCommand(const QVector<InstancePropertyPair> &propertyList) const
{
    QVector<PropertyValueContainer> valueVector;

    foreach (const InstancePropertyPair &property, propertyList) {
        const QString propertyName = property.second;
        const ServerNodeInstance instance = property.first;

        if ( instance.isValid()) {
            QVariant propertyValue = instance.property(propertyName);
            if (supportedVariantType(propertyValue.type()))
                valueVector.append(PropertyValueContainer(instance.instanceId(), propertyName, propertyValue, QString()));
        }
    }

    return ValuesChangedCommand(valueVector);
}

QStringList NodeInstanceServer::imports() const
{
    return m_importList;
}

void NodeInstanceServer::addImportString(const QString &import)
{
    m_importList.append(import);
}

QObject *NodeInstanceServer::dummyContextObject() const
{
    return m_dummyContextObject.data();
}

void NodeInstanceServer::sendDebugOutput(DebugOutputCommand::Type type, const QString &message)
{
    DebugOutputCommand command(message, type);
    nodeInstanceClient()->debugOutput(command);
}

void NodeInstanceServer::notifyPropertyChange(qint32 instanceid, const QString &propertyName)
{
    if (hasInstanceForId(instanceid))
        addChangedProperty(InstancePropertyPair(instanceForId(instanceid), propertyName));
}

void NodeInstanceServer::insertInstanceRelationship(const ServerNodeInstance &instance)
{
    Q_ASSERT(instance.isValid());
    Q_ASSERT(!m_idInstanceHash.contains(instance.instanceId()));
    Q_ASSERT(!m_objectInstanceHash.contains(instance.internalObject()));
    m_objectInstanceHash.insert(instance.internalObject(), instance);
    m_idInstanceHash.insert(instance.instanceId(), instance);
}

void NodeInstanceServer::removeInstanceRelationsip(qint32 instanceId)
{
    if (hasInstanceForId(instanceId)) {
        ServerNodeInstance instance = instanceForId(instanceId);
        if (instance.isValid())
            instance.setId(QString());
        m_idInstanceHash.remove(instanceId);
        m_objectInstanceHash.remove(instance.internalObject());
        instance.makeInvalid();
    }
}

PixmapChangedCommand NodeInstanceServer::createPixmapChangedCommand(const QList<ServerNodeInstance> &instanceList) const
{
    QVector<ImageContainer> imageVector;

    foreach (const ServerNodeInstance &instance, instanceList) {
        if (instance.isValid() && instance.hasContent())
            imageVector.append(ImageContainer(instance.instanceId(), instance.renderImage(), instance.instanceId()));
    }

    return PixmapChangedCommand(imageVector);
}

void NodeInstanceServer::loadDummyDataFile(const QFileInfo& qmlFileInfo)
{
    QDeclarativeComponent component(engine(), qmlFileInfo.filePath());
    QObject *dummyData = component.create();
    if (component.isError()) {
        QList<QDeclarativeError> errors = component.errors();
        foreach (const QDeclarativeError &error, errors) {
            qWarning() << error;
        }
    }

    QVariant oldDummyDataObject = rootContext()->contextProperty(qmlFileInfo.completeBaseName());

    if (dummyData) {
        qWarning() << "Loaded dummy data:" << qmlFileInfo.filePath();
        rootContext()->setContextProperty(qmlFileInfo.completeBaseName(), dummyData);
        dummyData->setParent(this);
        m_dummyObjectList.append(DummyPair(qmlFileInfo.completeBaseName(), dummyData));
    }

    if (!oldDummyDataObject.isNull())
        delete oldDummyDataObject.value<QObject*>();

    if (!dummydataFileSystemWatcher()->files().contains(qmlFileInfo.filePath()))
        dummydataFileSystemWatcher()->addPath(qmlFileInfo.filePath());

    if (rootNodeInstance().isValid() && rootNodeInstance().internalObject()) {
        foreach (QDeclarativeContext *context, allSubContextsForObject(rootNodeInstance().internalObject()))
            setupDummysForContext(context);
    }
}

void NodeInstanceServer::loadDummyContextObjectFile(const QFileInfo& qmlFileInfo)
{
    delete m_dummyContextObject.data();

    QDeclarativeComponent component(engine(), qmlFileInfo.filePath());
    m_dummyContextObject = component.create();

    if (component.isError()) {
        QList<QDeclarativeError> errors = component.errors();
        foreach (const QDeclarativeError &error, errors) {
            qWarning() << error;
        }
    }

    if (m_dummyContextObject) {
        qWarning() << "Loaded dummy context object:" << qmlFileInfo.filePath();
        m_dummyContextObject->setParent(this);
    }

    if (!dummydataFileSystemWatcher()->files().contains(qmlFileInfo.filePath()))
        dummydataFileSystemWatcher()->addPath(qmlFileInfo.filePath());

    refreshBindings();
}

void NodeInstanceServer::loadDummyDataFiles(const QString& directory)
{
    QDir dir(directory, "*.qml");
    QList<QFileInfo> filePathList = dir.entryInfoList();
    foreach (const QFileInfo &qmlFileInfo, filePathList) {
        loadDummyDataFile(qmlFileInfo);
    }
}

void NodeInstanceServer::loadDummyDataContext(const QString& directory)
{
    QDir dir(directory+"/context", "*.qml");
    QList<QFileInfo> filePathList = dir.entryInfoList();
    QString baseName = QFileInfo(fileUrl().toLocalFile()).completeBaseName();
    foreach (const QFileInfo &qmlFileInfo, filePathList) {
        if (qmlFileInfo.completeBaseName() == baseName)
            loadDummyContextObjectFile(qmlFileInfo);
    }
}

QStringList NodeInstanceServer::dummyDataDirectories(const QString& directoryPath)
{
    QStringList dummyDataDirectoryList;
    QDir directory(directoryPath);
    while(true) {
        if (directory.isRoot() || !directory.exists())
            return dummyDataDirectoryList;

        if (directory.exists("dummydata"))
            dummyDataDirectoryList.prepend(directory.absoluteFilePath("dummydata"));

        directory.cdUp();
    }
}

}



