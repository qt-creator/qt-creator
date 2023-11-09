// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme

StudioControls.Dialog {
    id: root

    required property string message

    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    implicitWidth: 300
    modal: true

    contentItem: ColumnLayout {
        spacing: StudioTheme.Values.sectionColumnSpacing

        Text {
            Layout.fillWidth: true
            text: root.message
            color: StudioTheme.Values.themeTextColor
            wrapMode: Text.WordWrap
            leftPadding: 10
            rightPadding: 10
        }

        HelperWidgets.Button {
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

            text: qsTr("Close")
            onClicked: root.reject()
        }
    }

    onOpened: root.forceActiveFocus()
}
