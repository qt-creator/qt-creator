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

#include "qmlpropertychangesnodeinstance.h"
#include "qmlstatenodeinstance.h"

#include <qmlprivategate.h>

#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <QMutableListIterator>

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
        QmlPrivateGate::PropertyChanges::changeExpression(object(), name, expression);
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

} // namespace Internal
} // namespace QmlDesigner
