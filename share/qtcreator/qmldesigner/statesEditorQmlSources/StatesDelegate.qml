/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: myRoot

    color: baseColor

    property bool isBaseState
    property bool isCurrentState
    property color baseColor
    property string delegateStateName
    property string delegateStateImageSource
    property int delegateStateImageSize
    property bool delegateHasWhenCondition
    property string delegateWhenConditionString
    property bool hasAnnotation: checkAnnotation()
    property int topAreaHeight
    property int bottomAreaHeight
    property int stateMargin
    property int previewMargin
    property int columnSpacing

    readonly property bool isDefaultState: isDefault

    property int closeButtonMargin: 6
    property int textFieldMargin: 4
    property int highlightBorderWidth: 2

    signal delegateInteraction

    function autoComplete(text, pos, explicitComplete, filter) {
        var stringList = statesEditorModel.autoComplete(text, pos, explicitComplete)
        return stringList
    }

    function checkAnnotation() {
        return statesEditorModel.hasAnnotation(internalNodeId)
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onClicked: {
            focus = true
            root.currentStateInternalId = internalNodeId
            contextMenu.dismiss() // close potentially open context menu
            myRoot.delegateInteraction()
        }
    }

    StudioControls.AbstractButton {
        id: removeStateButton

        buttonIcon: StudioTheme.Constants.closeCross

        anchors.right: parent.right
        anchors.rightMargin: myRoot.closeButtonMargin
        anchors.top: parent.top
        anchors.topMargin: myRoot.closeButtonMargin

        visible: !isBaseState && isCurrentState

        onClicked: {
            myRoot.delegateInteraction()
            if (isDefaultState)
                statesEditorModel.resetDefaultState()

            root.deleteState(internalNodeId)
        }
    }

    StudioControls.Menu {
        id: contextMenu

        StudioControls.MenuItem {
            enabled: !isBaseState
            text: qsTr("Set when Condition")
            onTriggered: {
                bindingEditor.showWidget()
                bindingEditor.text = delegateWhenConditionString
                bindingEditor.prepareBindings()
                bindingEditor.updateWindowName()
            }
        }

        StudioControls.MenuItem {
            enabled: !isBaseState && delegateHasWhenCondition
            text: qsTr("Reset when Condition")
            onTriggered: {
               statesEditorModel.resetWhenCondition(internalNodeId)
            }
        }

        StudioControls.MenuItem {
            enabled: !isBaseState && !isDefaultState
            text: qsTr("Set as Default")
            onTriggered: {
                statesEditorModel.setStateAsDefault(internalNodeId)
            }
        }

        StudioControls.MenuItem {
            enabled: (!isBaseState && isDefaultState) || (isBaseState && modelHasDefaultState)
            text: qsTr("Reset Default")
            onTriggered: {
                statesEditorModel.resetDefaultState()
            }
        }

        StudioControls.MenuItem {
            enabled: !isBaseState
            text: (hasAnnotation ? qsTr("Edit Annotation")
                                 : qsTr("Add Annotation"))
            onTriggered: {
                statesEditorModel.setAnnotation(internalNodeId)
                hasAnnotation = checkAnnotation()
            }
        }

        StudioControls.MenuItem {
            enabled: !isBaseState && hasAnnotation
            text: qsTr("Remove Annotation")
            onTriggered: {
                statesEditorModel.removeAnnotation(internalNodeId)
                hasAnnotation = checkAnnotation()
            }
        }

        onClosed: {
            stateNameField.actionIndicator.forceVisible = false
        }

        onOpened: {
            hasAnnotation = checkAnnotation()
            myRoot.delegateInteraction()
        }
    }


    Rectangle {
        anchors.margins: (isDefaultState || (isBaseState && !modelHasDefaultState)) ? -myRoot.highlightBorderWidth : 0
        anchors.fill: column
        color: StudioTheme.Values.themeStateSeparator
        border.color: StudioTheme.Values.themeStateDefaultHighlight
        border.width: (isDefaultState || (isBaseState && !modelHasDefaultState)) ? myRoot.highlightBorderWidth : 0
    }


    Column {
        id: column

        anchors.margins: myRoot.stateMargin
        anchors.fill: parent
        spacing: expanded ? myRoot.columnSpacing : 0

        Rectangle {

            width: myRoot.width - 2 * myRoot.stateMargin
            height: myRoot.topAreaHeight

            color: StudioTheme.Values.themeStateBackground

            StudioControls.TextField {
                id: stateNameField

                property string oldValue

                width: StudioTheme.Values.height * 5.5

                anchors.top: parent.top
                anchors.topMargin: myRoot.textFieldMargin
                anchors.left: parent.left
                anchors.leftMargin: myRoot.textFieldMargin

                translationIndicatorVisible: false
                readOnly: isBaseState

                actionIndicator.icon.text: delegateHasWhenCondition
                                      ? StudioTheme.Constants.actionIconBinding
                                      : StudioTheme.Constants.actionIcon


                actionIndicator.onClicked: {
                    stateNameField.actionIndicator.forceVisible = true
                    contextMenu.popup()
                }

                onEditChanged: {
                    if (contextMenu.open && stateNameField.edit)
                        contextMenu.dismiss()
                }

                onActiveFocusChanged: {
                    if (activeFocus)
                         root.currentStateInternalId = internalNodeId
                }

                onEditingFinished: {
                    if (stateNameField.oldValue === stateNameField.text)
                        return

                    stateNameField.oldValue = stateNameField.text

                    if (stateNameField.text !== myRoot.delegateStateName)
                        statesEditorModel.renameState(internalNodeId, stateNameField.text)
                }

                Component.onCompleted: {
                    text = myRoot.delegateStateName
                }
            }

            Text {
                id: stateDefaultIndicator

                anchors.right: parent.right
                anchors.rightMargin: myRoot.previewMargin
                anchors.verticalCenter: stateNameField.verticalCenter

                color: StudioTheme.Values.themeTextColor
                font.italic: true
                font.pixelSize: StudioTheme.Values.myFontSize
                font.family: StudioTheme.Constants.font

                visible: (isDefaultState || (isBaseState && !modelHasDefaultState))

                text: qsTr("Default")
            }
        }


        Rectangle {
            id: stateImageArea
            width: myRoot.width - 2 * myRoot.stateMargin
            height: myRoot.bottomAreaHeight
            color: StudioTheme.Values.themeStateBackground
            visible: expanded

            Image {
                anchors.fill: stateImageBackground
                source: "images/checkers.png"
                fillMode: Image.Tile
            }

            Rectangle {
                id: stateImageBackground
                anchors.centerIn: parent
                width: Math.round(stateImage.paintedWidth) + 2 * StudioTheme.Values.border
                height: Math.round(stateImage.paintedHeight) + 2 * StudioTheme.Values.border
                color: "transparent"
                border.width: StudioTheme.Values.border
                border.color: StudioTheme.Values.themeStatePreviewOutline
            }

            Image {
                id: stateImage
                anchors.margins: myRoot.previewMargin
                anchors.centerIn: parent
                anchors.fill: parent
                source: delegateStateImageSource
                sourceSize.width: delegateStateImageSize
                sourceSize.height: delegateStateImageSize
                fillMode: Image.PreserveAspectFit
            }
        }
    }

    BindingEditor {
        id: bindingEditor

        property string newWhenCondition

        property Timer timer: Timer {
            id: timer
            running: false
            interval: 50
            repeat: false
            onTriggered: statesEditorModel.setWhenCondition(internalNodeId, bindingEditor.newWhenCondition)
        }

        stateModelNodeProperty: statesEditorModel.stateModelNode()
        stateNameProperty: myRoot.delegateStateName

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
