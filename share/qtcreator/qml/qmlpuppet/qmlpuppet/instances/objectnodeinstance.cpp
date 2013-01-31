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

#include "objectnodeinstance.h"



#include <QEvent>
#include <QGraphicsScene>
#include <QDeclarativeContext>
#include <QDeclarativeError>
#include <QDeclarativeEngine>
#include <QDeclarativeProperty>
#include <QDeclarativeComponent>
#include <QSharedPointer>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QPixmapCache>
#if !defined(QT_NO_WEBKIT) && QT_VERSION < 0x050000
#include <QGraphicsWebView>
#endif
#include <QGraphicsObject>

#include <QTextDocument>
#include <QLibraryInfo>

#include <private/qdeclarativebinding_p.h>
#include <private/qdeclarativemetatype_p.h>
#include <private/qdeclarativevaluetype_p.h>
#include <private/qdeclarativetransition_p.h>
#include <private/qdeclarativeanimation_p.h>
#include <private/qdeclarativetimer_p.h>

namespace QmlDesigner {
namespace Internal {

ObjectNodeInstance::ObjectNodeInstance(QObject *object)
    : m_instanceId(-1),
    m_deleteHeldInstance(true),
    m_object(object),
    m_metaObject(0),
    m_isInPositioner(false)
{

}

ObjectNodeInstance::~ObjectNodeInstance()
{
    destroy();
}

void ObjectNodeInstance::destroy()
{
    if (deleteHeldInstance()) {
        // Remove from old property
        if (object()) {
            setId(QString());
            if (m_instanceId >= 0) {
                reparent(parentInstance(), m_parentProperty, ObjectNodeInstance::Pointer(), QString());
            }
        }

        if (object()) {
            QObject *obj = object();
            m_object.clear();
            delete obj;
        }
    }

    m_metaObject = 0;
    m_instanceId = -1;
}

void ObjectNodeInstance::setInstanceId(qint32 id)
{
    m_instanceId = id;
}

qint32 ObjectNodeInstance::instanceId() const
{
    return m_instanceId;
}

NodeInstanceServer *ObjectNodeInstance::nodeInstanceServer() const
{
    return m_nodeInstanceServer.data();
}

void ObjectNodeInstance::setNodeInstanceServer(NodeInstanceServer *server)
{
    Q_ASSERT(!m_nodeInstanceServer.data());

    m_nodeInstanceServer = server;
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
    const QMetaObject *metaObject = objectNodeInstance->object()->metaObject();
    m_metaObject = new NodeInstanceMetaObject(objectNodeInstance, nodeInstanceServer()->engine());
    for(int propertyIndex = QObject::staticMetaObject.propertyCount(); propertyIndex < metaObject->propertyCount(); propertyIndex++) {
        if (QDeclarativeMetaType::isQObject(metaObject->property(propertyIndex).userType())) {
            QObject *propertyObject = QDeclarativeMetaType::toQObject(metaObject->property(propertyIndex).read(objectNodeInstance->object()));
            if (propertyObject && hasPropertiesWitoutNotifications(propertyObject->metaObject())) {
                new NodeInstanceMetaObject(objectNodeInstance, propertyObject, metaObject->property(propertyIndex).name(), nodeInstanceServer()->engine());
            }
        }
    }

    m_signalSpy.setObjectNodeInstance(objectNodeInstance);
}

void ObjectNodeInstance::initialize(const ObjectNodeInstance::Pointer &objectNodeInstance)
{
    initializePropertyWatcher(objectNodeInstance);
}

void ObjectNodeInstance::setId(const QString &id)
{
    if (!m_id.isEmpty() && context()) {
        context()->engine()->rootContext()->setContextProperty(m_id, 0);
    }

    if (!id.isEmpty() && context()) {
        context()->engine()->rootContext()->setContextProperty(id, object()); // will also force refresh of all bindings
    }

    m_id = id;
}

QString ObjectNodeInstance::id() const
{
    return m_id;
}

bool ObjectNodeInstance::isQmlGraphicsItem() const
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

bool ObjectNodeInstance::isPositioner() const
{
    return false;
}

bool ObjectNodeInstance::isQuickItem() const
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

bool ObjectNodeInstance::isAnchoredBySibling() const
{
    return false;
}

bool ObjectNodeInstance::isAnchoredByChildren() const
{
    return false;
}

QPair<QString, ServerNodeInstance> ObjectNodeInstance::anchor(const QString &/*name*/) const
{
    return qMakePair(QString(), ServerNodeInstance());
}


static bool isList(const QDeclarativeProperty &property)
{
    return property.propertyTypeCategory() == QDeclarativeProperty::List;
}

static bool isObject(const QDeclarativeProperty &property)
{
    return (property.propertyTypeCategory() == QDeclarativeProperty::Object) ||
            //QVariant can also store QObjects. Lets trust our model.
           (QLatin1String(property.propertyTypeName()) == QLatin1String("QVariant"));
}

static QVariant objectToVariant(QObject *object)
{
    return QVariant::fromValue(object);
}

static bool hasFullImplementedListInterface(const QDeclarativeListReference &list)
{
    return list.isValid() && list.canCount() && list.canAt() && list.canAppend() && list.canClear();

}

static void removeObjectFromList(const QDeclarativeProperty &property, QObject *objectToBeRemoved, QDeclarativeEngine * engine)
{
    QDeclarativeListReference listReference(property.object(), property.name().toLatin1(), engine);

    if (!hasFullImplementedListInterface(listReference)) {
        qWarning() << "Property list interface not fully implemented for Class " << property.property().typeName() << " in property " << property.name() << "!";
        return;
    }

    int count = listReference.count();

    QObjectList objectList;

    for(int i = 0; i < count; i ++) {
        QObject *listItem = listReference.at(i);
        if (listItem && listItem != objectToBeRemoved)
            objectList.append(listItem);
    }

    listReference.clear();

    foreach(QObject *object, objectList)
        listReference.append(object);
}

void ObjectNodeInstance::removeFromOldProperty(QObject *object, QObject *oldParent, const QString &oldParentProperty)
{
    QDeclarativeProperty property(oldParent, oldParentProperty, context());

    if (!property.isValid())
        return;

    if (isList(property)) {
        removeObjectFromList(property, object, nodeInstanceServer()->engine());
    } else if (isObject(property)) {
        if (nodeInstanceServer()->hasInstanceForObject(oldParent)) {
            nodeInstanceServer()->instanceForObject(oldParent).resetProperty(oldParentProperty);
        }
    }

    if (object && object->parent())
        object->setParent(0);
}

void ObjectNodeInstance::addToNewProperty(QObject *object, QObject *newParent, const QString &newParentProperty)
{
    QDeclarativeProperty property(newParent, newParentProperty, context());

    if (isList(property)) {
        QDeclarativeListReference list = qvariant_cast<QDeclarativeListReference>(property.read());

        if (!hasFullImplementedListInterface(list)) {
            qWarning() << "Property list interface not fully implemented for Class " << property.property().typeName() << " in property " << property.name() << "!";
            return;
        }

        list.append(object);
    } else if (isObject(property)) {
        property.write(objectToVariant(object));
    }

    QGraphicsObject *graphicsObject = qobject_cast<QGraphicsObject*>(object);

    if (object && !(graphicsObject && graphicsObject->parentItem()))
        object->setParent(newParent);

    Q_ASSERT(objectToVariant(object).isValid());
}

void ObjectNodeInstance::reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const QString &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const QString &newParentProperty)
{
    if (oldParentInstance) {
        removeFromOldProperty(object(), oldParentInstance->object(), oldParentProperty);
        m_parentProperty.clear();
    }

    if (newParentInstance) {
        m_parentProperty = newParentProperty;
        addToNewProperty(object(), newParentInstance->object(), newParentProperty);
    }
}
QVariant ObjectNodeInstance::convertSpecialCharacter(const QVariant& value) const
{
    QVariant specialCharacterConvertedValue = value;
    if (value.type() == QVariant::String) {
        QString string = value.toString();
        string.replace(QLatin1String("\\n"), QLatin1String("\n"));
        string.replace(QLatin1String("\\t"), QLatin1String("\t"));
        specialCharacterConvertedValue = string;
    }

    return specialCharacterConvertedValue;
}


QVariant ObjectNodeInstance::fixResourcePaths(const QVariant &value)
{
    if (value.type() == QVariant::Url)
    {
        const QUrl url = value.toUrl();
        if (url.scheme() == QLatin1String("qrc")) {
            const QString path = QLatin1String("qrc:") +  url.path();
            QString qrcSearchPath = qgetenv("QMLDESIGNER_RC_PATHS");
            if (!qrcSearchPath.isEmpty()) {
                const QStringList searchPaths = qrcSearchPath.split(QLatin1Char(';'));
                foreach (const QString &qrcPath, searchPaths) {
                    const QStringList qrcDefintion = qrcPath.split(QLatin1Char('='));
                    if (qrcDefintion.count() == 2) {
                        QString fixedPath = path;
                        fixedPath.replace(QLatin1String("qrc:") + qrcDefintion.first(), qrcDefintion.last() + QLatin1Char('/'));
                        if (QFileInfo(fixedPath).exists()) {
                            fixedPath.replace(QLatin1String("//"), QLatin1String("/"));
                            fixedPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
                            return QUrl(fixedPath);
                        }
                    }
                }
            }
        }
    }
    if (value.type() == QVariant::String) {
        const QString str = value.toString();
        if (str.contains(QLatin1String("qrc:"))) {
            QString qrcSearchPath = qgetenv("QMLDESIGNER_RC_PATHS");
            if (!qrcSearchPath.isEmpty()) {
                const QStringList searchPaths = qrcSearchPath.split(QLatin1Char(';'));
                foreach (const QString &qrcPath, searchPaths) {
                    const QStringList qrcDefintion = qrcPath.split(QLatin1Char('='));
                    if (qrcDefintion.count() == 2) {
                        QString fixedPath = str;
                        fixedPath.replace(QLatin1String("qrc:") + qrcDefintion.first(), qrcDefintion.last() + QLatin1Char('/'));
                        if (QFileInfo(fixedPath).exists()) {
                            fixedPath.replace(QLatin1String("//"), QLatin1String("/"));
                            fixedPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
                            return fixedPath;
                        }
                    }
                }
            }
        }
    }
    return value;
}

void ObjectNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    QDeclarativeProperty property(object(), name, context());

