// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QtCore/qsize.h>

namespace QmlDesigner {

class Update3dViewStateCommand
{
    friend QDataStream &operator>>(QDataStream &in, Update3dViewStateCommand &command);
    friend QDebug operator<<(QDebug debug, const Update3dViewStateCommand &command);

public:
    enum Type { SizeChange, Empty };

    explicit Update3dViewStateCommand(const QSize &size);
    Update3dViewStateCommand() = default;

    QSize size() const;
    Type type() const;

private:
    QSize m_size;

    Type m_type = Empty;
};

QDataStream &operator<<(QDataStream &out, const Update3dViewStateCommand &command);
QDataStream &operator>>(QDataStream &in, Update3dViewStateCommand &command);

QDebug operator<<(QDebug debug, const Update3dViewStateCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::Update3dViewStateCommand)
