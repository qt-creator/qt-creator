/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlstatenodeinstance.h"
#include "nodeabstractproperty.h"

#include <private/qdeclarativestategroup_p.h>

#include "qmlpropertychangesnodeinstance.h"
#include <private/qdeclarativestateoperations_p.h>

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
        QmlStateNodeInstance::create(const NodeMetaInfo &metaInfo,
                                               QDeclarativeContext *context,
                                               QObject *objectToBeWrapped)
{
    Q_ASSERT(!objectToBeWrapped);
    QObject *object = createObject(metaInfo, context);
    QDeclarativeState *stateObject = qobject_cast<QDeclarativeState*>(object);
    Q_ASSERT(stateObject);

    Pointer instance(new QmlStateNodeInstance(stateObject));

    instance->populateResetValueHash();

    return instance;
}

void QmlStateNodeInstance::activateState()
{
    if (stateGroup()) {
        if (!isStateActive())
            nodeInstanceView()->setStateInstance(nodeInstanceView()->instanceForNode(modelNode()));
            stateGroup()->setState(property("name").toString());
    }
}

void QmlStateNodeInstance::deactivateState()
{
    if (stateGroup()) {
        if (isStateActive()) {
            nodeInstanceView()->clearStateInstance();
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
    Q_ASSERT(stateObject());
    Q_ASSERT(stateGroup());
    return (stateGroup()->state() == property("name"));
}

void QmlStateNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    bool hasParent = modelNode().hasParentProperty();
    bool isStateOfTheRootModelNode = !hasParent || (hasParent && modelNode().parentProperty().parentModelNode().isRootNode());
    if (name == "when" && isStateOfTheRootModelNode)
        return;

    ObjectNodeInstance::setPropertyVariant(name, value);
}

void QmlStateNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    bool hasParent = modelNode().hasParentProperty();
    bool isStateOfTheRootModelNode = !hasParent || (hasParent && modelNode().parentProperty().parentModelNode().isRootNode());
    if (name == "when" && isStateOfTheRootModelNode)
        return;

    ObjectNodeInstance::setPropertyBinding(name, expression);
}

bool QmlStateNodeInstance::updateStateVariant(const NodeInstance &target, const QString &propertyName, const QVariant &value)
{
    // iterate over propertychange object and update values
    QDeclarativeListReference listReference(stateObject(), "changes");
    for (int i = 0; i < listReference.count(); i++) {
        //We also have parent and anchor changes
        QmlPropertyChangesObject *changeObject  = qobject_cast<QmlPropertyChangesObject*>(listReference.at(i));
        if (changeObject && target.isWrappingThisObject(changeObject->targetObject()))
                return changeObject->updateStateVariant(propertyName, value);
    }

    return false;
}

bool QmlStateNodeInstance::updateStateBinding(const NodeInstance &target, const QString &propertyName, const QString &expression)
{
    // iterate over propertychange object and update binding
    QDeclarativeListReference listReference(stateObject(), "changes");
    for (int i = 0; i < listReference.count(); i++) {
        QmlPropertyChangesObject *changeObject  = qobject_cast<QmlPropertyChangesObject*>(listReference.at(i));
        if (changeObject && target.isWrappingThisObject(changeObject->targetObject()))
                return changeObject->updateStateBinding(propertyName, expression);
    }

    return false;
}

bool QmlStateNodeInstance::resetStateProperty(const NodeInstance &target, const QString &propertyName, const QVariant &resetValue)
{
    // iterate over propertychange object and reset propertry
    QDeclarativeListReference listReference(stateObject(), "changes");
    for (int i = 0; i < listReference.count(); i++) {
        QmlPropertyChangesObject *changeObject  = qobject_cast<QmlPropertyChangesObject*>(listReference.at(i));
        if (changeObject && target.isWrappingThisObject(changeObject->targetObject()))
                return changeObject->resetStateProperty(propertyName, resetValue);
    }

    return false;
}

} // namespace Internal
} // namespace QmlDesigner
