// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "changebindingscommand.h"

#include <QDebug>

namespace QmlDesigner {

QDebug operator <<(QDebug debug, const ChangeBindingsCommand &command)
{
    return debug.nospace() << "PropertyValueContainer(bindingChanges: " << command.bindingChanges << ")";
}

} // namespace QmlDesigner
