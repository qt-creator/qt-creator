// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertycontainer.h"
#include "propertyparser.h"
#include "rewritertracing.h"

#include <QVariant>
#include <QString>
#include <QDebug>

namespace QmlDesigner {

using namespace QmlDesigner::Internal;

using NanotraceHR::keyValue;
using RewriterTracing::category;

// Creates invalid PropertyContainer
PropertyContainer::PropertyContainer()
{
    NanotraceHR::Tracer tracer{"property container constructor", category()};
}

PropertyContainer::PropertyContainer(const PropertyName &name, const QString &type, const QVariant &value)
        : m_name(name), m_type(type), m_value(value)
{
    NanotraceHR::Tracer tracer{"property container constructor with args", category()};

    Q_ASSERT_X(!name.isEmpty(), Q_FUNC_INFO, "Name of property cannot be empty");
    Q_ASSERT_X(!type.isEmpty(), Q_FUNC_INFO, "Type of property cannot be empty");
}

void PropertyContainer::set(const PropertyName &name, const QString &type, const QVariant &value)
{
    NanotraceHR::Tracer tracer{"property container set", category()};
    m_name = name;
    m_type = type;
    m_value = value;
}

bool PropertyContainer::isValid() const
{
    NanotraceHR::Tracer tracer{"property container is valid", category()};

    return !m_name.isEmpty() && m_value.isValid();
}

PropertyName PropertyContainer::name() const
{
    NanotraceHR::Tracer tracer{"property container name", category()};

    return m_name;
}

QVariant PropertyContainer::value() const
{
    NanotraceHR::Tracer tracer{"property container value", category()};

    if (m_value.typeId() == QMetaType::QString)
        m_value = PropertyParser::read(m_type, m_value.toString());
    return m_value;
}


void PropertyContainer::setValue(const QVariant &value)
{
    NanotraceHR::Tracer tracer{"property container set value", category()};

    m_value = value;
}

QString PropertyContainer::type() const
{
    NanotraceHR::Tracer tracer{"property container type", category()};

    return m_type;
}

QDataStream &operator<<(QDataStream &stream, const PropertyContainer &propertyContainer)
{
    NanotraceHR::Tracer tracer{"property container stream output", category()};
    Q_ASSERT(!propertyContainer.name().isEmpty());
    Q_ASSERT(!propertyContainer.type().isEmpty());
    Q_ASSERT(propertyContainer.value().isValid());
    stream << propertyContainer.name();
    stream << propertyContainer.type();
    stream << propertyContainer.value();


    return stream;
}

QDataStream &operator>>(QDataStream &stream, PropertyContainer &propertyContainer)
{
    NanotraceHR::Tracer tracer{"property container stream input", category()};

    stream >> propertyContainer.m_name;
    stream >> propertyContainer.m_type;
    stream >> propertyContainer.m_value;

    Q_ASSERT(!propertyContainer.name().isEmpty());

    return stream;
}

QDebug operator<<(QDebug debug, const PropertyContainer &propertyContainer)
{
    debug << propertyContainer.m_name;
    debug << propertyContainer.m_type;
    debug << propertyContainer.m_value;

    return debug.space();
}

} //namespace QmlDesigner

