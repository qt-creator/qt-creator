/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "objectnodeinstance.h"

#include "qmlgraphicsitemnodeinstance.h"
#include "graphicsobjectnodeinstance.h"
#include "graphicsviewnodeinstance.h"
#include "graphicsscenenodeinstance.h"
#include "graphicswidgetnodeinstance.h"
#include "qmlviewnodeinstance.h"
#include "widgetnodeinstance.h"
#include "proxywidgetnodeinstance.h"

#include <invalidreparentingexception.h>
#include <invalidnodeinstanceexception.h>
#include <notimplementedexception.h>
#include <noanchoringpossibleexception.h>

#include <variantproperty.h>
#include <nodelistproperty.h>
#include <metainfo.h>
#include <propertymetainfo.h>
#include <qmlmetaproperty.h>

#include <QEvent>
#include <QGraphicsScene>
#include <QmlContext>
#include <QmlError>
#include <QmlBinding>
#include <QmlMetaType>
#include <QmlEngine>
#include <QSharedPointer>

#include <private/qmlcontext_p.h>
#include <private/qmllistaccessor_p.h>
#include <private/qmlvaluetype_p.h>
#include <private/qmlgraphicsanchors_p.h>
#include <private/qmlgraphicsrectangle_p.h> // to get QmlGraphicsPen
#include <private/qmetaobjectbuilder_p.h>
#include <private/qmlvmemetaobject_p.h>
#include <private/qobject_p.h>



