/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "nodeinstanceserver.h"

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
#include "addimportcommand.h"
#include "childrenchangedcommand.h"
#include "completecomponentcommand.h"
#include "componentcompletedcommand.h"
#include "createscenecommand.h"

#include "dummycontextobject.h"

#include <iostream>
#include <stdio.h>


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

    m_importList.append("import Qt 4.7\n");
    connect(m_childrenChangeEventFilter.data(), SIGNAL(childrenChanged(QObject*)), this, SLOT(emitParentChanged(QObject*)));
}

NodeInstanceServer::~NodeInstanceServer()
{
    delete m_declarativeView.data();
}

QList<ServerNodeInstance>  NodeInstanceServer::createInstances(const QVector<InstanceContainer> &containerVector)
{
    Q_ASSERT(m_declarativeView);
    QList<ServerNodeInstance> instanceList;
    foreach(const InstanceContainer &instanceContainer, containerVector) {
        ServerNodeInstance instance = ServerNodeInstance::create(this, instanceContainer);
        insertInstanceRelationship(instance);
        instanceList.append(instance);
        instance.internalObject()->installEventFilter(childrenChangeEventFilter());
        if (instanceContainer.instanceId() == 0) {
            m_rootNodeInstance = instance;
            QGraphicsObject *rootGraphicsObject = qobject_cast<QGraphicsObject*>(instance.internalObject());
            if (rootGraphicsObject) {
                m_declarativeView->scene()->addItem(rootGraphicsObject);
                m_declarativeView->setSceneRect(rootGraphicsObject->boundingRect());
            }

        }
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
    initializeDeclarativeView();
    QList<ServerNodeInstance> instanceList = setupScene(command);

    refreshBindings();

    nodeInstanceClient()->informationChanged(createAllInformationChangedCommand(instanceList, true));
    nodeInstanceClient()->valuesChanged(createValuesChangedCommand(instanceList));
    sendChildrenChangedCommand(instanceList);
    nodeInstanceClient()->pixmapChanged(createPixmapChangedCommand(instanceList));
    nodeInstanceClient()->componentCompleted(createComponentCompletedCommand(instanceList));

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

    delete m_declarativeView.data();
}

void NodeInstanceServer::removeInstances(const RemoveInstancesCommand &command)
{
    if (activeStateInstance().isValid())
        activeStateInstance().deactivateState();

    foreach(qint32 instanceId, command.instanceIds()) {
        removeInstanceRelationsip(instanceId);
    }

    if (activeStateInstance().isValid())
        activeStateInstance().activateState();


    refreshBindings();
    startRenderTimer();
}

void NodeInstanceServer::removeProperties(const RemovePropertiesCommand &command)
{
    bool hasDynamicProperties = false;
    foreach(const PropertyAbstractContainer &container, command.properties()) {
        hasDynamicProperties |= container.isDynamic();
        resetInstanceProperty(container);
    }

    if (hasDynamicProperties)
        refreshBindings();

    startRenderTimer();
}

void NodeInstanceServer::reparentInstances(const QVector<ReparentContainer> &containerVector)
{
    foreach(const ReparentContainer &container, containerVector) {
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

    foreach(qint32 instanceId, command.instances()) {
        if (hasInstanceForId(instanceId)) {
            ServerNodeInstance instance = instanceForId(instanceId);
            instance.doComponentComplete();
            instanceList.append(instance);
        }
    }

    m_completedComponentList.append(instanceList);
    refreshBindings();

    nodeInstanceClient()->valuesChanged(createValuesChangedCommand(instanceList));
    nodeInstanceClient()->informationChanged(createAllInformationChangedCommand(instanceList, true));
    nodeInstanceClient()->pixmapChanged(createPixmapChangedCommand(instanceList));

    startRenderTimer();
}

void NodeInstanceServer::addImports(const QVector<AddImportContainer> &containerVector)
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

        foreach(const QString &importPath, container.importPaths()) { // this is simply ugly
            engine()->addImportPath(importPath);
            engine()->addPluginPath(importPath);
        }
    }

    delete m_importComponent.data();
    delete m_importComponentObject.data();

    m_importComponent = new QDeclarativeComponent(engine(), 0);
    QString componentString;
    foreach(const QString &importStatement, m_importList)
        componentString += QString("%1").arg(importStatement);

    componentString += QString("Item {}\n");

    m_importComponent->setData(componentString.toUtf8(), fileUrl());
    m_importComponentObject = m_importComponent->create();

    if (!m_importComponent->errorString().isEmpty())
        qDebug() << "QmlDesigner.NodeInstances: import wrong: " << m_importComponent->errorString();
}

