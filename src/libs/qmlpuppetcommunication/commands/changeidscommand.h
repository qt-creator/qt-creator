// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QVector>
#include <QDataStream>

#include "idcontainer.h"

namespace QmlDesigner {

class ChangeIdsCommand
{
public:
    friend QDataStream &operator>>(QDataStream &in, ChangeIdsCommand &command)
    {
        in >> command.ids;
        return in;
    }

    friend QDataStream &operator<<(QDataStream &out, const ChangeIdsCommand &command)
    {
        out << command.ids;
        return out;
    }
    friend QDebug operator <<(QDebug debug, const ChangeIdsCommand &command);

    QVector<IdContainer> ids;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangeIdsCommand)
