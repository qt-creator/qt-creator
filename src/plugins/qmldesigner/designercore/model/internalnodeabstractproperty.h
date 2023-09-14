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
    using Pointer = std::shared_ptr<InternalNodeAbstractProperty>;
    using WeakPointer = std::weak_ptr<InternalNodeAbstractProperty>;

    virtual bool isEmpty() const = 0;
    virtual int count() const = 0;
    virtual int indexOf(const InternalNodePointer &node) const = 0;

    bool isValid() const override;

protected:
    InternalNodeAbstractProperty(const PropertyName &name,
                                 const InternalNodePointer &propertyOwner,
                                 PropertyType propertyType);
    virtual void remove(const InternalNodePointer &node) = 0;
    virtual void add(const InternalNodePointer &node) = 0;
};

} // namespace Internal
} // namespace QmlDesigner
