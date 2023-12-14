// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    id: root
    caption: qsTr("Insight")

    anchors.left: parent.left
    anchors.right: parent.right

    property string defaultItem: qsTr("[None]")

    function addDefaultItem(arr)
    {
        var copy = arr.slice()
        copy.unshift(root.defaultItem)
        return copy
    }

    SectionLayout {
/*
        PropertyLabel { text: qsTr("Category") }

        SecondColumnLayout {
            Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

            StudioControls.ComboBox {
                id: comboBox
                property var backendValue: backendValues.InsightCategory_category
                property var valueFromBackend: comboBox.backendValue === undefined ? 0 : comboBox.backendValue.value

                onValueFromBackendChanged: comboBox.invalidate()
                onModelChanged: comboBox.invalidate()

                actionIndicatorVisible: false
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                width: implicitWidth
                model: root.addDefaultItem(insightCategories)
                editable: false

                onCompressedActivated: function(index, reason) {
                    if (comboBox.backendValue === undefined)
                        return

                    verifyInsightImport()

                    if (index === 0)
                        comboBox.backendValue.resetValue()
                    else
                        comboBox.backendValue.value = comboBox.currentText
                }

                Connections {
                    target: modelNodeBackend
                    function onSelectionToBeChanged() {
                        comboBox.popup.close()
                    }
                }

                function invalidate() {
                    var index = comboBox.find(comboBox.valueFromBackend)
                    if (index < 0) {
                        if (comboBox.valueFromBackend === "") {
                            comboBox.currentIndex = 0
                            comboBox.labelColor = StudioTheme.Values.themeTextColor
                        } else {
                            comboBox.currentIndex = index
                            comboBox.editText = comboBox.valueFromBackend
                            comboBox.labelColor = StudioTheme.Values.themeError
                        }
                    } else {
                        if (index !== comboBox.currentIndex)
                            comboBox.currentIndex = index

                        comboBox.labelColor = StudioTheme.Values.themeTextColor
                    }
                }
                Component.onCompleted: comboBox.invalidate()
            }

            ExpandingSpacer {}
        }
*/
        PropertyLabel {
            text: qsTr("Object name")
            tooltip: qsTr("Sets the object name of the component.")
        }

        SecondColumnLayout {
            LineEdit {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: StudioTheme.Values.singleControlColumnWidth
                backendValue: backendValues.objectName
                text: backendValues.objectName.value
                showTranslateCheckBox: false
                enabled: !modelNodeBackend.multiSelection
            }

            ExpandingSpacer {}
        }
    }
}
