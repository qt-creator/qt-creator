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

        StudioControls.CheckBox {
            text: qsTr("Live Update")
            actionIndicatorVisible: false
            style: StudioTheme.Values.viewBarControlStyle
            checked: root.rootEditor ? root.rootEditor.liveUpdate : false
            onToggled: root.rootEditor.liveUpdate = checked
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: StudioTheme.Values.toolbarHorizontalMargin
        }

        Item { // Spacer
            Layout.fillWidth: true
            Layout.preferredHeight: 1
        }

        FooterButton {
            buttonIcon: qsTr("Close")
            onClicked: root.rootEditor.close()
        }

        FooterButton {
            buttonIcon: qsTr("Apply")
            onClicked: root.rootEditor.rebakeRequested()
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
