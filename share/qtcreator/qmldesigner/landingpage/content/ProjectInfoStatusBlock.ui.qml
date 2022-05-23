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
import QtQuick.Controls 2.15
import LandingPage
import QdsLandingPageTheme as Theme

Rectangle {
    id: projectInfo
    height: 300
    color: Theme.Values.themeBackgroundColorNormal
    border.color: Theme.Values.themeBackgroundColorNormal
    border.width: 0
    property bool qdsInstalled: qdsVersionText.text.length > 0
    property bool projectFileExists: false
    property string qdsVersion: "UNKNOWN"
    property string qtVersion: "UNKNOWN"
    property alias cmakeListText: cmakeList.text
    property alias generateCmakeButton: generateCmakeButton
    property alias generateProjectFileButton: generateProjectFileButton

    Item {
        id: projectFileInfoBox
        width: projectInfo.width
        height: 150
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 30

        Text {
            id: projectFileInfoTitle
            text: qsTr("QML PROJECT FILE INFO")
            font.family: Theme.Values.baseFont
            font.pixelSize: Constants.fontSizeSubtitle
            anchors.top: parent.top
            horizontalAlignment: Text.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 10
        }

        Item {
            id: projectFileInfoVersionBox
            width: parent.width
            height: 150
            visible: projectFileExists
            anchors.top: projectFileInfoTitle.bottom
            anchors.topMargin: 0
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                id: qtVersionText
                text: qsTr("Qt Version - ") + qtVersion
                font.family: Theme.Values.baseFont
                font.pixelSize: Constants.fontSizeSubtitle
                anchors.top: parent.top
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 10
            }

            Text {
                id: qdsVersionText
                text: qsTr("Qt Design Studio Version - ") + qdsVersion
                font.family: Theme.Values.baseFont
                font.pixelSize: Constants.fontSizeSubtitle
                anchors.top: qtVersionText.bottom
                horizontalAlignment: Text.AlignHCenter
                anchors.topMargin: 10
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        Item {
            id: projectFileInfoMissingBox
            width: parent.width
            height: 200
            visible: !projectFileInfoVersionBox.visible
            anchors.top: projectFileInfoTitle.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 0

            Text {
                id: projectFileInfoMissingText
                text: qsTr("No QML project file found - Would you like to create one?")
                font.family: Theme.Values.baseFont
                font.pixelSize: Constants.fontSizeSubtitle
                anchors.top: parent.top
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 10
            }

            PushButton {
                id: generateProjectFileButton
                anchors.top: projectFileInfoMissingText.bottom
                text: "Generate"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: Constants.buttonDefaultMargin
            }
        }
    }

    Item {
        id: cmakeInfoBox
        width: projectInfo.width
        height: 200
        anchors.top: projectFileInfoBox.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 40

        Text {
            id: cmakeInfoTitle
            text: qsTr("CMAKE RESOURCE FILES")
            font.family: Theme.Values.baseFont
            font.pixelSize: Constants.fontSizeSubtitle
            anchors.top: parent.top
            horizontalAlignment: Text.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 10
        }

        Item {
            id: cmakeListBox
            width: 150
            height: 40
            visible: cmakeListText.length > 0
            anchors.top: cmakeInfoTitle.bottom
            anchors.topMargin: 10
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                id: cmakeList
                text: qsTr("")
                font.family: "TitilliumWeb"
                font.pixelSize: Constants.fontSizeSubtitle
                anchors.top: parent.top
                horizontalAlignment: Text.AlignHCenter
                anchors.topMargin: 0
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        Item {
            id: cmakeMissingBox
            width: cmakeInfoBox.width
            height: 200
            visible: cmakeListText.length === 0
            anchors.top: cmakeInfoTitle.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 10

            Text {
                id: cmakeMissingText
                text: qsTr("No resource files found - Would you like to generate them?")
                font.family: Theme.Values.baseFont
                font.pixelSize: Constants.fontSizeSubtitle
                anchors.top: parent.top
                horizontalAlignment: Text.AlignHCenter
                anchors.topMargin: 10
                anchors.horizontalCenter: parent.horizontalCenter
            }

            PushButton {
                id: generateCmakeButton
                anchors.top: cmakeMissingText.bottom
                text: "Generate"
                anchors.topMargin: Constants.buttonDefaultMargin
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
