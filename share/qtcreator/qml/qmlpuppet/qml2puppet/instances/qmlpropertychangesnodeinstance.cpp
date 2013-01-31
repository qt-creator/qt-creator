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

#include "qmlpropertychangesnodeinstance.h"
#include "qmlstatenodeinstance.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <private/qqmlbinding_p.h>
#include <QMutableListIterator>


#include <private/qquickstate_p_p.h>
#include <private/qquickpropertychanges_p.h>
#include <private/qqmlproperty_p.h>

namespace QmlDesigner {
namespace Internal {

QmlPropertyChangesNodeInstance::QmlPropertyChangesNodeInstance(QQuickPropertyChanges *propertyChangesObject) :
        ObjectNodeInstance(propertyChangesObject)
{
}

QmlPropertyChangesNodeInstance::Pointer QmlPropertyChangesNodeInstance::create(QObject *object)
{
    QQuickPropertyChanges *propertyChange = qobject_cast<QQuickPropertyChanges*>(object);

    Q_ASSERT(propertyChange);

    Pointer instance(new QmlPropertyChangesNodeInstance(propertyChange));

    instance->populateResetHashes();

    return instance;
}

void QmlPropertyChangesNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    QMetaObject metaObject = QQuickPropertyChanges::staticMetaObject;

    if (metaObject.indexOfProperty(name.toLatin1()) > 0) { // 'restoreEntryValues', 'explicit'
        ObjectNodeInstance::setPropertyVariant(name, value);
    } else {
        changesObject()->changeValue(name.toLatin1(), value);
        QObject *targetObject = changesObject()->object();
        if (targetObject && nodeInstanceServer()->activeStateInstance().isWrappingThisObject(changesObject()->state())) {
            ServerNodeInstance targetInstance = nodeInstanceServer()->instanceForObject(targetObject);
            targetInstance.setPropertyVariant(name, value);
        }
    }
}

void QmlPropertyChangesNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    QMetaObject metaObject = QQuickPropertyChanges::staticMetaObject;

    if (metaObject.indexOfProperty(name.toLatin1()) > 0) { // 'restoreEntryValues', 'explicit'
        ObjectNodeInstance::setPropertyBinding(name, expression);
    } else {
        changesObject()->changeExpression(name.toLatin1(), expression);
    }
}

QVariant QmlPropertyChangesNodeInstance::property(const QString &name) const
{
    return changesObject()->property(name.toLatin1());
}

void QmlPropertyChangesNodeInstance::resetProperty(const QString &name)
{
    changesObject()->removeProperty(name.toLatin1());
}


void QmlPropertyChangesNodeInstance::reparent(const ServerNodeInstance &oldParentInstance, const QString &oldParentProperty, const ServerNodeInstance &newParentInstance, const QString &newParentProperty)
{
    changesObject()->detachFromState();

    ObjectNodeInstance::reparent(oldParentInstance.internalInstance(), oldParentProperty, newParentInstance.internalInstance(), newParentProperty);

    changesObject()->attachToState();
}

QQuickPropertyChanges *QmlPropertyChangesNodeInstance::changesObject() const
{
    Q_ASSERT(qobject_cast<QQuickPropertyChanges*>(object()));
    return static_cast<QQuickPropertyChanges*>(object());
}

} // namespace Internal
} // namespace QmlDesigner
