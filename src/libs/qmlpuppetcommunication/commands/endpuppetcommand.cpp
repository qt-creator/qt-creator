// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "endpuppetcommand.h"

#include <QDebug>

namespace QmlDesigner {

EndPuppetCommand::EndPuppetCommand() = default;

QDataStream &operator<<(QDataStream &out, const EndPuppetCommand &/*command*/)
{
    return out;
}

QDataStream &operator>>(QDataStream &in, EndPuppetCommand &/*command*/)
{
    return in;
}

QDebug operator <<(QDebug debug, const EndPuppetCommand &/*command*/)
{
    return debug.nospace() << "EndPuppetCommand()";
}

} // namespace QmlDesigner
