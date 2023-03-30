// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme

Dialog {
    id: root

    required property string message

    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    implicitWidth: 300
    modal: true

    contentItem: Column {
        spacing: 20
        width: parent.width

        Text {
            text: root.message
            color: StudioTheme.Values.themeTextColor
            wrapMode: Text.WordWrap
            width: root.width
            leftPadding: 10
            rightPadding: 10
        }

        HelperWidgets.Button {
            text: qsTr("Close")
            anchors.right: parent.right
            onClicked: root.reject()
        }
    }

    onOpened: root.forceActiveFocus()
}
