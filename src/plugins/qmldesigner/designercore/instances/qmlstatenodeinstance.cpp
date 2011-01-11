/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlstatenodeinstance.h"
#include "nodeabstractproperty.h"

#include <private/qdeclarativestategroup_p.h>

#include "qmlpropertychangesnodeinstance.h"
#include <private/qdeclarativestateoperations_p.h>

#include <invalidnodeinstanceexception.h>

namespace QmlDesigner {
namespace Internal {

/**
  \class QmlStateNodeInstance

  QmlStateNodeInstance manages a QDeclarativeState object.
  */

QmlStateNodeInstance::QmlStateNodeInstance(QDeclarativeState *object) :
        ObjectNodeInstance(object)
{
}

QmlStateNodeInstance::Pointer
        QmlStateNodeInstance::create(QObject *object)
{
    QDeclarativeState *stateObject = qobject_cast<QDeclarativeState*>(object);

    if (stateObject == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

    Pointer instance(new QmlStateNodeInstance(stateObject));

    instance->populateResetValueHash();

    return instance;
}

void QmlStateNodeInstance::activateState()
{
    if (stateGroup()) {
        if (!isStateActive()) {
            nodeInstanceServer()->setStateInstance(nodeInstanceServer()->instanceForObject(object()));
            stateGroup()->setState(property("name").toString());
        }
    }
}

void QmlStateNodeInstance::deactivateState()
{
    if (stateGroup()) {
        if (isStateActive()) {
            nodeInstanceServer()->clearStateInstance();
            stateGroup()->setState(QString());
        }
    }
}

QDeclarativeState *QmlStateNodeInstance::stateObject() const
{
    Q_ASSERT(object());
    Q_ASSERT(qobject_cast<QDeclarativeState*>(object()));
    return static_cast<QDeclarativeState*>(object());
}

QDeclarativeStateGroup *QmlStateNodeInstance::stateGroup() const
{
    return stateObject()->stateGroup();
}

bool QmlStateNodeInstance::isStateActive() const
{
    qDebug() << stateObject();
    qDebug() << stateGroup();
    if (stateGroup())
        qDebug() << "state name" << stateGroup()->state() << property("name");
    return stateObject() && stateGroup() && stateGroup()->state() == property("name");
}

void QmlStateNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    qDebug() << __FUNCTION__ << stateObject() << name << value;
    bool hasParent = parent();
    bool isStateOfTheRootModelNode = parentInstance() && parentInstance()->isRootNodeInstance();
    if (name == "when" && (!hasParent || isStateOfTheRootModelNode))
        return;

    ObjectNodeInstance::setPropertyVariant(name, value);
}

void QmlStateNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    bool hasParent = parent();
    bool isStateOfTheRootModelNode = parentInstance() && parentInstance()->isRootNodeInstance();
    if (name == "when" && (!hasParent || isStateOfTheRootModelNode))
        return;

    ObjectNodeInstance::setPropertyBinding(name, expression);
}

bool QmlStateNodeInstance::updateStateVariant(const ObjectNodeInstance::Pointer &target, const QString &propertyName, const QVariant &value)
{
    return stateObject()->changeValueInRevertList(target->object(), propertyName.toLatin1(), value);
}

bool QmlStateNodeInstance::updateStateBinding(const ObjectNodeInstance::Pointer &target, const QString &propertyName, const QString &expression)
{
    return stateObject()->changeValueInRevertList(target->object(), propertyName.toLatin1(), expression);
}

bool QmlStateNodeInstance::resetStateProperty(const ObjectNodeInstance::Pointer &target, const QString &propertyName, const QVariant & /* resetValue */)
{
    return stateObject()->removeEntryFromRevertList(target->object(), propertyName.toLatin1());
}

} // namespace Internal
} // namespace QmlDesigner
