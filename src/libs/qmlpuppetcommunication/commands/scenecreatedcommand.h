// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>

namespace QmlDesigner {

class SceneCreatedCommand
{
public:
    friend QDataStream &operator<<(QDataStream &out, const SceneCreatedCommand &) { return out; }

    friend QDataStream &operator>>(QDataStream &in, SceneCreatedCommand &) { return in; }
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::SceneCreatedCommand)
