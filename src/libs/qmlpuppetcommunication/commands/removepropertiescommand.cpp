// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "removepropertiescommand.h"

#include <QDebug>

namespace QmlDesigner {

RemovePropertiesCommand::RemovePropertiesCommand() = default;

RemovePropertiesCommand::RemovePropertiesCommand(const QVector<PropertyAbstractContainer> &properties)
    : m_properties(properties)
{
}

const QVector<PropertyAbstractContainer> RemovePropertiesCommand::properties() const
{
    return m_properties;
}

QDataStream &operator<<(QDataStream &out, const RemovePropertiesCommand &command)
{
    out << command.properties();

    return out;
}

QDataStream &operator>>(QDataStream &in, RemovePropertiesCommand &command)
{
    in >> command.m_properties;

    return in;
}

QDebug operator <<(QDebug debug, const RemovePropertiesCommand &command)
{
    return debug.nospace() << "RemovePropertiesCommand(properties: " << command.m_properties << ")";
}

}
