// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QPair>

namespace QmlDesigner {

enum InformationName {
    NoName,
    NoInformationChange = NoName,
    AllStates,
    Size,
    BoundingRect,
    BoundingRectPixmap,
    Transform,
    HasAnchor,
    Anchor,
    InstanceTypeForProperty,
    PenWidth,
    Position,
    IsInLayoutable,
    SceneTransform,
    IsResizable,
    IsMovable,
    IsAnchoredByChildren,
    IsAnchoredBySibling,
    HasContent,
    HasBindingForProperty,
    ContentTransform,
    ContentItemTransform,
    ContentItemBoundingRect,
    MoveView,
    ShowView,
    ResizeView,
    HideView
};
}
