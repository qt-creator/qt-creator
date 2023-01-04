// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "puppetalivecommand.h"

namespace QmlDesigner {

PuppetAliveCommand::PuppetAliveCommand() = default;

QDataStream &operator<<(QDataStream &out, const PuppetAliveCommand &/*command*/)
{
    return out;
}

QDataStream &operator>>(QDataStream &in, PuppetAliveCommand &/*command*/)
{
    return in;
}

QDebug operator <<(QDebug debug, const PuppetAliveCommand &/*command*/)
{
    return debug.nospace() << "PuppetAliveCommand()";
}

} // namespace QmlDesigner