void NodeInstanceServer::addImport(const AddImportCommand &command)
{
    addImports(QVector<AddImportContainer>() << command.import());
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
    foreach(const PropertyValueContainer &container, command.valueChanges()) {
        hasDynamicProperties |= container.isDynamic();
        setInstancePropertyVariant(container);
    }

    if (hasDynamicProperties)
        refreshBindings();

    startRenderTimer();
}


void NodeInstanceServer::changePropertyBindings(const ChangeBindingsCommand &command)
{
    bool hasDynamicProperties = false;
    foreach(const PropertyBindingContainer &container, command.bindingChanges()) {
        hasDynamicProperties |= container.isDynamic();
        setInstancePropertyBinding(container);
    }

    if (hasDynamicProperties)
        refreshBindings();

    startRenderTimer();
}

void NodeInstanceServer::changeIds(const ChangeIdsCommand &command)
{
    foreach(const IdContainer &container, command.ids()) {
        if (hasInstanceForId(container.instanceId()))
            instanceForId(container.instanceId()).setId(container.id());
    }

    refreshBindings();
    startRenderTimer();
}

QDeclarativeEngine *NodeInstanceServer::engine() const
{
    if (m_declarativeView)
        return m_declarativeView->engine();

    return 0;
}

QDeclarativeContext *NodeInstanceServer::context() const
{
    if (m_importComponentObject) {
        QDeclarativeContext *importComponentContext = QDeclarativeEngine::contextForObject(m_importComponentObject.data());
        if (importComponentContext) // this should be the default
            return importComponentContext;
    }

    if (engine())
        return engine()->rootContext();

    return 0;
}

QDeclarativeView *NodeInstanceServer::delcarativeView() const
{
    return m_declarativeView.data();
}

const QVector<NodeInstanceServer::InstancePropertyPair> NodeInstanceServer::changedPropertyList() const
{
    return m_changedPropertyList;
}

void NodeInstanceServer::clearChangedPropertyList()
{
    m_changedPropertyList.clear();
}

void NodeInstanceServer::refreshBindings()
{
    static int counter = 0;
    engine()->rootContext()->setContextProperty(QString("__%1").arg(counter++), 0); // refreshing bindings
}

