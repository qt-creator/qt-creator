/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.11
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.3

import HelperWidgets 2.0
import QtQuickDesignerTheme 1.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Dialog {
    id: dialogWindow
    width: 1200
    height: 650
    title: qsTr("Gradient Picker")

    signal saved
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

    standardButtons: Dialog.NoButton

    Rectangle {
        anchors.fill: parent
        anchors.margins: -12
        anchors.bottomMargin: -70
        color: StudioTheme.Values.themeColumnBackground

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 13
            anchors.bottomMargin: 71

            TabView {
                id: presetTabView
                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                Layout.fillHeight: true
                Layout.fillWidth: true

                Tab {
                    title: qsTr("System Presets")
                    anchors.fill: parent

                    GradientPresetTabContent {
                        id: defaultTabContent
                        viewModel: defaultPresetListModel
                        editableName: false
                    }
                }

                Tab {
                    title: qsTr("User Presets")
                    anchors.fill: parent

                    GradientPresetTabContent {
                        id: customTabContent
                        viewModel: customPresetListModel
                        editableName: true
                        onPresetNameChanged: customPresetListModel.changePresetName(id, name)

                        property int deleteId

                        onDeleteButtonClicked: {
                            deleteId = id
                            deleteDialog.open()
                        }

                        MessageDialog {
                            id: deleteDialog
                            visible: false
                            modality: Qt.WindowModal
                            standardButtons: Dialog.No | Dialog.Yes
                            title: qsTr("Delete preset?")
                            text: qsTr("Are you sure you want to delete this preset?")
                            onAccepted: customPresetListModel.deletePreset(customTabContent.deleteId)
                        }
                    }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignBottom | Qt.AlignRight
                Layout.topMargin: 5

                Button { id: buttonClose; text: qsTr("Close"); onClicked: { dialogWindow.reject(); } }
                Button { id: buttonSave; text: qsTr("Save"); onClicked: { dialogWindow.saved(); } }
                Button { id: buttonApply; text: qsTr("Apply"); onClicked: { dialogWindow.apply(); } }
            }
        }
    }
}
