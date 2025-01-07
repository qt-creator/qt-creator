// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nodeinstanceglobal.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QQmlContext>
#include <QQmlListReference>
#include <QQuickItem>

namespace QmlDesigner {

class NodeInstanceServer;

namespace Internal {

class ObjectNodeInstance;
typedef QSharedPointer<ObjectNodeInstance> ObjectNodeInstancePointer;
typedef QWeakPointer<ObjectNodeInstance> ObjectNodeInstanceWeakPointer;

namespace QmlPrivateGate {

class ComponentCompleteDisabler
{
public:
    ComponentCompleteDisabler();

    ~ComponentCompleteDisabler();
};

    void createNewDynamicProperty(QObject *object, QQmlEngine *engine, const QString &name);
    void registerNodeInstanceMetaObject(QObject *object, QQmlEngine *engine);
    QVariant fixResourcePaths(const QVariant &value);
    QObject *createPrimitive(const QString &typeName, int majorNumber, int minorNumber, QQmlContext *context);
    QObject *createComponent(const QUrl &componentUrl, QQmlContext *context);
    void tweakObjects(QObject *object);
    bool isPropertyBlackListed(const QmlDesigner::PropertyName &propertyName);
    PropertyNameList propertyNameListForWritableProperties(QObject *object,
                                                           const PropertyName &baseName = PropertyName(),
                                                           QObjectList *inspectedObjects = nullptr);
    PropertyNameList allPropertyNames(QObject *object);
    bool hasFullImplementedListInterface(const QQmlListReference &list);

    void registerCustomData(QObject *object);
    QVariant getResetValue(QObject *object, const PropertyName &propertyName);
    void doResetProperty(QObject *object, QQmlContext *context, const PropertyName &propertyName);
    bool hasValidResetBinding(QObject *object, const PropertyName &propertyName);

    bool hasBindingForProperty(QObject *object, QQmlContext *context, const PropertyName &propertyName, bool *hasChanged);
    void setPropertyBinding(QObject *object, QQmlContext *context, const PropertyName &propertyName, const QString &expression);
    void keepBindingFromGettingDeleted(QObject *object, QQmlContext *context, const PropertyName &propertyName);

    void emitComponentComplete(QObject *item);
    void doComponentCompleteRecursive(QObject *object, NodeInstanceServer *nodeInstanceServer);

    bool objectWasDeleted(QObject *object);
    void disableNativeTextRendering(QQuickItem *item);
    void disableTextCursor(QQuickItem *item);
    void disableTransition(QObject *object);
    void disableBehaivour(QObject *object);
    void stopUnifiedTimer();
    bool isPropertyQObject(const QMetaProperty &metaProperty);
    QObject *readQObjectProperty(const QMetaProperty &metaProperty, QObject *object);

    namespace States {

    bool isStateActive(QObject *object, QQmlContext *context);
    void activateState(QObject *object, QQmlContext *context);
    void deactivateState(QObject *object);
    bool changeValueInRevertList(QObject *state, QObject *target, const PropertyName &propertyName, const QVariant &value);
    bool updateStateBinding(QObject *state, QObject *target, const PropertyName &propertyName, const QString &expression);
    bool resetStateProperty(QObject *state, QObject *target, const PropertyName &propertyName, const QVariant &);

    } //namespace States

    namespace PropertyChanges {

    void detachFromState(QObject *propertyChanges);
    void attachToState(QObject *propertyChanges);
    QObject *targetObject(QObject *propertyChanges);
    void removeProperty(QObject *propertyChanges, const PropertyName &propertyName);
    QVariant getProperty(QObject *propertyChanges, const PropertyName &propertyName);
    void changeValue(QObject *propertyChanges, const PropertyName &propertyName, const QVariant &value);
    void changeExpression(QObject *propertyChanges, const PropertyName &propertyName, const QString &expression);
    QObject *stateObject(QObject *propertyChanges);
    bool isNormalProperty(const PropertyName &propertyName);

    } // namespace PropertyChanges


    bool isSubclassOf(QObject *object, const QByteArray &superTypeName);
    void getPropertyCache(QObject *object, QQmlEngine *engine);

    void registerNotifyPropertyChangeCallBack(void (*callback)(QObject *, const PropertyName &));

    void registerFixResourcePathsForObjectCallBack();

} // namespace QmlPrivateGate
} // namespace Internal
} // namespace QmlDesigner
