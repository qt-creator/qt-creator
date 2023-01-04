// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QVector>

#include "reparentcontainer.h"

namespace QmlDesigner {

class ReparentInstancesCommand
{
    friend QDataStream &operator>>(QDataStream &in, ReparentInstancesCommand &command);
    friend QDebug operator <<(QDebug debug, const ReparentInstancesCommand &command);

public:
    ReparentInstancesCommand();
    explicit ReparentInstancesCommand(const QVector<ReparentContainer> &container);

    const QVector<ReparentContainer> reparentInstances() const;

private:
    QVector<ReparentContainer> m_reparentInstanceVector;
};

QDataStream &operator<<(QDataStream &out, const ReparentInstancesCommand &command);
QDataStream &operator>>(QDataStream &in, ReparentInstancesCommand &command);

QDebug operator <<(QDebug debug, const ReparentInstancesCommand &command);

} //

Q_DECLARE_METATYPE(QmlDesigner::ReparentInstancesCommand)
