// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QVector>

#include "propertybindingcontainer.h"

namespace QmlDesigner {

class ChangeBindingsCommand
{
public:
    friend QDataStream &operator>>(QDataStream &in, ChangeBindingsCommand &command) {
        in >> command.bindingChanges;
        return in;
    }

    friend QDataStream &operator<<(QDataStream &out, const ChangeBindingsCommand &command) {
        out << command.bindingChanges;
        return out;
    }

    friend QDebug operator <<(QDebug debug, const ChangeBindingsCommand &command);

    QVector<PropertyBindingContainer> bindingChanges;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangeBindingsCommand)
