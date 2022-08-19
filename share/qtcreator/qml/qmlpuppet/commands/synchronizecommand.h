// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QVector>

#include "propertyvaluecontainer.h"

namespace QmlDesigner {

class SynchronizeCommand
{
    friend QDataStream &operator>>(QDataStream &in, SynchronizeCommand &command);
    friend bool operator ==(const SynchronizeCommand &first, const SynchronizeCommand &second);

public:
    SynchronizeCommand();
    explicit SynchronizeCommand(int synchronizeId);

    int synchronizeId() const;

private:
    int m_synchronizeId;
};

QDataStream &operator<<(QDataStream &out, const SynchronizeCommand &command);
QDataStream &operator>>(QDataStream &in, SynchronizeCommand &command);

bool operator ==(const SynchronizeCommand &first, const SynchronizeCommand &second);
QDebug operator <<(QDebug debug, const SynchronizeCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::SynchronizeCommand)
