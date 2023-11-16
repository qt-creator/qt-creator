// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme
import StudioControls as StudioControls
import HelperWidgets as HelperWidgets

StudioControls.PopupDialog {
    property alias backend: form.backend

    titleBar: Row {
        spacing: 30 // TODO
        anchors.fill: parent

        Text {
            color: StudioTheme.Values.themeTextColor
            text: qsTr("Owner")
            font.pixelSize: StudioTheme.Values.myFontSize
            anchors.verticalCenter: parent.verticalCenter

            HelperWidgets.ToolTipArea {
                anchors.fill: parent
                tooltip: qsTr("The owner of the property")
            }
        }

        Text {
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.myFontSize
            anchors.verticalCenter: parent.verticalCenter
            text: form.backend.targetNode
        }
    }

    BindingsDialogForm {
        id: form
    }
}