namespace QmlDesigner {
namespace Internal {


ChildrenChangeEventFilter::ChildrenChangeEventFilter(QObject *parent)
    : QObject(parent)
{
}


bool ChildrenChangeEventFilter::eventFilter(QObject * /*object*/, QEvent *event)
{
    switch (event->type()) {
        case QEvent::ChildAdded:
        case QEvent::ChildRemoved:
            {
                QChildEvent *childEvent = static_cast<QChildEvent*>(event);
                emit childrenChanged(childEvent->child()); break;
            }
        default: break;
    }

    return false;
}

ObjectNodeInstance::ObjectNodeInstance(QObject *object)
    : m_deleteHeldInstance(true),
    m_object(object),
    m_metaObject(0)
{

}

ObjectNodeInstance::~ObjectNodeInstance()
{
    destroy();
}

static bool isChildrenProperty(const QString &name)
{
    return name == "data" || name == "children" || name == "resources";
}

static void specialRemoveParentForQmlGraphicsItemChildren(QObject *object)
{
    QmlGraphicsItem *item = qobject_cast<QmlGraphicsItem*>(object);
    if (item && item->scene())
        item->scene()->removeItem(item);

    object->setParent(0);
}

void ObjectNodeInstance::destroy()
{
    if (deleteHeldInstance()) {
        // Remove from old property
        if (object() && modelNode().isValid() && modelNode().parentProperty().isValid()) {
            NodeAbstractProperty parentProperty = modelNode().parentProperty();
            ModelNode parentNode = parentProperty.parentModelNode();
            if (parentNode.isValid() && nodeInstanceView()->hasInstanceForNode(parentNode)) {
                NodeInstance parentInstance = nodeInstanceView()->instanceForNode(parentNode);
                if (parentInstance.isQmlGraphicsItem() && isChildrenProperty(parentProperty.name())) {
                    specialRemoveParentForQmlGraphicsItemChildren(object());
                } else {
                    removeFromOldProperty(object(), parentInstance.internalObject(), parentProperty.name());
                }
            }
        }

        if (!m_id.isEmpty()) {
            context()->engine()->rootContext()->setContextProperty(m_id, 0);
        }

        if (object()) {
            QObject *obj = object();
            m_object.clear();
            delete obj;
        }
    }
}

ModelNode ObjectNodeInstance::modelNode() const
{
    return m_modelNode;
}

void ObjectNodeInstance::setModelNode(const ModelNode &node)
{
    m_modelNode = node;
}

NodeInstanceView *ObjectNodeInstance::nodeInstanceView() const
{
    return m_nodeInstanceView.data();
}

void ObjectNodeInstance::setNodeInstanceView(NodeInstanceView *view)
{
    Q_ASSERT(!m_nodeInstanceView.data());

    m_nodeInstanceView = view;
}

static bool hasPropertiesWitoutNotifications(const QMetaObject *metaObject)
{
    for(int propertyIndex = QObject::staticMetaObject.propertyCount(); propertyIndex < metaObject->propertyCount(); propertyIndex++) {
        if (!metaObject->property(propertyIndex).hasNotifySignal())
            return true;
    }

    return false;
}

void ObjectNodeInstance::initializePropertyWatcher(const ObjectNodeInstance::Pointer &objectNodeInstance)
{
    m_metaObject = new NodeInstanceMetaObject(objectNodeInstance);
    const QMetaObject *metaObject = objectNodeInstance->object()->metaObject();
    for(int propertyIndex = QObject::staticMetaObject.propertyCount(); propertyIndex < metaObject->propertyCount(); propertyIndex++) {
        if (QmlMetaType::isQObject(metaObject->property(propertyIndex).userType())) {
            QObject *propertyObject = QmlMetaType::toQObject(metaObject->property(propertyIndex).read(objectNodeInstance->object()));
            if (propertyObject && hasPropertiesWitoutNotifications(propertyObject->metaObject())) {
                new NodeInstanceMetaObject(objectNodeInstance, propertyObject, metaObject->property(propertyIndex).name());
            }
        }
    }

    m_signalSpy.setObjectNodeInstance(objectNodeInstance);
}

void ObjectNodeInstance::setId(const QString &id)
{
    if (!m_id.isEmpty()) {
        context()->engine()->rootContext()->setContextProperty(m_id, 0);
    }

    if (!id.isEmpty()) {
        context()->engine()->rootContext()->setContextProperty(id, object()); // will also force refresh of all bindings
    }

    m_id = id;
}

bool ObjectNodeInstance::isQmlGraphicsItem() const
{
    return false;
}

bool ObjectNodeInstance::isGraphicsScene() const
{
    return false;
}

bool ObjectNodeInstance::isGraphicsView() const
{
    return false;
}

bool ObjectNodeInstance::isGraphicsWidget() const
{
    return false;
}

bool ObjectNodeInstance::isProxyWidget() const
{
    return false;
}

bool ObjectNodeInstance::isWidget() const
{
    return false;
}

bool ObjectNodeInstance::isQmlView() const
{
    return false;
}

bool ObjectNodeInstance::isGraphicsObject() const
{
    return false;
}

bool ObjectNodeInstance::isTransition() const
{
    return false;
}

bool ObjectNodeInstance::equalGraphicsItem(QGraphicsItem * /*item*/) const
{
    return false;
}

QTransform ObjectNodeInstance::transform() const
{
    return QTransform();
}

QTransform ObjectNodeInstance::customTransform() const
{
    return QTransform();
}

QTransform ObjectNodeInstance::sceneTransform() const
{
    return QTransform();
}

double ObjectNodeInstance::rotation() const
{
    return 0.0;
}

double ObjectNodeInstance::scale() const
{
    return 1.0;
}

QList<QGraphicsTransform *> ObjectNodeInstance::transformations() const
{
    QList<QGraphicsTransform *> transformationsList;

    return transformationsList;
}

QPointF ObjectNodeInstance::transformOriginPoint() const
{
    return QPoint();
}

double ObjectNodeInstance::zValue() const
{
    return 0.0;
}

double ObjectNodeInstance::opacity() const
{
    return 1.0;
}

bool ObjectNodeInstance::hasAnchor(const QString &/*name*/) const
{
    return false;
}

bool ObjectNodeInstance::isAnchoredBy() const
{
    return false;
}

QPair<QString, NodeInstance> ObjectNodeInstance::anchor(const QString &/*name*/) const
{
    return qMakePair(QString(), NodeInstance());
}


static bool isList(const QmlMetaProperty &metaProperty)
{
    return metaProperty.propertyCategory() == QmlMetaProperty::List;
}

static bool isObject(const QmlMetaProperty &metaProperty)
{
    return metaProperty.propertyCategory() == QmlMetaProperty::Object;
}

static QVariant objectToVariant(QObject *object)
{
    return QVariant::fromValue(object);
}

static void removeObjectFromList(const QmlMetaProperty &metaProperty, QObject *object, QmlEngine *engine)
{
    // ### Very few QML lists ever responded to removes
}

void ObjectNodeInstance::removeFromOldProperty(QObject *object, QObject *oldParent, const QString &oldParentProperty)
{
    QmlMetaProperty metaProperty = QmlMetaProperty::createProperty(oldParent, oldParentProperty, context());

    if (isList(metaProperty)) {
        removeObjectFromList(metaProperty, object, nodeInstanceView()->engine());
    } else if (isObject(metaProperty)) {
        resetProperty(object, oldParentProperty);
    }

    object->setParent(0);
}

void ObjectNodeInstance::addToNewProperty(QObject *object, QObject *newParent, const QString &newParentProperty)
{
    QmlMetaProperty metaProperty = QmlMetaProperty::createProperty(newParent, newParentProperty, context());

    if (isList(metaProperty)) {
        QmlListReference list = qvariant_cast<QmlListReference>(metaProperty.read());
        list.append(object);
    } else if (isObject(metaProperty)) {
        metaProperty.write(objectToVariant(object));
    }
    object->setParent(newParent);

    Q_ASSERT(objectToVariant(object).isValid());
}

static void specialSetParentForQmlGraphicsItemChildren(QObject *object, QmlGraphicsItem *parent)
{
    QmlGraphicsItem *item = qobject_cast<QmlGraphicsItem*>(object);
    if (item)
        item->setParentItem(parent);
    else
        object->setParent(parent);
}

void ObjectNodeInstance::reparent(const NodeInstance &oldParentInstance, const QString &oldParentProperty, const NodeInstance &newParentInstance, const QString &newParentProperty)
{
    if (oldParentInstance.isValid()) {
        if (oldParentInstance.isQmlGraphicsItem() && isChildrenProperty(oldParentProperty))
            specialRemoveParentForQmlGraphicsItemChildren(object());
        else
            removeFromOldProperty(object(), oldParentInstance.internalObject(), oldParentProperty);
    }

    if (newParentInstance.isValid()) {
        if (newParentInstance.isQmlGraphicsItem() && isChildrenProperty(newParentProperty))
            specialSetParentForQmlGraphicsItemChildren(object(), qobject_cast<QmlGraphicsItem*>(newParentInstance.internalObject()));
        else
            addToNewProperty(object(), newParentInstance.internalObject(), newParentProperty);
    }

    refreshBindings(context()->engine()->rootContext());
}

void ObjectNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    QmlMetaProperty qmlMetaProperty = QmlMetaProperty::createProperty(object(), name, context());

