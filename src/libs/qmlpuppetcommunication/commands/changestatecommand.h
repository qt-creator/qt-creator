// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QVector>

#include "propertyvaluecontainer.h"

namespace QmlDesigner {

class ChangeStateCommand
{
    friend QDataStream &operator>>(QDataStream &in, ChangeStateCommand &command);
    friend QDebug operator <<(QDebug debug, const ChangeStateCommand &command);

public:
    ChangeStateCommand();
    explicit ChangeStateCommand(qint32 stateInstanceId);

    qint32 stateInstanceId() const;

private:
    qint32 m_stateInstanceId;
};

QDataStream &operator<<(QDataStream &out, const ChangeStateCommand &command);
QDataStream &operator>>(QDataStream &in, ChangeStateCommand &command);

QDebug operator <<(QDebug debug, const ChangeStateCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangeStateCommand)
