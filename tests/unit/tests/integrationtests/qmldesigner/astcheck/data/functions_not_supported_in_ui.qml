// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

Item {
    id: root

    required property string message

    anchors.centerIn: parent    
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

    onOpened: root.reject()
}
