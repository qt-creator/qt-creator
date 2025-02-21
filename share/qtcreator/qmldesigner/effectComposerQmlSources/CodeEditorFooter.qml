// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Rectangle {
    id: root

    property var rootEditor: shaderEditor

    color: StudioTheme.Values.themeToolbarBackground
    implicitHeight: rowLayout.height

    RowLayout {
        id: rowLayout

        width: parent.width
        anchors.verticalCenter: parent.verticalCenter

        spacing: StudioTheme.Values.controlGap

        StudioControls.Switch {
            id: liveUpdateButton

            objectName: "BtnLiveUpdate"

            text: qsTr("Live Update")
            actionIndicatorVisible: false
            style: StudioTheme.ControlStyle {
                squareControlSize: Qt.size(20, 20)
                switchPadding: 2
            }
            checked: root.rootEditor ? root.rootEditor.liveUpdate : false
            onToggled: root.rootEditor.liveUpdate = liveUpdateButton.checked
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: StudioTheme.Values.toolbarHorizontalMargin
        }

        Item { // Spacer
            Layout.fillWidth: true
            Layout.preferredHeight: 1
        }

        FooterButton {
            objectName: "BtnClose"
            buttonIcon: qsTr("Close")
            onClicked: root.rootEditor.close()
        }

        FooterButton {
            objectName: "BtnApply"
            buttonIcon: qsTr("Apply")
            onClicked: root.rootEditor.rebakeRequested()
            enabled: !liveUpdateButton.checked
            Layout.rightMargin: StudioTheme.Values.toolbarHorizontalMargin
        }
    }

    component FooterButton: StudioControls.AbstractButton {
        iconFontFamily: StudioTheme.Constants.font.family
        style: StudioTheme.Values.viewBarControlStyle
        checkable: false
        Layout.alignment: Qt.AlignVCenter
        Layout.preferredWidth: 100
    }
}
