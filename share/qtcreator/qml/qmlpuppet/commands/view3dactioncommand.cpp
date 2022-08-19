// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "view3dactioncommand.h"

#include <QDebug>
#include <QDataStream>

namespace QmlDesigner {

View3DActionCommand::View3DActionCommand(Type type, const QVariant &value)
    : m_type(type)
    , m_value(value)
{
}

View3DActionCommand::View3DActionCommand(int pos)
    : m_type(ParticlesSeek)
    , m_value(pos)
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

View3DActionCommand::Type View3DActionCommand::type() const
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
    out << qint32(command.type());

    return out;
}

QDataStream &operator>>(QDataStream &in, View3DActionCommand &command)
{
    QVariant value;
    qint32 type;
    in >> value;
    in >> type;
    command.m_value = value;
    command.m_type = View3DActionCommand::Type(type);

    return in;
}

QDebug operator<<(QDebug debug, const View3DActionCommand &command)
{
    return debug.nospace() << "View3DActionCommand(type: "
                           << command.m_type << ","
                           << command.m_value << ")\n";
}

} // namespace QmlDesigner
