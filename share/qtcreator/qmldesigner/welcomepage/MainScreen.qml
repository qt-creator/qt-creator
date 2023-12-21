// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import WelcomeScreen 1.0
import projectmodel 1.0
import DataModels 1.0

Rectangle {
    id: appBackground
    height: Constants.height
    color: Constants.currentThemeBackground
    width: 1842
    //anchors.fill: parent //this is required to make it responsive but commented out to force minimum size to work
    property int pageIndex: 0
    property bool designMode: !(typeof (Constants.projectModel.designMode) === "undefined")

    signal openUiTour
    signal closeUiTour

    function uiTourClosed() {
        recentProjects.checked = true
    }

    TestControlPanel {
        id: controlPanel
        x: 1644
        width: 220
        height: 127
        visible: appBackground.designMode
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 15
        anchors.rightMargin: 56
    }

    ColumnLayout {
        id: openCreatelayout
        y: 150
        anchors.left: parent.left
        anchors.right: thumbnails.left
        anchors.rightMargin: 20
        spacing: 15
        anchors.leftMargin: 20

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
        y: 422
        anchors.left: parent.left
        anchors.right: thumbnails.left
        anchors.rightMargin: 20
        anchors.leftMargin: 20
        spacing: 15

        CheckButton {
            id: recentProjects
            text: qsTr("Recent Projects")
            autoExclusive: true
            checked: true
            Layout.fillWidth: true

            Connections {
                target: recentProjects
                function onClicked(mouse) { appBackground.pageIndex = 0 }
            }
        }

        CheckButton {
            id: examples
            text: qsTr("Examples")
            autoExclusive: true
            Layout.fillWidth: true

            Connections {
                target: examples
                function onClicked(mouse) { appBackground.pageIndex = 1 }
            }
        }

        CheckButton {
            id: tutorials
            text: qsTr("Tutorials")
            autoExclusive: true
            Layout.fillWidth: true

            Connections {
                target: tutorials
                function onClicked(mouse) { appBackground.pageIndex = 2 }
            }
        }

        CheckButton {
            id: tours
            text: qsTr("UI Tour")
            autoExclusive: true
            Layout.fillWidth: true

            Connections {
                target: tours
                function onClicked(mouse) { appBackground.pageIndex = 3 }
            }
        }
    }

    BrandBar {
        id: brandBar
        y: 0
        anchors.left: parent.left
        anchors.leftMargin: 20

        Rectangle {
            id: loadProgress
            x: 4
            y: 120
            width: Constants.loadingProgress * 10
            height: 4
            color: Constants.currentGlobalText
            opacity: Constants.loadingProgress > 90 ? (100 - Constants.loadingProgress) / 10 : 1
        }
    }

    MainGridStack {
        id: thumbnails
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 150
        anchors.rightMargin: 56
        anchors.bottomMargin: 54
        anchors.leftMargin: 290
        stackLayoutCurrentIndex: appBackground.pageIndex
    }

    RowLayout {
        id: linkRow
        y: 1041
        height: 25
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 290
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

    BlogBanner {
        id: blogBanner
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.right: thumbnails.left
        anchors.rightMargin: 20
        y: 657
    }
}
