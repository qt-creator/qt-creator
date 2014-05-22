/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1

Rectangle {
    border.width: 1
    property bool isCurrentState
    property color gradiantBaseColor
    property string delegateStateName
    property string delegateStateImageSource
    property int delegateStateImageSize

    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.lighter(gradiantBaseColor, 6) }
        GradientStop { position: 0.2; color: Qt.lighter(gradiantBaseColor, 2.5) }
        GradientStop { position: 0.8; color: Qt.lighter(gradiantBaseColor, 1.5) }
        GradientStop { position: 1.0; color: Qt.darker(gradiantBaseColor) }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.currentStateInternalId = nodeId
    }

    ToolButton {
        id: removeStateButton

        anchors.right: parent.right
        anchors.verticalCenter: stateNameField.verticalCenter
        height: 16
        width: 16
        iconSource: "images/darkclose.png"

        onClicked: root.deleteState(nodeId)
    }

    TextField {
        id: stateNameField

        y: 2
        anchors.left: parent.left
        // use the spacing which the image to the delegate rectangle has
        anchors.leftMargin: (delegateWidth - delegateStateImageSize) / 2
        anchors.right: removeStateButton.left

        Component.onCompleted: text = delegateStateName
        onEditingFinished: {
            if (text != delegateStateName)
                statesEditorModel.renameState(nodeId, text)
        }
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
            border.color: "black"
        }
        Image {
            id: stateImage
            anchors.centerIn: parent
            source: delegateStateImageSource
        }
    }

}
