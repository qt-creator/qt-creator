// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import HelperWidgets 2.0

StudioControls.AbstractButton {
    id: button

    property alias tooltip: toolTipArea.tooltip

    style: StudioTheme.Values.toolbarButtonStyle
    hover: toolTipArea.containsMouse

    ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
        // Without setting the acceptedButtons property the clicked event won't
        // reach the AbstractButton, it will be consumed by the ToolTipArea
        acceptedButtons: Qt.NoButton
    }
}
