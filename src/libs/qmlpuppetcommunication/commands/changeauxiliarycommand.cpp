// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "changeauxiliarycommand.h"

#include <QDebug>

namespace QmlDesigner {

QDebug operator <<(QDebug debug, const ChangeAuxiliaryCommand &command)
{
    return debug.nospace() << "ChangeAuxiliaryCommand(auxiliaryChanges: " << command.auxiliaryChanges << ")";
}

} // namespace QmlDesigner
