// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "puppettocreatorcommand.h"

namespace QmlDesigner {

// A generic command that can hold a variant data from puppet to creator

PuppetToCreatorCommand::PuppetToCreatorCommand(Type type, const QVariant &data)
    : m_type(type)
    , m_data(data)
{

}

QDataStream &operator<<(QDataStream &out, const PuppetToCreatorCommand &command)
{
    out << qint32(command.type());
    out << command.data();
    return out;
}

QDataStream &operator>>(QDataStream &in, PuppetToCreatorCommand &command)
{
    qint32 type;
    in >> type;
    command.m_type = PuppetToCreatorCommand::Type(type);
    in >> command.m_data;
    return in;
}

} // namespace QmlDesigner
