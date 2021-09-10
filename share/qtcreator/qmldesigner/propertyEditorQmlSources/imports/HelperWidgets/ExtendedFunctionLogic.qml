/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

import QtQuick 2.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0

Item {
    id: extendedFunctionButton

    property variant backendValue
    property bool isBoundBackend: {
        if (backendValue !== undefined && backendValue.isBound !== undefined)
            return backendValue.isBound

        return false
    }
    property string backendExpression: {
        if (backendValue !== undefined && backendValue.expression !== undefined)
            return backendValue.expression

        return ""
    }

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
        if (backendValue === undefined) {
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
            if (backendValue.complexNode !== undefined
                    && backendValue.complexNode.exists) {

            } else {
                extendedFunctionButton.glyph = StudioTheme.Constants.actionIcon
            }
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
                    exportMenuItem.checked = backendValue.hasPropertyAlias()
                    exportMenuItem.enabled = !backendValue.isAttachedProperty()
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
                            backendValue.exportPopertyAsAlias()
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
