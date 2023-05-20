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

    InternalSignalHandlerProperty(const PropertyName &name, const InternalNodePointer &propertyOwner);

    bool isValid() const override;

    QString source() const;
    void setSource(const QString &source);

    bool isSignalHandlerProperty() const override;


private:
    QString m_source;
};

class InternalSignalDeclarationProperty : public InternalProperty
{
public:
    using Pointer = std::shared_ptr<InternalSignalDeclarationProperty>;

    InternalSignalDeclarationProperty(const PropertyName &name,
                                      const InternalNodePointer &propertyOwner);

    bool isValid() const override;

    QString signature() const;
    void setSignature(const QString &source);

    bool isSignalDeclarationProperty() const override;

private:
    QString m_signature;
};

} // namespace Internal
} // namespace QmlDesigner
