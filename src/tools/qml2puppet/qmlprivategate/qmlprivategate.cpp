// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprivategate.h"

#include <objectnodeinstance.h>
#include <nodeinstanceserver.h>

#include <QQuickItem>
#include <QQmlComponent>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QJsonValue>

#include <private/qabstractfileengine_p.h>
#include <private/qfsfileengine_p.h>

#include <private/qquickdesignersupport_p.h>
#include <private/qquickdesignersupportmetainfo_p.h>
#include <private/qquickdesignersupportitems_p.h>
#include <private/qquickdesignersupportproperties_p.h>
#include <private/qquickdesignersupportpropertychanges_p.h>
#include <private/qquickdesignersupportstates_p.h>
#include <private/qqmldata_p.h>
#include <private/qqmlcomponentattached_p.h>

#include <private/qabstractanimation_p.h>
#include <private/qobject_p.h>
#include <private/qquickbehavior_p.h>
#include <private/qquicktext_p.h>
#include <private/qquicktextinput_p.h>
#include <private/qquicktextedit_p.h>
#include <private/qquicktransition_p.h>
#include <private/qquickloader_p.h>

#include <private/qquickanimation_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmltimer_p.h>
#include <private/qqmlanybinding_p.h>

#ifdef QUICK3D_MODULE
#include <private/qquick3dobject_p.h>
#include <private/qquick3drepeater_p.h>
#endif



