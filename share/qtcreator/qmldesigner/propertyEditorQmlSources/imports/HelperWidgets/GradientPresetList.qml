// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import Qt.labs.platform
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Window {
    id: root
    width: 1200
    height: 650
    title: qsTr("Gradient Picker")
    flags: Qt.Dialog

    signal saved
    signal applied
    signal accepted
    property alias gradientData: gradientPickerData

    QtObject {
        id: gradientPickerData
        property var stops
        property var colors
        property int stopsCount
        property int presetID
        property int presetType // default(0) or custom(1)
        property Item selectedItem
    }

    function addGradient(stopsPositions, stopsColors, stopsCount) {
        customPresetListModel.addGradient(stopsPositions, stopsColors, stopsCount)
    }

    function updatePresets() {
        customPresetListModel.readPresets()
    }

    GradientPresetDefaultListModel { id: defaultPresetListModel }
    GradientPresetCustomListModel { id: customPresetListModel }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -12
        anchors.bottomMargin: -70
        color: StudioTheme.Values.themePanelBackground

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 13
            anchors.bottomMargin: 71

            StudioControls.TabBar {
                id: presetTabBar

                Layout.fillWidth: true

                StudioControls.TabButton { text: qsTr("System Presets") }
                StudioControls.TabButton { text: qsTr("User Presets") }
            }

            StackLayout {
                width: parent.width
                currentIndex: presetTabBar.currentIndex

                GradientPresetTabContent {
                    id: defaultTabContent
                    viewModel: defaultPresetListModel
                    editableName: false
                }

                GradientPresetTabContent {
                    id: customTabContent
                    viewModel: customPresetListModel
                    editableName: true
                    onPresetNameChanged: function(id, name) {
                        customPresetListModel.changePresetName(id, name)
                    }

                    property int deleteId

                    onDeleteButtonClicked: function(id) {
                        deleteId = id
                        deleteDialog.open()
                    }

                    MessageDialog {
                        id: deleteDialog
                        visible: false
                        modality: Qt.WindowModal
                        buttons: StandardButton.No | StandardButton.Yes
                        title: qsTr("Delete preset?")
                        text: qsTr("Are you sure you want to delete this preset?")
                        onAccepted: customPresetListModel.deletePreset(customTabContent.deleteId)
                    }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignBottom | Qt.AlignRight
                Layout.topMargin: 5

                Button { id: buttonClose; text: qsTr("Close"); onClicked: { root.hide(); } }
                Button { id: buttonSave; text: qsTr("Save"); onClicked: { root.saved(); } }
                Button { id: buttonApply; text: qsTr("Apply"); onClicked: { root.applied(); root.hide(); } }
            }
        }
    }
}