    qmlMetaProperty.write(value);
}

void ObjectNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    QmlContext *qmlContext = QmlEngine::contextForObject(object());

    QmlMetaProperty metaProperty = QmlMetaProperty::createProperty(object(), name, context());
    if (metaProperty.isValid() && metaProperty.isProperty()) {
        QmlBinding *qmlBinding = new QmlBinding(expression, object(), qmlContext);
        qmlBinding->setTarget(metaProperty);
        qmlBinding->setTrackChange(true);
        QmlAbstractBinding *oldBinding = metaProperty.setBinding(qmlBinding);
        delete oldBinding;
        qmlBinding->update();
    } else {
        qWarning() << "Cannot set binding for property" << name << ": property is unknown for type"
                   << (modelNode().isValid() ? modelNode().type() : "unknown");
    }
}

void ObjectNodeInstance::deleteObjectsInList(const QmlMetaProperty &metaProperty)
{
    QObjectList objectList;
    QmlListReference list = qvariant_cast<QmlListReference>(metaProperty.read());

    for(int i = 0; i < list.count(); i++) {
        objectList += list.at(i);
    }

    list.clear();
}

void ObjectNodeInstance::resetProperty(const QString &name)
{
    resetProperty(object(), name);

    if (name == "font.pixelSize")
        resetProperty(object(), "font.pointSize");

    if (name == "font.pointSize")
        resetProperty(object(), "font.pixelSize");
}

