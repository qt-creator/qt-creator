// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmetatype.h>

namespace QmlDesigner {

class ClearSceneCommand
{
public:
    ClearSceneCommand();
};

QDataStream &operator<<(QDataStream &out, const ClearSceneCommand &command);
QDataStream &operator>>(QDataStream &in, ClearSceneCommand &command);

QDebug operator <<(QDebug debug, const ClearSceneCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ClearSceneCommand)
