// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nodeinstancesignalspy.h"
#include "objectnodeinstance.h"

#include <qmlprivategate.h>

#include <QMetaProperty>
#include <QMetaObject>
#include <QDebug>
#include <QSharedPointer>

#include <QQmlProperty>

namespace QmlDesigner {
namespace Internal {

NodeInstanceSignalSpy::NodeInstanceSignalSpy() :
    QObject()
{
    blockSignals(true);
}

void NodeInstanceSignalSpy::setObjectNodeInstance(const ObjectNodeInstance::Pointer &nodeInstance)
{
    methodeOffset = QObject::staticMetaObject.methodCount() + 1;
    registerObject(nodeInstance->object());
    m_objectNodeInstance = nodeInstance;

}

void NodeInstanceSignalSpy::registerObject(QObject *spiedObject)
{
    if (m_registeredObjectList.contains(spiedObject)) // prevent cycles
        return;

    m_registeredObjectList.append(spiedObject);
    for (int index = QObject::staticMetaObject.propertyOffset();
         index < spiedObject->metaObject()->propertyCount();
         index++) {
        QMetaProperty metaProperty = spiedObject->metaObject()->property(index);

        registerProperty(metaProperty, spiedObject);
        registerChildObject(metaProperty, spiedObject);
    }
}

void NodeInstanceSignalSpy::registerProperty(const QMetaProperty &metaProperty, QObject *spiedObject, const PropertyName &propertyPrefix)
{
    if (metaProperty.isReadable()
            && metaProperty.isWritable()
            && !QmlPrivateGate::isPropertyQObject(metaProperty)
            && metaProperty.hasNotifySignal()) {
        QMetaMethod metaMethod = metaProperty.notifySignal();
        QMetaObject::connect(spiedObject, metaMethod.methodIndex(), this, methodeOffset, Qt::DirectConnection);

        m_indexPropertyHash.insert(methodeOffset, propertyPrefix + PropertyName(metaProperty.name()));


        methodeOffset++;
    }
}

void NodeInstanceSignalSpy::registerChildObject(const QMetaProperty &metaProperty, QObject *spiedObject)
{
    if (metaProperty.isReadable()
            && !metaProperty.isWritable()
            && QmlPrivateGate::isPropertyQObject(metaProperty)
            && QLatin1String(metaProperty.name()) != QLatin1String("parent")) {
        QObject *childObject = QmlPrivateGate::readQObjectProperty(metaProperty, spiedObject);

        if (childObject) {
            for (int index = QObject::staticMetaObject.propertyOffset();
                 index < childObject->metaObject()->propertyCount();
                 index++) {
                QMetaProperty childMetaProperty = childObject->metaObject()->property(index);
                registerProperty(childMetaProperty, childObject, PropertyName(metaProperty.name()) + '.');
            }
        }
    }
}

int NodeInstanceSignalSpy::qt_metacall(QMetaObject::Call call, int methodId, void **a)
{
    if (call == QMetaObject::InvokeMetaMethod && methodId > QObject::staticMetaObject.methodCount()) {
        ObjectNodeInstance::Pointer nodeInstance = m_objectNodeInstance.toStrongRef();

        if (nodeInstance && nodeInstance->nodeInstanceServer() && nodeInstance->isValid()) {
            const QList<PropertyName> values = m_indexPropertyHash.values(methodId);
            for (const PropertyName &propertyName : values)
                nodeInstance->nodeInstanceServer()->notifyPropertyChange(nodeInstance->instanceId(), propertyName);
        }

    }

    return QObject::qt_metacall(call, methodId, a);
}

} // namespace Internal
} // namespace QmlDesigner
