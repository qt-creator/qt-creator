// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "internalnodeabstractproperty.h"
#include "internalnode_p.h"

namespace QmlDesigner {
namespace Internal {

InternalNodeAbstractProperty::InternalNodeAbstractProperty(const PropertyName &name, const InternalNode::Pointer &propertyOwner)
    : InternalProperty(name, propertyOwner)
{
}

bool InternalNodeAbstractProperty::isNodeAbstractProperty() const
{
    return true;
}

bool InternalNodeAbstractProperty::isValid() const
{
    return InternalProperty::isValid() && isNodeAbstractProperty();
}

} // namespace Internal
} // namespace QmlDesigner
