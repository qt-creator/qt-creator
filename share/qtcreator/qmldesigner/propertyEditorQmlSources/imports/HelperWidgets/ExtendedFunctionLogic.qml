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

import QtQuick 2.1
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import QtQuickDesignerTheme 1.0

Item {
    id: extendedFunctionButton

    property variant backendValue
    property bool isBoundBackend: backendValue.isBound
    property string backendExpression: backendValue.expression

    property string glyph: StudioTheme.Constants.actionIcon
    property string color: StudioTheme.Constants.themeTextColor
    property alias menuLoader: menuLoader

    signal reseted

    function show() {
        menuLoader.show()
    }

    function setIcon() {
        extendedFunctionButton.color = StudioTheme.Values.themeTextColor
        if (backendValue === null) {
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
            if (backendValue.complexNode !== null
                    && backendValue.complexNode.exists) {

            } else {
                extendedFunctionButton.glyph = StudioTheme.Constants.actionIcon
            }
        }
    }

    onBackendValueChanged: {
        setIcon()
    }

    onIsBoundBackendChanged: {
        setIcon()
    }

    onBackendExpressionChanged: {
        setIcon()
    }

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
                }

                StudioControls.MenuItem {
                    text: qsTr("Reset")
                    onTriggered: {
                        transaction.start()
                        backendValue.resetValue()
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
                    onTriggered: {
                        if (checked)
                            backendValue.exportPopertyAsAlias()
                        else
                            backendValue.removeAliasExport()
                    }
                    checkable: true
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

        sourceComponent: Component {
            Item {
                id: expressionDialog
                anchors.fill: parent

                Component.onCompleted: {
                    textField.text = backendValue.expression
                    textField.forceActiveFocus()
                }

                Rectangle {
                    anchors.fill: parent
                    color: Theme.qmlDesignerBackgroundColorDarker()
                    opacity: 0.6
                }

                MouseArea {
                    anchors.fill: parent
                    onDoubleClicked: expressionDialog.visible = false
                }

                Rectangle {
                    x: 4
                    Component.onCompleted: {
                        var pos = itemPane.mapFromItem(
                                    extendedFunctionButton.parent, 0, 0)
                        y = pos.y + 2
                    }

                    width: parent.width - 8
                    height: 260

                    radius: 2
                    color: Theme.qmlDesignerBackgroundColorDarkAlternate()
                    border.color: Theme.qmlDesignerBorderColor()

                    Label {
                        x: 8
                        y: 6
                        font.bold: true
                        text: qsTr("Binding Editor")
                    }
                    ExpressionTextField {
                        id: textField
                        onRejected: expressionDialogLoader.visible = false
                        onAccepted: {
                            backendValue.expression = textField.text.trim()
                            expressionDialogLoader.visible = false
                        }
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        anchors.topMargin: 24
                        anchors.bottomMargin: 32
                    }
                }
            }
        }
    }
}
