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

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import HelperWidgets 2.0
import QtQuickDesignerTheme 1.0

Rectangle {
    border.width: 1
    property bool isBaseState
    property bool isCurrentState
    property color baseColor
    property string delegateStateName
    property string delegateStateImageSource
    property int delegateStateImageSize
    property bool delegateHasWhenCondition
    property string delegateWhenConditionString
    readonly property bool isDefaultState: isDefault

    color: baseColor
    border.color: Theme.qmlDesignerBorderColor()

    function autoComplete(text, pos, explicitComplete, filter) {
        var stringList = statesEditorModel.autoComplete(text, pos, explicitComplete)
        return stringList
    }

    MouseArea {
        anchors.fill: parent

        acceptedButtons: Qt.LeftButton
        onClicked: {
            focus = true
            root.currentStateInternalId = internalNodeId
        }
    }

    ToolButton {
        id: removeStateButton

        style: ButtonStyle {
            background: Rectangle {
                color: control.hovered ? Qt.lighter(baseColor, 1.2)  : "transparent"
                Image {
                    source: "image://icons/close"
                    height: 16
                    width: 16
                }
            }
        }


        anchors.right: parent.right
        anchors.rightMargin: 2
        anchors.verticalCenter: stateNameField.verticalCenter
        height: 16
        width: 16
        visible: !isBaseState

        onClicked: {
            if (isDefaultState)
                statesEditorModel.resetDefaultState()

            root.deleteState(internalNodeId)
        }
    }

    Image {
        id: whenButton
        visible: !isBaseState || (isBaseState && modelHasDefaultState)
        width: 14
        height: 14
        x: 4
        y: 6
        source: {
            if (whenMouseArea.containsMouse)
                return "image://icons/submenu"

            return delegateHasWhenCondition ? "image://icons/expression" : "image://icons/placeholder"

        }

        MouseArea {
            id: whenMouseArea
            hoverEnabled: true
            anchors.fill: parent
            onClicked: contextMenu.popup()
        }
        Menu {
            id: contextMenu

            MenuItem {
                visible: !isBaseState
                text: qsTr("Set when Condition")
                onTriggered: {
                    bindingEditor.showWidget()
                    bindingEditor.text = delegateWhenConditionString
                    bindingEditor.prepareBindings()
                }
            }

            MenuItem {
                visible: !isBaseState && delegateHasWhenCondition
                text: qsTr("Reset when Condition")
                onTriggered: {
                   statesEditorModel.resetWhenCondition(internalNodeId)
                }
            }

            MenuItem {
                visible: !isBaseState && !isDefaultState
                text: qsTr("Set as Default")
                onTriggered: {
                    statesEditorModel.setStateAsDefault(internalNodeId)
                }
            }

            MenuItem {
                visible: (!isBaseState && isDefaultState) || (isBaseState && modelHasDefaultState)
                text: qsTr("Reset Default")
                onTriggered: {
                    statesEditorModel.resetDefaultState()
                }
            }
        }
    }

    TextField {
        id: stateNameField
        y: 4
        font.pixelSize: Theme.smallFontPixelSize()
        anchors.left: whenButton.right
        // use the spacing which the image to the delegate rectangle has
        anchors.leftMargin: 4
        anchors.right: removeStateButton.left
        anchors.rightMargin: 4
        style: DesignerTextFieldStyle {
            background: Rectangle {
                implicitWidth: 100
                implicitHeight: font.pixelSize + padding.top + padding.bottom
                color: ((isBaseState && modelHasDefaultState) ? "transparent"
                         : Theme.color(Theme.FancyToolButtonSelectedColor))
                border.color: ((isBaseState && !modelHasDefaultState) || isDefaultState) ? "#ffd700"
                                : (isBaseState && modelHasDefaultState) ? "transparent"
                                : Theme.qmlDesignerBackgroundColorDarker()
            }
        }
        readOnly: isBaseState

        onActiveFocusChanged: {
            if (activeFocus)
                 root.currentStateInternalId = internalNodeId
        }

        Component.onCompleted: {
            text = delegateStateName
        }

        property string oldValue

        onEditingFinished: {
            if (stateNameField.oldValue === stateNameField.text)
                return

            stateNameField.oldValue = stateNameField.text

            if (stateNameField.text != delegateStateName)
                statesEditorModel.renameState(internalNodeId, stateNameField.text)
        }
    }

    Item {
        id: stateImageArea
        anchors.topMargin: 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: stateNameField.bottom

        height: delegateStateImageSize + 2
        width: delegateStateImageSize + 2

        visible: expanded
        Rectangle {
            anchors.margins: -1
            anchors.fill: stateImage
            border.width: 1
            border.color: Theme.qmlDesignerBackgroundColorDarker()
        }
        Image {
            id: stateImage
            anchors.centerIn: parent
            source: delegateStateImageSource
            sourceSize.width: delegateStateImageSize
            sourceSize.height: delegateStateImageSize
        }
    }

    Text {
        id: stateDefaultIndicator
        anchors.left: whenButton.left
        anchors.leftMargin: 0
        anchors.right: removeStateButton.left
        anchors.rightMargin: 4
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4

        color: Theme.color(Theme.PanelTextColorLight)
        font.italic: true
        font.pixelSize: Theme.smallFontPixelSize()

        visible: expanded && (isDefaultState || (isBaseState && !modelHasDefaultState))

        text: ("* " + qsTr("Default"))
    }

    BindingEditor {
        property string newWhenCondition

        property Timer timer: Timer {
            id: timer
            running: false
            interval: 50
            repeat: false
            onTriggered: statesEditorModel.setWhenCondition(internalNodeId, bindingEditor.newWhenCondition)
        }

        id: bindingEditor

        stateModelNodeProperty: statesEditorModel.stateModelNode()

        onRejected: {
            hideWidget()
        }
        onAccepted: {
            bindingEditor.newWhenCondition = bindingEditor.text.trim()
            timer.start()
            hideWidget()
        }
    }

}