    if (!property.isValid())
        return;

    QVariant fixedValue = fixResourcePaths(value);

    QVariant oldValue = property.read();
    if (oldValue.type() == QVariant::Url) {
        QUrl url = oldValue.toUrl();
        QString path = url.toLocalFile();
        if (QFileInfo(path).exists() && nodeInstanceServer() && !path.isEmpty())
            nodeInstanceServer()->removeFilePropertyFromFileSystemWatcher(object(), name, path);
    }

    if (hasValidResetBinding(name)) {
        QDeclarativePropertyPrivate::setBinding(property, 0, QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
    }

    bool isWritten = property.write(convertSpecialCharacter(fixedValue));

    if (!isWritten)
        qDebug() << "ObjectNodeInstance.setPropertyVariant: Cannot be written: " << object() << name << fixedValue;

    QVariant newValue = property.read();
    if (newValue.type() == QVariant::Url) {
        QUrl url = newValue.toUrl();
        QString path = url.toLocalFile();
        if (QFileInfo(path).exists() && nodeInstanceServer() && !path.isEmpty())
            nodeInstanceServer()->addFilePropertyToFileSystemWatcher(object(), name, path);
    }
}

void ObjectNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    QDeclarativeProperty property(object(), name, context());

    if (!property.isValid())
        return;

