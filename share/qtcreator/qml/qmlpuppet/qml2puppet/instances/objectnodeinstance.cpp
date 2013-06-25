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
#include <QQmlContext>
#include <QQmlError>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QQmlComponent>
#include <QSharedPointer>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QPixmapCache>
#include <QQuickItem>
#include <QQmlParserStatus>
#include <QTextDocument>
#include <QLibraryInfo>

#include <private/qqmlbinding_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qquicktransition_p.h>
#include <private/qquickanimation_p.h>
#include <private/qqmltimer_p.h>
#include <private/qqmlengine_p.h>
#include <designersupport.h>


namespace {
class ComponentCompleteDisabler
{
public:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 1, 0))
    ComponentCompleteDisabler()
    {
        DesignerSupport::disableComponentComplete();
    }

    ~ComponentCompleteDisabler()
    {
        DesignerSupport::enableComponentComplete();
    }
#else
    ComponentCompleteDisabler()
    {
    //nothing not available yet
    }
#endif
};

} //namespace

static bool isPropertyBlackListed(const QmlDesigner::PropertyName &propertyName)
{
    if (propertyName.contains(".") && propertyName.contains("__"))
        return true;

    if (propertyName.count(".") > 1)
        return true;

    return false;
}

namespace QmlDesigner {
namespace Internal {

ObjectNodeInstance::ObjectNodeInstance(QObject *object)
    : m_object(object),
      m_metaObject(0),
      m_instanceId(-1),
      m_deleteHeldInstance(true),
      m_isInLayoutable(false)
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
                reparent(parentInstance(), m_parentProperty, ObjectNodeInstance::Pointer(), PropertyName());
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

void ObjectNodeInstance::initializePropertyWatcher(const ObjectNodeInstance::Pointer &objectNodeInstance)
{
    m_metaObject = NodeInstanceMetaObject::createNodeInstanceMetaObject(objectNodeInstance, nodeInstanceServer()->engine());
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

bool ObjectNodeInstance::isQuickWindow() const
{
    return false;
}

bool ObjectNodeInstance::isGraphical() const
{
    return false;
}

bool ObjectNodeInstance::isLayoutable() const
{
    return true;
}

bool ObjectNodeInstance::equalGraphicsItem(QGraphicsItem * /*item*/) const
{
    return false;
}

QTransform ObjectNodeInstance::transform() const
{
    return QTransform();
}

QTransform ObjectNodeInstance::contentTransform() const
{
    return QTransform();
}

QTransform ObjectNodeInstance::customTransform() const
{
    return QTransform();
}

QTransform ObjectNodeInstance::contentItemTransform() const
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

bool ObjectNodeInstance::hasAnchor(const PropertyName &/*name*/) const
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

QPair<PropertyName, ServerNodeInstance> ObjectNodeInstance::anchor(const PropertyName &/*name*/) const
{
    return qMakePair(PropertyName(), ServerNodeInstance());
}


static bool isList(const QQmlProperty &property)
{
    return property.propertyTypeCategory() == QQmlProperty::List;
}

static bool isObject(const QQmlProperty &property)
{
    return (property.propertyTypeCategory() == QQmlProperty::Object) ||
            //QVariant can also store QObjects. Lets trust our model.
           (QLatin1String(property.propertyTypeName()) == QLatin1String("QVariant"));
}

static QVariant objectToVariant(QObject *object)
{
    return QVariant::fromValue(object);
}

static bool hasFullImplementedListInterface(const QQmlListReference &list)
{
    return list.isValid() && list.canCount() && list.canAt() && list.canAppend() && list.canClear();
}

static void removeObjectFromList(const QQmlProperty &property, QObject *objectToBeRemoved, QQmlEngine * engine)
{
    QQmlListReference listReference(property.object(), property.name().toUtf8(), engine);

    if (!hasFullImplementedListInterface(listReference)) {
        qWarning() << "Property list interface not fully implemented for Class " << property.property().typeName() << " in property " << property.name() << "!";
        return;
    }

    int count = listReference.count();

    QObjectList objectList;

    for (int i = 0; i < count; i ++) {
        QObject *listItem = listReference.at(i);
        if (listItem && listItem != objectToBeRemoved)
            objectList.append(listItem);
    }

    listReference.clear();

    foreach (QObject *object, objectList)
        listReference.append(object);
}

void ObjectNodeInstance::removeFromOldProperty(QObject *object, QObject *oldParent, const PropertyName &oldParentProperty)
{
    QQmlProperty property(oldParent, oldParentProperty, context());

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

void ObjectNodeInstance::addToNewProperty(QObject *object, QObject *newParent, const PropertyName &newParentProperty)
{
    QQmlProperty property(newParent, newParentProperty, context());

    if (object)
        object->setParent(newParent);

    if (isList(property)) {
        QQmlListReference list = qvariant_cast<QQmlListReference>(property.read());

        if (!hasFullImplementedListInterface(list)) {
            qWarning() << "Property list interface not fully implemented for Class " << property.property().typeName() << " in property " << property.name() << "!";
            return;
        }

        list.append(object);
    } else if (isObject(property)) {
        property.write(objectToVariant(object));
    }

    Q_ASSERT(objectToVariant(object).isValid());
}

void ObjectNodeInstance::reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const PropertyName &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const PropertyName &newParentProperty)
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

void ObjectNodeInstance::updateAllDirtyNodesRecursive()
{
}

void ObjectNodeInstance::setPropertyVariant(const PropertyName &name, const QVariant &value)
{
    QQmlProperty property(object(), name, context());

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
        QQmlPropertyPrivate::setBinding(property, 0, QQmlPropertyPrivate::BypassInterceptor | QQmlPropertyPrivate::DontRemoveBinding);
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

void ObjectNodeInstance::setPropertyBinding(const PropertyName &name, const QString &expression)
{
    QQmlProperty property(object(), name, context());

    if (!property.isValid())
        return;

    if (property.isProperty()) {
        QQmlBinding *binding = new QQmlBinding(expression, object(), context());
        binding->setTarget(property);
        binding->setNotifyOnValueChanged(true);
        QQmlAbstractBinding *oldBinding = QQmlPropertyPrivate::setBinding(property, binding);
        if (oldBinding && !hasValidResetBinding(name))
            oldBinding->destroy();
        binding->update();
        if (binding->hasError()) {
            //qDebug() <<" ObjectNodeInstance.setPropertyBinding has Error: " << object() << name << expression << binding->error(engine()).toString();
            if (property.property().userType() == QVariant::String)
                property.write(QVariant(QString("#%1#").arg(expression)));
        }

    } else {
        qWarning() << "ObjectNodeInstance.setPropertyBinding: Cannot set binding for property" << name << ": property is unknown for type";
    }
}

void ObjectNodeInstance::deleteObjectsInList(const QQmlProperty &property)
{
    QObjectList objectList;
    QQmlListReference list = qvariant_cast<QQmlListReference>(property.read());

    if (!hasFullImplementedListInterface(list)) {
        qWarning() << "Property list interface not fully implemented for Class " << property.property().typeName() << " in property " << property.name() << "!";
        return;
    }

    for (int i = 0; i < list.count(); i++) {
        objectList += list.at(i);
    }

    list.clear();
}

void ObjectNodeInstance::resetProperty(const PropertyName &name)
{
    doResetProperty(name);

    if (name == "font.pixelSize")
        doResetProperty("font.pointSize");

    if (name == "font.pointSize")
        doResetProperty("font.pixelSize");
}

void ObjectNodeInstance::refreshProperty(const PropertyName &name)
{
    QQmlProperty property(object(), name, context());

    if (!property.isValid())
        return;

    QVariant oldValue(property.read());

    if (property.isResettable())
        property.reset();
    else
        property.write(resetValue(name));

    if (oldValue.type() == QVariant::Url) {
        QByteArray key = oldValue.toUrl().toEncoded(QUrl::UrlFormattingOption(0x100));
        QString pixmapKey = QString::fromUtf8(key.constData(), key.count());
        QPixmapCache::remove(pixmapKey);
    }

    property.write(oldValue);
}

bool ObjectNodeInstance::hasBindingForProperty(const PropertyName &name, bool *hasChanged) const
{
    if (isPropertyBlackListed(name))
        return false;

    QQmlProperty property(object(), name, context());

    bool hasBinding = QQmlPropertyPrivate::binding(property);

    if (hasChanged) {
        *hasChanged = hasBinding != m_hasBindingHash.value(name, false);
        if (*hasChanged)
            m_hasBindingHash.insert(name, hasBinding);
    }

    return QQmlPropertyPrivate::binding(property);
}

void ObjectNodeInstance::doResetProperty(const PropertyName &propertyName)
{
    m_modelAbstractPropertyHash.remove(propertyName);

    QQmlProperty property(object(), propertyName, context());

    if (!property.isValid())
        return;

    QVariant oldValue = property.read();
    if (oldValue.type() == QVariant::Url) {
        QUrl url = oldValue.toUrl();
        QString path = url.toLocalFile();
        if (QFileInfo(path).exists() && nodeInstanceServer())
            nodeInstanceServer()->removeFilePropertyFromFileSystemWatcher(object(), propertyName, path);
    }


    QQmlAbstractBinding *binding = QQmlPropertyPrivate::binding(property);
    if (binding && !(hasValidResetBinding(propertyName) && resetBinding(propertyName) == binding)) {
        binding->setEnabled(false, 0);
        binding->destroy();
    }


    if (hasValidResetBinding(propertyName)) {
        QQmlAbstractBinding *binding = resetBinding(propertyName);
        QQmlPropertyPrivate::setBinding(property, binding, QQmlPropertyPrivate::DontRemoveBinding);
        binding->update();
    } else if (property.isResettable()) {
        property.reset();
    } else if (property.propertyTypeCategory() == QQmlProperty::List) {
        QQmlListReference list = qvariant_cast<QQmlListReference>(property.read());

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

QVariant ObjectNodeInstance::property(const PropertyName &name) const
{
    if (m_modelAbstractPropertyHash.contains(name))
        return QVariant::fromValue(m_modelAbstractPropertyHash.value(name));

    // TODO: handle model nodes

    if (isPropertyBlackListed(name))
        return QVariant();

    QQmlProperty property(object(), name, context());
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

PropertyNameList allPropertyNames(QObject *object, const PropertyName &baseName = PropertyName(), QObjectList *inspectedObjects = new QObjectList)
{
    PropertyNameList propertyNameList;


    if (inspectedObjects== 0 || inspectedObjects->contains(object))
        return propertyNameList;

    inspectedObjects->append(object);


    const QMetaObject *metaObject = object->metaObject();
    for (int index = 0; index < metaObject->propertyCount(); ++index) {
        QMetaProperty metaProperty = metaObject->property(index);
        QQmlProperty declarativeProperty(object, QLatin1String(metaProperty.name()));
        if (declarativeProperty.isValid() && declarativeProperty.propertyTypeCategory() == QQmlProperty::Object) {
            if (declarativeProperty.name() != "parent") {
                QObject *childObject = QQmlMetaType::toQObject(declarativeProperty.read());
                if (childObject)
                    propertyNameList.append(allPropertyNames(childObject, baseName +  PropertyName(metaProperty.name()) + '.', inspectedObjects));
            }
        } else if (QQmlValueTypeFactory::valueType(metaProperty.userType())) {
            QQmlValueType *valueType = QQmlValueTypeFactory::valueType(metaProperty.userType());
            valueType->setValue(metaProperty.read(object));
            propertyNameList.append(allPropertyNames(valueType, baseName +  PropertyName(metaProperty.name()) + '.', inspectedObjects));
        } else  {
            propertyNameList.append(baseName + PropertyName(metaProperty.name()));
        }
    }

    return propertyNameList;
}

PropertyNameList ObjectNodeInstance::propertyNames() const
{
    if (isValid())
        return allPropertyNames(object());
    return PropertyNameList();
}

QString ObjectNodeInstance::instanceType(const PropertyName &name) const
{
    if (isPropertyBlackListed(name))
        return QLatin1String("undefined");

    QQmlProperty property(object(), name, context());
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

    QQuickTransition *transition = qobject_cast<QQuickTransition*>(object);
    QQuickAbstractAnimation *animation = qobject_cast<QQuickAbstractAnimation*>(object);
    QQmlTimer *timer = qobject_cast<QQmlTimer*>(object);
    if (transition) {
       transition->setFromState("");
       transition->setToState("");
    } else if (animation) {
//        QQuickScriptAction *scriptAimation = qobject_cast<QQuickScriptAction*>(animation);
//        if (scriptAimation) FIXME
//            scriptAimation->setScript(QQmlScriptString());
        animation->setLoops(1);
        animation->complete();
        animation->setDisableUserControl();
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
                && QQmlMetaType::isQObject(metaProperty.userType())) {
            if (metaProperty.name() != QLatin1String("parent")) {
                QObject *propertyObject = QQmlMetaType::toQObject(metaProperty.read(object));
                allSubObject(propertyObject, objectList);
            }

        }

        // search recursive in property object lists
        if (metaProperty.isReadable()
                && QQmlMetaType::isList(metaProperty.userType())) {
            QQmlListReference list(object, metaProperty.name());
            if (list.canCount() && list.canAt()) {
                for (int i = 0; i < list.count(); i++) {
                    QObject *propertyObject = list.at(i);
                    allSubObject(propertyObject, objectList);

                }
            }
        }
    }

    // search recursive in object children list
    foreach (QObject *childObject, object->children()) {
        allSubObject(childObject, objectList);
    }

    // search recursive in quick item childItems list
    QQuickItem *quickItem = qobject_cast<QQuickItem*>(object);
    if (quickItem) {
        foreach (QQuickItem *childItem, quickItem->childItems()) {
            allSubObject(childItem, objectList);
        }
    }
}

static void disableTiledBackingStore(QObject *object)
{
    Q_UNUSED(object);
}

static void addToPropertyNameListIfNotBlackListed(PropertyNameList *propertyNameList, const PropertyName &propertyName)
{
    if (!isPropertyBlackListed(propertyName))
        propertyNameList->append(propertyName);
}

PropertyNameList propertyNameListForWritableProperties(QObject *object, const PropertyName &baseName = PropertyName(), QObjectList *inspectedObjects = new QObjectList())
{
    PropertyNameList propertyNameList;

    if (inspectedObjects == 0 || inspectedObjects->contains(object))
        return propertyNameList;

    inspectedObjects->append(object);

    const QMetaObject *metaObject = object->metaObject();
    for (int index = 0; index < metaObject->propertyCount(); ++index) {
        QMetaProperty metaProperty = metaObject->property(index);
        QQmlProperty declarativeProperty(object, QLatin1String(metaProperty.name()));
        if (declarativeProperty.isValid() && !declarativeProperty.isWritable() && declarativeProperty.propertyTypeCategory() == QQmlProperty::Object) {
            if (declarativeProperty.name() != "parent") {
                QObject *childObject = QQmlMetaType::toQObject(declarativeProperty.read());
                if (childObject)
                    propertyNameList.append(propertyNameListForWritableProperties(childObject, baseName +  PropertyName(metaProperty.name()) + '.', inspectedObjects));
            }
        } else if (QQmlValueTypeFactory::valueType(metaProperty.userType())) {
            QQmlValueType *valueType = QQmlValueTypeFactory::valueType(metaProperty.userType());
            valueType->setValue(metaProperty.read(object));
            propertyNameList.append(propertyNameListForWritableProperties(valueType, baseName +  PropertyName(metaProperty.name()) + '.', inspectedObjects));
        }

        if (metaProperty.isReadable() && metaProperty.isWritable()) {
            addToPropertyNameListIfNotBlackListed(&propertyNameList, baseName + PropertyName(metaProperty.name()));
        }
    }

    return propertyNameList;
}

static void fixResourcePathsForObject(QObject *object)
{
    if (qgetenv("QMLDESIGNER_RC_PATHS").isEmpty())
        return;

    PropertyNameList propertyNameList = propertyNameListForWritableProperties(object);

    foreach (const PropertyName &propertyName, propertyNameList) {
        QQmlProperty property(object, propertyName, QQmlEngine::contextForObject(object));

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
    foreach (QObject* childObject, objectList) {
        disableTiledBackingStore(childObject);
        stopAnimation(childObject);
        fixResourcePathsForObject(childObject);
    }
}

QObject *ObjectNodeInstance::createComponentWrap(const QString &nodeSource, const QStringList &imports, QQmlContext *context)
{
    ComponentCompleteDisabler disableComponentComplete;

    Q_UNUSED(disableComponentComplete)

    QQmlComponent *component = new QQmlComponent(context->engine());

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
        QQmlEngine::setContextForObject(object, context);

    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);

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

QObject *ObjectNodeInstance::createComponent(const QString &componentPath, QQmlContext *context)
{
    ComponentCompleteDisabler disableComponentComplete;

    Q_UNUSED(disableComponentComplete)

    QQmlComponent component(context->engine(), fixComponentPathForIncompatibleQt(componentPath));
    QObject *object = component.beginCreate(context);

    tweakObjects(object);
    component.completeCreate();

    if (component.isError()) {
        qDebug() << componentPath;
        foreach (const QQmlError &error, component.errors())
            qDebug() << error;
    }

    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);

    return object;
}

QObject *ObjectNodeInstance::createCustomParserObject(const QString &nodeSource, const QStringList &imports, QQmlContext *context)
{
    ComponentCompleteDisabler disableComponentComplete;

    Q_UNUSED(disableComponentComplete)

    QQmlComponent component(context->engine());

    QByteArray importArray;
    foreach (const QString &import, imports) {
        importArray.append(import.toUtf8());
    }

    QByteArray data(nodeSource.toUtf8());

    data.prepend(importArray);

    component.setData(data, context->baseUrl().resolved(QUrl("createCustomParserObject.qml")));

    QObject *object = component.beginCreate(context);
    tweakObjects(object);
    component.completeCreate();

    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);

    return object;
}

QObject *ObjectNodeInstance::createPrimitive(const QString &typeName, int majorNumber, int minorNumber, QQmlContext *context)
{
    QObject *object = 0;
    QQmlType *type = QQmlMetaType::qmlType(typeName.toUtf8(), majorNumber, minorNumber);
    if (type)  {
        if (type->typeName() == "QQmlComponent") {
            object = new QQmlComponent(context->engine(), 0);
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
        QQmlEngine::setContextForObject(object, context);

    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);

    return object;
}

QObject *ObjectNodeInstance::object() const
{
        if (!m_object.isNull() && !QObjectPrivate::get(m_object.data())->wasDeleted)
            return m_object.data();
        return 0;
}

QQuickItem *ObjectNodeInstance::contentItem() const
{
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

bool ObjectNodeInstance::isInLayoutable() const
{
    return m_isInLayoutable;
}

void ObjectNodeInstance::setInLayoutable(bool isInLayoutable)
{
    m_isInLayoutable = isInLayoutable;
}

void ObjectNodeInstance::refreshLayoutable()
{
}

void ObjectNodeInstance::updateAnchors()
{
}

QQmlContext *ObjectNodeInstance::context() const
{
    if (nodeInstanceServer())
        return nodeInstanceServer()->context();

    qWarning() << "Error: No NodeInstanceServer";
    return 0;
}

QQmlEngine *ObjectNodeInstance::engine() const
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
    PropertyNameList propertyNameList = propertyNameListForWritableProperties(object());

    foreach (const PropertyName &propertyName, propertyNameList) {
        QQmlProperty property(object(), propertyName, QQmlEngine::contextForObject(object()));

        QQmlAbstractBinding::Pointer binding = QQmlAbstractBinding::getPointer(QQmlPropertyPrivate::binding(property));
        if (binding) {
            m_resetBindingHash.insert(propertyName, binding);
        } else if (property.isWritable()) {
            m_resetValueHash.insert(propertyName, property.read());
        }
    }
}

QQmlAbstractBinding *ObjectNodeInstance::resetBinding(const PropertyName &propertyName) const
{
    return m_resetBindingHash.value(propertyName).data();
}

bool ObjectNodeInstance::hasValidResetBinding(const PropertyName &propertyName) const
{
    return m_resetBindingHash.contains(propertyName) &&  m_resetBindingHash.value(propertyName).data();
}

QVariant ObjectNodeInstance::resetValue(const PropertyName &propertyName) const
{
    return m_resetValueHash.value(propertyName);
}

void ObjectNodeInstance::setResetValue(const PropertyName &propertyName, const QVariant &value)
{
    m_resetValueHash.insert(propertyName, value);
}

QImage ObjectNodeInstance::renderImage() const
{
    return QImage();
}

QImage ObjectNodeInstance::renderPreviewImage(const QSize & /*previewImageSize*/) const
{
    return QImage();
}

QObject *ObjectNodeInstance::parent() const
{
    if (!object())
        return 0;

    return object()->parent();
}

QObject *ObjectNodeInstance::parentObject(QObject *object)
{
    QQuickItem *quickItem = qobject_cast<QQuickItem*>(object);
    if (quickItem && quickItem->parentItem()) {

        //QQuickRootItem is used by Window and we want to return the Window as parent
        if (strcmp(quickItem->metaObject()->className(), "QQuickRootItem"))
            return object->parent();

        return quickItem->parentItem();
    }

    return object->parent();
}

void ObjectNodeInstance::doComponentCompleteRecursive(QObject *object, NodeInstanceServer *nodeInstanceServer)
{
    if (object) {
        QQuickItem *item = qobject_cast<QQuickItem*>(object);

        if (item && DesignerSupport::isComponentComplete(item))
            return;

        QList<QObject*> childList = object->children();

        if (item) {
            foreach (QQuickItem *childItem, item->childItems()) {
                if (!childList.contains(childItem))
                    childList.append(childItem);
            }
        }

        foreach (QObject *child, childList) {
            if (!nodeInstanceServer->hasInstanceForObject(child))
                doComponentCompleteRecursive(child, nodeInstanceServer);
        }

        if (item) {
            static_cast<QQmlParserStatus*>(item)->componentComplete();
        } else {
            QQmlParserStatus *qmlParserStatus = dynamic_cast< QQmlParserStatus*>(object);
            if (qmlParserStatus)
                qmlParserStatus->componentComplete();
        }
    }
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
    return QRectF();
}

QRectF ObjectNodeInstance::contentItemBoundingBox() const
{
    return QRectF();
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

bool ObjectNodeInstance::updateStateVariant(const ObjectNodeInstance::Pointer &/*target*/, const PropertyName &/*propertyName*/, const QVariant &/*value*/)
{
    return false;
}

bool ObjectNodeInstance::updateStateBinding(const ObjectNodeInstance::Pointer &/*target*/, const PropertyName &/*propertyName*/, const QString &/*expression*/)
{
    return false;
}

bool ObjectNodeInstance::resetStateProperty(const ObjectNodeInstance::Pointer &/*target*/, const PropertyName &/*propertyName*/, const QVariant &/*resetValue*/)
{
    return false;
}

void ObjectNodeInstance::doComponentComplete()
{
    doComponentCompleteRecursive(object(), nodeInstanceServer());
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

