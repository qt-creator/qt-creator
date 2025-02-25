// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import ScriptEditorBackend
import ScriptsEditor as ScriptsEditor

Section {
    id: root
    caption: qsTr("Script")

    SectionLayout {
        PropertyLabel {
            text: ""
            tooltip: ""
        }

        SecondColumnLayout {
            Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

            AbstractButton {
                id: editScriptButton
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                width: StudioTheme.Values.singleControlColumnWidth
                buttonIcon: qsTr("Edit script")
                iconFontFamily: StudioTheme.Constants.font.family
                onClicked: dialog.show(root)
            }

            ExpandingSpacer {}
        }
    }

    data: [
        StudioControls.PopupDialog {
            id: dialog

            readonly property real horizontalSpacing: 10
            readonly property real verticalSpacing: 12
            readonly property real columnWidth: (dialog.width - dialog.horizontalSpacing - (2 * dialog.anchorGap)) / 2

            property var backend: ScriptEditorBackend

            titleBar: Row {
                id: titleBarRow
                spacing: 30
                anchors.fill: parent

                Text {
                    color: StudioTheme.Values.themeTextColor
                    text: qsTr("Action")
                    font.pixelSize: StudioTheme.Values.myFontSize
                    anchors.verticalCenter: parent.verticalCenter

                    ToolTipArea {
                        anchors.fill: parent
                        tooltip: qsTr("Sets an action that is associated with the selected <b>ScriptAction</b>.")
                    }
                }

                ScriptsEditor.ActionsComboBox {
                    id: action
                    style: StudioTheme.Values.connectionPopupControlStyle
                    width: 180
                    anchors.verticalCenter: parent.verticalCenter
                    backend: dialog.backend
                }
            }

            ScriptsEditor.ScriptEditorForm {
                id: scriptEditor

                anchors.left: parent.left
                anchors.right: parent.right

                horizontalSpacing: dialog.horizontalSpacing
                verticalSpacing: dialog.verticalSpacing
                columnWidth: dialog.columnWidth
                spacing: dialog.verticalSpacing

                backend: dialog.backend

                currentAction: action.currentValue ?? StatementDelegate.Custom
            }
        },

        Connections {
            target: modelNodeBackend
            function onSelectionChanged() {
                dialog.backend.update()
            }
        },

        TapHandler {
            onTapped: scriptEditor.forceActiveFocus()
        }
    ]
}