    if (property.isProperty()) {
        QDeclarativeBinding *binding = new QDeclarativeBinding(expression, object(), context(), object());
        binding->setTarget(property);
        binding->setNotifyOnValueChanged(true);
        QDeclarativeAbstractBinding *oldBinding = QDeclarativePropertyPrivate::setBinding(property, binding);
        if (oldBinding && !hasValidResetBinding(name))
            oldBinding->destroy();
        binding->update();
        if (binding->hasError()) {
            qDebug() <<" ObjectNodeInstance.setPropertyBinding has Error: " << object() << name << expression << binding->error().toString();
            if (property.property().userType() == QVariant::String)
                property.write(QVariant(QString("#%1#").arg(expression)));
        }

    } else {
        qWarning() << "ObjectNodeInstance.setPropertyBinding: Cannot set binding for property" << name << ": property is unknown for type";
    }
}

void ObjectNodeInstance::deleteObjectsInList(const QDeclarativeProperty &property)
{
    QObjectList objectList;
    QDeclarativeListReference list = qvariant_cast<QDeclarativeListReference>(property.read());

    if (!hasFullImplementedListInterface(list)) {
        qWarning() << "Property list interface not fully implemented for Class " << property.property().typeName() << " in property " << property.name() << "!";
        return;
    }

    for(int i = 0; i < list.count(); i++) {
        objectList += list.at(i);
    }

    list.clear();
}

