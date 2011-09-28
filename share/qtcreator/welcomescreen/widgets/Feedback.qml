/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

import QtQuick 1.1
import qtcomponents 1.0 as Components

Rectangle {
    property alias searchVisible: lineEdit.visible
    id: inner_background
    height: 42
    color: "#f3f3f3"

    Rectangle { color: "#444"; width: parent.width; height: 1; anchors.top: parent.top; anchors.left: parent.left; anchors.topMargin: -1 }

    property bool _hasDesktopTheme: welcomeMode.platform() === "linux"

    Rectangle {
        color: "black"
        height: 1
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

    Item {
        id: item
        states: [
            State {
                name: "invisble"
                when: item.width < 400
                PropertyChanges {
                    target: item
                    opacity: 0
                }
            }
        ]

        transitions: [
            Transition {
                PropertyAnimation {
                    property: "opacity"
                    duration: 50
                }
            }
        ]

        anchors.fill: parent

        // whitelist

        Button {
            id: feedbackButton
            x: 510
            y: 2
            text: qsTr("Feedback")
            anchors.rightMargin: 8
            anchors.right: parent.right
            anchors.verticalCenterOffset: 0
            height: 28
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 5
            onClicked: welcomeMode.sendFeedback()
            tooltip: qsTr("Help us make Qt Creator even better")
        }



        LineEdit {
            id: lineEdit
            height: 26
            anchors.rightMargin: 8
            anchors.right: feedbackButton.left
            anchors.leftMargin: 8
            placeholderText: qsTr("Search in Tutorials, Examples and Demos")
            focus: true
            anchors.left: createProjectButton.right
            anchors.verticalCenter: parent.verticalCenter
            onTextChanged: examplesModel.parseSearchString(text)
        }

        Button {
            id: openProjectButton
            y: 2
            height: 28
            text: qsTr("Open Project...")
            anchors.left: parent.left
            anchors.leftMargin: 8
            focus: false
            iconSource: _hasDesktopTheme ? "image://desktoptheme/document-open" : ""
            onClicked: welcomeMode.openProject();
            anchors.verticalCenter: parent.verticalCenter
        }

        Button {
            id: createProjectButton
            y: 2
            text: qsTr("Create Project...")
            anchors.left: openProjectButton.right
            anchors.leftMargin: 8
            anchors.verticalCenterOffset: 0
            iconSource: _hasDesktopTheme ? "image://desktoptheme/document-new" : ""
            onClicked: welcomeMode.newProject();
            height: 28
            anchors.margins: 5
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
