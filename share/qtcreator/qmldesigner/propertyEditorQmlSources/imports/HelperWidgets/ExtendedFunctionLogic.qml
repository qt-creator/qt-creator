// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import HelperWidgets

Item {
    id: extendedFunctionButton

    property variant backendValue
    property bool isBoundBackend: backendValue?.isBound ?? false
    property string backendExpression: backendValue?.expression ?? ""

    property string glyph: StudioTheme.Constants.actionIcon
    property string color: StudioTheme.Values.themeTextColor
    property alias menuLoader: menuLoader

    signal reseted

    property bool menuVisible: false

    function show() {
        menuLoader.show()
    }

    function setIcon() {
        extendedFunctionButton.color = StudioTheme.Values.themeTextColor
        if (!backendValue) {
            extendedFunctionButton.glyph = StudioTheme.Constants.actionIcon
        } else if (backendValue.isBound) {
            if (backendValue.isTranslated) {
                // translations are a special case
                extendedFunctionButton.glyph = StudioTheme.Constants.actionIcon
            } else {
                extendedFunctionButton.glyph = StudioTheme.Constants.actionIconBinding
                extendedFunctionButton.color = StudioTheme.Values.themeInteraction
            }
        } else {
            if (!backendValue.complexNode || !backendValue.complexNode.exists)
                extendedFunctionButton.glyph = StudioTheme.Constants.actionIcon
        }
    }

    onBackendValueChanged: setIcon()
    onIsBoundBackendChanged: setIcon()
    onBackendExpressionChanged: setIcon()

    Loader {
        id: menuLoader

        active: false

        function show() {
            active = true
            item.popup()
        }

        sourceComponent: Component {
            StudioControls.Menu {
                id: menu

                onAboutToShow: {
                    exportMenuItem.checked = backendValue?.hasPropertyAlias() ?? false
                    exportMenuItem.enabled = !(backendValue?.isAttachedProperty() ?? false)
                    extendedFunctionButton.menuVisible = true
                }
                onAboutToHide: extendedFunctionButton.menuVisible = false

                Connections {
                    target: modelNodeBackend
                    function onSelectionChanged() { menu.close() }
                }

                StudioControls.MenuItem {
                    text: qsTr("Reset")
                    onTriggered: {
                        transaction.start()
                        backendValue.resetValue()
                        transaction.end()
                        extendedFunctionButton.reseted()
                    }
                }

                StudioControls.MenuItem {
                    text: qsTr("Set Binding")
                    onTriggered: expressionDialogLoader.show()
                }

                StudioControls.MenuItem {
                    id: exportMenuItem
                    text: qsTr("Export Property as Alias")
                    checkable: true
                    onTriggered: {
                        if (checked)
                            backendValue.exportPropertyAsAlias()
                        else
                            backendValue.removeAliasExport()
                    }
                }

                StudioControls.MenuItem {
                    text: qsTr("Insert Keyframe")
                    enabled: hasActiveTimeline
                    onTriggered: insertKeyframe(backendValue.name)
                }
            }
        }
    }

    Loader {
        id: expressionDialogLoader
        parent: itemPane
        anchors.fill: parent
        visible: false
        active: visible

        function show() {
            expressionDialogLoader.visible = true
        }

        sourceComponent: Item {
            id: bindingEditorParent

            Component.onCompleted: {
                bindingEditor.showWidget()
                bindingEditor.text = backendValue.expression
                bindingEditor.prepareBindings()
                bindingEditor.updateWindowName()
            }

            BindingEditor {
                id: bindingEditor

                backendValueProperty: backendValue
                modelNodeBackendProperty: modelNodeBackend

                onRejected: {
                    hideWidget()
                    expressionDialogLoader.visible = false
                }

                onAccepted: {
                    backendValue.expression = bindingEditor.text.trim()
                    hideWidget()
                    expressionDialogLoader.visible = false
                }
            }
        }
    }
}
