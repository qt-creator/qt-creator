// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectnodeinstance.h"

QT_BEGIN_NAMESPACE
class QQmlComponent;
QT_END_NAMESPACE

namespace QmlDesigner {
namespace Internal {

class ComponentNodeInstance : public ObjectNodeInstance
{
public:
    using Pointer = QSharedPointer<ComponentNodeInstance>;
    using WeakPointer = QWeakPointer<ComponentNodeInstance>;

    ComponentNodeInstance(QQmlComponent *component);
    static Pointer create(QObject *objectToBeWrapped);

    bool hasContent() const override;

    void setNodeSource(const QString &source) override;

private: //function
    QQmlComponent *component() const;

};

} // Internal
} // QmlDesigner
