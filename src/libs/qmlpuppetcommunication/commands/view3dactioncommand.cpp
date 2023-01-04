// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "view3dactioncommand.h"

#include <QDebug>
#include <QDataStream>

namespace QmlDesigner {

View3DActionCommand::View3DActionCommand(View3DActionType type, const QVariant &value)
    : m_type(type)
    , m_value(value)
{
}

bool View3DActionCommand::isEnabled() const
{
    return m_value.toBool();
}

QVariant View3DActionCommand::value() const
{
    return m_value;
}

View3DActionType View3DActionCommand::type() const
{
    return m_type;
}

int View3DActionCommand::position() const
{
    bool ok = false;
    int result = m_value.toInt(&ok);
    if (!ok) {
        qWarning() << "View3DActionCommand: returning a position that is not int; command type = "
                   << m_type;
    }

    return result;
}

QDataStream &operator<<(QDataStream &out, const View3DActionCommand &command)
{
    out << command.value();
    out << command.type();

    return out;
}

QDataStream &operator>>(QDataStream &in, View3DActionCommand &command)
{
    in >> command.m_value;
    in >> command.m_type;

    return in;
}

QDebug operator<<(QDebug debug, const View3DActionCommand &command)
{
    return debug.nospace() << "View3DActionCommand(type: "
                           << command.m_type << ","
                           << command.m_value << ")\n";
}

template<typename Enumeration>
constexpr std::underlying_type_t<Enumeration> to_underlying(Enumeration enumeration) noexcept
{
    static_assert(std::is_enum_v<Enumeration>, "to_underlying expect an enumeration");
    return static_cast<std::underlying_type_t<Enumeration>>(enumeration);
}

QDebug operator<<(QDebug debug, View3DActionType type)
{
    return debug.nospace() << to_underlying(type);
}

} // namespace QmlDesigner
