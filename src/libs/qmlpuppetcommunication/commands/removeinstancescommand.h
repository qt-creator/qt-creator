// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDataStream>
#include <QList>
#include <QMetaType>

namespace QmlDesigner {

class RemoveInstancesCommand
{
    friend QDataStream &operator>>(QDataStream &in, RemoveInstancesCommand &command);
    friend QDebug operator <<(QDebug debug, const RemoveInstancesCommand &command);

public:
    RemoveInstancesCommand();
    explicit RemoveInstancesCommand(const QList<qint32> &idVector);

    const QList<qint32> instanceIds() const;

private:
    QList<qint32> m_instanceIdVector;
};

QDataStream &operator<<(QDataStream &out, const RemoveInstancesCommand &command);
QDataStream &operator>>(QDataStream &in, RemoveInstancesCommand &command);

QDebug operator <<(QDebug debug, const RemoveInstancesCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::RemoveInstancesCommand)
