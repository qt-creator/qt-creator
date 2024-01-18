// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Rectangle {
    id: root

    color: Theme.qmlDesignerBackgroundColorDarkAlternate()

    component Title : Text {
        color: StudioTheme.Values.themeTextColor
        font {
            pixelSize: StudioTheme.Values.baseFontSize
            bold: true
        }
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        Layout.fillWidth: true
        Layout.preferredHeight: 30
    }

    StudioControls.Dialog {
        id: renameDialog

        property string name: ""

        title: qsTr("Failed to rename \"" + renameDialog.name + "\".")
        standardButtons: Dialog.Close
        x: Math.round((root.width - renameDialog.width) / 2)
        y: Math.round((root.height - renameDialog.height) / 2)
        closePolicy: Popup.NoAutoClose

        Text {
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.baseFontSize
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            text: qsTr("Can't rename category, name already exists.")
            anchors.fill: parent
        }
    }

    HelperWidgets.ScrollView {
        clip: true
        anchors.fill: parent

        Column {
            id: column
            width: root.width

            HelperWidgets.Section {
                id: trackingSection
                caption: qsTr("Tracking")
                anchors.left: parent.left
                anchors.right: parent.right

                property int rowSpacing: 16

                ColumnLayout {
                    spacing: StudioTheme.Values.sectionRowSpacing
                    width: parent.width - StudioTheme.Values.sectionLayoutRightPadding

                    Text {
                        wrapMode: Text.WordWrap
                        color: StudioTheme.Values.themeTextColor
                        font.pixelSize: StudioTheme.Values.baseFontSize
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        text: qsTr("With tracking turned on, the application tracks user interactions for all component types in the selected predefined categories.")
                        Layout.fillWidth: true
                    }

                    Title { text: qsTr("Tracking") }

                    Row {
                        spacing: trackingSection.rowSpacing

                        StudioControls.Switch {
                            id: trackingSwitch
                            actionIndicatorVisible: false
                            checked: insightModel.enabled

                            onToggled: insightModel.setEnabled(trackingSwitch.checked)
                        }

                        Text {
                            color: StudioTheme.Values.themeTextColor
                            text: trackingSwitch.checked ? qsTr("Enabled") : qsTr("Disabled")
                            font.pixelSize: StudioTheme.Values.baseFontSize
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            height: trackingSwitch.height
                        }
                    }

                    Title { text: qsTr("Token") }

                    Text {
                        wrapMode: Text.WordWrap
                        color: StudioTheme.Values.themeTextColor
                        font.pixelSize: StudioTheme.Values.baseFontSize
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        text: qsTr("Tokens are used to match the data your application sends to your Qt Insight Organization.")
                        Layout.fillWidth: true
                    }

                    StudioControls.TextField {
                        id: tokenField

                        property string previousToken

                        enabled: trackingSwitch.checked
                        actionIndicatorVisible: false
                        translationIndicatorVisible: false
                        placeholderText: qsTr("Add token here")
                        text: insightModel.token
                        Layout.fillWidth: true

                        onEditingFinished: {
                            if (tokenField.previousToken === tokenField.text)
                                return

                            tokenField.previousToken = tokenField.text
                            insightModel.setToken(tokenField.text)
                        }

                        Component.onCompleted: tokenField.previousToken = tokenField.text
                    }

                    Title { text: qsTr("Send Cadence") }

                    Row {
                        spacing: trackingSection.rowSpacing

                        StudioControls.SpinBox {
                            id: cadenceSpinBox
                            enabled: trackingSwitch.checked
                            actionIndicatorVisible: false
                            value: insightModel.minutes
                            onValueModified: insightModel.setMinutes(cadenceSpinBox.value)

                            __devicePixelRatio: insightModel.devicePixelRatio()

                            onDragStarted: insightModel.hideCursor()
                            onDragEnded: insightModel.restoreCursor()
                            onDragging: insightModel.holdCursorInPlace()
                        }

                        Text {
                            color: trackingSwitch.checked ? StudioTheme.Values.themeTextColor
                                                          : StudioTheme.Values.themeTextColorDisabled
                            text: qsTr("minutes")
                            font.pixelSize: StudioTheme.Values.baseFontSize
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            height: cadenceSpinBox.height
                        }
                    }
                }
            }
/*
            HelperWidgets.Section {
                id: predefinedSection
                caption: qsTr("Predefined Categories")
                anchors.left: parent.left
                anchors.right: parent.right

                ColumnLayout {
                    enabled: trackingSwitch.checked
                    spacing: StudioTheme.Values.sectionRowSpacing
                    width: parent.width - StudioTheme.Values.sectionLayoutRightPadding

                    Text {
                        wrapMode: Text.WordWrap
                        color: StudioTheme.Values.themeTextColor
                        font.pixelSize: StudioTheme.Values.baseFontSize
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        text: qsTr("Select the categories to track")
                        Layout.fillWidth: true
                    }

                    Column {
                        id: predefinedTable

                        property int centerColumnWidth: predefinedTable.width
                                                        - selectAll.width
                                                        - 2 * predefinedTable.rowSpacing

                        readonly property int columnSpacing: 2
                        readonly property int rowSpacing: 8

                        Layout.fillWidth: true
                        spacing: predefinedTable.columnSpacing

                        Row {
                            spacing: predefinedTable.rowSpacing

                            StudioControls.CheckBox {
                                id: selectAll
                                actionIndicatorVisible: false
                                tristate: true
                                checkState: insightModel.predefinedSelectState
                                onClicked: insightModel.selectAllPredefined()
                            }

                            Text {
                                color: trackingSwitch.checked ? StudioTheme.Values.themeTextColor
                                                              : StudioTheme.Values.themeTextColorDisabled
                                font {
                                    pixelSize: StudioTheme.Values.baseFontSize
                                    bold: true
                                }
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                text: qsTr("Select all")
                                width: predefinedTable.centerColumnWidth
                                height: StudioTheme.Values.height
                            }
                        }

                        Item { width: 1; height: 4 }

                        Repeater {
                            model: insightModel
                            delegate: Row {
                                id: predefinedDelegate

                                required property int index

                                required property string categoryName
                                required property string categoryColor
                                required property string categoryType
                                required property bool categoryActive

                                visible: predefinedDelegate.categoryType === "predefined"
                                spacing: predefinedTable.rowSpacing

                                StudioControls.CheckBox {
                                    id: predefinedCheckBox
                                    actionIndicatorVisible: false
                                    checked: predefinedDelegate.categoryActive
                                    onToggled: insightModel.setCategoryActive(predefinedDelegate.index,
                                                                                predefinedCheckBox.checked)
                                }

                                Text {
                                    color: trackingSwitch.checked ? StudioTheme.Values.themeTextColor
                                                                  : StudioTheme.Values.themeTextColorDisabled
                                    font.pixelSize: StudioTheme.Values.baseFontSize
                                    horizontalAlignment: Text.AlignLeft
                                    verticalAlignment: Text.AlignVCenter
                                    text: predefinedDelegate.categoryName
                                    width: predefinedTable.centerColumnWidth
                                    height: StudioTheme.Values.height
                                }
                            }
                        }
                    }
                }
            }

            HelperWidgets.Section {
                id: customSection
                caption: qsTr("Custom Categories")
                anchors.left: parent.left
                anchors.right: parent.right

                ColumnLayout {
                    enabled: trackingSwitch.checked
                    spacing: StudioTheme.Values.sectionRowSpacing
                    width: parent.width - StudioTheme.Values.sectionLayoutRightPadding

                    Text {
                        wrapMode: Text.WordWrap
                        color: StudioTheme.Values.themeTextColor
                        font.pixelSize: StudioTheme.Values.baseFontSize
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        text: qsTr("Manage your own categories")
                        Layout.fillWidth: true
                    }

                    Column {
                        id: customTable

                        property int centerColumnWidth: predefinedTable.centerColumnWidth
                                                        - StudioTheme.Values.height
                                                        - predefinedTable.rowSpacing

                        Layout.fillWidth: true
                        spacing: predefinedTable.columnSpacing

                        Row {
                            spacing: predefinedTable.rowSpacing

                            StudioControls.CheckBox {
                                id: selectAllCustom
                                actionIndicatorVisible: false
                                tristate: true
                                checkState: insightModel.customSelectState
                                onClicked: insightModel.selectAllCustom()
                            }

                            Text {
                                color: trackingSwitch.checked ? StudioTheme.Values.themeTextColor
                                                              : StudioTheme.Values.themeTextColorDisabled
                                font {
                                    pixelSize: StudioTheme.Values.baseFontSize
                                    bold: true
                                }
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                text: qsTr("Select all")
                                width: predefinedTable.centerColumnWidth
                                height: StudioTheme.Values.height
                            }
                        }

                        Item { width: 1; height: 4 }

                        Repeater {
                            id: customRepeater
                            model: insightModel

                            delegate: Row {
                                id: customDelegate

                                required property int index

                                required property string categoryName
                                required property string categoryColor
                                required property string categoryType
                                required property bool categoryActive

                                visible: customDelegate.categoryType === "custom"
                                spacing: predefinedTable.rowSpacing

                                StudioControls.CheckBox {
                                    id: customCheckBox
                                    actionIndicatorVisible: false
                                    checked: customDelegate.categoryActive
                                    onToggled: insightModel.setCategoryActive(customDelegate.index,
                                                                                customCheckBox.checked)
                                }

                                StudioControls.TextField {
                                    id: categoryName

                                    property string previousName

                                    actionIndicatorVisible: false
                                    translationIndicatorVisible: false
                                    text: customDelegate.categoryName
                                    width: customTable.centerColumnWidth
                                    height: StudioTheme.Values.height
                                    onEditingFinished: {
                                        if (categoryName.text === categoryName.previousName)
                                            return

                                        var result = insightModel.renameCategory(customDelegate.index,
                                                                                 categoryName.text)

                                        if (!result) {
                                            renameDialog.name = categoryName.text
                                            categoryName.text = categoryName.previousName
                                            renameDialog.open()
                                        }
                                    }

                                    Component.onCompleted: categoryName.previousName = categoryName.text
                                }

                                StudioControls.AbstractButton {
                                    id: removeButton
                                    buttonIcon: StudioTheme.Constants.closeCross
                                    onClicked: insightModel.removeCateogry(customDelegate.index)
                                }
                            }
                        }

                        Item { width: 1; height: 4 }

                        Row {
                            spacing: StudioTheme.Values.checkBoxSpacing

                            StudioControls.AbstractButton {
                                id: plusButton
                                buttonIcon: StudioTheme.Constants.plus
                                onClicked: insightModel.addCategory()
                            }

                            Text {
                                color: trackingSwitch.checked ? StudioTheme.Values.themeTextColor
                                                              : StudioTheme.Values.themeTextColorDisabled
                                font.pixelSize: StudioTheme.Values.baseFontSize
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                text: qsTr("Add new Category")
                                width: predefinedTable.centerColumnWidth
                                height: StudioTheme.Values.height
                            }
                        }
                    }
                }
            }
*/
        }
    }
}
