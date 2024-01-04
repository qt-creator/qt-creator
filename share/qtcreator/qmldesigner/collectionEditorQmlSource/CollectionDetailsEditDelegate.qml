// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import CollectionDetails 1.0 as CollectionDetails
import StudioControls 1.0 as StudioControls
import StudioHelpers as StudioHelpers
import StudioTheme 1.0 as StudioTheme
import QtQuick.Templates as T

Item {
    id: root

    required property var columnType

    TableView.onCommit: {
        if (editorLoader.changesAccepted && edit !== editorLoader.acceptedValue)
            edit = editorLoader.acceptedValue
    }

    onActiveFocusChanged: {
        if (root.activeFocus && !editorLoader.triggered && editorLoader.item) {
            editorLoader.triggered = true
            editorLoader.item.open()
        }

        // active focus should be checked again, because it might be affected by editorLoader.item
        if (root.activeFocus && editorLoader.editor)
            editorLoader.editor.forceActiveFocus()
    }

    Loader {
        id: editorLoader

        active: true

        property var editor: editorLoader.item ? editorLoader.item.editor : null
        property var editValue: editorLoader.editor ? editorLoader.editor.editValue : null
        property var acceptedValue: null
        property bool changesAccepted: true
        property bool triggered: false

        Connections {
            id: modifierFocusConnection

            target: editorLoader.editor
            enabled: editorLoader.item !== undefined

            function onActiveFocusChanged() {
                if (!modifierFocusConnection.target.activeFocus) {
                    editorLoader.acceptedValue = editorLoader.editValue
                    root.TableView.commit()
                }
            }
        }

        Component {
            id: textEditor

            EditorPopup {
                editor: textField

                StudioControls.TextField {
                    id: textField

                    property alias editValue: textField.text

                    actionIndicator.visible: false
                    translationIndicatorVisible: false

                    onRejected: editorLoader.changesAccepted = false
                }
            }
        }

        Component {
            id: numberEditor

            EditorPopup {

                editor: numberField

                StudioControls.RealSpinBox {
                    id: numberField

                    property alias editValue: numberField.realValue

                    actionIndicator.visible: false
                    realFrom: -9e9
                    realTo: 9e9
                    realStepSize: 1.0
                    decimals: 6
                    trailingZeroes: false
                }
            }
        }

        Component {
            id: boolEditor

            EditorPopup {

                editor: boolField

                StudioControls.CheckBox {
                    id: boolField

                    property alias editValue: boolField.checked

                    actionIndicatorVisible: false
                }
            }
        }
    }

    component EditorPopup: T.Popup {
        id: editorPopup

        required property Item editor

        implicitHeight: contentHeight
        implicitWidth: contentWidth

        focus: true
        visible: false

        Connections {
            target: editorPopup.editor

            function onActiveFocusChanged() {
                if (!editorPopup.editor.activeFocus)
                    editorPopup.close()
                else if (edit)
                    editorPopup.editor.editValue = edit
            }
        }

        Connections {
            target: editorPopup.editor.Keys

            function onEscapePressed() {
                editorLoader.changesAccepted = false
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
                target: editorLoader
                sourceComponent: textEditor
            }
        },
        State {
            name: "number"
            when: columnType === CollectionDetails.DataType.Number

            PropertyChanges {
                target: editorLoader
                sourceComponent: numberEditor
            }
        },
        State {
            name: "bool"
            when: columnType === CollectionDetails.DataType.Boolean

            PropertyChanges {
                target: editorLoader
                sourceComponent: boolEditor
            }
        },
        State {
            name: "color"
            when: columnType === CollectionDetails.DataType.Color

            PropertyChanges {
                target: editorLoader
                sourceComponent: null
            }
        }
    ]
}
