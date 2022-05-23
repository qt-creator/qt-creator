/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick Studio Components.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick 2.15
import QtQuick.Controls 6.2
import LandingPage
import LandingPageApi
import QdsLandingPageTheme as Theme

Rectangle {
    id: rectangle2
    width: 1024
    height: 768
    color: Theme.Values.themeBackgroundColorNormal
    property bool qdsInstalled: true
    property alias openQtcButton: openQtc
    property alias openQdsButton: openQds
    property alias projectFileExists: projectInfoStatusBlock.projectFileExists
    property alias installButton: installQdsStatusBlock.installButton
    property alias generateCmakeButton: projectInfoStatusBlock.generateCmakeButton
    property alias generateProjectFileButton: projectInfoStatusBlock.generateProjectFileButton
    property alias qtVersion: projectInfoStatusBlock.qtVersion
    property alias qdsVersion: projectInfoStatusBlock.qdsVersion
    property alias cmakeLists: projectInfoStatusBlock.cmakeListText
    property alias installQdsBlockVisible: installQdsStatusBlock.visible
    property alias rememberCheckboxCheckState: rememberCheckbox.checkState

    Rectangle {
        id: logoArea
        width: parent.width
        height: 180
        color: Theme.Values.themeBackgroundColorNormal
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter

        Image {
            id: qdsLogo
            source: "logo.png"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 30
        }

        Text {
            id: qdsText
            text: qsTr("Qt Design Studio")
            font.pixelSize: Constants.fontSizeTitle
            font.family: Theme.Values.baseFont
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
        }
    }

    Item {
        id: statusBox
        anchors.top: logoArea.bottom
        anchors.bottom: buttonBox.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.leftMargin: 0

        LandingSeparator {
            id: topSeparator
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
        }

        InstallQdsStatusBlock {
            id: installQdsStatusBlock
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            visible: !qdsInstalled
        }

        ProjectInfoStatusBlock {
            id: projectInfoStatusBlock
            width: parent.width
            visible: !installQdsStatusBlock.visible
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }

        LandingSeparator {
            id: bottomSeparator
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    Rectangle {
        id: buttonBox
        width: parent.width
        height: 220
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        color: Theme.Values.themeBackgroundColorNormal

        Item {
            id: openQdsBox
            width: parent.width / 2
            height: parent.height
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            anchors.leftMargin: 0

            Text {
                id: openQdsText
                text: qsTr("Open with Qt Design Studio")
                font.pixelSize: Constants.fontSizeSubtitle
                font.family: Theme.Values.baseFont
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 50
            }

            PushButton {
                id: openQds
                anchors.top: openQdsText.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Open"
                anchors.topMargin: Constants.buttonSmallMargin
                enabled: qdsInstalled
            }
        }

        Item {
            id: openQtcBox
            width: parent.width / 2
            height: parent.height
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            anchors.rightMargin: 0

            Text {
                id: openQtcText
                text: qsTr("Open with Qt Creator - Text Mode")
                font.pixelSize: Constants.fontSizeSubtitle
                font.family: Theme.Values.baseFont
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 50
            }

            PushButton {
                id: openQtc
                anchors.top: openQtcText.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: Constants.buttonSmallMargin
                text: "Open"
            }
        }

        CheckBox {
            id: rememberCheckbox
            text: qsTr("Remember my choice")
            font.family: Theme.Values.baseFont
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 30
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
