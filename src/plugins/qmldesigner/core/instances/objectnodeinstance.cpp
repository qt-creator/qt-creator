/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QEvent>
#include <QGraphicsScene>
#include <QDeclarativeContext>
#include <QDeclarativeError>
#include <QDeclarativeEngine>
#include <QDeclarativeProperty>
#include <QSharedPointer>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QPixmapCache>

#include <private/qdeclarativebinding_p.h>
#include <private/qdeclarativemetatype_p.h>
#include <private/qdeclarativevaluetype_p.h>

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
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(object);
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
                reparent(parentInstance, parentProperty.name(), NodeInstance() , QString());
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
        if (QDeclarativeMetaType::isQObject(metaObject->property(propertyIndex).userType())) {
            QObject *propertyObject = QDeclarativeMetaType::toQObject(metaObject->property(propertyIndex).read(objectNodeInstance->object()));
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

bool ObjectNodeInstance::isQDeclarativeView() const
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


static bool isList(const QDeclarativeProperty &metaProperty)
{
    return metaProperty.propertyTypeCategory() == QDeclarativeProperty::List;
}

static bool isObject(const QDeclarativeProperty &metaProperty)
{
    return metaProperty.propertyTypeCategory() == QDeclarativeProperty::Object;
}

static QVariant objectToVariant(QObject *object)
{
    return QVariant::fromValue(object);
}

static void removeObjectFromList(const QDeclarativeProperty &metaProperty, QObject *objectToBeRemoved, QDeclarativeEngine * engine)
{
    QDeclarativeListReference listReference(metaProperty.object(), metaProperty.name().toLatin1(), engine);
    int count = listReference.count();

    QObjectList objectList;

    for(int i = 0; i < count; i ++) {
        QObject *listItem = listReference.at(i);
        if (listItem != objectToBeRemoved)
            objectList.append(listItem);
    }

    listReference.clear();

    foreach(QObject *object, objectList)
        listReference.append(object);
}

void ObjectNodeInstance::removeFromOldProperty(QObject *object, QObject *oldParent, const QString &oldParentProperty)
{
    QDeclarativeProperty metaProperty(oldParent, oldParentProperty, context());

    if (isList(metaProperty)) {
        removeObjectFromList(metaProperty, object, nodeInstanceView()->engine());
    } else if (isObject(metaProperty)) {
        resetProperty(object, oldParentProperty);
    }

    object->setParent(0);
}

void ObjectNodeInstance::addToNewProperty(QObject *object, QObject *newParent, const QString &newParentProperty)
{
    QDeclarativeProperty metaProperty(newParent, newParentProperty, context());

    if (isList(metaProperty)) {
        QDeclarativeListReference list = qvariant_cast<QDeclarativeListReference>(metaProperty.read());
        list.append(object);
    } else if (isObject(metaProperty)) {
        metaProperty.write(objectToVariant(object));
    }
    object->setParent(newParent);

    Q_ASSERT(objectToVariant(object).isValid());
}

static void specialSetParentForQmlGraphicsItemChildren(QObject *object, QDeclarativeItem *parent)
{
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(object);
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
            specialSetParentForQmlGraphicsItemChildren(object(), qobject_cast<QDeclarativeItem*>(newParentInstance.internalObject()));
        else
            addToNewProperty(object(), newParentInstance.internalObject(), newParentProperty);
    }

    refreshBindings(context()->engine()->rootContext());
}

void ObjectNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    QDeclarativeProperty property(object(), name, context());

    QVariant oldValue = property.read();
    if (oldValue.type() == QVariant::Url) {
        QUrl url = oldValue.toUrl();
        QString path = url.toLocalFile();
        if (QFileInfo(path).exists() && nodeInstanceView() && !path.isEmpty())
            nodeInstanceView()->removeFilePropertyFromFileSystemWatcher(object(), name, path);
    }


    property.write(value);

    QVariant newValue = property.read();
    if (newValue.type() == QVariant::Url) {
        QUrl url = newValue.toUrl();
        QString path = url.toLocalFile();
        if (QFileInfo(path).exists() && nodeInstanceView() && !path.isEmpty())
            nodeInstanceView()->addFilePropertyToFileSystemWatcher(object(), name, path);
    }


}

void ObjectNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    QDeclarativeContext *QDeclarativeContext = QDeclarativeEngine::contextForObject(object());

    QDeclarativeProperty metaProperty(object(), name, context());
    if (metaProperty.isValid() && metaProperty.isProperty()) {
        QDeclarativeBinding *binding = new QDeclarativeBinding(expression, object(), QDeclarativeContext);
        binding->setTarget(metaProperty);
        binding->setNotifyOnValueChanged(true);
        QDeclarativeAbstractBinding *oldBinding = QDeclarativePropertyPrivate::setBinding(metaProperty, binding);
        if (oldBinding)
            oldBinding->destroy();
        binding->update();
    } else {
        qWarning() << "Cannot set binding for property" << name << ": property is unknown for type"
                   << (modelNode().isValid() ? modelNode().type() : "unknown");
    }
}