void NodeInstanceServer::removeAllInstanceRelationships()
{
    // prevent destroyed() signals calling back

    foreach (ServerNodeInstance instance, m_objectInstanceHash.values()) {
        if (instance.isValid())
            instance.setId(QString());
    }

    //first  the root object
    if (rootNodeInstance().internalObject())
        rootNodeInstance().internalObject()->disconnect();

    rootNodeInstance().makeInvalid();


    foreach (ServerNodeInstance instance, m_objectInstanceHash.values()) {
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
        QList<ObjectPropertyPair> objectPropertyPairList = m_fileSystemWatcherHash.values();
        foreach(const ObjectPropertyPair &objectPropertyPair, objectPropertyPairList) {
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

        if (activeStateInstance().isValid() && !instance.isSubclassOf("Qt/PropertyChanges")) {
            bool statePropertyWasReseted = activeStateInstance().resetStateProperty(instance, name, instance.resetVariant(name));
            if (!statePropertyWasReseted)
                instance.resetProperty(name);
        } else {
            instance.resetProperty(name);
        }
    }
}


void NodeInstanceServer::setInstancePropertyBinding(const PropertyBindingContainer &bindingContainer)
{
    if (hasInstanceForId(bindingContainer.instanceId())) {
        ServerNodeInstance instance = instanceForId(bindingContainer.instanceId());

        const QString name = bindingContainer.name();
        const QString expression = bindingContainer.expression();


        if (activeStateInstance().isValid() && !instance.isSubclassOf("Qt/PropertyChanges")) {
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


        if (activeStateInstance().isValid() && !instance.isSubclassOf("Qt/PropertyChanges")) {
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

//        instance.paintUpdate();
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
        findItemChangesAndSendChangeCommands();
    }

    NodeInstanceServerInterface::timerEvent(event);
}

NodeInstanceClientInterface *NodeInstanceServer::nodeInstanceClient() const
{
    return m_nodeInstanceClient;
}

void NodeInstanceServer::sendChildrenChangedCommand(const QList<ServerNodeInstance> childList)
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

ChildrenChangedCommand NodeInstanceServer::createChildrenChangedCommand(const ServerNodeInstance &parentInstance, const QList<ServerNodeInstance> &instanceList) const
{
    QVector<qint32> instanceVector;

    foreach(const ServerNodeInstance &instance, instanceList)
        instanceVector.append(instance.instanceId());

    return ChildrenChangedCommand(parentInstance.instanceId(), instanceVector);
}

InformationChangedCommand NodeInstanceServer::createAllInformationChangedCommand(const QList<ServerNodeInstance> &instanceList, bool initial) const
{
    QVector<InformationContainer> informationVector;

    foreach(const ServerNodeInstance &instance, instanceList) {
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

    return InformationChangedCommand(informationVector);
}

ValuesChangedCommand NodeInstanceServer::createValuesChangedCommand(const QList<ServerNodeInstance> &instanceList) const
{
    QVector<PropertyValueContainer> valueVector;

    foreach(const ServerNodeInstance &instance, instanceList) {
        foreach(const QString &propertyName, instance.propertyNames()) {
            QVariant propertyValue = instance.property(propertyName);
            if (propertyValue.type() < QVariant::UserType)
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

        if( instance.isValid()) {
            QVariant propertyValue = instance.property(propertyName);
            if (propertyValue.type() < QVariant::UserType)
                valueVector.append(PropertyValueContainer(instance.instanceId(), propertyName, propertyValue, QString()));
        }
    }

    return ValuesChangedCommand(valueVector);
}

QStringList NodeInstanceServer::imports() const
{
    return m_importList;
}

QObject *NodeInstanceServer::dummyContextObject() const
{
    return m_dummyContextObject.data();
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
        if (instance.isValid())
            imageVector.append(ImageContainer(instance.instanceId(), instance.renderImage()));
    }

    return PixmapChangedCommand(imageVector);
}

bool NodeInstanceServer::nonInstanceChildIsDirty(QGraphicsObject *graphicsObject) const
{
    QGraphicsItemPrivate *d = QGraphicsItemPrivate::get(graphicsObject);
    if (d->dirtyChildren) {
        foreach(QGraphicsItem *child, graphicsObject->childItems()) {
            QGraphicsObject *childGraphicsObject = child->toGraphicsObject();
            if (hasInstanceForObject(childGraphicsObject))
                continue;

            QGraphicsItemPrivate *childPrivate = QGraphicsItemPrivate::get(child);
            if (childPrivate->dirty || nonInstanceChildIsDirty(childGraphicsObject))
                return true;
        }
    }

    return false;
}

void NodeInstanceServer::resetAllItems()
{
//     m_declarativeView->scene()->update();
//    m_declarativeView->viewport()->repaint();
    static_cast<QGraphicsScenePrivate*>(QObjectPrivate::get(m_declarativeView->scene()))->processDirtyItemsEmitted = true;

    foreach (QGraphicsItem *item, m_declarativeView->items())
         static_cast<QGraphicsScenePrivate*>(QObjectPrivate::get(m_declarativeView->scene()))->resetDirtyItem(item);
}

void NodeInstanceServer::initializeDeclarativeView()
{
    Q_ASSERT(!m_declarativeView.data());

    m_declarativeView = new QDeclarativeView;
#ifndef Q_WS_MAC
    m_declarativeView->setAttribute(Qt::WA_DontShowOnScreen, true);
#endif
    m_declarativeView->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    m_declarativeView->show();
#ifdef Q_WS_MAC
    m_declarativeView->setAttribute(Qt::WA_DontShowOnScreen, true);
#endif
    QUnifiedTimer::instance()->setSlowdownFactor(0.00001);
    QUnifiedTimer::instance()->setSlowModeEnabled(true);
}

QImage NodeInstanceServer::renderPreviewImage()
{
    QSize size = rootNodeInstance().boundingRect().size().toSize();
    size.scale(100, 100, Qt::KeepAspectRatio);

    QImage image(size, QImage::Format_ARGB32);
    image.fill(0xFFFFFFFF);

    if (m_declarativeView) {
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
        painter.setRenderHint(QPainter::NonCosmeticDefaultPen, true);

        m_declarativeView->scene()->render(&painter, image.rect(), rootNodeInstance().boundingRect().toRect(), Qt::IgnoreAspectRatio);
    }

    return image;
}

void NodeInstanceServer::loadDummyDataFile(const QFileInfo& qmlFileInfo)
{
    QDeclarativeComponent component(engine(), qmlFileInfo.filePath());
    QObject *dummyData = component.create();
    if(component.isError()) {
        QList<QDeclarativeError> errors = component.errors();
        foreach (const QDeclarativeError &error, errors) {
            qWarning() << error;
        }
    }

    QVariant oldDummyDataObject = m_declarativeView->rootContext()->contextProperty(qmlFileInfo.completeBaseName());

    if (dummyData) {
        qWarning() << "Loaded dummy data:" << qmlFileInfo.filePath();
        m_declarativeView->rootContext()->setContextProperty(qmlFileInfo.completeBaseName(), dummyData);
        dummyData->setParent(this);
    }

    if (!oldDummyDataObject.isNull())
        delete oldDummyDataObject.value<QObject*>();

    if (!dummydataFileSystemWatcher()->files().contains(qmlFileInfo.filePath()))
        dummydataFileSystemWatcher()->addPath(qmlFileInfo.filePath());
}

void NodeInstanceServer::loadDummyContextObjectFile(const QFileInfo& qmlFileInfo)
{
    delete m_dummyContextObject.data();

    QDeclarativeComponent component(engine(), qmlFileInfo.filePath());
    m_dummyContextObject = component.create();

    if(component.isError()) {
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
    QString baseName = QFileInfo(fileUrl().toLocalFile()).completeBaseName();
    foreach (const QFileInfo &qmlFileInfo, filePathList) {
        if (!qmlFileInfo.completeBaseName().contains("_dummycontext")) {
            loadDummyDataFile(qmlFileInfo);
        } else if (qmlFileInfo.completeBaseName() == baseName+"_dummycontext") {
            loadDummyContextObjectFile(qmlFileInfo);
        }
    }
}

QStringList dummyDataDirectories(const QString& directoryPath)
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

QList<ServerNodeInstance> NodeInstanceServer::setupScene(const CreateSceneCommand &command)
{
    if (!command.fileUrl().isEmpty()) {
        engine()->setBaseUrl(command.fileUrl());
        m_fileUrl = command.fileUrl();
    }

    addImports(command.imports());

    if (!command.fileUrl().isEmpty()) {
        QStringList dummyDataDirectoryList = dummyDataDirectories(QFileInfo(command.fileUrl().toLocalFile()).path());
        foreach(const QString &dummyDataDirectory, dummyDataDirectoryList)
            loadDummyDataFiles(dummyDataDirectory);
    }

    static_cast<QGraphicsScenePrivate*>(QObjectPrivate::get(m_declarativeView->scene()))->processDirtyItemsEmitted = true;

    QList<ServerNodeInstance> instanceList = createInstances(command.instances());
    reparentInstances(command.reparentInstances());

    foreach(const IdContainer &container, command.ids()) {
        if (hasInstanceForId(container.instanceId()))
            instanceForId(container.instanceId()).setId(container.id());
    }

    foreach(const PropertyValueContainer &container, command.valueChanges()) {
        if (container.isDynamic())
            setInstancePropertyVariant(container);
    }

    foreach(const PropertyValueContainer &container, command.valueChanges()) {
        if (!container.isDynamic())
            setInstancePropertyVariant(container);
    }

    foreach(const PropertyBindingContainer &container, command.bindingChanges()) {
        if (container.isDynamic())
            setInstancePropertyBinding(container);
    }

    foreach(const PropertyBindingContainer &container, command.bindingChanges()) {
        if (!container.isDynamic())
            setInstancePropertyBinding(container);
    }

    foreach(ServerNodeInstance instance, instanceList)
        instance.doComponentComplete();

    m_declarativeView->scene()->setSceneRect(rootNodeInstance().boundingRect());

    return instanceList;
}

void NodeInstanceServer::findItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;
    if (!inFunction) {
        inFunction = true;

        QSet<ServerNodeInstance> informationChangedInstanceSet;
        QVector<InstancePropertyPair> propertyChangedList;
        QSet<ServerNodeInstance> parentChangedSet;
        bool adjustSceneRect = false;

        if (m_declarativeView) {
            foreach (QGraphicsItem *item, m_declarativeView->items()) {
                QGraphicsObject *graphicsObject = item->toGraphicsObject();
                if (graphicsObject && hasInstanceForObject(graphicsObject)) {
                    ServerNodeInstance instance = instanceForObject(graphicsObject);
                    QGraphicsItemPrivate *d = QGraphicsItemPrivate::get(item);

                    if (d->dirtySceneTransform || d->geometryChanged || d->dirty)
                        informationChangedInstanceSet.insert(instance);

                    if (d->geometryChanged) {
                        if (instance.isRootNodeInstance())
                            m_declarativeView->scene()->setSceneRect(item->boundingRect());
                    }

                }
            }

            foreach (const InstancePropertyPair& property, m_changedPropertyList) {
                const ServerNodeInstance instance = property.first;
                const QString propertyName = property.second;

                if (instance.isRootNodeInstance() && (propertyName == "width" || propertyName == "height"))
                    adjustSceneRect = true;

                if (propertyName.contains("anchors") && informationChangedInstanceSet.contains(instance))
                    informationChangedInstanceSet.insert(instance);

                if (propertyName == "parent") {
                    informationChangedInstanceSet.insert(instance);
                    parentChangedSet.insert(instance);
                } else {
                    propertyChangedList.append(property);
                }
            }

            m_changedPropertyList.clear();
            resetAllItems();

            if (!informationChangedInstanceSet.isEmpty())
                nodeInstanceClient()->informationChanged(createAllInformationChangedCommand(informationChangedInstanceSet.toList()));

            if (!propertyChangedList.isEmpty())
                nodeInstanceClient()->valuesChanged(createValuesChangedCommand(propertyChangedList));

            if (!parentChangedSet.isEmpty())
                sendChildrenChangedCommand(parentChangedSet.toList());

            if (adjustSceneRect) {
                QRectF boundingRect = m_rootNodeInstance.boundingRect();
                if (boundingRect.isValid()) {
                    m_declarativeView->setSceneRect(boundingRect);
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
}



