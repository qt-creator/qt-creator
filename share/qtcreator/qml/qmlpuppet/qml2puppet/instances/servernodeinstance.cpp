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
#include "servernodeinstance.h"

#include "objectnodeinstance.h"
#include "dummynodeinstance.h"
#include "componentnodeinstance.h"
#include "qmltransitionnodeinstance.h"
#include "qmlpropertychangesnodeinstance.h"
#include "behaviornodeinstance.h"
#include "qmlstatenodeinstance.h"
#include "anchorchangesnodeinstance.h"
#include "positionernodeinstance.h"
#include "debugoutputcommand.h"

#include "quickitemnodeinstance.h"

#include "nodeinstanceserver.h"
#include "instancecontainer.h"

#include <QHash>
#include <QSet>
#include <QDebug>
#include <QQuickItem>
#include <private/qqmlengine_p.h>

#include <QQmlEngine>

/*!
  \class QmlDesigner::NodeInstance
  \ingroup CoreInstance
  \brief NodeInstance is a common handle for the actual object representation of a ModelNode.

   NodeInstance abstracts away the differences e.g. in terms of position and size
   for QWidget, QGraphicsView, QLayout etc objects. Multiple NodeInstance objects can share
   the pointer to the same instance object. The actual instance will be deleted when
   the last NodeInstance object referencing to it is deleted. This can be disabled by
   setDeleteHeldInstance().

   \see QmlDesigner::NodeInstanceView
*/

