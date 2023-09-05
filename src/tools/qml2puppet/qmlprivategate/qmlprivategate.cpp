// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprivategate.h"

#include <objectnodeinstance.h>
#include <nodeinstanceserver.h>

#include <QQuickItem>
#include <QQmlComponent>
#include <QFileInfo>
#include <QProcessEnvironment>

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

#ifdef QUICK3D_MODULE
#include <private/qquick3dobject_p.h>
#include <private/qquick3drepeater_p.h>
#endif

namespace QmlDesigner {

namespace Internal {

namespace QmlPrivateGate {

bool isPropertyBlackListed(const QmlDesigner::PropertyName &propertyName)
{
    return QQuickDesignerSupportProperties::isPropertyBlackListed(propertyName);
}

PropertyNameList allPropertyNames(QObject *object)
{
    return QQuickDesignerSupportProperties::allPropertyNames(object);
}

PropertyNameList propertyNameListForWritableProperties(QObject *object)
{
    return QQuickDesignerSupportProperties::propertyNameListForWritableProperties(object);
}

void tweakObjects(QObject *object)
{
    QQuickDesignerSupportItems::tweakObjects(object);
}

void createNewDynamicProperty(QObject *object,  QQmlEngine *engine, const QString &name)
{
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
    if (value.typeId() == QVariant::Url) {
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
    if (value.typeId() == QVariant::String) {
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
    QQuickDesignerSupportProperties::registerCustomData(object);
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
        return QQuickDesignerSupportProperties::getResetValue(object, propertyName);
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
        QQuickDesignerSupportProperties::doResetProperty(object, context, propertyName);
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
    return QQuickDesignerSupportProperties::hasValidResetBinding(object, propertyName);
}

bool hasBindingForProperty(QObject *object, QQmlContext *context, const PropertyName &propertyName, bool *hasChanged)
{
    return QQuickDesignerSupportProperties::hasBindingForProperty(object, context, propertyName, hasChanged);
}

void setPropertyBinding(QObject *object, QQmlContext *context, const PropertyName &propertyName, const QString &expression)
{
    QQuickDesignerSupportProperties::setPropertyBinding(object, context, propertyName, expression);
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
    QQuickDesignerSupportProperties::keepBindingFromGettingDeleted(object, context, propertyName);
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
    QAbstractFileEngine *create(const QString &fileName) const final;
};

QAbstractFileEngine *QrcEngineHandler::create(const QString &fileName) const
{
    if (fileName.startsWith(":/qt-project.org"))
        return nullptr;

    if (fileName.startsWith(":/qtquickplugin"))
        return nullptr;

    if (fileName.startsWith(":/")) {
        const QStringList searchPaths = qmlDesignerRCPath().split(';');
        for (const QString &qrcPath : searchPaths) {
            const QStringList qrcDefintion = qrcPath.split('=');
            if (qrcDefintion.count() == 2) {
                QString fixedPath = fileName;
                fixedPath.replace(":" + qrcDefintion.first(), qrcDefintion.last() + '/');

                if (fileName == fixedPath)
                    return nullptr;

                if (QFileInfo::exists(fixedPath)) {
                    fixedPath.replace("//", "/");
                    fixedPath.replace('\\', '/');
                    return new QFSFileEngine(fixedPath);
                }
            }
        }
    }

    return nullptr;
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
