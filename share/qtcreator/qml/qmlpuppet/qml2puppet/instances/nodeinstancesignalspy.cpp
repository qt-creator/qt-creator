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
            && QLatin1String(metaProperty.name()) != "parent") {
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
            foreach (const PropertyName &propertyName, m_indexPropertyHash.values(methodId))
                nodeInstance->nodeInstanceServer()->notifyPropertyChange(nodeInstance->instanceId(), propertyName);
        }

    }

    return QObject::qt_metacall(call, methodId, a);
}

} // namespace Internal
} // namespace QmlDesigner
