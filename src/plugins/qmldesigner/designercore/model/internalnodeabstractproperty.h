// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "internalproperty.h"


namespace QmlDesigner {
namespace Internal {

class InternalNodeAbstractProperty : public InternalProperty
{
    friend InternalNode;

public:
    using Pointer = QSharedPointer<InternalNodeAbstractProperty>;
    using WeakPointer = QWeakPointer<InternalNodeAbstractProperty>;

    bool isNodeAbstractProperty() const override;

    virtual QList<InternalNodePointer> allSubNodes() const = 0;
    virtual QList<InternalNodePointer> directSubNodes() const = 0;

    virtual bool isEmpty() const = 0;
    virtual int count() const = 0;
    virtual int indexOf(const InternalNodePointer &node) const = 0;

    bool isValid() const override;

    using InternalProperty::remove; // keep the virtual remove(...) function around

protected:
    InternalNodeAbstractProperty(const PropertyName &name, const InternalNodePointer &propertyOwner);
    virtual void remove(const InternalNodePointer &node) = 0;
    virtual void add(const InternalNodePointer &node) = 0;
};

} // namespace Internal
} // namespace QmlDesigner