NodeInstance ObjectNodeInstance::instanceForNode(const ModelNode &node, const QString &fullname)
{
    if (nodeInstanceView()->hasInstanceForNode(node)) {
        return nodeInstanceView()->instanceForNode(node);
    } else {
        NodeInstance instance(nodeInstanceView()->loadNode(node));
        m_modelAbstractPropertyHash.insert(fullname, instance);
        return instance;
    }
}

void ObjectNodeInstance::resetProperty(QObject *object, const QString &propertyName)
{
    m_modelAbstractPropertyHash.remove(propertyName);

    QmlMetaProperty qmlMetaProperty = QmlMetaProperty::createProperty(object, propertyName, context());
    QMetaProperty metaProperty = qmlMetaProperty.property();

    QmlAbstractBinding *binding = qmlMetaProperty.binding();
    if (binding) {
        binding->setEnabled(false, 0);
        delete binding;
    }

    if (metaProperty.isResettable()) {
        metaProperty.reset(object);
    } else if (qmlMetaProperty.isWritable()) {
        if (qmlMetaProperty.read() == resetValue(propertyName))
            return;
        qmlMetaProperty.write(resetValue(propertyName));
    } else if (qmlMetaProperty.propertyCategory() == QmlMetaProperty::List) {
        qvariant_cast<QmlListReference>(qmlMetaProperty.read()).clear();
    }
}

QVariant ObjectNodeInstance::property(const QString &name) const
{
    if (m_modelAbstractPropertyHash.contains(name))
        return QVariant::fromValue(m_modelAbstractPropertyHash.value(name));


    // TODO: handle model nodes

    QmlMetaProperty metaProperty = QmlMetaProperty::createProperty(object(), name, context());
    if (metaProperty.property().isEnumType()) {
        QVariant value = object()->property(name.toLatin1());
        return metaProperty.property().enumerator().valueToKey(value.toInt());
    }

    if (metaProperty.propertyType() == QVariant::Url) {
        QUrl url = metaProperty.read().toUrl();
        if (url.isEmpty())
            return QVariant();

        if (url.scheme() == "file") {
            int basePathLength = nodeInstanceView()->model()->fileUrl().toLocalFile().lastIndexOf('/');
            return QUrl(url.toLocalFile().mid(basePathLength + 1));
        }
    }

    return metaProperty.read();
}


void ObjectNodeInstance::setDeleteHeldInstance(bool deleteInstance)
{
    m_deleteHeldInstance = deleteInstance;
}

bool ObjectNodeInstance::deleteHeldInstance() const
{
    return m_deleteHeldInstance;
}

ObjectNodeInstance::Pointer ObjectNodeInstance::create(const NodeMetaInfo &nodeMetaInfo, QmlContext *context, QObject *objectToBeWrapped)
{
    QObject *object = 0;
    if (objectToBeWrapped)
        object = objectToBeWrapped;
    else
        object = createObject(nodeMetaInfo, context);

    Pointer instance(new ObjectNodeInstance(object));

    if (objectToBeWrapped)
        instance->setDeleteHeldInstance(false); // the object isn't owned

    instance->populateResetValueHash();

    return instance;
}

