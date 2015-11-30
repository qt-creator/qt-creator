/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1

Rectangle {
    border.width: 1
    property bool isBaseState
    property bool isCurrentState
    property color baseColor
    property string delegateStateName
    property string delegateStateImageSource
    property int delegateStateImageSize

    color: baseColor

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
                radius: 2
            }
        }


        anchors.right: parent.right
        anchors.rightMargin: 2
        anchors.verticalCenter: stateNameField.verticalCenter
        height: 16
        width: 16
        iconSource: "images/darkclose.png"
        visible: !isBaseState

        onClicked: root.deleteState(internalNodeId)
    }

    TextField {
        id: stateNameField
        y: 4
        font.pixelSize: 11
        anchors.left: parent.left
        // use the spacing which the image to the delegate rectangle has
        anchors.leftMargin: 4
        anchors.right: removeStateButton.left
        anchors.rightMargin: 4
        style: DesignerTextFieldStyle {}
        readOnly: isBaseState

        onActiveFocusChanged: {
            if (activeFocus)
                 root.currentStateInternalId = internalNodeId
        }

        Component.onCompleted: {
            text = delegateStateName
            if (isBaseState)
                __panel.visible = false
        }

        onEditingFinished: {
            if (text != delegateStateName)
                statesEditorModel.renameState(internalNodeId, text)
        }

        // as we change the background we need to change the text
        // color to see some text
        textColor: "#FFFFFF"
    }

    Item {
        id: stateImageArea
        anchors.topMargin: 1
        anchors.left: stateNameField.left
        anchors.top: stateNameField.bottom

        height: delegateStateImageSize + 2
        width: delegateStateImageSize + 2
        Rectangle {
            anchors.margins: -1
            anchors.fill: stateImage
            border.width: 1
            border.color: "#000000"
        }
        Image {
            id: stateImage
            anchors.centerIn: parent
            source: delegateStateImageSource
        }
    }

}
