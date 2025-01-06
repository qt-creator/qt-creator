// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
import LightUtils 1.0

IconGizmo {
    id: lightIconGizmo

    property color overlayColor: targetNode ? targetNode.color : "transparent"

    iconSource: targetNode
                ? targetNode instanceof DirectionalLight
                  ? "image://IconGizmoImageProvider/directional.png:" + overlayColor
                  : targetNode instanceof PointLight
                    ? "image://IconGizmoImageProvider/point.png:" + overlayColor
                    : "image://IconGizmoImageProvider/spot.png:" + overlayColor
                : "image://IconGizmoImageProvider/point.png:" + overlayColor
}
