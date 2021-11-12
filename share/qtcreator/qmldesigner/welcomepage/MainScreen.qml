/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
import QtQuick.Controls
import WelcomeScreen 1.0
import QtQuick.Layouts
import StudioControls 1.0 as StudioControls
import projectmodel 1.0

Rectangle {
    id: appBackground
    width: Constants.width
    height: Constants.height
    //anchors.fill: parent //this is required to make it responsive but commented out to force minimum size to work
    color: Constants.currentThemeBackground
    property int pageIndex: 0

    property bool designMode: !(typeof (Constants.projectModel.designMode) === "undefined")

    Rectangle {
        id: modeBar
        width: designMode ? 45 : 0
        color: Constants.modeBarCurrent
        border.color: "#00000000"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 0
    }

    TestControlPanel {
        id: controlPanel
        x: 1644
        width: 220
        height: 167
        visible: appBackground.designMode
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 15
        anchors.rightMargin: 56
    }

    ColumnLayout {
        id: openCreatelayout
        y: 188
        width: 250
        anchors.left: parent.left
        spacing: 15
        anchors.leftMargin: 70

        PushButton {
            id: createProject
            height: 50
            text: qsTr("Create Project ...")

            Layout.maximumHeight: 75
            Layout.minimumHeight: 25
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            decorated: true
            onClicked: Constants.projectModel.createProject()
        }

        PushButton {
            id: openProject
            height: 50
            text: qsTr("Open Project ...")

            Layout.maximumHeight: 75
            Layout.minimumHeight: 25
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            decorated: true
            onClicked: Constants.projectModel.openProject()
        }

        Text {
            id: newQtLabel
            width: 266
            height: 40
            color: Constants.currentGlobalText
            text: qsTr("New to Qt?")
            font.pixelSize: 24
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            Layout.fillWidth: true
        }

        PushButton {
            id: getStarted
            height: 50
            text: qsTr("Get Started")
            Layout.maximumHeight: 75
            Layout.minimumHeight: 25
            Layout.preferredHeight: 50
            Layout.fillWidth: true
            onClicked: Constants.projectModel.showHelp()
        }
    }

    ColumnLayout {
        id: currentPageMenuLayout
        y: 470
        width: 250
        anchors.left: parent.left
        anchors.leftMargin: 70
        spacing: 15

        CheckButton {
            id: recentProjects
            text: qsTr("Recent Projects")
            autoExclusive: true
            checked: true
            Layout.fillWidth: true

            Connections {
                target: recentProjects
                onClicked: appBackground.pageIndex = 0
            }
        }

        CheckButton {
            id: examples
            text: qsTr("Examples")
            autoExclusive: true
            Layout.fillWidth: true

            Connections {
                target: examples
                onClicked: appBackground.pageIndex = 1
            }
        }

        CheckButton {
            id: tutorials
            text: qsTr("Tutorials")
            autoExclusive: true
            Layout.fillWidth: true

            Connections {
                target: tutorials
                onClicked: appBackground.pageIndex = 2
            }
        }

        CheckButton {
            id: marketplace
            visible: !Constants.basic
            text: qsTr("Marketplace")
            autoExclusive: true
            Layout.fillWidth: true

            Connections {
                target: marketplace
                onClicked: appBackground.pageIndex = 3
            }
        }
    }

    BrandBar {
        id: brandBar
        y: 0
        anchors.left: parent.left
        anchors.leftMargin: 70
    }

    MainGridStack {
        id: thumbnails
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: Constants.basic ? 188 : 226
        anchors.rightMargin: 56
        anchors.bottomMargin: 54
        anchors.leftMargin: 355
        listStackLayoutCurrentIndex: appBackground.pageIndex
        stackLayoutCurrentIndex: appBackground.pageIndex
    }

    RowLayout {
        id: linkRow
        y: 1041
        height: 25
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 355
        anchors.rightMargin: 55
        anchors.bottomMargin: 14
        spacing: 0

        PushButton {
            id: userGuide
            text: qsTr("User Guide")
            fontpixelSize: 12
            Layout.minimumWidth: 100
            Layout.fillWidth: true
            Layout.preferredHeight: 25
            Layout.preferredWidth: 200
            decorated: true
            onClicked: Qt.openUrlExternally("https://doc.qt.io/qtdesignstudio/")
        }

        Item {
            id: spacer1
            width: 200
            height: 200
            Layout.fillWidth: true
            Layout.preferredWidth: 50
            Layout.preferredHeight: 25
        }

        PushButton {
            id: blog
            text: qsTr("Blog")
            fontpixelSize: 12
            Layout.minimumWidth: 100
            Layout.fillWidth: true
            Layout.preferredHeight: 25
            Layout.preferredWidth: 200
            decorated: true
            onClicked: Qt.openUrlExternally("https://blog.qt.io/")
        }

        Item {
            id: spacer2
            width: 200
            height: 200
            Layout.fillWidth: true
            Layout.preferredWidth: 50
            Layout.preferredHeight: 25
        }

        PushButton {
            id: forums
            text: qsTr("Forums")
            fontpixelSize: 12
            Layout.minimumWidth: 100
            Layout.fillWidth: true
            Layout.preferredHeight: 25
            Layout.preferredWidth: 200
            decorated: true
            onClicked: Qt.openUrlExternally("https://forum.qt.io/")
        }

        Item {
            id: spacer3
            width: 200
            height: 200
            Layout.fillWidth: true
            Layout.preferredWidth: 50
            Layout.preferredHeight: 25
        }

        PushButton {
            id: account
            text: qsTr("Account")
            fontpixelSize: 12
            Layout.minimumWidth: 100
            Layout.fillWidth: true
            Layout.preferredHeight: 25
            Layout.preferredWidth: 200
            decorated: true
            onClicked: Qt.openUrlExternally("https://login.qt.io/login")
        }

        Item {
            id: spacer4
            width: 200
            height: 200
            Layout.fillWidth: true
            Layout.preferredWidth: 50
            Layout.preferredHeight: 25
        }

        PushButton {
            id: getQt
            text: qsTr("Get Qt")
            fontpixelSize: 12
            Layout.minimumWidth: 100
            Layout.fillWidth: true
            Layout.preferredHeight: 25
            Layout.preferredWidth: 200
            decorated: true
            onClicked: Qt.openUrlExternally("https://www.qt.io/pricing")
        }

        Item {
            id: spacer5
            width: 200
            height: 200
            Layout.fillWidth: true
            Layout.preferredWidth: 50
            Layout.preferredHeight: 25
        }

        SocialButton {
            id: social
            text: ""
            Layout.minimumWidth: 100
            Layout.fillWidth: true
            Layout.preferredHeight: 25
            Layout.preferredWidth: 200
            decorated: true
        }
    }

    ViewToggle {
        id: listGridToggle
        x: 1803
        y: 188
        visible: !Constants.basic
        anchors.right: parent.right
        anchors.rightMargin: 56

        Connections {
            target: listGridToggle
            onViewDefaultChanged: Constants.isListView = !Constants.isListView
        }
    }

    RowLayout {
        y: 188
        height: 25
        visible: !Constants.basic
        anchors.left: parent.left
        anchors.right: listGridToggle.left
        layoutDirection: Qt.LeftToRight
        spacing: 0
        anchors.leftMargin: 355

        Search {
            id: searchBar
            Layout.maximumHeight: 25
            Layout.maximumWidth: 500
            Layout.minimumHeight: 25
            Layout.minimumWidth: 100
            Layout.preferredHeight: 25
            Layout.preferredWidth: 500
            Layout.fillWidth: true
        }

        StudioControls.ComboBox {
            id: tagsComboBox
            Layout.maximumHeight: 25
            Layout.maximumWidth: 100
            Layout.minimumHeight: 25
            Layout.minimumWidth: 25
            Layout.fillWidth: true
            Layout.preferredHeight: 25
            Layout.preferredWidth: 100
            hoverEnabled: false
            enabled: true
            model: ["Tags", "Favorite", "3D", "Cluster", "Medical"]
        }

        Item {
            id: spacer
            width: 200
            height: 200
            Layout.maximumHeight: 25
            Layout.minimumHeight: 25
            Layout.preferredHeight: 25
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        StudioControls.ComboBox {
            id: catagoriesComboBox
            Layout.minimumHeight: 25
            Layout.minimumWidth: 75
            Layout.maximumHeight: 25
            Layout.maximumWidth: 250
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            Layout.preferredHeight: 25
            Layout.preferredWidth: 250
            hoverEnabled: false
            enabled: true
            model: ["Catagories", "Qt 6", "Qt 5", "MCU", "Automotive"]
        }
    }
}

/*##^##
Designer {
    D{i:0;height:1080;width:1920}
}
##^##*/

