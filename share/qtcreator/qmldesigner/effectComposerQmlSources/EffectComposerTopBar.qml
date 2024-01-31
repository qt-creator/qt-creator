// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

Rectangle {
    id: root

    height: StudioTheme.Values.toolbarHeight
    color: StudioTheme.Values.themeToolbarBackground

    readonly property var backendModel: EffectComposerBackend.effectComposerModel

    signal addClicked
    signal saveClicked
    signal saveAsClicked
    signal assignToSelectedClicked

    Row {
        spacing: 5
        anchors.verticalCenter: parent.verticalCenter

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.add_medium
            tooltip: qsTr("Add new composition")
            enabled: root.backendModel.isEnabled
            onClicked: root.addClicked()
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.save_medium
            tooltip: qsTr("Save current composition")
            enabled: root.backendModel.isEnabled && (root.backendModel.hasUnsavedChanges
                                                  || root.backendModel.currentComposition === "")

            onClicked: root.saveClicked()
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.saveAs_medium
            tooltip: qsTr("Save current composition with a new name")
            enabled: root.backendModel.isEnabled && !root.backendModel.isEmpty

            onClicked: root.saveAsClicked()
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.assignTo_medium
            tooltip: qsTr("Assign current composition to selected item")
            enabled: root.backendModel.isEnabled && root.backendModel.currentComposition !== ""

            onClicked: root.assignToSelectedClicked()
        }
    }


    Text {
        readonly property string compName: root.backendModel.currentComposition

        text: compName !== "" ? compName : qsTr("Untitled")
        anchors.centerIn: parent
        color: StudioTheme.Values.themeTextColor
    }

    HelperWidgets.AbstractButton {
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 5
        anchors.right: parent.right

        style: StudioTheme.Values.viewBarButtonStyle
        buttonIcon: StudioTheme.Constants.help
        tooltip: qsTr("How to use Effect Composer:
1. Click \"+ Add Effect\" to add effect node
2. Adjust the effect nodes properties
3. Change the order of the effects, if you like
4. See the preview
5. Save in the library, if you wish to reuse the effect later") // TODO: revise with doc engineer

        onClicked: Qt.openUrlExternally("https://doc.qt.io/qtdesignstudio/qt-using-effect-maker-effects.html")
    }
}