void ObjectNodeInstance::deleteObjectsInList(const QDeclarativeProperty &metaProperty)
{
    QObjectList objectList;
    QDeclarativeListReference list = qvariant_cast<QDeclarativeListReference>(metaProperty.read());

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

void ObjectNodeInstance::refreshProperty(const QString &name)
{
    QDeclarativeProperty property(object(), name, context());

    QVariant oldValue(property.read());

    if (property.isResettable())
        property.reset();
    else
        property.write(resetValue(name));

    if (oldValue.type() == QVariant::Url) {
        QByteArray key = oldValue.toUrl().toEncoded(QUrl::FormattingOption(0x100));
        QString pixmapKey = QString::fromLatin1(key.constData(), key.count());
        QPixmapCache::remove(pixmapKey);
    }

    property.write(oldValue);
}

void ObjectNodeInstance::resetProperty(QObject *object, const QString &propertyName)
{
    m_modelAbstractPropertyHash.remove(propertyName);

    QDeclarativeProperty property(object, propertyName, context());

    QVariant oldValue = property.read();
    if (oldValue.type() == QVariant::Url) {
        QUrl url = oldValue.toUrl();
        QString path = url.toLocalFile();
        if (QFileInfo(path).exists() && nodeInstanceView())
            nodeInstanceView()->removeFilePropertyFromFileSystemWatcher(object, propertyName, path);
    }


    QDeclarativeAbstractBinding *binding = QDeclarativePropertyPrivate::binding(property);
    if (binding) {
        binding->setEnabled(false, 0);
        binding->destroy();
    }

    if (property.isResettable()) {
        property.reset();
    } else if (property.isWritable()) {
        if (property.read() == resetValue(propertyName))
            return;
        property.write(resetValue(propertyName));
    } else if (property.propertyTypeCategory() == QDeclarativeProperty::List) {
        qvariant_cast<QDeclarativeListReference>(property.read()).clear();
    }
}

QVariant ObjectNodeInstance::property(const QString &name) const
{
    if (m_modelAbstractPropertyHash.contains(name))
        return QVariant::fromValue(m_modelAbstractPropertyHash.value(name));


    // TODO: handle model nodes

    QDeclarativeProperty metaProperty(object(), name, context());
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

ObjectNodeInstance::Pointer ObjectNodeInstance::create(const NodeMetaInfo &nodeMetaInfo, QDeclarativeContext *context, QObject *objectToBeWrapped)
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

QObject* ObjectNodeInstance::createObject(const NodeMetaInfo &metaInfo, QDeclarativeContext *context)
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

QDeclarativeContext *ObjectNodeInstance::context() const
{
    QDeclarativeContext *context = QDeclarativeEngine::contextForObject(object());
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

QStringList propertyNameForWritableProperties(QObject *object, const QString &baseName = QString())
{
    QStringList propertyNameList;

    const QMetaObject *metaObject = object->metaObject();
    for (int index = 0; index < metaObject->propertyCount(); ++index) {
        QMetaProperty metaProperty = metaObject->property(index);
        if (metaProperty.isReadable() && !metaProperty.isWritable()) {
            QObject *childObject = QDeclarativeMetaType::toQObject(metaProperty.read(object));
            if (childObject)
                propertyNameList.append(propertyNameForWritableProperties(childObject, baseName +  QString::fromUtf8(metaProperty.name()) + '.'));
        } else if (QDeclarativeValueTypeFactory::valueType(metaProperty.userType())) {
            QDeclarativeValueType *valueType = QDeclarativeValueTypeFactory::valueType(metaProperty.userType());
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
        QDeclarativeProperty metaProperty(object(), propertyName, context());
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
void ObjectNodeInstance::refreshBindings(QDeclarativeContext *context)
{
    // TODO: Maybe do this via a timer to prevent update flooding

    static int i = 0;
    context->setContextProperty(QString("__dummy_%1").arg(i++), true);
}

bool ObjectNodeInstance::updateStateVariant(const NodeInstance &/*target*/, const QString &/*propertyName*/, const QVariant &/*value*/)
{
    return false;
}

bool ObjectNodeInstance::updateStateBinding(const NodeInstance &/*target*/, const QString &/*propertyName*/, const QString &/*expression*/)
{
    return false;
}

bool ObjectNodeInstance::resetStateProperty(const NodeInstance &/*target*/, const QString &/*propertyName*/, const QVariant &/*resetValue*/)
{
    return false;
}

}
}

