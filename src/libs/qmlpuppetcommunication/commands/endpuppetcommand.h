// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmetatype.h>

namespace QmlDesigner {

class EndPuppetCommand
{
public:
    EndPuppetCommand();
};

QDataStream &operator<<(QDataStream &out, const EndPuppetCommand &command);
QDataStream &operator>>(QDataStream &in, EndPuppetCommand &command);

QDebug operator <<(QDebug debug, const EndPuppetCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::EndPuppetCommand)
