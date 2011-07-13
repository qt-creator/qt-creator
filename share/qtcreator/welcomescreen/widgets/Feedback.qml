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

Rectangle {
    id: inner_background
    height: 32

    gradient: Gradient{
        GradientStop{color: "#eee" ; position: 0}
        GradientStop{color: "#bbb" ; position: 1}
    }

    Rectangle { color: "#444"; width: parent.width; height: 1; anchors.top: parent.top; anchors.left: parent.left }
    Rectangle { color: "white"; width: parent.width; height: 1; anchors.top: parent.top; anchors.topMargin: 1 ; anchors.left: parent.left }

    // whitelist
    property bool _hasDesktopTheme: welcomeMode.platform() === "linux"


    Components.Button {
        id: feedbackButton
        text: qsTr("Feedback")
        iconSource: "qrc:welcome/images/feedback_arrow.png"
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.margins: 5
        onClicked: welcomeMode.sendFeedback()
    }

    InsetText {
        id: feedbackText
        property bool canShow: parent.width > 630

        anchors.verticalCenter: parent.verticalCenter
        anchors.left: feedbackButton.right
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.margins: 5
        opacity: canShow ? 1 : 0
        Behavior on opacity{NumberAnimation{easing.type: Easing.OutSine}}
        width: canShow ? paintedWidth : 0
        mainColor: "#444"
        text: qsTr("Help us make Qt Creator even better")
    }

    Components.Button {
        id: openProjectButton
        text: qsTr("Open Project...")
        iconSource: _hasDesktopTheme ? "image://desktoptheme/document-open" : ""
        onClicked: welcomeMode.openProject();
        anchors.right: createProjectButton.left
        anchors.margins: 5
        anchors.verticalCenter: parent.verticalCenter
    }

    Components.Button {
        id: createProjectButton
        text: qsTr("Create Project...")
        iconSource: _hasDesktopTheme ? "image://desktoptheme/document-new" : ""
        onClicked: welcomeMode.newProject();
        anchors.right: parent.right
        anchors.margins: 5
        anchors.verticalCenter: parent.verticalCenter
    }
}
