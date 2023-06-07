// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "internalproperty.h"

namespace QmlDesigner {
namespace Internal {

class InternalSignalHandlerProperty : public InternalProperty
{
public:
    using Pointer = std::shared_ptr<InternalSignalHandlerProperty>;
    static constexpr PropertyType type = PropertyType::SignalHandler;

    InternalSignalHandlerProperty(const PropertyName &name, const InternalNodePointer &propertyOwner);

    bool isValid() const override;

    QString source() const;
    void setSource(const QString &source);

private:
    QString m_source;
};

class InternalSignalDeclarationProperty : public InternalProperty
{
public:
    using Pointer = std::shared_ptr<InternalSignalDeclarationProperty>;
    static constexpr PropertyType type = PropertyType::SignalDeclaration;

    InternalSignalDeclarationProperty(const PropertyName &name,
                                      const InternalNodePointer &propertyOwner);

    bool isValid() const override;

    QString signature() const;
    void setSignature(const QString &source);

private:
    QString m_signature;
};

} // namespace Internal
} // namespace QmlDesigner
