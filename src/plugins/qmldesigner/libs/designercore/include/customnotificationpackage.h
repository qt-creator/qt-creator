// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modelnode.h"

#include <QEvent>
#include <QSize>

namespace QmlDesigner {

struct InputEvent
{
    QEvent *event;
};

struct Resize3DCanvas
{
    QSize size;
};

struct NodePreviewImage
{
    ModelNode modelNode;
    ModelNode renderNode;
    QSize size;
    QByteArray requestId;
};

using CustomNotificationPackage = std::variant<InputEvent, Resize3DCanvas, NodePreviewImage>;

} // namespace QmlDesigner