namespace QmlDesigner {

/*!
\brief Constructor - creates a invalid NodeInstance


\see NodeInstanceView
*/
ServerNodeInstance::ServerNodeInstance()
{
}

/*!
\brief Destructor

*/
ServerNodeInstance::~ServerNodeInstance()
{
}

/*!
\brief Constructor - creates a valid NodeInstance

*/
ServerNodeInstance::ServerNodeInstance(const Internal::ObjectNodeInstance::Pointer &abstractInstance)
  : m_nodeInstance(abstractInstance)
{

}


ServerNodeInstance::ServerNodeInstance(const ServerNodeInstance &other)
  : m_nodeInstance(other.m_nodeInstance)
{
}

ServerNodeInstance &ServerNodeInstance::operator=(const ServerNodeInstance &other)
{
    m_nodeInstance = other.m_nodeInstance;
    return *this;
}

/*!
\brief Paints the NodeInstance with this painter.
\param painter used QPainter
*/
void ServerNodeInstance::paint(QPainter *painter)
{
    m_nodeInstance->paint(painter);
}

QImage ServerNodeInstance::renderImage() const
{
    return m_nodeInstance->renderImage();
}

bool ServerNodeInstance::isRootNodeInstance() const
{
    return isValid() && m_nodeInstance->isRootNodeInstance();
}

bool ServerNodeInstance::isSubclassOf(QObject *object, const QByteArray &superTypeName)
{
    if (object == 0)
        return false;

    const QMetaObject *metaObject = object->metaObject();

    while (metaObject) {
         QQmlType *qmlType =  QQmlMetaType::qmlType(metaObject);
         if (qmlType && qmlType->qmlTypeName() == superTypeName) // ignore version numbers
             return true;

         if (metaObject->className() == superTypeName)
             return true;

         metaObject = metaObject->superClass();
    }

    return false;
}

void ServerNodeInstance::setNodeSource(const QString &source)
{
    m_nodeInstance->setNodeSource(source);
}

bool ServerNodeInstance::isSubclassOf(const QString &superTypeName) const
{
    return isSubclassOf(internalObject(), superTypeName.toUtf8());
}

/*!
\brief Creates a new NodeInstace for this NodeMetaInfo

\param metaInfo MetaInfo for which a Instance should be created
\param context QQmlContext which should be used
\returns Internal Pointer of a NodeInstance
\see NodeMetaInfo
*/
Internal::ObjectNodeInstance::Pointer ServerNodeInstance::createInstance(QObject *objectToBeWrapped)
{
    Internal::ObjectNodeInstance::Pointer instance;

    if (objectToBeWrapped == 0)
        instance = Internal::DummyNodeInstance::create();
    else if (isSubclassOf(objectToBeWrapped, "QQuickBasePositioner"))
        instance = Internal::PositionerNodeInstance::create(objectToBeWrapped);
    else if (isSubclassOf(objectToBeWrapped, "QQuickItem"))
        instance = Internal::QuickItemNodeInstance::create(objectToBeWrapped);
    else if (isSubclassOf(objectToBeWrapped, "QQmlComponent"))
        instance = Internal::ComponentNodeInstance::create(objectToBeWrapped);
    else if (objectToBeWrapped->inherits("QQmlAnchorChanges"))
        instance = Internal::AnchorChangesNodeInstance::create(objectToBeWrapped);
    else if (isSubclassOf(objectToBeWrapped, "QQuickPropertyChanges"))
        instance = Internal::QmlPropertyChangesNodeInstance::create(objectToBeWrapped);
    else if (isSubclassOf(objectToBeWrapped, "QQuickState"))
        instance = Internal::QmlStateNodeInstance::create(objectToBeWrapped);
    else if (isSubclassOf(objectToBeWrapped, "QQuickTransition"))
        instance = Internal::QmlTransitionNodeInstance::create(objectToBeWrapped);
    else if (isSubclassOf(objectToBeWrapped, "QQuickBehavior"))
        instance = Internal::BehaviorNodeInstance::create(objectToBeWrapped);
    else if (isSubclassOf(objectToBeWrapped, "QObject"))
        instance = Internal::ObjectNodeInstance::create(objectToBeWrapped);
    else
        instance = Internal::DummyNodeInstance::create();


    return instance;
}

ServerNodeInstance ServerNodeInstance::create(NodeInstanceServer *nodeInstanceServer, const InstanceContainer &instanceContainer, ComponentWrap componentWrap)
{
    Q_ASSERT(instanceContainer.instanceId() != -1);
    Q_ASSERT(nodeInstanceServer);

    QObject *object = 0;
    if (componentWrap == WrapAsComponent) {
        object = Internal::ObjectNodeInstance::createComponentWrap(instanceContainer.nodeSource(), nodeInstanceServer->imports(), nodeInstanceServer->context());
    } else if (!instanceContainer.nodeSource().isEmpty()) {
        object = Internal::ObjectNodeInstance::createCustomParserObject(instanceContainer.nodeSource(), nodeInstanceServer->imports(), nodeInstanceServer->context());
        if (object == 0)
            nodeInstanceServer->sendDebugOutput(DebugOutputCommand::ErrorType, QLatin1String("Custom parser object could not be created."));
    } else if (!instanceContainer.componentPath().isEmpty()) {
        object = Internal::ObjectNodeInstance::createComponent(instanceContainer.componentPath(), nodeInstanceServer->context());
        if (object == 0)
            nodeInstanceServer->sendDebugOutput(DebugOutputCommand::ErrorType, QString("Component with path %1 could not be created.").arg(instanceContainer.componentPath()));
    } else {
        object = Internal::ObjectNodeInstance::createPrimitive(instanceContainer.type(), instanceContainer.majorNumber(), instanceContainer.minorNumber(), nodeInstanceServer->context());
        if (object == 0)
            nodeInstanceServer->sendDebugOutput(DebugOutputCommand::ErrorType, QLatin1String("Item could not be created."));
    }

    if (object == 0) {
        if (instanceContainer.metaType() == InstanceContainer::ItemMetaType) { //If we cannot instanciate the object but we know it has to be an Ttem, we create an Item instead.
            object = Internal::ObjectNodeInstance::createPrimitive("QtQuick/Item", 2, 0, nodeInstanceServer->context());

            if (object == 0)
                object = new QQuickItem;
        } else {
            object = new QObject;
        }
   }


    QQmlEnginePrivate::get(nodeInstanceServer->engine())->cache(object->metaObject());

    ServerNodeInstance instance(createInstance(object));

    instance.internalInstance()->setNodeInstanceServer(nodeInstanceServer);

    instance.internalInstance()->setInstanceId(instanceContainer.instanceId());

    instance.internalInstance()->initialize(instance.m_nodeInstance);

    //QObject::connect(instance.internalObject(), SIGNAL(destroyed(QObject*)), nodeInstanceView, SLOT(removeIdFromContext(QObject*)));

    return instance;
}

void ServerNodeInstance::reparent(const ServerNodeInstance &oldParentInstance, const QString &oldParentProperty, const ServerNodeInstance &newParentInstance, const QString &newParentProperty)
{
    m_nodeInstance->reparent(oldParentInstance.m_nodeInstance, oldParentProperty, newParentInstance.m_nodeInstance, newParentProperty);
}

/*!
\brief Returns the parent NodeInstance of this NodeInstance.

    If there is not parent than the parent is invalid.

\returns Parent NodeInstance.
*/
ServerNodeInstance ServerNodeInstance::parent() const
{
    return m_nodeInstance->parentInstance();
}

bool ServerNodeInstance::hasParent() const
{
    return m_nodeInstance->parent();
}

bool ServerNodeInstance::isValid() const
{
    return m_nodeInstance && m_nodeInstance->isValid();
}


/*!
\brief Returns if the NodeInstance is a QGraphicsItem.
\returns true if this NodeInstance is a QGraphicsItem
*/
bool ServerNodeInstance::equalGraphicsItem(QGraphicsItem *item) const
{
    return m_nodeInstance->equalGraphicsItem(item);
}

/*!
\brief Returns the bounding rect of the NodeInstance.
\returns QRectF of the NodeInstance
*/
QRectF ServerNodeInstance::boundingRect() const
{
    QRectF boundingRect(m_nodeInstance->boundingRect());

//
//    if (modelNode().isValid()) { // TODO implement recursiv stuff
//        if (qFuzzyIsNull(boundingRect.width()))
//            boundingRect.setWidth(nodeState().property("width").value().toDouble());
//
//        if (qFuzzyIsNull(boundingRect.height()))
//            boundingRect.setHeight(nodeState().property("height").value().toDouble());
//    }

    return boundingRect;
}

void ServerNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    m_nodeInstance->setPropertyVariant(name, value);

}

