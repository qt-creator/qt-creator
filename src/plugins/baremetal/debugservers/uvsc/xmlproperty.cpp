// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmlnodevisitor.h"
#include "xmlproperty.h"

namespace BareMetal::Gen::Xml {

Property::Property(QByteArray name, QVariant value)
{
    setName(std::move(name));
    setValue(std::move(value));
}

void Property::appendProperty(QByteArray name, QVariant value)
{
    appendChild<Property>(std::move(name), std::move(value));
}

void Property::appendMultiLineProperty(QByteArray key, QStringList values, QChar sep)
{
    const QString line = values.join(std::move(sep));
    appendProperty(std::move(key), QVariant::fromValue(line));
}

void Property::accept(INodeVisitor *visitor) const
{
    visitor->visitPropertyStart(this);

    for (const auto &child : children())
        child->accept(visitor);

    visitor->visitPropertyEnd(this);
}

} // BareMetal::Gen::Xml
