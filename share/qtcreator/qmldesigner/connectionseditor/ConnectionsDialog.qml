// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

PopupDialog {
    titleBar: Row {
        spacing: 30 // TODO
        anchors.fill: parent

        Text {
            color: StudioTheme.Values.themeTextColor
            text: qsTr("Target")
            font.pixelSize: StudioTheme.Values.myFontSize
            anchors.verticalCenter: parent.verticalCenter
        }

        StudioControls.TopLevelComboBox {
            id: target
            style: StudioTheme.Values.connectionPopupControlStyle
            width: 180
            anchors.verticalCenter: parent.verticalCenter
            model: ["mySpinbox", "foo", "backendObject"]
        }
    }

    ConnectionsDialogForm {}
}