QObject* ObjectNodeInstance::createObject(const NodeMetaInfo &metaInfo, QmlContext *context)
{
    QObject *object = metaInfo.createInstance(context);

    if (object == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

    return object;
}

QObject *ObjectNodeInstance::object() const
{
    return m_object.data();
}

bool ObjectNodeInstance::hasContent() const
{
    return false;
}

void ObjectNodeInstance::updateAnchors()
{
}

QmlContext *ObjectNodeInstance::context() const
{
    QmlContext *context = QmlEngine::contextForObject(object());
    if (context)
        return context;
    else if (nodeInstanceView())
        return nodeInstanceView()->engine()->rootContext();

    return 0;
}

void ObjectNodeInstance::paintUpdate()
{
}

void ObjectNodeInstance::activateState()
{
}

void ObjectNodeInstance::deactivateState()
{
}

void ObjectNodeInstance::refreshState()
{
}

QStringList propertyNameForWritableProperties(QObject *object, const QString &baseName = QString())
{
    QStringList propertyNameList;

    const QMetaObject *metaObject = object->metaObject();
    for (int index = 0; index < metaObject->propertyCount(); ++index) {
        QMetaProperty metaProperty = metaObject->property(index);
        if (metaProperty.isReadable() && !metaProperty.isWritable()) {
            QObject *childObject = QmlMetaType::toQObject(metaProperty.read(object));
            if (childObject)
                propertyNameList.append(propertyNameForWritableProperties(childObject, baseName +  QString::fromUtf8(metaProperty.name()) + '.'));
        } else if (QmlValueTypeFactory::valueType(metaProperty.userType())) {
            QmlValueType *valueType = QmlValueTypeFactory::valueType(metaProperty.userType());
            valueType->setValue(metaProperty.read(object));
            propertyNameList.append(propertyNameForWritableProperties(valueType, baseName +  QString::fromUtf8(metaProperty.name()) + '.'));
        } else if (metaProperty.isReadable() && metaProperty.isWritable()) {
            propertyNameList.append(baseName + QString::fromUtf8(metaProperty.name()));
        }
    }

    return propertyNameList;
}

void ObjectNodeInstance::populateResetValueHash()
{
    QStringList propertyNameList = propertyNameForWritableProperties(object());

    foreach(const QString &propertyName, propertyNameList) {
        QmlMetaProperty metaProperty = QmlMetaProperty::createProperty(object(), propertyName, context());
        if (metaProperty.isWritable())
            m_resetValueHash.insert(propertyName, metaProperty.read());
    }
}

QVariant ObjectNodeInstance::resetValue(const QString &propertyName) const
{
    return m_resetValueHash.value(propertyName);
}

void ObjectNodeInstance::paint(QPainter * /*painter*/) const
{
}

bool ObjectNodeInstance::isTopLevel() const
{
    return false;
}

QObject *ObjectNodeInstance::parent() const
{
    if (!object())
        return 0;

    return object()->parent();
}

QRectF ObjectNodeInstance::boundingRect() const
{
    return QRect();
}

QPointF ObjectNodeInstance::position() const
{
    return QPointF();
}

QSizeF ObjectNodeInstance::size() const
{
    return QSizeF();
}

int ObjectNodeInstance::penWidth() const
{
    return 0;
}

bool ObjectNodeInstance::isVisible() const
{
    return false;
}

void ObjectNodeInstance::setVisible(bool /*isVisible*/)
{
}

static bool metaObjectHasNotPropertyName(NodeInstanceMetaObject *metaObject, const QString &propertyName)
{
    for (int i = 0; i < metaObject->count(); i++) {
        if (metaObject->name(i) == propertyName)
            return false;
    }

    return true;
}

void ObjectNodeInstance::createDynamicProperty(const QString &name, const QString &/*typeName*/)
{
    if (metaObjectHasNotPropertyName(m_metaObject, name))
        m_metaObject->createNewProperty(name);
}

/**
  Force all bindings in this or a sub context to be re-evaluated.
  */
void ObjectNodeInstance::refreshBindings(QmlContext *context)
{
    // TODO: Maybe do this via a timer to prevent update flooding
    QmlContextPrivate::get(context)->refreshExpressions();
}

}
}

