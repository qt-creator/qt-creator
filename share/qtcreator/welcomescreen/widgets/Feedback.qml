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

import QtQuick 1.0
import components 1.0 as Components

BorderImage {
    id: inner_background
    height: openProjectButton.height + 10
    source: "qrc:welcome/images/background_center_frame_v2.png"
    border.left: 2
    border.right: 2

    Rectangle { color: "#4D295B7F"; width: parent.width; height: 1; anchors.top: parent.top; anchors.left: parent.left }

    Components.QStyleItem { id: styleItem; visible: false }

    // whitelist
    property bool _hasDesktopTheme: welcomeMode.platform() === "linux"

    Button {
        id: feedbackButton
        text: qsTr("Feedback")
        iconSource: "qrc:welcome/images/feedback_arrow.png"
        height: 32
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.margins: 5
        onClicked: welcomeMode.sendFeedback()
    }

    Text {
        id: feedbackText
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 5
        anchors.leftMargin: 10
        anchors.left: feedbackButton.right
        text: qsTr("Help us make Qt Creator even better")
    }


    Button {
        id: openProjectButton
        text: qsTr("Open Project...")
        iconSource: _hasDesktopTheme ? "image://desktoptheme/document-open" : ""
        onClicked: welcomeMode.openProject();
        anchors.right: createProjectButton.left
        anchors.margins: 5
        height: 32
        anchors.verticalCenter: parent.verticalCenter
    }

    Button {
        id: createProjectButton
        text: qsTr("Create Project...")
        iconSource: _hasDesktopTheme ? "image://desktoptheme/document-new" : ""
        onClicked: welcomeMode.newProject();
        height: 32
        anchors.margins: 5
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
    }
}
