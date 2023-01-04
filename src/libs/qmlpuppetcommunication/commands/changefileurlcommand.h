// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmetatype.h>
#include <QDataStream>
#include <QUrl>

namespace QmlDesigner {

class ChangeFileUrlCommand
{
public:
    friend QDataStream &operator>>(QDataStream &in, ChangeFileUrlCommand &command)
    {
        in >> command.fileUrl;
        return in;
    }

    friend QDataStream &operator<<(QDataStream &out, const ChangeFileUrlCommand &command)
    {
        out << command.fileUrl;
        return out;
    }

    friend QDebug operator <<(QDebug debug, const ChangeFileUrlCommand &command);

    QUrl fileUrl;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangeFileUrlCommand)
