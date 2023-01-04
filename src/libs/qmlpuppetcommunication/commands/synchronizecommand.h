// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDebug>
#include <QMetaType>
#include <QVector>

namespace QmlDesigner {

class SynchronizeCommand
{
    friend QDataStream &operator>>(QDataStream &in, SynchronizeCommand &command);
    friend bool operator ==(const SynchronizeCommand &first, const SynchronizeCommand &second);

public:
    SynchronizeCommand()
        : m_synchronizeId(-1)
    {}
    explicit SynchronizeCommand(int synchronizeId)
        : m_synchronizeId(synchronizeId)
    {}

    int synchronizeId() const { return m_synchronizeId; }

    friend QDataStream &operator<<(QDataStream &out, const SynchronizeCommand &command)
    {
        out << command.synchronizeId();

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SynchronizeCommand &command)
    {
        in >> command.m_synchronizeId;

        return in;
    }

    friend bool operator==(const SynchronizeCommand &first, const SynchronizeCommand &second)
    {
        return first.m_synchronizeId == second.m_synchronizeId;
    }

    friend QDebug operator<<(QDebug debug, const SynchronizeCommand &command)
    {
        return debug.nospace() << "SynchronizeCommand(synchronizeId: " << command.synchronizeId()
                               << ")";
    }

private:
    int m_synchronizeId;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::SynchronizeCommand)
