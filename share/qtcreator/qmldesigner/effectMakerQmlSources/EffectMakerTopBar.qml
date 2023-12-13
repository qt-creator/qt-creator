// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuickDesignerTheme
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme 1.0 as StudioTheme
import EffectMakerBackend

Rectangle {
    id: root

    width: parent.width
    height: StudioTheme.Values.toolbarHeight
    color: StudioTheme.Values.themeToolbarBackground

    signal addClicked
    signal saveClicked
    signal saveAsClicked

    Row {
        spacing: 5
        anchors.verticalCenter: parent.verticalCenter

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.add_medium
            tooltip: qsTr("Add new composition")

            onClicked: root.addClicked()
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.save_medium
            tooltip: qsTr("Save current composition")
            enabled: EffectMakerBackend.effectMakerModel.hasUnsavedChanges
                  || EffectMakerBackend.effectMakerModel.currentComposition === ""

            onClicked: root.saveClicked()
        }

        HelperWidgets.AbstractButton {
            style: StudioTheme.Values.viewBarButtonStyle
            buttonIcon: StudioTheme.Constants.saveAs_medium
            tooltip: qsTr("Save current composition with a new name")
            enabled: !EffectMakerBackend.effectMakerModel.isEmpty

            onClicked: root.saveAsClicked()
        }
    }


    Text {
        readonly property string compName: EffectMakerBackend.effectMakerModel.currentComposition

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
        tooltip: qsTr("How to use Effect Maker:
1. Click \"+ Add Effect\" to add effect node
2. Adjust the effect nodes properties
3. Change the order of the effects, if you like
4. See the preview
5. Save in the library, if you wish to reuse the effect later") // TODO: revise with doc engineer
    }
}
