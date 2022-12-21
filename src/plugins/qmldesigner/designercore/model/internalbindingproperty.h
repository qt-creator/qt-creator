// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "internalproperty.h"

namespace QmlDesigner {
namespace Internal {

class InternalBindingProperty : public InternalProperty
{
public:
    using Pointer = QSharedPointer<InternalBindingProperty>;

    static Pointer create(const PropertyName &name, const InternalNodePointer &propertyOwner);

    bool isValid() const override;

    QString expression() const;
    void setExpression(const QString &expression);

    void setDynamicExpression(const TypeName &type, const QString &expression);


    bool isBindingProperty() const override;

protected:
    InternalBindingProperty(const PropertyName &name, const InternalNodePointer &propertyOwner);

private:
    QString m_expression;
};

} // namespace Internal
} // namespace QmlDesigner
