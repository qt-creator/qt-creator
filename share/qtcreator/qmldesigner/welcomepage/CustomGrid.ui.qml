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

import QtQuick
import WelcomeScreen
import QtQuick.Layouts 6.0

Item {
    id: gridContainer
    width: 1460
    height: 760
    state: "qds"
    property alias mymodel: mainThumbnails.model
    property alias myDelegate: mainThumbnails.delegate

    CustomScrollView {
        anchors.fill: parent
        anchors.leftMargin: 20
        id: mainThumbnailsView
        GridView {
            id: mainThumbnails
            boundsBehavior: Flickable.StopAtBounds

            contentHeight: 220
            contentWidth: 220

            cellWidth: 240
            cellHeight: 240

            delegate: ThumbnailDelegate {
                tagged: true
                likable: true
                downloadable: true
            }
        }
    }

    Column {
        id: sessionsColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: 5
        anchors.rightMargin: 985
        anchors.topMargin: 72
        anchors.leftMargin: 0

        MyRecentSessionListDelegate {
            id: recentSessionListDelegate
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillHeight: false
            Layout.minimumHeight: 58
            Layout.preferredHeight: 58
            Layout.fillWidth: true
            Layout.maximumWidth: 475
            Layout.minimumWidth: 250
            Layout.preferredWidth: 475
            anchors.rightMargin: 0
            anchors.leftMargin: 0
        }

        MyRecentSessionListDelegate {
            id: recentSessionListDelegate1
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillHeight: false
            Layout.minimumHeight: 58
            Layout.preferredHeight: 58
            Layout.fillWidth: true
            Layout.minimumWidth: 250
            Layout.maximumWidth: 475
            anchors.leftMargin: 0
            anchors.rightMargin: 0
            Layout.preferredWidth: 475
        }

        MyRecentSessionListDelegate {
            id: recentSessionListDelegate2
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillHeight: false
            Layout.minimumHeight: 58
            Layout.preferredHeight: 58
            Layout.fillWidth: true
            Layout.minimumWidth: 250
            Layout.maximumWidth: 475
            anchors.leftMargin: 0
            anchors.rightMargin: 0
            Layout.preferredWidth: 475
        }
    }

    PushButton {
        id: manangeSessionsButton
        x: 306
        y: 0
        width: 169
        height: 58
        text: "Manage Sessions"
    }

    Text {
        id: sessionsLabel
        x: 0
        y: 0
        width: 327
        height: 61
        color: Constants.currentGlobalText
        text: qsTr("Sessions")
        font.pixelSize: 22
        verticalAlignment: Text.AlignVCenter
    }

    GridView {
        id: listView
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        flow: GridView.FlowTopToBottom
        anchors.bottomMargin: 0
        anchors.topMargin: 0
        anchors.rightMargin: 0
        anchors.leftMargin: 503
        cellHeight: 500
        cellWidth: 495

        MyRecentProjectListDelegate {
            id: myRecentProjectListDelegate
            y: 0
            anchors.left: parent.left
            anchors.right: parent.right
        }
    }

    states: [
        State {
            name: "qds"
            when: Constants.defaultBrand && !Constants.isListView

            PropertyChanges {
                target: manangeSessionsButton
                visible: false
            }

            PropertyChanges {
                target: sessionsLabel
                visible: false
            }

            PropertyChanges {
                target: listView
                x: 503
                y: 0
                visible: false
            }

            PropertyChanges {
                target: mainThumbnails
                visible: true
            }

            PropertyChanges {
                target: myRecentProjectListDelegate
                x: 503
                y: 0
                width: 957
                height: 58
                visible: false
            }

            PropertyChanges {
                target: sessionsColumn
                visible: false
            }
        },
        State {
            name: "creator"
            when: !Constants.defaultBrand && !Constants.isListView

            PropertyChanges {
                target: mainThumbnailsView
                visible: true
                anchors.leftMargin: 500
            }

            PropertyChanges {
                target: gridContainer
                clip: true
            }

            PropertyChanges {
                target: listView
                visible: false
            }

            PropertyChanges {
                target: sessionsLabel
                visible: true
            }

            PropertyChanges {
                target: manangeSessionsButton
                visible: true
            }

            PropertyChanges {
                target: myRecentProjectListDelegate
                visible: false
            }

            PropertyChanges {
                target: sessionsColumn
                visible: true
            }
        },
        State {
            name: "qdsList"
            when: Constants.defaultBrand && Constants.isListView

            PropertyChanges {
                target: manangeSessionsButton
                visible: false
            }

            PropertyChanges {
                target: sessionsLabel
                visible: false
            }

            PropertyChanges {
                target: mainThumbnails
                visible: false
            }

            PropertyChanges {
                target: listView
                x: 20
                y: 0
                width: 1440
                height: 760
                visible: true
                anchors.leftMargin: 0
            }

            PropertyChanges {
                target: myRecentProjectListDelegate
                x: 20
                y: 0
                width: 1440
                height: 58
                visible: true
            }

            PropertyChanges {
                target: sessionsColumn
                visible: false
            }
        },
        State {
            name: "creatorList"
            when: !Constants.defaultBrand && Constants.isListView
            PropertyChanges {
                target: mainThumbnails
                visible: false
                anchors.leftMargin: 500
            }

            PropertyChanges {
                target: gridContainer
                clip: true
            }

            PropertyChanges {
                target: sessionsLabel
                visible: true
            }

            PropertyChanges {
                target: manangeSessionsButton
                visible: true
            }

            PropertyChanges {
                target: myRecentProjectListDelegate
                x: 503
                y: 0
                width: 957
                height: 58
                visible: true
            }

            PropertyChanges {
                target: listView
                visible: true
                anchors.leftMargin: 503
            }

            PropertyChanges {
                target: sessionsColumn
                visible: true
            }

            PropertyChanges {
                target: recentSessionListDelegate
                Layout.preferredHeight: 58
                Layout.minimumHeight: 58
                Layout.fillWidth: true
            }

            PropertyChanges {
                target: recentSessionListDelegate1
                Layout.preferredHeight: 58
            }

            PropertyChanges {
                target: recentSessionListDelegate2
                Layout.minimumHeight: 58
                Layout.preferredHeight: 58
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;height:760;width:1460}
}
##^##*/