void ServerNodeInstance::setPropertyDynamicVariant(const QString &name, const QString &typeName, const QVariant &value)
{
    m_nodeInstance->createDynamicProperty(name, typeName);
    m_nodeInstance->setPropertyVariant(name, value);
}

void ServerNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    m_nodeInstance->setPropertyBinding(name, expression);
}

void ServerNodeInstance::setPropertyDynamicBinding(const QString &name, const QString &typeName, const QString &expression)
{
    m_nodeInstance->createDynamicProperty(name, typeName);
    m_nodeInstance->setPropertyBinding(name, expression);
}

void ServerNodeInstance::resetProperty(const QString &name)
{
    m_nodeInstance->resetProperty(name);
}

void ServerNodeInstance::refreshProperty(const QString &name)
{
    m_nodeInstance->refreshProperty(name);
}

void ServerNodeInstance::setId(const QString &id)
{
    m_nodeInstance->setId(id);
}

/*!
\brief Returns the property value of the property of this NodeInstance.
\returns QVariant value
*/
QVariant ServerNodeInstance::property(const QString &name) const
{
    return m_nodeInstance->property(name);
}

QStringList ServerNodeInstance::propertyNames() const
{
    return m_nodeInstance->propertyNames();
}

bool ServerNodeInstance::hasBindingForProperty(const QString &name, bool *hasChanged) const
{
    return m_nodeInstance->hasBindingForProperty(name, hasChanged);
}

/*!
\brief Returns the property default value of the property of this NodeInstance.
\returns QVariant default value which is the reset value to
*/
QVariant ServerNodeInstance::defaultValue(const QString &name) const
{
    return m_nodeInstance->resetValue(name);
}

/*!
\brief Returns the type of the property of this NodeInstance.
*/
QString ServerNodeInstance::instanceType(const QString &name) const
{
    return m_nodeInstance->instanceType(name);
}

void ServerNodeInstance::makeInvalid()
{
    if (m_nodeInstance)
        m_nodeInstance->destroy();
    m_nodeInstance.clear();
}

bool ServerNodeInstance::hasContent() const
{
    return m_nodeInstance->hasContent();
}

bool ServerNodeInstance::isResizable() const
{
    return m_nodeInstance->isResizable();
}

bool ServerNodeInstance::isMovable() const
{
    return m_nodeInstance->isMovable();
}

bool ServerNodeInstance::isInPositioner() const
{
    return m_nodeInstance->isInPositioner();
}

bool ServerNodeInstance::hasAnchor(const QString &name) const
{
    return m_nodeInstance->hasAnchor(name);
}

int ServerNodeInstance::penWidth() const
{
    return m_nodeInstance->penWidth();
}

bool ServerNodeInstance::isAnchoredBySibling() const
{
    return m_nodeInstance->isAnchoredBySibling();
}

bool ServerNodeInstance::isAnchoredByChildren() const
{
    return m_nodeInstance->isAnchoredByChildren();
}

QPair<QString, ServerNodeInstance> ServerNodeInstance::anchor(const QString &name) const
{
    return m_nodeInstance->anchor(name);
}

QDebug operator<<(QDebug debug, const ServerNodeInstance &instance)
{
    if (instance.isValid()) {
        debug.nospace() << "ServerNodeInstance("
                << instance.instanceId() << ", "
                << instance.internalObject() << ", "
                << instance.id() << ", "
                << instance.parent() << ')';
    } else {
        debug.nospace() << "ServerNodeInstance(invalid)";
    }

    return debug.space();
}

uint qHash(const ServerNodeInstance &instance)
{
    return ::qHash(instance.instanceId());
}

