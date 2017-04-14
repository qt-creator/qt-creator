/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlprivategate.h"

#include "metaobject.h"
#include "designercustomobjectdata.h"

#include <objectnodeinstance.h>
#include <nodeinstanceserver.h>

#include <QQuickItem>
#include <QQmlComponent>
#include <QFileInfo>

#include <private/qquickdesignersupport_p.h>
#include <private/qquickdesignersupportmetainfo_p.h>
#include <private/qquickdesignersupportitems_p.h>
#include <private/qquickdesignersupportproperties_p.h>
#include <private/qquickdesignersupportpropertychanges_p.h>
#include <private/qquickdesignersupportstates_p.h>

namespace QmlDesigner {

namespace Internal {

namespace QmlPrivateGate {

bool isPropertyBlackListed(const QmlDesigner::PropertyName &propertyName)
{
    return QQuickDesignerSupportProperties::isPropertyBlackListed(propertyName);
}

PropertyNameList allPropertyNames(QObject *object,
                                  const PropertyName &baseName,
                                  QObjectList *inspectedObjects)
{
    return QQuickDesignerSupportProperties::allPropertyNames(object, baseName, inspectedObjects);
}

PropertyNameList propertyNameListForWritableProperties(QObject *object,
                                                       const PropertyName &baseName,
                                                       QObjectList *inspectedObjects)
{
    return QQuickDesignerSupportProperties::propertyNameListForWritableProperties(object, baseName, inspectedObjects);
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

QObject *createPrimitive(const QString &typeName, int majorNumber, int minorNumber, QQmlContext *context)
{
    return QQuickDesignerSupportItems::createPrimitive(typeName, majorNumber, minorNumber, context);
}

QVariant fixResourcePaths(const QVariant &value)
{
    if (value.type() == QVariant::Url)
    {
        const QUrl url = value.toUrl();
        if (url.scheme() == QLatin1String("qrc")) {
            const QString path = QLatin1String("qrc:") +  url.path();
            QString qrcSearchPath = QString::fromLocal8Bit(qgetenv("QMLDESIGNER_RC_PATHS"));
            if (!qrcSearchPath.isEmpty()) {
                const QStringList searchPaths = qrcSearchPath.split(QLatin1Char(';'));
                foreach (const QString &qrcPath, searchPaths) {
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
    if (value.type() == QVariant::String) {
        const QString str = value.toString();
        if (str.contains(QLatin1String("qrc:"))) {
            QString qrcSearchPath = QString::fromLocal8Bit(qgetenv("QMLDESIGNER_RC_PATHS"));
            if (!qrcSearchPath.isEmpty()) {
                const QStringList searchPaths = qrcSearchPath.split(QLatin1Char(';'));
                foreach (const QString &qrcPath, searchPaths) {
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


void fixResourcePathsForObject(QObject *object)
{
    if (qEnvironmentVariableIsEmpty("QMLDESIGNER_RC_PATHS"))
        return;

    PropertyNameList propertyNameList = propertyNameListForWritableProperties(object);

    foreach (const PropertyName &propertyName, propertyNameList) {
        QQmlProperty property(object, QString::fromUtf8(propertyName), QQmlEngine::contextForObject(object));

        const QVariant value  = property.read();
        const QVariant fixedValue = fixResourcePaths(value);
        if (value != fixedValue) {
            property.write(fixedValue);
        }
    }
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

void doComponentCompleteRecursive(QObject *object, NodeInstanceServer *nodeInstanceServer)
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
    QQuickDesignerSupportProperties::getPropertyCache(object, engine);
}

void registerNotifyPropertyChangeCallBack(void (*callback)(QObject *, const PropertyName &))
{
    QQuickDesignerSupportMetaInfo::registerNotifyPropertyChangeCallBack(callback);
}

ComponentCompleteDisabler::ComponentCompleteDisabler()
{
    DesignerSupport::disableComponentComplete();
}

ComponentCompleteDisabler::~ComponentCompleteDisabler()
{
    DesignerSupport::enableComponentComplete();
}

void registerFixResourcePathsForObjectCallBack()
{
    QQuickDesignerSupportItems::registerFixResourcePathsForObjectCallBack(&fixResourcePathsForObject);
}

} // namespace QmlPrivateGate
} // namespace Internal
} // namespace QmlDesigner
