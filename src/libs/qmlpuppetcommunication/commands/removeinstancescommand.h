// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QVector>
#include <QDataStream>

#include "instancecontainer.h"

namespace QmlDesigner {

class RemoveInstancesCommand
{
    friend QDataStream &operator>>(QDataStream &in, RemoveInstancesCommand &command);
    friend QDebug operator <<(QDebug debug, const RemoveInstancesCommand &command);

public:
    RemoveInstancesCommand();
    explicit RemoveInstancesCommand(const QVector<qint32> &idVector);

    const QVector<qint32> instanceIds() const;

private:
    QVector<qint32> m_instanceIdVector;
};

QDataStream &operator<<(QDataStream &out, const RemoveInstancesCommand &command);
QDataStream &operator>>(QDataStream &in, RemoveInstancesCommand &command);

QDebug operator <<(QDebug debug, const RemoveInstancesCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::RemoveInstancesCommand)
