// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlstatenodeinstance.h"

#include "qmlpropertychangesnodeinstance.h"

#include <qmlprivategate.h>
#include <private/qquickdesignersupport_p.h>

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

void setAllNodesDirtyRecursive([[maybe_unused]] QQuickItem *parentItem)
{
    if (!parentItem)
        return;
    const QList<QQuickItem *> children = parentItem->childItems();
    for (QQuickItem *childItem : children)
        setAllNodesDirtyRecursive(childItem);
    QQuickDesignerSupport::addDirty(parentItem, QQuickDesignerSupport::Content);
}

void QmlStateNodeInstance::activateState()
{
    if (!QmlPrivateGate::States::isStateActive(object(), context())
            && nodeInstanceServer()->hasInstanceForObject(object())) {
        nodeInstanceServer()->setStateInstance(nodeInstanceServer()->instanceForObject(object()));
        QmlPrivateGate::States::activateState(object(), context());

        setAllNodesDirtyRecursive(nodeInstanceServer()->rootItem());
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
    if (name == "when")
        return;

    ObjectNodeInstance::setPropertyVariant(name, value);
}

void QmlStateNodeInstance::setPropertyBinding(const PropertyName &name, const QString &expression)
{
    if (name == "when")
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

void QmlStateNodeInstance::reparent(const ObjectNodeInstance::Pointer &oldParentInstance,
                                    const PropertyName &oldParentProperty,
                                    const ObjectNodeInstance::Pointer &newParentInstance,
                                    const PropertyName &newParentProperty)
{
    ServerNodeInstance oldState = nodeInstanceServer()->activeStateInstance();

    ObjectNodeInstance::reparent(oldParentInstance,
                                 oldParentProperty,
                                 newParentInstance,
                                 newParentProperty);

    if (oldState.isValid())
        oldState.activateState();
}

} // namespace Internal
} // namespace QmlDesigner
