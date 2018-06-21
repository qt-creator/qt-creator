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
import QtQuick.Controls 1.0 as Controls
import QtQuick.Window 2.0
import QtQuick.Controls.Styles 1.1
import "Constants.js" as Constants
import QtQuickDesignerTheme 1.0

Item {
    width: 14
    height: 14

    id: extendedFunctionButton

    property variant backendValue

    property string icon: "image://icons/placeholder"
    property string hoverIcon: "image://icons/submenu"

    property bool active: true

    signal reseted

    function setIcon() {
        if (backendValue == null) {
            extendedFunctionButton.icon = "image://icons/placeholder"
        } else if (backendValue.isBound) {
            if (backendValue.isTranslated) {
                //translations are a special case
                extendedFunctionButton.icon = "image://icons/placeholder"
            } else {
                extendedFunctionButton.icon = "image://icons/expression"
            }
        } else {
            if (backendValue.complexNode != null
                    && backendValue.complexNode.exists) {

            } else {
                extendedFunctionButton.icon = "image://icons/placeholder"
            }
        }
    }

    onBackendValueChanged: {
        setIcon()
    }

    property bool isBoundBackend: backendValue.isBound
    property string backendExpression: backendValue.expression

    onActiveChanged: {
        if (active) {
            setIcon()
            opacity = 1
        } else {
            opacity = 0
        }
    }

    onIsBoundBackendChanged: {
        setIcon()
    }

    onBackendExpressionChanged: {
        setIcon()
    }

    Image {
        width: 14
        height: 14
        source: extendedFunctionButton.icon
        anchors.centerIn: parent
    }

    MouseArea {
        hoverEnabled: true
        anchors.fill: parent
        onHoveredChanged: {
            if (containsMouse)
                icon = hoverIcon
            else
                setIcon()
        }
        onClicked: {
            menuLoader.show()
        }
    }

    Loader {
        id: menuLoader

        active: false

        function show() {
            active = true
            item.popup()
        }

        sourceComponent: Component {
            Controls.Menu {

                id: menu

                onAboutToShow: {
                    exportMenuItem.checked = backendValue.hasPropertyAlias()
                    exportMenuItem.enabled = !backendValue.isAttachedProperty()
                }

                Controls.MenuItem {
                    text: qsTr("Reset")
                    onTriggered: {
                        transaction.start()
                        backendValue.resetValue()
                        backendValue.resetValue()
                        transaction.end()
                        extendedFunctionButton.reseted()
                    }
                }
                Controls.MenuItem {
                    text: qsTr("Set Binding")
                    onTriggered: expressionDialogLoader.show()
                }
                Controls.MenuItem {
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

                Controls.MenuItem {
                    text: qsTr("Insert Keyframe")
                    visible: hasActiveTimeline
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
