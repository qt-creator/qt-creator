// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <nodeinstanceglobal.h>

#include <QMetaType>
#include <QVariant>

namespace QmlDesigner {

class View3DActionCommand
{
    friend QDataStream &operator>>(QDataStream &in, View3DActionCommand &command);
    friend QDebug operator<<(QDebug debug, const View3DActionCommand &command);

public:
    View3DActionCommand(View3DActionType type, const QVariant &value);

    View3DActionCommand() = default;

    bool isEnabled() const;
    QVariant value() const;
    View3DActionType type() const;
    int position() const;

private:
    View3DActionType m_type = View3DActionType::Empty;
    QVariant m_value;
};

QDataStream &operator<<(QDataStream &out, const View3DActionCommand &command);
QDataStream &operator>>(QDataStream &in, View3DActionCommand &command);

QDebug operator<<(QDebug debug, const View3DActionCommand &command);
QDebug operator<<(QDebug debug, View3DActionType type);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::View3DActionCommand)
