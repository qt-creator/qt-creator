// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDataStream>
#include <QDebug>
#include <QMetaType>
#include <QVector>

#include "propertyvaluecontainer.h"

namespace QmlDesigner {

class ChangeAuxiliaryCommand
{
public:
    friend QDataStream &operator>>(QDataStream &in, ChangeAuxiliaryCommand &command)
    {
        in >> command.auxiliaryChanges;
        return in;
    }

    friend QDataStream &operator<<(QDataStream &out, const ChangeAuxiliaryCommand &command)
    {
        out << command.auxiliaryChanges;
        return out;
    }

    friend QDebug operator <<(QDebug debug, const ChangeAuxiliaryCommand &command);

    QVector<PropertyValueContainer> auxiliaryChanges;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangeAuxiliaryCommand)