namespace QmlDesigner {

namespace Internal {

static void addToPropertyNameListIfNotBlackListed(QQuickDesignerSupport::PropertyNameList *propertyNameList,
                                                  const QQuickDesignerSupport::PropertyName &propertyName)
{
    if (!QQuickDesignerSupportProperties::isPropertyBlackListed(propertyName))
        propertyNameList->append(propertyName);
}

static QQuickDesignerSupport::PropertyNameList propertyNameListForWritablePropertiesInternal(QObject *object,
                                                       const QQuickDesignerSupport::PropertyName &baseName,
                                                       QObjectList *inspectedObjects,
                                                       int depth = 0)
{
    QQuickDesignerSupport::PropertyNameList propertyNameList;

    if (depth > 2)
        return propertyNameList;

    if (!inspectedObjects->contains(object))
        inspectedObjects->append(object);

    const QMetaObject *metaObject = object->metaObject();
    for (int index = 0; index < metaObject->propertyCount(); ++index) {
        QMetaProperty metaProperty = metaObject->property(index);
        QQmlProperty declarativeProperty(object, QString::fromUtf8(metaProperty.name()));
        if (declarativeProperty.isValid() && !declarativeProperty.isWritable() && declarativeProperty.propertyTypeCategory() == QQmlProperty::Object) {
            if (declarativeProperty.name() != QLatin1String("parent")) {
                QObject *childObject = QQmlMetaType::toQObject(declarativeProperty.read());
                if (childObject)
                    propertyNameList.append(propertyNameListForWritablePropertiesInternal(childObject,
                                                                                  baseName +  QQuickDesignerSupport::PropertyName(metaProperty.name())
                                                                                  + '.', inspectedObjects,
                                                                                  depth + 1));
            }
        } else if (QQmlGadgetPtrWrapper *valueType
                   = QQmlGadgetPtrWrapper::instance(qmlEngine(object), metaProperty.metaType())) {


            const QVariant value = metaProperty.read(object);
            QMetaType jsType = QMetaType::fromType<QJSValue>();
            int userType = value.userType();

            if (userType != jsType.id()) {
                valueType->setValue(value);
                propertyNameList.append(propertyNameListForWritablePropertiesInternal(valueType,
                                                                                      baseName +  QQuickDesignerSupport::PropertyName(metaProperty.name())
                                                                                      + '.', inspectedObjects,
                                                                                      depth + 1));
            }

        }

        if (metaProperty.isReadable() && metaProperty.isWritable()) {
            addToPropertyNameListIfNotBlackListed(&propertyNameList,
                                                  baseName + QQuickDesignerSupport::PropertyName(metaProperty.name()));
        }
    }

    return propertyNameList;
}

static QQuickDesignerSupport::PropertyNameList propertyNameListForWritablePropertiesFork(QObject *object)
{
    QObjectList localObjectList;
    return propertyNameListForWritablePropertiesInternal(object, {}, &localObjectList);
}

static QQuickDesignerSupport::PropertyNameList allPropertyNamesFork(QObject *object,
                                  const QQuickDesignerSupport::PropertyName &baseName = QQuickDesignerSupport::PropertyName(),
                                  QObjectList *inspectedObjects = nullptr,
                                  int depth = 0)
{
    QQuickDesignerSupport::PropertyNameList propertyNameList;

    QObjectList localObjectList;

    if (inspectedObjects == nullptr)
        inspectedObjects = &localObjectList;

    if (depth > 2)
        return propertyNameList;

    if (!inspectedObjects->contains(object))
        inspectedObjects->append(object);

    const QMetaObject *metaObject = object->metaObject();

    QStringList deferredPropertyNames;
    const int namesIndex = metaObject->indexOfClassInfo("DeferredPropertyNames");
    if (namesIndex != -1) {
        QMetaClassInfo classInfo = metaObject->classInfo(namesIndex);
        deferredPropertyNames = QString::fromUtf8(classInfo.value()).split(QLatin1Char(','));
    }

    for (int index = 0; index < metaObject->propertyCount(); ++index) {
        QMetaProperty metaProperty = metaObject->property(index);
        QQmlProperty declarativeProperty(object, QString::fromUtf8(metaProperty.name()));
        if (declarativeProperty.isValid() && declarativeProperty.propertyTypeCategory() == QQmlProperty::Object) {
            if (declarativeProperty.name() != QLatin1String("parent")
                    && !deferredPropertyNames.contains(declarativeProperty.name())) {
                QObject *childObject = QQmlMetaType::toQObject(declarativeProperty.read());
                if (childObject)
                    propertyNameList.append(allPropertyNamesFork(childObject,
                                                             baseName
                                                             + QQuickDesignerSupport::PropertyName(metaProperty.name())
                                                             + '.', inspectedObjects,
                                                             depth + 1));
            }
        } else if (QQmlGadgetPtrWrapper *valueType
                   = QQmlGadgetPtrWrapper::instance(qmlEngine(object), metaProperty.metaType())) {

            const QVariant value = metaProperty.read(object);
            propertyNameList.append(baseName + QQuickDesignerSupport::PropertyName(metaProperty.name()));
            const QJsonValue jsonValue = value.toJsonValue();

            if (value.isValid() && jsonValue.isNull()) {
                QMetaType jsType = QMetaType::fromType<QJSValue>();

                int userType = value.userType();

                if (userType != jsType.id()) {
                    valueType->setValue(value);
                    propertyNameList.append(allPropertyNamesFork(valueType,
                                                                 baseName
                                                                 + QQuickDesignerSupport::PropertyName(metaProperty.name())
                                                                 + '.', inspectedObjects,
                                                                 depth + 1));
                }


            }
        } else  {
            addToPropertyNameListIfNotBlackListed(&propertyNameList,
                                                  baseName + QQuickDesignerSupport::PropertyName(metaProperty.name()));
        }
    }

    return propertyNameList;
}

class DesignerCustomObjectDataFork
{
public:
    static void registerData(QObject *object);
    static DesignerCustomObjectDataFork *get(QObject *object);
    static QVariant getResetValue(QObject *object, const QQuickDesignerSupport::PropertyName &propertyName);
    static void doResetProperty(QObject *object, QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName);
    static bool hasValidResetBinding(QObject *object, const QQuickDesignerSupport::PropertyName &propertyName);
    static bool hasBindingForProperty(QObject *object,
                                      QQmlContext *context,
                                      const QQuickDesignerSupport::PropertyName &propertyName,
                                      bool *hasChanged);
    static void setPropertyBinding(QObject *object,
                                   QQmlContext *context,
                                   const QQuickDesignerSupport::PropertyName &propertyName,
                                   const QString &expression);
    static void keepBindingFromGettingDeleted(QObject *object,
                                              QQmlContext *context,
                                              const QQuickDesignerSupport::PropertyName &propertyName);
    void handleDestroyed();

private:
    DesignerCustomObjectDataFork(QObject *object);
    void populateResetHashes();
    QObject *object() const;
    QVariant getResetValue(const QQuickDesignerSupport::PropertyName &propertyName) const;
    void doResetProperty(QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName);
    bool hasValidResetBinding(const QQuickDesignerSupport::PropertyName &propertyName) const;
    QQmlAnyBinding getResetBinding(const QQuickDesignerSupport::PropertyName &propertyName) const;
    bool hasBindingForProperty(QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName, bool *hasChanged) const;
    void setPropertyBinding(QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName, const QString &expression);
    void keepBindingFromGettingDeleted(QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName);