void ObjectNodeInstance::resetProperty(const QString &name)
{
    doResetProperty(name);

    if (name == "font.pixelSize")
        doResetProperty("font.pointSize");

    if (name == "font.pointSize")
        doResetProperty("font.pixelSize");
}

void ObjectNodeInstance::refreshProperty(const QString &name)
{
    QDeclarativeProperty property(object(), name, context());

    if (!property.isValid())
        return;

    QVariant oldValue(property.read());

    if (property.isResettable())
        property.reset();
    else
        property.write(resetValue(name));

    if (oldValue.type() == QVariant::Url) {
#if QT_VERSION >= 0x050000
        QByteArray key = oldValue.toUrl().toEncoded(QUrl::UrlFormattingOption(0x100));
#else
        QByteArray key = oldValue.toUrl().toEncoded(QUrl::FormattingOption(0x100));
#endif
        QString pixmapKey = QString::fromLatin1(key.constData(), key.count());
        QPixmapCache::remove(pixmapKey);
    }

    property.write(oldValue);
}

bool ObjectNodeInstance::hasBindingForProperty(const QString &name, bool *hasChanged) const
{
    QDeclarativeProperty property(object(), name, context());

    bool hasBinding = QDeclarativePropertyPrivate::binding(property);

    if (hasChanged) {
        *hasChanged = hasBinding != m_hasBindingHash.value(name, false);
        if (*hasChanged)
            m_hasBindingHash.insert(name, hasBinding);
    }

    return QDeclarativePropertyPrivate::binding(property);
}

void ObjectNodeInstance::doResetProperty(const QString &propertyName)
{
    m_modelAbstractPropertyHash.remove(propertyName);

    QDeclarativeProperty property(object(), propertyName, context());

    if (!property.isValid())
        return;

    QVariant oldValue = property.read();
    if (oldValue.type() == QVariant::Url) {
        QUrl url = oldValue.toUrl();
        QString path = url.toLocalFile();
        if (QFileInfo(path).exists() && nodeInstanceServer())
            nodeInstanceServer()->removeFilePropertyFromFileSystemWatcher(object(), propertyName, path);
    }


    QDeclarativeAbstractBinding *binding = QDeclarativePropertyPrivate::binding(property);
    if (binding && !(hasValidResetBinding(propertyName) && resetBinding(propertyName) == binding)) {
        binding->setEnabled(false, 0);
        binding->destroy();
    }


    if (hasValidResetBinding(propertyName)) {
        QDeclarativeAbstractBinding *binding = resetBinding(propertyName);
        QDeclarativePropertyPrivate::setBinding(property, binding, QDeclarativePropertyPrivate::DontRemoveBinding);
        binding->update();
    } else if (property.isResettable()) {
        property.reset();
    } else if (property.propertyTypeCategory() == QDeclarativeProperty::List) {
        QDeclarativeListReference list = qvariant_cast<QDeclarativeListReference>(property.read());

        if (!hasFullImplementedListInterface(list)) {
            qWarning() << "Property list interface not fully implemented for Class " << property.property().typeName() << " in property " << property.name() << "!";
            return;
        }

        list.clear();
    } else if (property.isWritable()) {
        if (property.read() == resetValue(propertyName))
            return;

        property.write(resetValue(propertyName));
    }
}

QVariant ObjectNodeInstance::property(const QString &name) const
{
    if (m_modelAbstractPropertyHash.contains(name))
        return QVariant::fromValue(m_modelAbstractPropertyHash.value(name));

    // TODO: handle model nodes

    QDeclarativeProperty property(object(), name, context());
    if (property.property().isEnumType()) {
        QVariant value = property.read();
        return property.property().enumerator().valueToKey(value.toInt());
    }

    if (property.propertyType() == QVariant::Url) {
        QUrl url = property.read().toUrl();
        if (url.isEmpty())
            return QVariant();

        if (url.scheme() == "file") {
            int basePathLength = nodeInstanceServer()->fileUrl().toLocalFile().lastIndexOf('/');
            return QUrl(url.toLocalFile().mid(basePathLength + 1));
        }
    }

    return property.read();
}

