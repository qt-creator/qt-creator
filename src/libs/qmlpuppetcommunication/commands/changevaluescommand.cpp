// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "changevaluescommand.h"

#include <QDebug>

namespace QmlDesigner {

ChangeValuesCommand::ChangeValuesCommand() = default;

ChangeValuesCommand::ChangeValuesCommand(const QVector<PropertyValueContainer> &valueChangeVector)
    : m_valueChangeVector (valueChangeVector)
{
}

const QVector<PropertyValueContainer> ChangeValuesCommand::valueChanges() const
{
    return m_valueChangeVector;
}

QDataStream &operator<<(QDataStream &out, const ChangeValuesCommand &command)
{
    out << command.valueChanges();

    return out;
}

QDataStream &operator>>(QDataStream &in, ChangeValuesCommand &command)
{
    in >> command.m_valueChangeVector;

    return in;
}

QDebug operator <<(QDebug debug, const ChangeValuesCommand &command)
{
    return debug.nospace() << "ChangeValuesCommand(valueChanges: " << command.m_valueChangeVector << ")";
}

} // namespace QmlDesigner
