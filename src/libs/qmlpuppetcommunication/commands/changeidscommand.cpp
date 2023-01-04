// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "changeidscommand.h"

#include <QDebug>

namespace QmlDesigner {

QDebug operator <<(QDebug debug, const ChangeIdsCommand &command)
{
    return debug.nospace() << "ChangeIdsCommand(ids: " << command.ids << ")";
}

} // namespace QmlDesigner