QStringList allPropertyNames(QObject *object, const QString &baseName = QString(), QObjectList *inspectedObjects = new QObjectList)
{
    QStringList propertyNameList;


    if (inspectedObjects== 0 || inspectedObjects->contains(object))
        return propertyNameList;

    inspectedObjects->append(object);


    const QMetaObject *metaObject = object->metaObject();
    for (int index = 0; index < metaObject->propertyCount(); ++index) {
        QMetaProperty metaProperty = metaObject->property(index);
        QDeclarativeProperty declarativeProperty(object, QLatin1String(metaProperty.name()));
        if (declarativeProperty.isValid() && declarativeProperty.propertyTypeCategory() == QDeclarativeProperty::Object) {
            if (declarativeProperty.name() != "parent") {
                QObject *childObject = QDeclarativeMetaType::toQObject(declarativeProperty.read());
                if (childObject)
                    propertyNameList.append(allPropertyNames(childObject, baseName +  QString::fromUtf8(metaProperty.name()) + '.', inspectedObjects));
            }
        } else if (QDeclarativeValueTypeFactory::valueType(metaProperty.userType())) {
            QDeclarativeValueType *valueType = QDeclarativeValueTypeFactory::valueType(metaProperty.userType());
            valueType->setValue(metaProperty.read(object));
            propertyNameList.append(allPropertyNames(valueType, baseName +  QString::fromUtf8(metaProperty.name()) + '.', inspectedObjects));
        } else  {
            propertyNameList.append(baseName + QString::fromUtf8(metaProperty.name()));
        }
    }

    return propertyNameList;
}

QStringList ObjectNodeInstance::propertyNames() const
{
    if (isValid())
        return allPropertyNames(object());
    return QStringList();
}

QString ObjectNodeInstance::instanceType(const QString &name) const
{
    QDeclarativeProperty property(object(), name, context());
    if (!property.isValid())
        return QLatin1String("undefined");
    return property.propertyTypeName();
}

QList<ServerNodeInstance> ObjectNodeInstance::childItems() const
{
    return QList<ServerNodeInstance>();
}

QList<ServerNodeInstance>  ObjectNodeInstance::stateInstances() const
{
    return QList<ServerNodeInstance>();
}

void ObjectNodeInstance::setNodeSource(const QString & /*source*/)
{
}

void ObjectNodeInstance::setDeleteHeldInstance(bool deleteInstance)
{
    m_deleteHeldInstance = deleteInstance;
}

bool ObjectNodeInstance::deleteHeldInstance() const
{
    return m_deleteHeldInstance;
}

ObjectNodeInstance::Pointer ObjectNodeInstance::create(QObject *object)
{
    Pointer instance(new ObjectNodeInstance(object));

    instance->populateResetHashes();

    return instance;
}

static void stopAnimation(QObject *object)
{
    if (object == 0)
        return;

    QDeclarativeTransition *transition = qobject_cast<QDeclarativeTransition*>(object);
    QDeclarativeAbstractAnimation *animation = qobject_cast<QDeclarativeAbstractAnimation*>(object);
    QDeclarativeScriptAction *scriptAimation = qobject_cast<QDeclarativeScriptAction*>(object);
    QDeclarativeTimer *timer = qobject_cast<QDeclarativeTimer*>(object);
    if(transition) {
       transition->setFromState("");
       transition->setToState("");
    } else if (animation) {
        if (scriptAimation) {
            scriptAimation->setScript(QDeclarativeScriptString());
        animation->setLoops(1);
        animation->complete();
        animation->setDisableUserControl();
        }
    } else if (timer) {
        timer->blockSignals(true);
    }
}

