// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "objectnodeinstance.h"

#include <enumeration.h>
#include <qmlprivategate.h>

#include <QDebug>
#include <QDir>
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
#include <QQmlExpression>
#include <QQmlParserStatus>
#include <QTextDocument>
#include <QLibraryInfo>
#include <QJSValue>

#include <private/qquickstategroup_p.h>

#include <qquickitem.h>

static bool isSimpleExpression(const QString &expression)
{
    if (expression.startsWith(QStringLiteral("{")))
        return false;

    return true;
}

namespace QmlDesigner {
namespace Internal {

ObjectNodeInstance::ObjectNodeInstance(QObject *object)
    : m_object(object),
      m_instanceId(-1),
      m_deleteHeldInstance(true),
      m_isInLayoutable(false)
{
    if (object)
        QObject::connect(m_object.data(), &QObject::destroyed, [this, object] {
            handleObjectDeletion(object);
        });
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

    m_instanceId = -1;
}

void ObjectNodeInstance::handleObjectDeletion(QObject *object)
{
    // We must pass the m_instanceId here, because this instance is no longer
    // valid, so the wrapper ServerNodeInstance will report -1 for id.
    if (m_nodeInstanceServer)
        m_nodeInstanceServer->removeInstanceRelationsipForDeletedObject(object, m_instanceId);
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
    m_signalSpy.setObjectNodeInstance(objectNodeInstance);
}

void ObjectNodeInstance::watchProperty(const PropertyName &name)
{
    m_signalSpy.registerDynamicProperty(name, object());
}

void ObjectNodeInstance::initialize(const ObjectNodeInstance::Pointer &objectNodeInstance,
                                    InstanceContainer::NodeFlags /*flags*/)
{
    initializePropertyWatcher(objectNodeInstance);
}

void ObjectNodeInstance::setId(const QString &id)
{
    if (!m_id.isEmpty() && context()) {
        context()->engine()->rootContext()->setContextProperty(m_id, nullptr);
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

bool ObjectNodeInstance::isLayoutable() const
{
    return false;
}

bool ObjectNodeInstance::isRenderable() const
{
    return false;
}

bool ObjectNodeInstance::isPropertyChange() const
{
    return false;
}

bool ObjectNodeInstance::isComposedEffect() const
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
    return {PropertyName(), ServerNodeInstance()};
}


static bool isList(const QQmlProperty &property)
{
    return property.propertyTypeCategory() == QQmlProperty::List;
}

static bool isQJSValue(const QQmlProperty &property)
{
    return property.isValid() && !strcmp(property.propertyTypeName(), "QJSValue");
}

static bool isObject(const QQmlProperty &property)
{
    /* QVariant and QJSValue can also store QObjects. Lets trust our model. */
    return property.isValid() && (property.propertyTypeCategory() == QQmlProperty::Object
            || !strcmp(property.propertyTypeName(), "QVariant")
            || isQJSValue(property));
}

static QVariant objectToVariant(QObject *object)
{
    return QVariant::fromValue(object);
}

static void removeObjectFromList(const QQmlProperty &property,
                                 QObject *objectToBeRemoved,
                                 [[maybe_unused]] QQmlEngine *engine)
{
    QQmlListReference listReference(property.object(), property.name().toUtf8());

    if (!QmlPrivateGate::hasFullImplementedListInterface(listReference)) {
        qWarning() << "Property list interface not fully implemented for Class " << property.property().typeName() << " in property " << property.name() << "!";
        return;
    }

    int count = listReference.count();

    QList<QPointer<QObject>> objectList;

    for (int i = 0; i < count; i ++) {
        QObject *listItem = listReference.at(i);
        if (listItem && listItem != objectToBeRemoved)
            objectList.append(listItem);
    }

    listReference.clear();

    for (QObject *object : std::as_const(objectList)) {
        if (object)
            listReference.append(object);
    }
}

void ObjectNodeInstance::removeFromOldProperty(QObject *object, QObject *oldParent, const PropertyName &oldParentProperty)
{
    QQmlProperty property(oldParent, QString::fromUtf8(oldParentProperty), context());

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
        object->setParent(nullptr);
}

void ObjectNodeInstance::addToNewProperty(QObject *object, QObject *newParent, const PropertyName &newParentProperty)
{
    QQmlProperty property(newParent, QString::fromUtf8(newParentProperty), context());

    if (object)
        object->setParent(newParent);

    if (isList(property)) {
        QQmlListReference list = qvariant_cast<QQmlListReference>(property.read());

        if (!list.isValid() || !list.canAppend()) {
            qWarning() << "Property list interface not fully implemented for Class " << property.property().typeName() << " in property " << property.name() << "!";
            return;
        }

        list.append(object);
    } else if (isObject(property)) {
        if (isQJSValue(property)) /* In this case we have to explcitly generate and convert a QJSValue */
            property.write(QVariant::fromValue(engine()->newQObject(object)));
        else
            property.write(objectToVariant(object));

        if (QQuickItem *item = qobject_cast<QQuickItem *>(object))
            if (QQuickItem *newParentItem = qobject_cast<QQuickItem *>(newParent))
                item->setParentItem(newParentItem);
    }

    Q_ASSERT(objectToVariant(object).isValid());
}

void ObjectNodeInstance::reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const PropertyName &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const PropertyName &newParentProperty)
{
    if (oldParentInstance && !oldParentInstance->ignoredProperties().contains(oldParentProperty)) {
        removeFromOldProperty(object(), oldParentInstance->object(), oldParentProperty);
        m_parentProperty.clear();
    }

    if (newParentInstance && !newParentInstance->ignoredProperties().contains(newParentProperty)) {
        m_parentProperty = newParentProperty;
        addToNewProperty(object(), newParentInstance->object(), newParentProperty);
    }
}

QVariant ObjectNodeInstance::convertSpecialCharacter(const QVariant& value) const
{
    QVariant specialCharacterConvertedValue = value;
    if (value.typeId() == QMetaType::QString) {
        QString string = value.toString();
        string.replace(QLatin1String("\\n"), QLatin1String("\n"));
        string.replace(QLatin1String("\\t"), QLatin1String("\t"));
        specialCharacterConvertedValue = string;
    }

    return specialCharacterConvertedValue;
}

void ObjectNodeInstance::updateAllDirtyNodesRecursive()
{
}

PropertyNameList ObjectNodeInstance::ignoredProperties() const
{
    return PropertyNameList();
}

void ObjectNodeInstance::setHiddenInEditor(bool b)
{
    m_isHiddenInEditor = b;
}

bool ObjectNodeInstance::isHiddenInEditor() const
{
    return m_isHiddenInEditor;
}

void ObjectNodeInstance::setLockedInEditor(bool b)
{
    m_isLockedInEditor = b;
}

bool ObjectNodeInstance::isLockedInEditor() const
{
    return m_isLockedInEditor;
}

bool ObjectNodeInstance::isComponentWrap() const
{
    return m_isComponentWrap;
}

void ObjectNodeInstance::setComponentWrap(bool wrap)
{
    m_isComponentWrap = wrap;
}

void ObjectNodeInstance::setModifiedFlag(bool b)
{
    m_isModified = b;
}

void ObjectNodeInstance::handleNewDynamicProperty(const PropertyName &name)
{
    watchProperty(name);
}

QVariant ObjectNodeInstance::convertEnumToValue(const QVariant &value, const PropertyName &name)
{
    Q_ASSERT(value.canConvert<Enumeration>());
    int propertyIndex = object()->metaObject()->indexOfProperty(name);
    QMetaProperty metaProperty = object()->metaObject()->property(propertyIndex);

    QVariant adjustedValue;
    Enumeration enumeration = value.value<Enumeration>();
    if (metaProperty.isValid() && metaProperty.isEnumType()) {
        adjustedValue = metaProperty.enumerator().keyToValue(enumeration.toName());
    } else {
        QQmlExpression expression(context(), object(), enumeration.toString());
        adjustedValue =  expression.evaluate();
        if (expression.hasError())
            qDebug() << "Enumeration cannot be evaluated:" << object() << name << enumeration;
    }
    return adjustedValue;
}

void ObjectNodeInstance::setPropertyVariant(const PropertyName &name, const QVariant &value)
{
    if (name == "state" && object() && object()->metaObject()
        && object()->metaObject()->className() == QByteArrayLiteral("QQuickStateGroup"))
        return;

    if (ignoredProperties().contains(name))
        return;

    if (m_isModified)
        return;

    QQmlProperty property(object(), QString::fromUtf8(name), context());

    if (!property.isValid())
        return;

    QVariant adjustedValue;
    if (value.canConvert<Enumeration>())
        adjustedValue = convertEnumToValue(value, name);
    else
        adjustedValue = QmlPrivateGate::fixResourcePaths(value);


    QVariant oldValue = property.read();
    if (oldValue.typeId() == QMetaType::QUrl) {
        QUrl url = oldValue.toUrl();
        QString path = url.toLocalFile();
        if (QFileInfo::exists(path) && nodeInstanceServer() && !path.isEmpty())
            nodeInstanceServer()->removeFilePropertyFromFileSystemWatcher(object(), name, path);
    }

    if (hasValidResetBinding(name)) {
        QmlPrivateGate::keepBindingFromGettingDeleted(object(), context(), name);
    }

    bool isWritten = property.write(convertSpecialCharacter(adjustedValue));

    if (!isWritten)
        qDebug() << "ObjectNodeInstance.setPropertyVariant: Cannot be written: " << object() << name << adjustedValue;

    QVariant newValue = property.read();
    if (newValue.typeId() == QMetaType::QUrl) {
        QUrl url = newValue.toUrl();
        QString path = url.toLocalFile();
        if (QFileInfo::exists(path) && nodeInstanceServer() && !path.isEmpty())
            nodeInstanceServer()->addFilePropertyToFileSystemWatcher(object(), name, path);
    }
}

void ObjectNodeInstance::setPropertyBinding(const PropertyName &name, const QString &expression)
{
    if (ignoredProperties().contains(name))
        return;

    if (!isSimpleExpression(expression))
        return;

    bool qmlExpressionHasError = false;

    QStringList idlist;
    for (const auto &instance : nodeInstanceServer()->nodeInstances())
        idlist.append(instance.id());

    // Always set ids using the root context, since they are defined there anyway
    if (idlist.contains(expression)) {
        QmlPrivateGate::setPropertyBinding(object(),
            context()->engine()->rootContext(), name, expression);
        return;
    }

    if (!isPropertyChange()) {
        QQmlExpression qmlExpression(context(), object(), expression);
        qmlExpression.evaluate();
        qmlExpressionHasError = qmlExpression.hasError();
    }

    if (qmlExpressionHasError) {
        QmlPrivateGate::setPropertyBinding(object(),
                                           context()->engine()->rootContext(),
                                           name,
                                           expression);
    } else {
        QmlPrivateGate::setPropertyBinding(object(), context(), name, expression);
    }
}

void ObjectNodeInstance::deleteObjectsInList(const QQmlProperty &property)
{
    QObjectList objectList;
    QQmlListReference list = qvariant_cast<QQmlListReference>(property.read());

    if (!QmlPrivateGate::hasFullImplementedListInterface(list)) {
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
    if (ignoredProperties().contains(name))
        return;

    doResetProperty(name);

    if (name == "font.pixelSize")
        doResetProperty("font.pointSize");

    if (name == "font.pointSize")
        doResetProperty("font.pixelSize");
}

void ObjectNodeInstance::refreshProperty(const PropertyName &name)
{
    QQmlProperty property(object(), QString::fromUtf8(name), context());

    if (!property.isValid())
        return;

    QVariant oldValue(property.read());

    if (property.isResettable())
        property.reset();
    else
        property.write(resetValue(name));

    if (oldValue.typeId() == QMetaType::QUrl) {
        QByteArray key = oldValue.toUrl().toEncoded(QUrl::UrlFormattingOption(0x100));
        QString pixmapKey = QString::fromUtf8(key.constData(), key.size());
        QPixmapCache::remove(pixmapKey);
    }

    property.write(oldValue);
}

bool ObjectNodeInstance::hasBindingForProperty(const PropertyName &propertyName, bool *hasChanged) const
{
    return QmlPrivateGate::hasBindingForProperty(object(), context(), propertyName, hasChanged);
}

void ObjectNodeInstance::doResetProperty(const PropertyName &propertyName)
{
    QmlPrivateGate::doResetProperty(object(), context(), propertyName);
}

static bool isPropertyBlackListed(const PropertyName &propertyName)
{
    if (propertyName.contains(".") && propertyName.contains("__"))
        return true;
    if (propertyName.count(".") > 2)
        return true;
    return false;
}

QVariant ObjectNodeInstance::property(const PropertyName &name) const
{
    if (ignoredProperties().contains(name))
        return QVariant();

    // TODO: handle model nodes

    if (isPropertyBlackListed(name))
        return QVariant();

    QQmlProperty property(object(), QString::fromUtf8(name), context());
    if (property.property().isEnumType()) {
        QVariant value = property.read();
        QMetaEnum me = property.property().enumerator();
        return QVariant::fromValue<Enumeration>(Enumeration(me.scope(), me.valueToKey(value.toInt())));
    }

    if (property.propertyType() == QMetaType::QUrl) {
        QUrl url = property.read().toUrl();
        if (url.isEmpty())
            return QVariant();

        if (url.scheme() == "file") {
            QFileInfo fi{nodeInstanceServer()->fileUrl().toLocalFile()};
            return QUrl{fi.absoluteDir().relativeFilePath(url.toLocalFile())};
        }
    }

    return property.read();
}

void ObjectNodeInstance::ensureVector3DDotProperties(PropertyNameList &list) const
{
    const PropertyNameList properties = { "rotation", "scale", "pivot" };
    for (const auto &property : properties) {
        if (list.contains(property) && instanceType(property) == "QVector3D") {
            const PropertyNameList dotProperties = { "x", "y", "z" };
            for (const auto &dotProperty : dotProperties) {
                const PropertyName dotPropertyName = property + "." + dotProperty;
                if (!list.contains(dotPropertyName))
                    list.append(dotPropertyName);
            }
        }
    }
}

void ObjectNodeInstance::ensureValueTypeProperties(PropertyNameList &list) const
{
    const PropertyNameList pointDotProperties = {"x", "y"};
    const PropertyNameList sizeDotProperties = {"width", "height"};
    const PropertyNameList rectDotProperties = {"x", "y", "width", "height"};

    PropertyNameList valueTypeProperties;

    for (const auto &property : list) {
        const QString name = instanceType(property);
        PropertyNameList dotProperties;

        if (name == "QPoint" || name == "QPointF")
            dotProperties = pointDotProperties;

        if (name == "QSize" || name == "QSizeF")
            dotProperties = sizeDotProperties;

        if (name == "QRect" || name == "QRectF")
            dotProperties = rectDotProperties;

        for (const auto &dotProperty : dotProperties)
            valueTypeProperties.append(property + "." + dotProperty);
    }

    for (const auto &valueTypeProperty : valueTypeProperties) {
        if (!list.contains(valueTypeProperty))
            list.append(valueTypeProperty);
    }
}

PropertyNameList ObjectNodeInstance::propertyNames() const
{
    PropertyNameList list;
    if (isValid())
        list = QmlPrivateGate::allPropertyNames(object());

    ensureVector3DDotProperties(list);
    ensureValueTypeProperties(list);

    return list;
}

QString ObjectNodeInstance::instanceType(const PropertyName &name) const
{
    if (isPropertyBlackListed(name))
        return QLatin1String("undefined");

    QQmlProperty property(object(), QString::fromUtf8(name), context());
    if (!property.isValid())
        return QLatin1String("undefined");
    return QString::fromUtf8(property.propertyTypeName());
}

QList<ServerNodeInstance> ObjectNodeInstance::childItems() const
{
    return {};
}

QList<QQuickItem *> ObjectNodeInstance::allItemsRecursive() const
{
    return {};
}

QList<ServerNodeInstance> ObjectNodeInstance::stateInstances() const
{
    if (auto group = qobject_cast<QQuickStateGroup*>(object())) {
        QList<ServerNodeInstance> instanceList;
        const QList<QQuickState *> stateList = group->states();
        for (QQuickState *state : stateList) {
            if (state && nodeInstanceServer()->hasInstanceForObject(state))
                instanceList.append(nodeInstanceServer()->instanceForObject(state));
        }
        return instanceList;
    }

    return {};
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

QObject *ObjectNodeInstance::createPrimitive(const QString &typeName, int majorNumber, int minorNumber, QQmlContext *context)
{
    QString polishTypeName = typeName;
    if (typeName == "QtQuick.Controls/Popup" || typeName == "QtQuick.Controls/Drawer"
        || typeName == "QtQuick.Controls/Dialog" || typeName == "QtQuick.Controls/Menu"
        || typeName == "QtQuick.Controls/ToolTip"
        || typeName == "QtQuick.Controls/ApplicationWindow")
        polishTypeName = "QtQuick/Item";

    const QHash<QString, QString> mockHash = {{"QtQuick.Controls/SwipeView","qrc:/qtquickplugin/mockfiles/SwipeView.qml"},
                                             {"QtQuick.Dialogs/Dialog","qrc:/qtquickplugin/mockfiles/Dialog.qml"}};

    QObject *object = nullptr;

    if (mockHash.contains(typeName))
        object = QmlPrivateGate::createComponent(mockHash.value(typeName), context);
    else if (majorNumber != -1 && minorNumber != -1)
        object = QmlPrivateGate::createPrimitive(polishTypeName, majorNumber, minorNumber, context);
    else
        object = QmlPrivateGate::createPrimitive(polishTypeName, 0, 0, context);

    /* Let's try to create the primitive from source, since with incomplete meta info this might be a pure
     * QML type. This is the case for example if a C++ type is mocked up with a QML file.
     */

    if (!object && majorNumber == 6 && minorNumber == 0) {
        object = QmlPrivateGate::createPrimitive(polishTypeName, 2, 15, context);
        if (!object)
            object = QmlPrivateGate::createPrimitive(polishTypeName, 1, 0, context);
    }

    if (!object)
        object = createPrimitiveFromSource(polishTypeName, majorNumber, minorNumber, context);

    return object;
}

QObject *ObjectNodeInstance::createPrimitiveFromSource(const QString &typeName, int majorNumber, int minorNumber, QQmlContext *context)
{
    if (typeName.isEmpty())
        return nullptr;

    QStringList parts = typeName.split("/");
    const QString unqualifiedTypeName = parts.last();
    parts.removeLast();

    if (parts.isEmpty())
        return nullptr;

    QString importString = parts.join(".");
    if (majorNumber >= 0)
        importString = importString + " " + QString::number(majorNumber) + "."
                       + QString::number(minorNumber);
    if (importString == "QtQuick 1.0") /* Workaround for implicit QQml import */
        importString = "QtQuick 2.0";
    QString source = "import " + importString + "\n" + unqualifiedTypeName + " {\n" + "}\n";
    return createCustomParserObject(source, "", context);
}

QObject *ObjectNodeInstance::createComponentWrap(const QString &nodeSource, const QByteArray &importCode, QQmlContext *context)
{
    [[maybe_unused]] QmlPrivateGate::ComponentCompleteDisabler disableComponentComplete;

    QQmlComponent *component = new QQmlComponent(context->engine());

    QByteArray data(nodeSource.toUtf8());
    if (data.isEmpty()) {
        // Add a fake root element as an empty component is not valid and crashes in some cases
        data.append("QtObject{}");
    }
    data.prepend(importCode);
    component->setData(data, context->baseUrl().resolved(QUrl("createComponent.qml")));
    QObject *object = component;
    QmlPrivateGate::tweakObjects(object);
    QQmlEngine::setContextForObject(object, context);
    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);

    if (component->isError()) {
        qWarning() << "Error in:" << Q_FUNC_INFO << component->url().toString();
        const QList<QQmlError> errors = component->errors();
        for (const QQmlError &error : errors)
            qWarning() << error;
        qWarning() << "file data:\n" << data;
    }
    return object;
}

//The component might also be shipped with Creator.
//To avoid trouble with import "." we use the component shipped with Creator.
inline static QString fixComponentPathForIncompatibleQt(const QString &componentPath)
{
    QString result = componentPath;
    const QLatin1String importString("/imports/");

    if (componentPath.contains(importString)) {
        int index = componentPath.indexOf(importString) + 8;
        const QString relativeImportPath = componentPath.right(componentPath.length() - index);
        QString fixedComponentPath = QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath) + relativeImportPath;
        fixedComponentPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
        if (QFileInfo::exists(fixedComponentPath))
            return fixedComponentPath;
        QString fixedPath = QFileInfo(fixedComponentPath).path();
        if (fixedPath.endsWith(QLatin1String(".1.0"))) {
        //plugin directories might contain the version number
            fixedPath.chop(4);
            fixedPath += QLatin1Char('/') + QFileInfo(componentPath).fileName();
            if (QFileInfo::exists(fixedPath))
                return fixedPath;
        }
    }

    return result;
}

QObject *ObjectNodeInstance::createComponent([[maybe_unused]] const QString &componentPath,
                                             QQmlContext *context)
{
    QmlPrivateGate::ComponentCompleteDisabler disableComponentComplete;

    QQmlComponent component(context->engine(), fixComponentPathForIncompatibleQt(componentPath));

    QObject *object = nullptr;
    if (!component.isError()) {
        object = component.beginCreate(context);
        QmlPrivateGate::tweakObjects(object);
        component.completeCreate();
        QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
    }

    if (component.isError()) {
        qDebug() << componentPath;
        const QList<QQmlError> errors = component.errors();
        for (const QQmlError &error : errors)
            qWarning() << error;
    }

    if (object)
        object->setProperty("__designer_url__", QUrl::fromLocalFile(componentPath));

    return object;
}

QObject *ObjectNodeInstance::createComponent(const QUrl &componentUrl, QQmlContext *context)
{
    return QmlPrivateGate::createComponent(componentUrl, context);
}

QObject *ObjectNodeInstance::createCustomParserObject(const QString &nodeSource,
                                                      const QByteArray &importCode,
                                                      QQmlContext *context)
{
    [[maybe_unused]] QmlPrivateGate::ComponentCompleteDisabler disableComponentComplete;

    QQmlComponent component(context->engine());

    QByteArray data(nodeSource.toUtf8());
    data.prepend(importCode);
    component.setData(data, context->baseUrl().resolved(QUrl("createCustomParserObject.qml")));
    QObject *object = component.beginCreate(context);
    if (object) {
        QmlPrivateGate::tweakObjects(object);
        component.completeCreate();
        QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
    }

    if (component.isError()) {
        qWarning() << "Error in:" << Q_FUNC_INFO << component.url().toString();
        const QList<QQmlError> errors = component.errors();
        for (const QQmlError &error : errors)
            qWarning() << error;
        qWarning() << "file data:\n" << data;
    }
    return object;
}

QObject *ObjectNodeInstance::object() const
{
        if (!m_object.isNull() && !QmlPrivateGate::objectWasDeleted(m_object.data()))
            return m_object.data();
        return nullptr;
}

QQuickItem *ObjectNodeInstance::contentItem() const
{
    return nullptr;
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
    return nullptr;
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

QStringList ObjectNodeInstance::allStates() const
{
    return {};
}

void ObjectNodeInstance::populateResetHashes()
{
    QmlPrivateGate::registerCustomData(object());
}

bool ObjectNodeInstance::hasValidResetBinding(const PropertyName &propertyName) const
{
    return QmlPrivateGate::hasValidResetBinding(object(), propertyName);
}

QVariant ObjectNodeInstance::resetValue(const PropertyName &propertyName) const
{
    return QmlPrivateGate::getResetValue(object(), propertyName);
}

QImage ObjectNodeInstance::renderImage() const
{
    return QImage();
}

QImage ObjectNodeInstance::renderPreviewImage(const QSize & /*previewImageSize*/) const
{
    return QImage();
}

QSharedPointer<QQuickItemGrabResult> ObjectNodeInstance::createGrabResult() const
{
    return {};
}

QObject *ObjectNodeInstance::parent() const
{
    if (!object())
        return nullptr;

    return object()->parent();
}

QObject *ObjectNodeInstance::parentObject(QObject *object)
{
    QQuickItem *quickItem = qobject_cast<QQuickItem*>(object);
    if (quickItem && quickItem->parentItem())
        return quickItem->parentItem();

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

QSizeF ObjectNodeInstance::implicitSize() const
{
    return QSizeF();
}

int ObjectNodeInstance::penWidth() const
{
    return 0;
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
    QmlPrivateGate::doComponentCompleteRecursive(object(), nodeInstanceServer());
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

