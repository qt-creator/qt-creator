// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import CollectionDetails 1.0 as CollectionDetails
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioHelpers as StudioHelpers
import StudioTheme 1.0 as StudioTheme
import QtQuick.Templates as T

Item {
    id: root
    required property var columnType

    property var __modifier : textEditor
    property bool __changesAccepted: true

    TableView.onCommit: {
        if (root.__changesAccepted)
            edit = __modifier.editor.editValue
    }

    Component.onCompleted: {
        __changesAccepted = true
        if (edit && edit !== "")
            root.__modifier.editor.editValue = edit
    }

    onActiveFocusChanged: {
        if (root.activeFocus)
            root.__modifier.editor.forceActiveFocus()
    }

    Connections {
        id: modifierFocusConnection

        target: root.__modifier.editor

        function onActiveFocusChanged() {
            if (!modifierFocusConnection.target.activeFocus)
                root.TableView.commit()
        }
    }

    EditorPopup {
        id: textEditor

        editor: textField

        StudioControls.TextField {
            id: textField

            property alias editValue: textField.text

            actionIndicator.visible: false
            translationIndicatorVisible: false

            onRejected: root.__changesAccepted = false
        }
    }

    EditorPopup {
        id: numberEditor

        editor: numberField

        StudioControls.RealSpinBox {
            id: numberField

            property alias editValue: numberField.realValue

            actionIndicator.visible: false
            realFrom: -9e9
            realTo: 9e9
            realStepSize: 1.0
            decimals: 6
        }
    }

    EditorPopup {
        id: boolEditor

        editor: boolField

        StudioControls.CheckBox {
            id: boolField

            property alias editValue: boolField.checked

            actionIndicatorVisible: false
        }
    }

    EditorPopup {
        id: colorEditor

        editor: colorPicker

        implicitHeight: colorPicker.height + topPadding + bottomPadding
        implicitWidth: colorPicker.width + leftPadding + rightPadding
        padding: 8

        StudioHelpers.ColorBackend {
            id: colorBackend
        }

        StudioControls.ColorEditorPopup {
            id: colorPicker

            property alias editValue: colorBackend.color
            color: colorBackend.color

            width: 200

            Keys.onEnterPressed: colorPicker.focus = false

            onActivateColor: function(color) {
                colorBackend.activateColor(color)
            }
        }

        background: Rectangle {
            color: StudioTheme.Values.themeControlBackgroundInteraction
            border.color: StudioTheme.Values.themeInteraction
            border.width: StudioTheme.Values.border
        }
    }

    component EditorPopup: T.Popup {
        id: editorPopup

        required property Item editor

        implicitHeight: contentHeight
        implicitWidth: contentWidth

        enabled: visible
        visible: false

        Connections {
            target: editorPopup.editor

            function onActiveFocusChanged() {
                if (!editorPopup.editor.activeFocus)
                    editorPopup.close()
            }
        }

        Connections {
            target: editorPopup.editor.Keys

            function onEscapePressed() {
                root.__changesAccepted = false
                editorPopup.close()
            }
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
