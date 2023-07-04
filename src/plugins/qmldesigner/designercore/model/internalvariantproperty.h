// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "internalproperty.h"

namespace QmlDesigner {
namespace Internal {

class InternalVariantProperty : public InternalProperty
{
public:
    using Pointer = std::shared_ptr<InternalVariantProperty>;
    static constexpr PropertyType type = PropertyType::Variant;

    InternalVariantProperty(const PropertyName &name, const InternalNodePointer &propertyOwner);

    bool isValid() const override;

    QVariant value() const;
    void setValue(const QVariant &value);

    void setDynamicValue(const TypeName &type, const QVariant &value);

private:
    QVariant m_value;
};

} // namespace Internal
} // namespace QmlDesigner
