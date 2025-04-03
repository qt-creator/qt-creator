// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets
import QtQuick.Layouts
import QtQuick.Controls as Controls

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Surface")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Flip Grid")
                tooltip: qsTr("Flip horizontal grid")
                Layout.fillWidth: true
            }
            SecondColumnLayout {
                CheckBox {
                    backendValue: backendValues.flipHorizontalGrid
                    Layout.fillWidth: true
                }
            }
            PropertyLabel {
                text: qsTr("Polar Coordinates")
                tooltip: qsTr("Use polar coordinates")
                Layout.fillWidth: true
            }
            SecondColumnLayout {
                CheckBox {
                    id: polarCheckbox
                    backendValue: backendValues.polar
                    Layout.fillWidth: true
                }
            }
            PropertyLabel {
                text: qsTr("Label Offset")
                tooltip: qsTr("Normalized horizontal radial label offset")
                Layout.fillWidth: true
                visible: polarCheckbox.checked
            }
            SecondColumnLayout {
                visible: polarCheckbox.checked
                SpinBox {
                    backendValue: backendValues.radialLabelOffset
                    minimumValue: 0.0
                    maximumValue: 1.0
                    stepSize: 0.01
                    decimals: 2
                    Layout.fillWidth: true
                }
            }
            PropertyLabel {
                text: qsTr("Selection Mode")
                tooltip: qsTr("Surface point selection mode")
                Layout.fillWidth: true
            }
            SecondColumnLayout {
                id: selectionLayout
                property bool isInModel: backendValue.isInModel;
                property bool isInSubState: backendValue.isInSubState;
                property bool selectionChangedFlag: selectionChanged
                property variant backendValue: backendValues.selectionMode
                property variant valueFromBackend: backendValue.value
                property string enumScope: "Graphs3D.SelectionFlag"
                property string enumSeparator: " | "
                property int checkedCount: 0
                property bool item: false
                property bool row: false
                property bool column: false
                property bool slice: false
                property bool multi: false

                function checkValue(checkedVariable, variableText, expressionBase) {
                    var expressionStr = expressionBase
                    if (checkedVariable) {
                        if (expressionStr !== "") {
                            expressionStr += enumSeparator
                        }
                        expressionStr += enumScope
                        expressionStr += "."
                        expressionStr += variableText
                        checkedCount++
                    }
                    return expressionStr
                }

                function composeSelectionMode() {
                    var expressionStr = ""
                    checkedCount = 0
                    expressionStr = checkValue(item, "Item", expressionStr)
                    expressionStr = checkValue(row, "Row", expressionStr)
                    expressionStr = checkValue(column, "Column", expressionStr)
                    expressionStr = checkValue(slice, "Slice", expressionStr)
                    expressionStr = checkValue(multi, "MultiSeries", expressionStr)

                    if (checkedCount === 0)
                        backendValue.expression = enumScope + ".None"
                    else
                        backendValue.expression = expressionStr
                }

                function evaluate() {
                    if (backendValue.value === undefined)
                        return

                    item = (backendValue.expression.indexOf("Item") !== -1)
                    row = (backendValue.expression.indexOf("Row") !== -1)
                    column = (backendValue.expression.indexOf("Column") !== -1)
                    slice = (backendValue.expression.indexOf("Slice") !== -1)
                    multi = (backendValue.expression.indexOf("MultiSeries") !== -1)

                    itemBox.checked = item
                    rowBox.checked = row
                    columnBox.checked = column
                    sliceBox.checked = slice
                    multiSeriesBox.checked = multi
                }

                onSelectionChangedFlagChanged: evaluate()

                onIsInModelChanged: evaluate()

                onIsInSubStateChanged: evaluate()

                onBackendValueChanged: evaluate()

                onValueFromBackendChanged: evaluate()

                ColumnLayout {
                    anchors.fill: parent

                    Controls.CheckBox {
                        id: itemBox
                        text: "Item"
                        Layout.fillWidth: true
                        onClicked: {
                            selectionLayout.item = checked
                            selectionLayout.composeSelectionMode()
                        }
                    }
                    Controls.CheckBox {
                        id: rowBox
                        text: "Row"
                        Layout.fillWidth: true
                        onClicked: {
                            selectionLayout.row = checked
                            selectionLayout.composeSelectionMode()
                        }
                    }
                    Controls.CheckBox {
                        id: columnBox
                        text: "Column"
                        Layout.fillWidth: true
                        onClicked: {
                            selectionLayout.column = checked
                            selectionLayout.composeSelectionMode()
                        }
                    }
                    Controls.CheckBox {
                        id: sliceBox
                        text: "Slice"
                        Layout.fillWidth: true
                        onClicked: {
                            selectionLayout.slice = checked
                            selectionLayout.composeSelectionMode()
                        }
                    }
                    Controls.CheckBox {
                        id: multiSeriesBox
                        text: "MultiSeries"
                        Layout.fillWidth: true
                        onClicked: {
                            selectionLayout.multi = checked
                            selectionLayout.composeSelectionMode()
                        }
                    }
                }
            }
        }
    }

    GraphsSection {}

    GraphsCameraSection {}
}
