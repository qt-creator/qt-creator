// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpropertychangesnodeinstance.h"
#include "qmlstatenodeinstance.h"

#include <qmlprivategate.h>

#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlExpression>

namespace QmlDesigner {
namespace Internal {

QmlPropertyChangesNodeInstance::QmlPropertyChangesNodeInstance(QObject *propertyChangesObject) :
        ObjectNodeInstance(propertyChangesObject)
{
}

QmlPropertyChangesNodeInstance::Pointer QmlPropertyChangesNodeInstance::create(QObject *object)
{
    Pointer instance(new QmlPropertyChangesNodeInstance(object));

    instance->populateResetHashes();

    return instance;
}

void QmlPropertyChangesNodeInstance::setPropertyVariant(const PropertyName &name, const QVariant &value)
{
    if (QmlPrivateGate::PropertyChanges::isNormalProperty(name)) { // 'restoreEntryValues', 'explicit'
        ObjectNodeInstance::setPropertyVariant(name, value);
    } else {
        QmlPrivateGate::PropertyChanges::changeValue(object(), name, value);
        QObject *targetObject = QmlPrivateGate::PropertyChanges::targetObject(object());
        if (targetObject
                && nodeInstanceServer()->activeStateInstance().
                isWrappingThisObject(QmlPrivateGate::PropertyChanges::stateObject(object()))) {
            if (nodeInstanceServer()->hasInstanceForObject(targetObject)) {
                ServerNodeInstance targetInstance = nodeInstanceServer()->instanceForObject(targetObject);
                targetInstance.setPropertyVariant(name, value);
            }
        }
    }
}

void QmlPropertyChangesNodeInstance::setPropertyBinding(const PropertyName &name, const QString &expression)
{
    if (QmlPrivateGate::PropertyChanges::isNormalProperty(name)) { // 'restoreEntryValues', 'explicit'
        ObjectNodeInstance::setPropertyBinding(name, expression);
    } else {
        QObject *state = QmlPrivateGate::PropertyChanges::stateObject(object());

        ServerNodeInstance activeState = nodeInstanceServer()->activeStateInstance();
        auto activeStateInstance = activeState.internalInstance();

        const bool inState = activeStateInstance && activeStateInstance->object() == state;

        if (inState)
            activeState.deactivateState();

        QmlPrivateGate::PropertyChanges::changeExpression(object(), name, expression);

        if (inState)
            activeState.activateState();
    }
}

QVariant QmlPropertyChangesNodeInstance::property(const PropertyName &name) const
{
    return QmlPrivateGate::PropertyChanges::getProperty(object(), name);
}

void QmlPropertyChangesNodeInstance::resetProperty(const PropertyName &name)
{
    QmlPrivateGate::PropertyChanges::removeProperty(object(), name);
}


void QmlPropertyChangesNodeInstance::reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const PropertyName &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const PropertyName &newParentProperty)
{
    QmlPrivateGate::PropertyChanges::detachFromState(object());

    ObjectNodeInstance::reparent(oldParentInstance, oldParentProperty, newParentInstance, newParentProperty);

    QmlPrivateGate::PropertyChanges::attachToState(object());
}

bool QmlPropertyChangesNodeInstance::isPropertyChange() const
{
    return true;
}

} // namespace Internal
} // namespace QmlDesigner