void allSubObject(QObject *object, QObjectList &objectList)
{
    // don't add null pointer and stop if the object is already in the list
    if (!object || objectList.contains(object))
        return;

    objectList.append(object);

    for (int index = QObject::staticMetaObject.propertyOffset();
         index < object->metaObject()->propertyCount();
         index++) {
        QMetaProperty metaProperty = object->metaObject()->property(index);

        // search recursive in property objects
        if (metaProperty.isReadable()
                && metaProperty.isWritable()
                && QDeclarativeMetaType::isQObject(metaProperty.userType())) {
            if (metaProperty.name() != QLatin1String("parent")) {
                QObject *propertyObject = QDeclarativeMetaType::toQObject(metaProperty.read(object));
                allSubObject(propertyObject, objectList);
            }

        }

        // search recursive in property object lists
        if (metaProperty.isReadable()
                && QDeclarativeMetaType::isList(metaProperty.userType())) {
            QDeclarativeListReference list(object, metaProperty.name());
            if (list.canCount() && list.canAt()) {
                for (int i = 0; i < list.count(); i++) {
                    QObject *propertyObject = list.at(i);
                    allSubObject(propertyObject, objectList);

                }
            }
        }
    }

    // search recursive in object children list
    foreach(QObject *childObject, object->children()) {
        allSubObject(childObject, objectList);
    }

    // search recursive in graphics item childItems list
    QGraphicsObject *graphicsObject = qobject_cast<QGraphicsObject*>(object);
    if (graphicsObject) {
        foreach(QGraphicsItem *item, graphicsObject->childItems()) {
            QGraphicsObject *childObject = item->toGraphicsObject();
            allSubObject(childObject, objectList);
        }
    }
}

static void disableTiledBackingStore(QObject *object)
{
#if !defined(QT_NO_WEBKIT) && QT_VERSION < 0x050000
    QGraphicsWebView *webView = qobject_cast<QGraphicsWebView*>(object);
    if (webView)
        webView->settings()->setAttribute(QWebSettings::TiledBackingStoreEnabled, false);
#else
    Q_UNUSED(object);
#endif
}

QStringList propertyNameForWritableProperties(QObject *object, const QString &baseName = QString(), QObjectList *inspectedObjects = new QObjectList())
{
    QStringList propertyNameList;

    if (inspectedObjects == 0 || inspectedObjects->contains(object))
        return propertyNameList;

    inspectedObjects->append(object);

    const QMetaObject *metaObject = object->metaObject();
    for (int index = 0; index < metaObject->propertyCount(); ++index) {
        QMetaProperty metaProperty = metaObject->property(index);
        QDeclarativeProperty declarativeProperty(object, QLatin1String(metaProperty.name()));
        if (declarativeProperty.isValid() && !declarativeProperty.isWritable() && declarativeProperty.propertyTypeCategory() == QDeclarativeProperty::Object) {
            if (declarativeProperty.name() != "parent") {
                QObject *childObject = QDeclarativeMetaType::toQObject(declarativeProperty.read());
                if (childObject)
                    propertyNameList.append(propertyNameForWritableProperties(childObject, baseName +  QString::fromUtf8(metaProperty.name()) + '.', inspectedObjects));
            }
        } else if (QDeclarativeValueTypeFactory::valueType(metaProperty.userType())) {
            QDeclarativeValueType *valueType = QDeclarativeValueTypeFactory::valueType(metaProperty.userType());
            valueType->setValue(metaProperty.read(object));
            propertyNameList.append(propertyNameForWritableProperties(valueType, baseName +  QString::fromUtf8(metaProperty.name()) + '.', inspectedObjects));
        } else if (metaProperty.isReadable() && metaProperty.isWritable()) {
            propertyNameList.append(baseName + QString::fromUtf8(metaProperty.name()));
        }
    }

    return propertyNameList;
}

static void fixResourcePathsForObject(QObject *object)
{
    if (qgetenv("QMLDESIGNER_RC_PATHS").isEmpty())
        return;

    QStringList propertyNameList = propertyNameForWritableProperties(object);

    foreach (const QString &propertyName, propertyNameList) {
        QDeclarativeProperty property(object, propertyName, QDeclarativeEngine::contextForObject(object));

        const QVariant value  = property.read();
        const QVariant fixedValue = ObjectNodeInstance::fixResourcePaths(value);
        if (value != fixedValue) {
            property.write(fixedValue);
        }
    }
}

void tweakObjects(QObject *object)
{
    QObjectList objectList;
    allSubObject(object, objectList);
    foreach(QObject* childObject, objectList) {
        disableTiledBackingStore(childObject);
        stopAnimation(childObject);
        fixResourcePathsForObject(childObject);
    }
}

