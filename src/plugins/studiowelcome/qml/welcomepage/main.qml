/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.10
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import welcome 1.0
import projectmodel 1.0
import StudioFonts 1.0

Item {
    width: 1024
    height: 786

    Rectangle {
        id: rectangle
        anchors.fill: parent
        visible: true
        color: "#2d2e30"

        StackLayout {
            id: stackLayout
            anchors.margins: 10
            anchors.top: topLine.bottom
            anchors.bottom: bottomLine.top
            anchors.right: parent.right
            anchors.left: parent.left

            ScrollView{
                ProjectsGrid {
                    model: ProjectModel {
                        id: projectModel
                    }
                    onItemSelected: projectModel.openProjectAt(index)
                }
            }

            ScrollView {
                ProjectsGrid {
                    model: ExamplesModel {}
                    onItemSelected: projectModel.openExample(item.projectName, item.qmlFileName)
                }
            }

            ScrollView{
                ProjectsGrid {
                    model: TutorialsModel {}
                    onItemSelected: Qt.openUrlExternally(item.url)
                }
            }
        }
        Rectangle {
            id: topLine
            height: 1
            color: "#bababa"
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 200
        }

        Rectangle {
            id: bottomLine
            height: 1
            color: "#bababa"
            anchors.left: topLine.left
            anchors.right: topLine.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 60
        }

        Row {
            x: 8
            y: 160
            spacing: 26

            MyTabButton {
                text: qsTr("Recent Projects")
                bottomPadding: 6
                topPadding: 6.8
                checked: true
                onClicked: stackLayout.currentIndex = 0
            }

            MyTabButton {
                text: qsTr("Examples")
                onClicked: stackLayout.currentIndex = 1
            }

            MyTabButton {
                text: qsTr("Tutorials")
                onClicked: stackLayout.currentIndex = 2
            }
        }

        AccountImage {
            id: account
            x: 946
            y: 29
            anchors.right: parent.right
            anchors.rightMargin: 40
        }

        GridLayout {
            y: 78
            anchors.horizontalCenter: parent.horizontalCenter
            columnSpacing: 10
            rows: 2
            columns: 2

            Text {
                id: welcomeTo
                color: Constants.textDefaultColor
                text: qsTr("Welcome to")
                renderType: Text.NativeRendering
                font.pixelSize: 22
                font.family: StudioFonts.titilliumWeb_regular
            }

            Text {
                id: qtDesignStudio
                color: "#4cd265"
                text: qsTr("Qt Design Studio")
                renderType: Text.NativeRendering
                font.family: StudioFonts.titilliumWeb_regular
                font.pixelSize: 22
            }

            MyButton {
                text: qsTr("Create New")
                onClicked: projectModel.createProject()
            }

            MyButton {
                text: qsTr("Open Project")
                onClicked: projectModel.openProject()
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            }
        }

        RowLayout {
            y: 732
            height: 28
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 26
            spacing: 50

            MyButton {
                text: qsTr("Help")
                onClicked: projectModel.showHelp()
            }

            MyButton {
                text: qsTr("Community")
                onClicked: Qt.openUrlExternally("https://forum.qt.io/")
            }

            MyButton {
                text: qsTr("Blog")
                onClicked: Qt.openUrlExternally("http://blog.qt.io/")
            }
        }

        Text {
            id: qtDesignStudio1
            x: 891
            y: 171
            color: "#ffffff"
            text: qsTr("Community Edition")
            anchors.right: parent.right
            anchors.rightMargin: 23
            font.weight: Font.Light
            font.pixelSize: 14
            font.family: StudioFonts.titilliumWeb_regular
            renderType: Text.NativeRendering
            visible: projectModel.communityVersion
        }
    }
}
