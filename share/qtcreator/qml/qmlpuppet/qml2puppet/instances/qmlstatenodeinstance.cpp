/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "qmlstatenodeinstance.h"

#include <qmlprivategate.h>

#include "qmlpropertychangesnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

/**
  \class QmlStateNodeInstance

  QmlStateNodeInstance manages a QQuickState object.
  */

QmlStateNodeInstance::QmlStateNodeInstance(QObject *object) :
        ObjectNodeInstance(object)
{
}

QmlStateNodeInstance::Pointer
        QmlStateNodeInstance::create(QObject *object)
{
    Pointer instance(new QmlStateNodeInstance(object));

    instance->populateResetHashes();

    return instance;
}

void QmlStateNodeInstance::activateState()
{
    if (!QmlPrivateGate::States::isStateActive(object(), context())
            && nodeInstanceServer()->hasInstanceForObject(object())) {
        nodeInstanceServer()->setStateInstance(nodeInstanceServer()->instanceForObject(object()));
        QmlPrivateGate::States::activateState(object(), context());
    }
}

void QmlStateNodeInstance::deactivateState()
{
    if (QmlPrivateGate::States::isStateActive(object(), context())) {
        nodeInstanceServer()->clearStateInstance();
        QmlPrivateGate::States::deactivateState(object());
    }
}

void QmlStateNodeInstance::setPropertyVariant(const PropertyName &name, const QVariant &value)
{
    bool hasParent = parent();
    bool isStateOfTheRootModelNode = parentInstance() && parentInstance()->isRootNodeInstance();
    if (name == "when" && (!hasParent || isStateOfTheRootModelNode))
        return;

    ObjectNodeInstance::setPropertyVariant(name, value);
}

void QmlStateNodeInstance::setPropertyBinding(const PropertyName &name, const QString &expression)
{
    bool hasParent = parent();
    bool isStateOfTheRootModelNode = parentInstance() && parentInstance()->isRootNodeInstance();
    if (name == "when" && (!hasParent || isStateOfTheRootModelNode))
        return;

    ObjectNodeInstance::setPropertyBinding(name, expression);
}

bool QmlStateNodeInstance::updateStateVariant(const ObjectNodeInstance::Pointer &target, const PropertyName &propertyName, const QVariant &value)
{
    return QmlPrivateGate::States::changeValueInRevertList(object(), target->object(), propertyName, value);
}

bool QmlStateNodeInstance::updateStateBinding(const ObjectNodeInstance::Pointer &target, const PropertyName &propertyName, const QString &expression)
{
    return QmlPrivateGate::States::updateStateBinding(object(), target->object(), propertyName, expression);
}

bool QmlStateNodeInstance::resetStateProperty(const ObjectNodeInstance::Pointer &target, const PropertyName &propertyName, const QVariant & resetValue)
{
    return QmlPrivateGate::States::resetStateProperty(object(), target->object(), propertyName, resetValue);
}

} // namespace Internal
} // namespace QmlDesigner