QObject *ObjectNodeInstance::createComponentWrap(const QString &nodeSource, const QStringList &imports, QDeclarativeContext *context)
{
    QDeclarativeComponent *component = new QDeclarativeComponent(context->engine());

    QByteArray importArray;

    foreach (const QString &import, imports) {
        importArray.append(import.toUtf8());
    }

    QByteArray data(nodeSource.toUtf8());

    data.prepend(importArray);

    component->setData(data, context->baseUrl().resolved(QUrl("createComponent.qml")));

    QObject *object = component;
    tweakObjects(object);

    if (object && context)
        QDeclarativeEngine::setContextForObject(object, context);

    QDeclarativeEngine::setObjectOwnership(object, QDeclarativeEngine::CppOwnership);

    return object;
}


//The component might also be shipped with Creator.
//To avoid trouble with import "." we use the component shipped with Creator.
static inline QString fixComponentPathForIncompatibleQt(const QString &componentPath)
{
    QString result = componentPath;
    const QLatin1String importString("/imports/");

    if (componentPath.contains(importString)) {
        int index = componentPath.indexOf(importString) + 8;
        const QString relativeImportPath = componentPath.right(componentPath.length() - index);
        QString fixedComponentPath = QLibraryInfo::location(QLibraryInfo::ImportsPath) + relativeImportPath;
        fixedComponentPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
        if (QFileInfo(fixedComponentPath).exists())
            return fixedComponentPath;
        QString fixedPath = QFileInfo(fixedComponentPath).path();
        if (fixedPath.endsWith(QLatin1String(".1.0"))) {
        //plugin directories might contain the version number
            fixedPath.chop(4);
            fixedPath += QLatin1Char('/') + QFileInfo(componentPath).fileName();
            if (QFileInfo(fixedPath).exists())
                return fixedPath;
        }
    }

    return result;
}

QObject *ObjectNodeInstance::createComponent(const QString &componentPath, QDeclarativeContext *context)
{    
    QDeclarativeComponent component(context->engine(), QUrl::fromLocalFile(fixComponentPathForIncompatibleQt(componentPath)));
    QObject *object = component.beginCreate(context);

    tweakObjects(object);
    component.completeCreate();

    if (component.isError()) {
        qDebug() << componentPath;
        foreach(const QDeclarativeError &error, component.errors())
            qDebug() << error;
    }

    QDeclarativeEngine::setObjectOwnership(object, QDeclarativeEngine::CppOwnership);

    return object;
}

QObject *ObjectNodeInstance::createCustomParserObject(const QString &nodeSource, const QStringList &imports, QDeclarativeContext *context)
{
    QDeclarativeComponent component(context->engine());

    QByteArray importArray;
    foreach(const QString &import, imports) {
        importArray.append(import.toUtf8());
    }

    QByteArray data(nodeSource.toUtf8());

    data.prepend(importArray);

    component.setData(data, context->baseUrl().resolved(QUrl("createCustomParserObject.qml")));

    QObject *object = component.beginCreate(context);
    tweakObjects(object);
    component.completeCreate();

    QDeclarativeEngine::setObjectOwnership(object, QDeclarativeEngine::CppOwnership);

    return object;
}

QObject *ObjectNodeInstance::createPrimitive(const QString &typeName, int majorNumber, int minorNumber, QDeclarativeContext *context)
{
    QObject *object = 0;
    QDeclarativeType *type = QDeclarativeMetaType::qmlType(typeName.toUtf8(), majorNumber, minorNumber);
    if (type)  {
        if (type->typeName() == "QDeclarativeComponent") {
            object = new QDeclarativeComponent(context->engine(), 0);
        } else  {
            object = type->create();
        }
    } else {
        qWarning() << "QuickDesigner: Cannot create an object of type"
                   << QString("%1 %2,%3").arg(typeName).arg(majorNumber).arg(minorNumber)
                   << "- type isn't known to declarative meta type system";
    }

    tweakObjects(object);

    if (object && context)
        QDeclarativeEngine::setContextForObject(object, context);

    QDeclarativeEngine::setObjectOwnership(object, QDeclarativeEngine::CppOwnership);

    return object;
}

QObject *ObjectNodeInstance::object() const
{
        if (!m_object.isNull() && !QObjectPrivate::get(m_object.data())->wasDeleted)
            return m_object.data();
        return 0;
}

bool ObjectNodeInstance::hasContent() const
{
    return false;
}

bool ObjectNodeInstance::isResizable() const
{
    return false;
}

