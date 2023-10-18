// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import CollectionDetails 1.0 as CollectionDetails
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: root
    required property var columnType

    property var __modifier : textEditor

    width: itemColumn.width
    height: itemColumn.height

    TableView.onCommit: edit = __modifier.editValue

    Component.onCompleted: {
        if (edit && edit !== "")
            root.__modifier.editValue = edit
    }

    onActiveFocusChanged: {
        if (root.activeFocus)
            root.__modifier.forceActiveFocus()
    }

    Connections {
        id: modifierFocusConnection
        target: root.__modifier
        function onActiveFocusChanged() {
            if (!modifierFocusConnection.target.activeFocus)
                TableView.commit()
        }
    }

    Column {
        id: itemColumn

        StudioControls.TextField {
            id: textEditor

            property alias editValue: textEditor.text

            actionIndicator.visible: false
            translationIndicatorVisible: false
            enabled: visible
            visible: false
        }

        StudioControls.RealSpinBox {
            id: numberEditor

            property alias editValue: numberEditor.realValue

            actionIndicator.visible: false
            enabled: visible
            visible: false

            realFrom: -9e9
            realTo: 9e9
            realStepSize: 1.0
            decimals: 6
        }

        StudioControls.CheckBox {
            id: boolEditor

            property alias editValue: boolEditor.checked

            actionIndicatorVisible: false
            enabled: visible
            visible: false
        }

        HelperWidgets.ColorPicker {
            id: colorEditor

            property alias editValue: colorEditor.color

            width: 100
            enabled: visible
            visible: false
        }
    }

    states: [
        State {
            name: "default"
            when: columnType !== CollectionDetails.DataType.Boolean
                  && columnType !== CollectionDetails.DataType.Color
                  && columnType !== CollectionDetails.DataType.Number

            PropertyChanges {
                target: root
                __modifier: textEditor
            }

            PropertyChanges {
                target: textEditor
                visible: true
                focus: true
            }
        },
        State {
            name: "number"
            when: columnType === CollectionDetails.DataType.Number

            PropertyChanges {
                target: root
                __modifier: numberEditor
            }

            PropertyChanges {
                target: numberEditor
                visible: true
                focus: true
            }
        },
        State {
            name: "bool"
            when: columnType === CollectionDetails.DataType.Boolean

            PropertyChanges {
                target: root
                __modifier: boolEditor
            }

            PropertyChanges {
                target: boolEditor
                visible: true
                focus: true
            }
        },
        State {
            name: "color"
            when: columnType === CollectionDetails.DataType.Color

            PropertyChanges {
                target: root
                __modifier: colorEditor
            }

            PropertyChanges {
                target: colorEditor
                visible: true
                focus: true
            }
        }
    ]
}
