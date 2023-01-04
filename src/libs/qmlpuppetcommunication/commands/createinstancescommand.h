// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QVector>

#include "instancecontainer.h"

namespace QmlDesigner {

class CreateInstancesCommand
{
    friend QDataStream &operator>>(QDataStream &in, CreateInstancesCommand &command);

public:
    CreateInstancesCommand();
    explicit CreateInstancesCommand(const QVector<InstanceContainer> &container);

    QVector<InstanceContainer> instances() const;

private:
    QVector<InstanceContainer> m_instanceVector;
};

QDataStream &operator<<(QDataStream &out, const CreateInstancesCommand &command);
QDataStream &operator>>(QDataStream &in, CreateInstancesCommand &command);

QDebug operator <<(QDebug debug, const CreateInstancesCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::CreateInstancesCommand)
