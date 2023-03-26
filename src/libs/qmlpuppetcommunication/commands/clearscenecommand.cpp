// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clearscenecommand.h"

#include <QDebug>

namespace QmlDesigner {

ClearSceneCommand::ClearSceneCommand() = default;

QDataStream &operator<<(QDataStream &out, const ClearSceneCommand &/*command*/)
{
    return out;
}

QDataStream &operator>>(QDataStream &in, ClearSceneCommand &/*command*/)
{
    return in;
}

QDebug operator <<(QDebug debug, const ClearSceneCommand &/*command*/)
{
     return debug.nospace() << "ClearSceneCommand()";
}

} // namespace QmlDesigner
