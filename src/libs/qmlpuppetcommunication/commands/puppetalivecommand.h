// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QDebug>

namespace QmlDesigner {

class PuppetAliveCommand
{
    friend QDataStream &operator>>(QDataStream &in, PuppetAliveCommand &command);
    friend QDebug operator <<(QDebug debug, const PuppetAliveCommand &command);

public:
    PuppetAliveCommand();
};

QDataStream &operator<<(QDataStream &out, const PuppetAliveCommand &command);
QDataStream &operator>>(QDataStream &in, PuppetAliveCommand &command);

QDebug operator <<(QDebug debug, const PuppetAliveCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PuppetAliveCommand)