bool operator==(const ServerNodeInstance &first, const ServerNodeInstance &second)
{
    return first.instanceId() == second.instanceId();
}

bool ServerNodeInstance::isWrappingThisObject(QObject *object) const
{
    return internalObject() && internalObject() == object;
}

/*!
\brief Returns the position in parent coordiantes.
\returns QPointF of the position of the instance.
*/
QPointF ServerNodeInstance::position() const
{
    return m_nodeInstance->position();
}

/*!
\brief Returns the size in local coordiantes.
\returns QSizeF of the size of the instance.
*/
QSizeF ServerNodeInstance::size() const
{
    QSizeF instanceSize = m_nodeInstance->size();

    return instanceSize;
}

QTransform ServerNodeInstance::transform() const
{
    return m_nodeInstance->transform();
}

/*!
\brief Returns the transform matrix of the instance.
\returns QTransform of the instance.
*/
QTransform ServerNodeInstance::customTransform() const
{
    return m_nodeInstance->customTransform();
}

QTransform ServerNodeInstance::sceneTransform() const
{
    return m_nodeInstance->sceneTransform();
}

double ServerNodeInstance::rotation() const
{
    return m_nodeInstance->rotation();
}

double ServerNodeInstance::scale() const
{
    return m_nodeInstance->scale();
}

QList<QGraphicsTransform *> ServerNodeInstance::transformations() const
{
    return m_nodeInstance->transformations();
}

QPointF ServerNodeInstance::transformOriginPoint() const
{
    return m_nodeInstance->transformOriginPoint();
}

double ServerNodeInstance::zValue() const
{
    return m_nodeInstance->zValue();
}

/*!
\brief Returns the opacity of the instance.
\returns 0.0 mean transparent and 1.0 opaque.
*/
double ServerNodeInstance::opacity() const
{
    return m_nodeInstance->opacity();
}


void ServerNodeInstance::setDeleteHeldInstance(bool deleteInstance)
{
    m_nodeInstance->setDeleteHeldInstance(deleteInstance);
}


void ServerNodeInstance::paintUpdate()
{
    m_nodeInstance->paintUpdate();
}

QObject *ServerNodeInstance::internalObject() const
{
    if (m_nodeInstance.isNull())
        return 0;

    return m_nodeInstance->object();
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
QQuickItem *ServerNodeInstance::internalSGItem() const
{
    return qobject_cast<QQuickItem*>(internalObject());
}
#endif

void ServerNodeInstance::activateState()
{
    m_nodeInstance->activateState();
}

void ServerNodeInstance::deactivateState()
{
    m_nodeInstance->deactivateState();
}

bool ServerNodeInstance::updateStateVariant(const ServerNodeInstance &target, const QString &propertyName, const QVariant &value)
{
    return m_nodeInstance->updateStateVariant(target.internalInstance(), propertyName, value);
}

bool ServerNodeInstance::updateStateBinding(const ServerNodeInstance &target, const QString &propertyName, const QString &expression)
{
    return m_nodeInstance->updateStateBinding(target.internalInstance(), propertyName, expression);
}

QVariant ServerNodeInstance::resetVariant(const QString &propertyName) const
{
    return m_nodeInstance->resetValue(propertyName);
}

bool ServerNodeInstance::resetStateProperty(const ServerNodeInstance &target, const QString &propertyName, const QVariant &resetValue)
{
    return m_nodeInstance->resetStateProperty(target.internalInstance(), propertyName, resetValue);
}

/*!
 Makes types used in node instances known to the Qml engine. To be called once at initialization time.
*/
void ServerNodeInstance::registerQmlTypes()
{
//    qmlRegisterType<QmlDesigner::Internal::QmlPropertyChangesObject>();
}

void ServerNodeInstance::doComponentComplete()
{
    m_nodeInstance->doComponentComplete();
}

QList<ServerNodeInstance> ServerNodeInstance::childItems() const
{
    return m_nodeInstance->childItems();
}

QString ServerNodeInstance::id() const
{
    return m_nodeInstance->id();
}

qint32 ServerNodeInstance::instanceId() const
{
    if (isValid()) {
        return m_nodeInstance->instanceId();
    } else {
        return -1;
    }
}

QObject* ServerNodeInstance::testHandle() const
{
    return internalObject();
}

QList<ServerNodeInstance> ServerNodeInstance::stateInstances() const
{
    return m_nodeInstance->stateInstances();
}

Internal::ObjectNodeInstance::Pointer ServerNodeInstance::internalInstance() const
{
    return m_nodeInstance;
}

} // namespace QmlDesigner