    QObject *m_object;
    QHash<QQuickDesignerSupport::PropertyName, QVariant> m_resetValueHash;
    QHash<QQuickDesignerSupport::PropertyName, QQmlAnyBinding> m_resetBindingHash;
    mutable QHash<QQuickDesignerSupport::PropertyName, bool> m_hasBindingHash;
};

typedef QHash<QObject*, DesignerCustomObjectDataFork*> CustomObjectDataHash;
Q_GLOBAL_STATIC(CustomObjectDataHash, s_designerObjectToDataHash)

struct HandleDestroyedFunctor {
  DesignerCustomObjectDataFork *data;
  void operator()() { data->handleDestroyed(); }
};

using namespace Qt::StringLiterals;

DesignerCustomObjectDataFork::DesignerCustomObjectDataFork(QObject *object)
    : m_object(object)
{
    if (object) {
        populateResetHashes();
        s_designerObjectToDataHash()->insert(object, this);

        HandleDestroyedFunctor functor;
        functor.data = this;
        QObject::connect(object, &QObject::destroyed, functor);
    }
}

void DesignerCustomObjectDataFork::registerData(QObject *object)
{
    new DesignerCustomObjectDataFork(object);
}

DesignerCustomObjectDataFork *DesignerCustomObjectDataFork::get(QObject *object)
{
    return s_designerObjectToDataHash()->value(object);
}

QVariant DesignerCustomObjectDataFork::getResetValue(QObject *object, const QQuickDesignerSupport::PropertyName &propertyName)
{
    DesignerCustomObjectDataFork* data = get(object);

    if (data)
        return data->getResetValue(propertyName);

    return QVariant();
}

void DesignerCustomObjectDataFork::doResetProperty(QObject *object, QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName)
{
    DesignerCustomObjectDataFork* data = get(object);

    if (data)
        data->doResetProperty(context, propertyName);
}

bool DesignerCustomObjectDataFork::hasValidResetBinding(QObject *object, const QQuickDesignerSupport::PropertyName &propertyName)
{
    DesignerCustomObjectDataFork* data = get(object);

    if (data)
        return data->hasValidResetBinding(propertyName);

    return false;
}

bool DesignerCustomObjectDataFork::hasBindingForProperty(QObject *object,
                                                     QQmlContext *context,
                                                     const QQuickDesignerSupport::PropertyName &propertyName,
                                                     bool *hasChanged)
{
    DesignerCustomObjectDataFork* data = get(object);

    if (data)
        return data->hasBindingForProperty(context, propertyName, hasChanged);

    return false;
}

void DesignerCustomObjectDataFork::setPropertyBinding(QObject *object,
                                                  QQmlContext *context,
                                                  const QQuickDesignerSupport::PropertyName &propertyName,
                                                  const QString &expression)
{
    DesignerCustomObjectDataFork* data = get(object);

    if (data)
        data->setPropertyBinding(context, propertyName, expression);
}

void DesignerCustomObjectDataFork::keepBindingFromGettingDeleted(QObject *object,
                                                             QQmlContext *context,
                                                             const QQuickDesignerSupport::PropertyName &propertyName)
{
    DesignerCustomObjectDataFork* data = get(object);

    if (data)
        data->keepBindingFromGettingDeleted(context, propertyName);
}

void DesignerCustomObjectDataFork::populateResetHashes()
{
    const QQuickDesignerSupport::PropertyNameList propertyNameList =
            propertyNameListForWritablePropertiesFork(object());

    const QMetaObject *mo = object()->metaObject();
    QByteArrayList deferredPropertyNames;
    const int namesIndex = mo->indexOfClassInfo("DeferredPropertyNames");
    if (namesIndex != -1) {
        QMetaClassInfo classInfo = mo->classInfo(namesIndex);
        deferredPropertyNames = QByteArray(classInfo.value()).split(',');
    }

    for (const QQuickDesignerSupport::PropertyName &propertyName : propertyNameList) {

        if (deferredPropertyNames.contains(propertyName))
            continue;

        QQmlProperty property(object(), QString::fromUtf8(propertyName), QQmlEngine::contextForObject(object()));

        auto binding = QQmlAnyBinding::ofProperty(property);

        if (binding) {
            m_resetBindingHash.insert(propertyName, binding);
        } else if (property.isWritable()) {
            m_resetValueHash.insert(propertyName, property.read());
        }
    }
}

QObject *DesignerCustomObjectDataFork::object() const
{
    return m_object;
}

QVariant DesignerCustomObjectDataFork::getResetValue(const QQuickDesignerSupport::PropertyName &propertyName) const
{
    return m_resetValueHash.value(propertyName);
}

void DesignerCustomObjectDataFork::doResetProperty(QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName)
{
    QQmlProperty property(object(), QString::fromUtf8(propertyName), context);

    if (!property.isValid())
        return;

    // remove existing binding
    QQmlAnyBinding::takeFrom(property);


    if (hasValidResetBinding(propertyName)) {
        QQmlAnyBinding binding = getResetBinding(propertyName);
        binding.installOn(property);

        if (binding.isAbstractPropertyBinding()) {
            // for new style properties, we will evaluate during setBinding anyway
            static_cast<QQmlBinding *>(binding.asAbstractBinding())->update();
        }

    } else if (property.isResettable()) {
        property.reset();
    } else if (property.propertyTypeCategory() == QQmlProperty::List) {
        QQmlListReference list = qvariant_cast<QQmlListReference>(property.read());

        if (!QQuickDesignerSupportProperties::hasFullImplementedListInterface(list)) {
            qWarning() << "Property list interface not fully implemented for Class " << property.property().typeName() << " in property " << property.name() << "!";
            return;
        }

        list.clear();
    } else if (property.isWritable()) {
        if (property.read() == getResetValue(propertyName))
            return;

        property.write(getResetValue(propertyName));
    }
}

bool DesignerCustomObjectDataFork::hasValidResetBinding(const QQuickDesignerSupport::PropertyName &propertyName) const
{
    return m_resetBindingHash.contains(propertyName) &&  m_resetBindingHash.value(propertyName);
}

QQmlAnyBinding DesignerCustomObjectDataFork::getResetBinding(const QQuickDesignerSupport::PropertyName &propertyName) const
{
    return m_resetBindingHash.value(propertyName);
}

bool DesignerCustomObjectDataFork::hasBindingForProperty(QQmlContext *context,
                                                     const QQuickDesignerSupport::PropertyName &propertyName,
                                                     bool *hasChanged) const
{
    if (QQuickDesignerSupportProperties::isPropertyBlackListed(propertyName))
        return false;

    QQmlProperty property(object(), QString::fromUtf8(propertyName), context);

    bool hasBinding = QQmlAnyBinding::ofProperty(property);

    if (hasChanged) {
        *hasChanged = hasBinding != m_hasBindingHash.value(propertyName, false);
        if (*hasChanged)
            m_hasBindingHash.insert(propertyName, hasBinding);
    }

    return hasBinding;
}

void DesignerCustomObjectDataFork::setPropertyBinding(QQmlContext *context,
                                                  const QQuickDesignerSupport::PropertyName &propertyName,
                                                  const QString &expression)
{
    QQmlProperty property(object(), QString::fromUtf8(propertyName), context);

    if (!property.isValid())
        return;

    if (property.isProperty()) {
        QString url = u"@designer"_s;
        int lineNumber = 0;
        QQmlAnyBinding binding = QQmlAnyBinding::createFromCodeString(property,
                                                                      expression, object(), QQmlContextData::get(context), url, lineNumber);

        binding.installOn(property);
        if (binding.isAbstractPropertyBinding()) {
            // for new style properties, we will evaluate during setBinding anyway
            static_cast<QQmlBinding *>(binding.asAbstractBinding())->update();
        }

        if (binding.hasError()) {
            if (property.property().userType() == QMetaType::QString)
                property.write(QVariant(QLatin1Char('#') + expression + QLatin1Char('#')));
        }

    } else {
        qWarning() << Q_FUNC_INFO << ": Cannot set binding for property" << propertyName << ": property is unknown for type";
    }
}

void DesignerCustomObjectDataFork::keepBindingFromGettingDeleted(QQmlContext *context,
                                                             const QQuickDesignerSupport::PropertyName &propertyName)
{
    //Refcounting is taking care
    Q_UNUSED(context);
    Q_UNUSED(propertyName);
}

void DesignerCustomObjectDataFork::handleDestroyed()
{
    s_designerObjectToDataHash()->remove(m_object);
    delete this;
}



namespace QmlPrivateGate {

bool isPropertyBlackListed(const QmlDesigner::PropertyName &propertyName)
{
    return QQuickDesignerSupportProperties::isPropertyBlackListed(propertyName);
}

PropertyNameList allPropertyNames(QObject *object)
{
    return allPropertyNamesFork(object);
    //return QQuickDesignerSupportProperties::allPropertyNames(object);
}

PropertyNameList propertyNameListForWritableProperties(QObject *object)
{
    return propertyNameListForWritablePropertiesFork(object);
    //return QQuickDesignerSupportProperties::propertyNameListForWritableProperties(object);
}

void tweakObjects(QObject *object)
{
    QQuickDesignerSupportItems::tweakObjects(object);
}

void createNewDynamicProperty(QObject *object,  QQmlEngine *engine, const QString &name)
{
    QQmlProperty qmlProp(object, name, engine->contextForObject(object));
    if (!qmlProp.isValid())
        QQuickDesignerSupportProperties::createNewDynamicProperty(object, engine, name);
}

void registerNodeInstanceMetaObject(QObject *object, QQmlEngine *engine)
{
    QQuickDesignerSupportProperties::registerNodeInstanceMetaObject(object, engine);
}

static bool isMetaObjectofType(const QMetaObject *metaObject, const QByteArray &type)
{
    if (metaObject) {
        if (metaObject->className() == type)
            return true;

        return isMetaObjectofType(metaObject->superClass(), type);
    }

    return false;
}

static bool isQuickStyleItem(QObject *object)
{
    if (object)
        return isMetaObjectofType(object->metaObject(), "QQuickStyleItem");

    return false;
}

static bool isDelegateModel(QObject *object)
{
    if (object)
        return isMetaObjectofType(object->metaObject(), "QQmlDelegateModel");

    return false;
}

static bool isConnections(QObject *object)
{
    if (object)
        return isMetaObjectofType(object->metaObject(), "QQmlConnections");

    return false;
}

// This is used in share/qtcreator/qml/qmlpuppet/qml2puppet/instances/objectnodeinstance.cpp
QObject *createPrimitive(const QString &typeName, int majorNumber, int minorNumber, QQmlContext *context)
{
    QTypeRevision revision = QTypeRevision::zero();
    if (majorNumber > 0)
        revision = QTypeRevision::fromVersion(majorNumber, minorNumber);
    return QQuickDesignerSupportItems::createPrimitive(typeName, revision, context);
}

static QString qmlDesignerRCPath()
{
    static const QString qmlDesignerRcPathsString = QString::fromLocal8Bit(
        qgetenv("QMLDESIGNER_RC_PATHS"));
    return qmlDesignerRcPathsString;
}

QVariant fixResourcePaths(const QVariant &value)
{
    if (value.typeId() == QMetaType::QUrl) {
        const QUrl url = value.toUrl();
        if (url.scheme() == QLatin1String("qrc")) {
            const QString path = QLatin1String("qrc:") +  url.path();
            if (!qmlDesignerRCPath().isEmpty()) {
                const QStringList searchPaths = qmlDesignerRCPath().split(QLatin1Char(';'));
                for (const QString &qrcPath : searchPaths) {
                    const QStringList qrcDefintion = qrcPath.split(QLatin1Char('='));
                    if (qrcDefintion.count() == 2) {
                        QString fixedPath = path;
                        fixedPath.replace(QLatin1String("qrc:") + qrcDefintion.first(), qrcDefintion.last() + QLatin1Char('/'));
                        if (QFileInfo::exists(fixedPath)) {
                            fixedPath.replace(QLatin1String("//"), QLatin1String("/"));
                            fixedPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
                            return QUrl::fromLocalFile(fixedPath);
                        }
                    }
                }
            }
        }
    }
    if (value.typeId() == QMetaType::QString) {
        const QString str = value.toString();
        if (str.contains(QLatin1String("qrc:"))) {
            if (!qmlDesignerRCPath().isEmpty()) {
                const QStringList searchPaths = qmlDesignerRCPath().split(QLatin1Char(';'));
                for (const QString &qrcPath : searchPaths) {
                    const QStringList qrcDefintion = qrcPath.split(QLatin1Char('='));
                    if (qrcDefintion.count() == 2) {
                        QString fixedPath = str;
                        fixedPath.replace(QLatin1String("qrc:") + qrcDefintion.first(), qrcDefintion.last() + QLatin1Char('/'));
                        if (QFileInfo::exists(fixedPath)) {
                            fixedPath.replace(QLatin1String("//"), QLatin1String("/"));
                            fixedPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
                            return QUrl::fromLocalFile(fixedPath);
                        }
                    }
                }
            }
        }
    }
    return value;
}

QObject *createComponent(const QUrl &componentUrl, QQmlContext *context)
{
    return QQuickDesignerSupportItems::createComponent(componentUrl, context);
}

bool hasFullImplementedListInterface(const QQmlListReference &list)
{
    return list.isValid() && list.canCount() && list.canAt() && list.canAppend() && list.canClear();
}

void registerCustomData(QObject *object)
{
     DesignerCustomObjectDataFork::registerData(object);
    //QQuickDesignerSupportProperties::registerCustomData(object);
}

QVariant getResetValue(QObject *object, const PropertyName &propertyName)
{
    if (propertyName == "Layout.rowSpan")
        return 1;
    else if (propertyName == "Layout.columnSpan")
        return 1;
    else if (propertyName == "Layout.fillHeight")
        return false;
    else if (propertyName == "Layout.fillWidth")
        return false;
    else
         return DesignerCustomObjectDataFork::getResetValue(object, propertyName);
        //return QQuickDesignerSupportProperties::getResetValue(object, propertyName);
}

static void setProperty(QObject *object, QQmlContext *context, const PropertyName &propertyName, const QVariant &value)
{
    QQmlProperty property(object, QString::fromUtf8(propertyName), context);
    property.write(value);
}

void doResetProperty(QObject *object, QQmlContext *context, const PropertyName &propertyName)
{
    if (propertyName == "Layout.rowSpan")
        setProperty(object, context, propertyName, getResetValue(object, propertyName));
    else if (propertyName == "Layout.columnSpan")
        setProperty(object, context, propertyName, getResetValue(object, propertyName));
    else if (propertyName == "Layout.fillHeight")
        setProperty(object, context, propertyName, getResetValue(object, propertyName));
    else if (propertyName == "Layout.fillWidth")
        setProperty(object, context, propertyName, getResetValue(object, propertyName));
    else
        DesignerCustomObjectDataFork::doResetProperty(object, context, propertyName);

       //QQuickDesignerSupportProperties::doResetProperty(object, context, propertyName);
}

bool hasValidResetBinding(QObject *object, const PropertyName &propertyName)
{
    if (propertyName == "Layout.rowSpan")
        return true;
    else if (propertyName == "Layout.columnSpan")
        return true;
    else if (propertyName == "Layout.fillHeight")
        return true;
    else if (propertyName == "Layout.fillWidth")
        return true;
    return
            DesignerCustomObjectDataFork::hasValidResetBinding(object, propertyName);
            //QQuickDesignerSupportProperties::hasValidResetBinding(object, propertyName);
}

bool hasBindingForProperty(QObject *object, QQmlContext *context, const PropertyName &propertyName, bool *hasChanged)
{
    return DesignerCustomObjectDataFork::hasBindingForProperty(object, context, propertyName, hasChanged);
    //return QQuickDesignerSupportProperties::hasBindingForProperty(object, context, propertyName, hasChanged);
}

void setPropertyBinding(QObject *object, QQmlContext *context, const PropertyName &propertyName, const QString &expression)
{
    DesignerCustomObjectDataFork::setPropertyBinding(object, context, propertyName, expression);

    //QQuickDesignerSupportProperties::setPropertyBinding(object, context, propertyName, expression);
}

void emitComponentComplete(QObject *item)
{
    if (!item)
        return;

    QQmlData *data = QQmlData::get(item);
    if (data && data->context) {
        QQmlComponentAttached *componentAttached = data->context->componentAttacheds();
        while (componentAttached) {
            if (componentAttached->parent()) {
                if (componentAttached->parent() == item)
                    emit componentAttached->completed();
            }
            componentAttached = componentAttached->next();
        }
    }
}

void doComponentCompleteRecursive(QObject *object, NodeInstanceServer *nodeInstanceServer)
{
    if (object) {
        QQuickItem *item = qobject_cast<QQuickItem*>(object);

        if (item && QQuickDesignerSupport::isComponentComplete(item))
            return;
#ifdef QUICK3D_MODULE
        auto obj3d = qobject_cast<QQuick3DRepeater *>(object);
        if (obj3d && QQuick3DObjectPrivate::get(obj3d)->componentComplete)
            return;
#endif

        if (!nodeInstanceServer->hasInstanceForObject(item))
            emitComponentComplete(object);
        QList<QObject*> childList = object->children();

        if (item) {
            const QList<QQuickItem *> childItems = item->childItems();
            for (QQuickItem *childItem : childItems){
                if (!childList.contains(childItem))
                    childList.append(childItem);
            }
        }

        for (QObject *child : std::as_const(childList)) {
            if (!nodeInstanceServer->hasInstanceForObject(child))
                doComponentCompleteRecursive(child, nodeInstanceServer);
        }

        if (!isQuickStyleItem(object) && !isDelegateModel(object) && !isConnections(object)) {
            if (item) {
                static_cast<QQmlParserStatus *>(item)->componentComplete();
            } else {
                QQmlParserStatus *qmlParserStatus = dynamic_cast<QQmlParserStatus *>(object);
                if (qmlParserStatus) {
                    qmlParserStatus->componentComplete();
                    auto *anim = dynamic_cast<QQuickAbstractAnimation *>(object);
                    if (anim && ViewConfig::isParticleViewMode()) {
                        nodeInstanceServer->addAnimation(anim);
                        anim->setEnableUserControl();
                        anim->stop();
                    }
                }
            }
        }
    }
}


void keepBindingFromGettingDeleted(QObject *object, QQmlContext *context, const PropertyName &propertyName)
{
    DesignerCustomObjectDataFork::keepBindingFromGettingDeleted(object, context, propertyName);
    //QQuickDesignerSupportProperties::keepBindingFromGettingDeleted(object, context, propertyName);
}

bool objectWasDeleted(QObject *object)
{
    return QQuickDesignerSupportItems::objectWasDeleted(object);
}

void disableNativeTextRendering(QQuickItem *item)
{
    QQuickDesignerSupportItems::disableNativeTextRendering(item);
}

void disableTextCursor(QQuickItem *item)
{
    QQuickDesignerSupportItems::disableTextCursor(item);
}

void disableTransition(QObject *object)
{
    QQuickDesignerSupportItems::disableTransition(object);
}

void disableBehaivour(QObject *object)
{
    QQuickDesignerSupportItems::disableBehaivour(object);
}

void stopUnifiedTimer()
{
    QQuickDesignerSupportItems::stopUnifiedTimer();
}

bool isPropertyQObject(const QMetaProperty &metaProperty)
{
    return QQuickDesignerSupportProperties::isPropertyQObject(metaProperty);
}

QObject *readQObjectProperty(const QMetaProperty &metaProperty, QObject *object)
{
    return QQuickDesignerSupportProperties::readQObjectProperty(metaProperty, object);
}

namespace States {

bool isStateActive(QObject *object, QQmlContext *context)
{

    return QQuickDesignerSupportStates::isStateActive(object, context);
}

void activateState(QObject *object, QQmlContext *context)
{
    QQuickDesignerSupportStates::activateState(object, context);
}

void deactivateState(QObject *object)
{
    QQuickDesignerSupportStates::deactivateState(object);
}

bool changeValueInRevertList(QObject *state, QObject *target, const PropertyName &propertyName, const QVariant &value)
{
    return QQuickDesignerSupportStates::changeValueInRevertList(state, target, propertyName, value);
}

bool updateStateBinding(QObject *state, QObject *target, const PropertyName &propertyName, const QString &expression)
{
    return QQuickDesignerSupportStates::updateStateBinding(state, target, propertyName, expression);
}

bool resetStateProperty(QObject *state, QObject *target, const PropertyName &propertyName, const QVariant &value)
{
    return QQuickDesignerSupportStates::resetStateProperty(state, target, propertyName, value);
}

} //namespace States

namespace PropertyChanges {

void detachFromState(QObject *propertyChanges)
{
    return QQuickDesignerSupportPropertyChanges::detachFromState(propertyChanges);
}

void attachToState(QObject *propertyChanges)
{
    return QQuickDesignerSupportPropertyChanges::attachToState(propertyChanges);
}

QObject *targetObject(QObject *propertyChanges)
{
    return QQuickDesignerSupportPropertyChanges::targetObject(propertyChanges);
}

void removeProperty(QObject *propertyChanges, const PropertyName &propertyName)
{
    QQuickDesignerSupportPropertyChanges::removeProperty(propertyChanges, propertyName);
}

QVariant getProperty(QObject *propertyChanges, const PropertyName &propertyName)
{
    return QQuickDesignerSupportPropertyChanges::getProperty(propertyChanges, propertyName);
}

void changeValue(QObject *propertyChanges, const PropertyName &propertyName, const QVariant &value)
{
    QQuickDesignerSupportPropertyChanges::changeValue(propertyChanges, propertyName, value);
}

void changeExpression(QObject *propertyChanges, const PropertyName &propertyName, const QString &expression)
{
    QQuickDesignerSupportPropertyChanges::changeExpression(propertyChanges, propertyName, expression);
}

QObject *stateObject(QObject *propertyChanges)
{
    return QQuickDesignerSupportPropertyChanges::stateObject(propertyChanges);
}

bool isNormalProperty(const PropertyName &propertyName)
{
    return QQuickDesignerSupportPropertyChanges::isNormalProperty(propertyName);
}

} // namespace PropertyChanges

bool isSubclassOf(QObject *object, const QByteArray &superTypeName)
{
    return QQuickDesignerSupportMetaInfo::isSubclassOf(object, superTypeName);
}

void getPropertyCache(QObject *object, QQmlEngine *engine)
{
    Q_UNUSED(engine);
    QQuickDesignerSupportProperties::getPropertyCache(object);
}

void registerNotifyPropertyChangeCallBack(void (*callback)(QObject *, const PropertyName &))
{
    QQuickDesignerSupportMetaInfo::registerNotifyPropertyChangeCallBack(callback);
}

ComponentCompleteDisabler::ComponentCompleteDisabler()
{
    QQuickDesignerSupport::disableComponentComplete();
}

ComponentCompleteDisabler::~ComponentCompleteDisabler()
{
    QQuickDesignerSupport::enableComponentComplete();
}

class QrcEngineHandler : public QAbstractFileEngineHandler
{
public:
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    std::unique_ptr<QAbstractFileEngine> create(const QString &fileName) const final;
#else
    QAbstractFileEngine *create(const QString &fileName) const final;
#endif
};

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
std::unique_ptr<QAbstractFileEngine>
#else
QAbstractFileEngine *
#endif
QrcEngineHandler::create(const QString &fileName) const
{
    if (fileName.startsWith(":/qt-project.org"))
        return {};

    if (fileName.startsWith(":/qtquickplugin"))
        return {};

    if (fileName.startsWith(":/")) {
        const QStringList searchPaths = qmlDesignerRCPath().split(';');
        for (const QString &qrcPath : searchPaths) {
            const QStringList qrcDefintion = qrcPath.split('=');
            if (qrcDefintion.count() == 2) {
                QString fixedPath = fileName;
                fixedPath.replace(":" + qrcDefintion.first(), qrcDefintion.last() + '/');

                if (fileName == fixedPath)
                    return {};

                if (QFileInfo::exists(fixedPath)) {
                    fixedPath.replace("//", "/");
                    fixedPath.replace('\\', '/');
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
                    return std::make_unique<QFSFileEngine>(fixedPath);
#else
                    return new QFSFileEngine(fixedPath);
#endif
                }
            }
        }
    }

    return {};
}

static QrcEngineHandler* s_qrcEngineHandler = nullptr;

class EngineHandlerDeleter
{
public:
    EngineHandlerDeleter()
    {}
    ~EngineHandlerDeleter()
    { delete s_qrcEngineHandler; }
};

void registerFixResourcePathsForObjectCallBack()
{
    static EngineHandlerDeleter deleter;

    if (!s_qrcEngineHandler)
        s_qrcEngineHandler = new QrcEngineHandler();
}

} // namespace QmlPrivateGate
} // namespace Internal
} // namespace QmlDesigner
