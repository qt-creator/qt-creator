// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QHash>
#include <QSharedPointer>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

namespace Internal {

class ObjectNodeInstance;

using ObjectNodeInstancePointer = QSharedPointer<ObjectNodeInstance>;
using ObjectNodeInstanceWeakPointer = QWeakPointer<ObjectNodeInstance>;

class NodeInstanceSignalSpy : public QObject
{
public:
    explicit NodeInstanceSignalSpy();

    void setObjectNodeInstance(const ObjectNodeInstancePointer &nodeInstance);

    int qt_metacall(QMetaObject::Call, int, void **) override;

protected:
    void registerObject(QObject *spiedObject);
    void registerProperty(const QMetaProperty &metaProperty, QObject *spiedObject, const PropertyName &propertyPrefix = PropertyName());
    void registerChildObject(const QMetaProperty &metaProperty, QObject *spiedObject);

private:
    int methodeOffset;
    QMultiHash<int, PropertyName> m_indexPropertyHash;
    QObjectList m_registeredObjectList;
    ObjectNodeInstanceWeakPointer m_objectNodeInstance;
};

} // namespace Internal
} // namespace QmlDesigner