bool ObjectNodeInstance::isMovable() const
{
    return false;
}

bool ObjectNodeInstance::isInPositioner() const
{
    return m_isInPositioner;
}

void ObjectNodeInstance::setInPositioner(bool isInPositioner)
{
    m_isInPositioner = isInPositioner;
}

void ObjectNodeInstance::refreshPositioner()
{
}

void ObjectNodeInstance::updateAnchors()
{
}

QDeclarativeContext *ObjectNodeInstance::context() const
{
    if (nodeInstanceServer())
        return nodeInstanceServer()->context();

    qWarning() << "Error: No NodeInstanceServer";
    return 0;
}

QDeclarativeEngine *ObjectNodeInstance::engine() const
{
    return nodeInstanceServer()->engine();
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

void ObjectNodeInstance::populateResetHashes()
{
    QStringList propertyNameList = propertyNameForWritableProperties(object());

    foreach(const QString &propertyName, propertyNameList) {
        QDeclarativeProperty property(object(), propertyName, QDeclarativeEngine::contextForObject(object()));

        QDeclarativeAbstractBinding::Pointer binding = QDeclarativeAbstractBinding::getPointer(QDeclarativePropertyPrivate::binding(property));
        if (binding) {
            m_resetBindingHash.insert(propertyName, binding);
        } else if (property.isWritable()) {
            m_resetValueHash.insert(propertyName, property.read());
        }
    }
}

QDeclarativeAbstractBinding *ObjectNodeInstance::resetBinding(const QString &propertyName) const
{
    return m_resetBindingHash.value(propertyName).data();
}

bool ObjectNodeInstance::hasValidResetBinding(const QString &propertyName) const
{
    return m_resetBindingHash.contains(propertyName) &&  m_resetBindingHash.value(propertyName).data();
}

QVariant ObjectNodeInstance::resetValue(const QString &propertyName) const
{
    return m_resetValueHash.value(propertyName);
}

void ObjectNodeInstance::setResetValue(const QString &propertyName, const QVariant &value)
{
    m_resetValueHash.insert(propertyName, value);
}

void ObjectNodeInstance::paint(QPainter * /*painter*/)
{
}

QImage ObjectNodeInstance::renderImage() const
{
    return QImage();
}

QObject *ObjectNodeInstance::parent() const
{
    if (!object())
        return 0;

    return object()->parent();
}

QObject *parentObject(QObject *object)
{
    QGraphicsObject *graphicsObject = qobject_cast<QGraphicsObject*>(object);
    if (graphicsObject)
        return graphicsObject->parentObject();

    return object->parent();
}

ObjectNodeInstance::Pointer ObjectNodeInstance::parentInstance() const
{
    QObject *parentHolder = parent();
    if (!nodeInstanceServer())
        return Pointer();

    while (parentHolder) {
        if (nodeInstanceServer()->hasInstanceForObject(parentHolder))
            return nodeInstanceServer()->instanceForObject(parentHolder).internalInstance();

        parentHolder = parentObject(parentHolder);
    }

    return Pointer();
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

void ObjectNodeInstance::createDynamicProperty(const QString &name, const QString &/*typeName*/)
{
    if (m_metaObject == 0) {
        qWarning() << "ObjectNodeInstance.createDynamicProperty: No Metaobject.";
        return;
    }

    m_metaObject->createNewProperty(name);
}

bool ObjectNodeInstance::updateStateVariant(const ObjectNodeInstance::Pointer &/*target*/, const QString &/*propertyName*/, const QVariant &/*value*/)
{
    return false;
}

bool ObjectNodeInstance::updateStateBinding(const ObjectNodeInstance::Pointer &/*target*/, const QString &/*propertyName*/, const QString &/*expression*/)
{
    return false;
}

bool ObjectNodeInstance::resetStateProperty(const ObjectNodeInstance::Pointer &/*target*/, const QString &/*propertyName*/, const QVariant &/*resetValue*/)
{
    return false;
}

void ObjectNodeInstance::doComponentComplete()
{

}

bool ObjectNodeInstance::isRootNodeInstance() const
{
    return nodeInstanceServer()->rootNodeInstance().isWrappingThisObject(object());
}

bool ObjectNodeInstance::isValid() const
{
    return instanceId() >= 0 && object();
}

}
}

