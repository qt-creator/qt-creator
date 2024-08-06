// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls as QuickControls
import StudioTheme 1.0 as StudioTheme

QuickControls.SplitView {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    handle: Rectangle {
        implicitWidth: control.orientation === Qt.Horizontal ? StudioTheme.Values.splitterThickness : control.width
        implicitHeight: control.orientation === Qt.Horizontal ? control.height : StudioTheme.Values.splitterThickness
        color: QuickControls.SplitHandle.pressed ? control.style.slider.handleInteraction
            : (QuickControls.SplitHandle.hovered ? control.style.slider.handleHover
                                     : "transparent")
    }
}
