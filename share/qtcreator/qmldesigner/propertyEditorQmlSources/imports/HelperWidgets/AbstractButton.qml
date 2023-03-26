// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

StudioControls.AbstractButton {
    id: button

    property alias tooltip: toolTipArea.tooltip

    hover: toolTipArea.containsMouse

    ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
        // Without setting the acceptedButtons property the clicked event won't
        // reach the AbstractButton, it will be consumed by the ToolTipArea
        acceptedButtons: Qt.NoButton
    }
}
